#include "vision_api.h"

#include"CommonBitmap.h"
#include"vision.h"

#include <climits>
#include <type_traits>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace vision {

// ---- internal helpers ------------------------------------------------------

int similarityToShift(double similarity)
{
	if(std::isnan(similarity) || similarity < 0 || similarity > 1){
		return -1;
	}
	return static_cast<int>((1.0 - similarity) * MAX_COLOR_SHIFT);
}

// 64-bit budget: template area * per-point shift easily overflows int for
// large templates with loose similarity.
static long long imageShiftBudget(Bitmap* templateImage, double similarity)
{
	return (long long)templateImage->width_ * templateImage->height_
	       * similarityToShift(similarity);
}

static long long featureShiftBudget(const FeatureCompositionRoot& feature,
                                    double similarity)
{
	return (long long)similarityToShift(similarity) * MAX_COLOR_SHIFT * feature.count;
}

// decodeColor/decodeColorComposition is malloc-based; wrap it so the
// normal path cannot leak.
class ColorCompositionGuard
{
	ColorComposition* ptr_;
public:
	explicit ColorCompositionGuard(ColorComposition* p) : ptr_(p) {}
	~ColorCompositionGuard() { freeColorComposition(ptr_); }
	ColorCompositionGuard(const ColorCompositionGuard&) = delete;
	ColorCompositionGuard& operator=(const ColorCompositionGuard&) = delete;
	ColorComposition* get() const { return ptr_; }
};

class FeatureGuard
{
	FeatureCompositionRoot* ptr_;
public:
	explicit FeatureGuard(FeatureCompositionRoot* p) : ptr_(p) {}
	~FeatureGuard() { freeFeatureComposition(ptr_); }
	FeatureGuard(const FeatureGuard&) = delete;
	FeatureGuard& operator=(const FeatureGuard&) = delete;
	FeatureCompositionRoot* get() const { return ptr_; }
};

// Dispatch a decoded composition (single node or multi-node list) to the
// templated core. `which` selects compare-with-index semantics.
template<class Func>
static auto dispatchComposition(ColorComposition* color, Func&& core)
{
	if(color->next == nullptr){
		switch (color->color.type) {
			case TColorType::ALONE:         return core((Color*)color->color.data);
			case TColorType::COLOR_GAMUT:   return core((ColorGamut*)color->color.data);
			case TColorType::NOT:           return core((ColorNot*)color->color.data);
			case TColorType::COLOR_GAMUT_NOT: return core((ColorGamutNot*)color->color.data);
			default: break;
		}
	}
	return core(color);
}

static bool decodeColorString(const char* str, ColorCompositionGuard& guard)
{
	size_t size = 0;
	while(str[size] != '\0') size++;
	guard.~ColorCompositionGuard();
	new(&guard) ColorCompositionGuard(decodeColor(str, (int)size));
	return guard.get() != nullptr;
}

// Screen-layout dispatch: the public API is format-agnostic; each entry
// point branches once on bitmap->format_ and lands in a fully specialized
// code path. Callers take the layout with a templated lambda:
//   dispatchLayout(bm, [&]<PixelChannels PC>(LayoutTag<PC>) { ... })
template <PixelChannels PC>
using LayoutTag = std::integral_constant<PixelChannels, PC>;

template<class Func>
static auto dispatchLayout(Bitmap* bitmap, Func&& fn)
{
	if(bitmap->format_ == PIXEL_BGRA){
		return fn(LayoutTag<BGRA_LAYOUT>{});
	}
	return fn(LayoutTag<RGBA_LAYOUT>{});
}

// Shared skeleton for the four color APIs: validate the pixel, decode the
// color string once (single-color fast path avoids any heap allocation),
// dispatch on the screen layout, then hand the decoded color object to the
// caller's templated lambda. `fail` is the API-specific error value.
template<class Fail, class Func>
static auto withDecodedColor(Bitmap* bitmap, int x, int y, const char* color,
                             double similarity, Fail fail, Func&& fn)
{
	int shift = similarityToShift(similarity);
	if(shift < 0 || color == nullptr || !isInBitmapScope(bitmap, x, y)){
		return fail;
	}
	size_t size = 0;
	while(color[size] != '\0') size++;
	if(size == 6){
		Color alone;
		if(decodeColor(color, (int)size, &alone)){
			return dispatchLayout(bitmap, [&]<PixelChannels PC>(LayoutTag<PC>){
				return fn(LayoutTag<PC>{}, &alone, shift);
			});
		}
		return fail;
	}
	ColorCompositionGuard guard(nullptr);
	if(!decodeColorString(color, guard)){
		return fail;
	}
	return dispatchLayout(bitmap, [&]<PixelChannels PC>(LayoutTag<PC>){
		return dispatchComposition(guard.get(), [&](auto c){
			return fn(LayoutTag<PC>{}, c, shift);
		});
	});
}

template<PixelChannels PC,class TColor>
static bool compareColorAt(Bitmap* bitmap, int x, int y, TColor color, int shift)
{
	return compareColor<PC>(computeCoordColor(bitmap, x, y), color, shift) != 0;
}

template<PixelChannels PC,class TColor>
static int whichColorAt(Bitmap* bitmap, int x, int y, TColor color, int shift)
{
	return compareColor<PC>(computeCoordColor(bitmap, x, y), color, shift);
}

template<PixelChannels PC,class TColor>
static int countColorRect(Bitmap* bitmap, int x, int y, int x2, int y2,
                          TColor color, int shift)
{
	return ::vision::getColorCount<PC>(bitmap, x, y, x2, y2, color, shift);
}

template<PixelChannels PC,class TColor>
static FindResult findColorRect(Bitmap* bitmap, int x, int y, int x2, int y2,
                                TColor color, int shift, int order)
{
	Point out(-1, -1);
	bool result = findColor<PC>(bitmap, x, y, x2, y2, color, shift, order, &out);
	if(!result){
		out.x = -1;
		out.y = -1;
	}
	FindResult r(result);
	r.point = out;
	r.index = result ? 1 : 0;
	return r;
}

// ---- image loading ---------------------------------------------------------

static bool loadImageByPath(const std::string& path, CommonBitmap* image)
{
	if(path.empty()){
		return false;
	}
	if(path[0] == std::filesystem::path::preferred_separator){
		return image->load(path.c_str());
	}
	// CommonBitmap::load consults the installed ResourceLoader for relative
	// paths and falls back to direct file access.
	return image->load(path.c_str());
}

bool loadImages(const char* names, size_t size, std::vector<CommonBitmap>& images)
{
	size_t start = 0;
	auto loadOne = [&](size_t end) -> bool {
		if(end == start){
			return true;   // empty segment between separators: skip
		}
		auto& p = images.emplace_back();
		std::string name(names + start, end - start);
		if(!loadImageByPath(name, &p)){
			return false;
		}
		return true;
	};
	for(size_t i = 0; i < size; i++){
		if(names[i] == '|'){
			if(!loadOne(i)){
				return false;
			}
			start = i + 1;
		}
	}
	if(!loadOne(size)){
		return false;
	}
	return true;
}

// ---- single pixel ----------------------------------------------------------

Color getColor(Bitmap* bitmap, int x, int y)
{
	// Assemble the logical 0xRRGGBB directly instead of reinterpreting
	// through Pixel (whose memory order contradicts its field names on
	// little-endian, which used to force a channel-swap trick).
	return dispatchLayout(bitmap, [&]<PixelChannels PC>(LayoutTag<PC>){
		const unsigned char* c = computeCoordColor(bitmap, x, y);
		unsigned r = c[PC.r];
		unsigned g = c[PC.g];
		unsigned b = c[PC.b];
		return Color((r << 16) | (g << 8) | b);
	});
}

// ---- color predicates ------------------------------------------------------

bool isColor(Bitmap* bitmap, int x, int y, const char* color, double similarity)
{
	return withDecodedColor(bitmap, x, y, color, similarity, false,
		[&]<PixelChannels PC>(LayoutTag<PC>, auto c, int shift){
			return compareColorAt<PC>(bitmap, x, y, c, shift);
		});
}

int whichColor(Bitmap* bitmap, int x, int y, const char* color, double similarity)
{
	return withDecodedColor(bitmap, x, y, color, similarity, 0,
		[&]<PixelChannels PC>(LayoutTag<PC>, auto c, int shift){
			int r = whichColorAt<PC>(bitmap, x, y, c, shift);
			if constexpr (std::is_same_v<decltype(c), ColorComposition*>){
				return r;          // composition: 1-based index already
			}else{
				return r ? 1 : 0;  // single node: boolean
			}
		});
}

static void normalizeRect(Bitmap* bitmap, int& x, int& y, int& x2, int& y2)
{
	if(x2 == -1) x2 = (int)bitmap->width_;
	if(y2 == -1) y2 = (int)bitmap->height_;
}

int getColorCount(Bitmap* bitmap, int x, int y, int x2, int y2,
                  const char* color, double similarity)
{
	normalizeRect(bitmap, x, y, x2, y2);
	if(!isInBitmapScope(bitmap, x, y, x2, y2)){
		return -1;
	}
	return withDecodedColor(bitmap, x, y, color, similarity, -1,
		[&]<PixelChannels PC>(LayoutTag<PC>, auto c, int shift){
			return countColorRect<PC>(bitmap, x, y, x2, y2, c, shift);
		});
}

FindResult findColor(Bitmap* bitmap, int x, int y, int x2, int y2,
                     const char* color, double similarity, int order)
{
	normalizeRect(bitmap, x, y, x2, y2);
	if(!isInBitmapScope(bitmap, x, y, x2, y2)){
		return FindResult(false);
	}
	return withDecodedColor(bitmap, x, y, color, similarity, FindResult(false),
		[&]<PixelChannels PC>(LayoutTag<PC>, auto c, int shift){
			return findColorRect<PC>(bitmap, x, y, x2, y2, c, shift, order);
		});
}

// ---- features --------------------------------------------------------------

bool isFeature(Bitmap* bitmap, int anchorX, int anchorY,
               const char* feature, double similarity)
{
	int shift = similarityToShift(similarity);
	if(shift < 0 || feature == nullptr || !isInBitmapScope(bitmap, anchorX, anchorY)){
		return false;
	}
	size_t size = 0;
	while(feature[size] != '\0') size++;
	FeatureCompositionRoot root;
	if(!decodeFeature(feature, (int)size, &root)){
		return false;
	}
	FeatureGuard guard(&root);
	long long budget = featureShiftBudget(root, similarity);
	return dispatchLayout(bitmap, [&]<PixelChannels PC>(LayoutTag<PC>){
		return vision::isFeature<PC>(bitmap, anchorX, anchorY, &root, (int)budget);
	});
}

FindResult findFeature(Bitmap* bitmap, int x, int y, int x2, int y2,
                       const char* feature, double similarity, int order)
{
	int shift = similarityToShift(similarity);
	if(shift < 0 || feature == nullptr){
		return FindResult(false);
	}
	normalizeRect(bitmap, x, y, x2, y2);
	if(!isInBitmapScope(bitmap, x, y, x2, y2)){
		return FindResult(false);
	}
	size_t size = 0;
	while(feature[size] != '\0') size++;
	FeatureCompositionRoot root;
	if(!decodeFeature(feature, (int)size, &root)){
		return FindResult(false);
	}
	FeatureGuard guard(&root);
	long long budget = featureShiftBudget(root, similarity);
	return dispatchLayout(bitmap, [&]<PixelChannels PC>(LayoutTag<PC>){
		FeatureFinder<PC> finder(bitmap, &root,
		                         (int)std::min<long long>(budget, INT_MAX));
		bool result = orderFindColor(bitmap, x, y, x2, y2, order, &finder);
		FindResult r(result);
		if(result){
			r.point = finder.getResult();
			r.index = 1;
		}
		return r;
	});
}

// ---- image templates -------------------------------------------------------

bool isImage(Bitmap* bitmap, int x, int y, Bitmap* templateImage, double similarity)
{
	if(templateImage == nullptr){
		return false;
	}
	return vision::isImage(bitmap, x, y, templateImage,
	                       (int)std::min<long long>(imageShiftBudget(templateImage, similarity), INT_MAX));
}

int whichImage(Bitmap* bitmap, int x, int y,
               const std::vector<CommonBitmap>& templates, double similarity)
{
	for(size_t i = 0; i < templates.size(); i++){
		if(isImage(bitmap, x, y,
		           const_cast<Bitmap*>((const Bitmap*)&templates[i]), similarity)){
			return (int)i + 1;
		}
	}
	return 0;
}

// Single-pass multi-template scan: one walk over the screen, every
// candidate position tries all templates (first match wins). N templates
// cost one traversal, not N. The matcher is stack-constructed with the
// screen layout bound, so the scan loop stays fully inlined.
FindResult findImage(Bitmap* bitmap, int x, int y, int x2, int y2,
                     const std::vector<CommonBitmap>& templates,
                     double similarity, int order)
{
	FindResult result(false);
	int shift = similarityToShift(similarity);
	if(templates.empty() || shift < 0){
		return result;
	}
	normalizeRect(bitmap, x, y, x2, y2);
	if(!isInBitmapScope(bitmap, x, y, x2, y2)){
		return result;
	}
	std::vector<Bitmap*> tplPtrs;
	std::vector<int> budgets;
	tplPtrs.reserve(templates.size());
	budgets.reserve(templates.size());
	for(auto& t : templates){
		long long b = imageShiftBudget((Bitmap*)&t, similarity);
		if(b > INT_MAX) b = INT_MAX;
		if(b < 0) b = 0;
		tplPtrs.push_back((Bitmap*)&t);
		budgets.push_back((int)b);
	}
	if(bitmap->format_ == PIXEL_BGRA){
		MultiTemplateMatcher<BGRA_LAYOUT> matcher(bitmap, tplPtrs.data(),
		                                          (int)tplPtrs.size(), budgets.data());
		if(orderFindColor(bitmap, x, y, x2, y2, order, &matcher)){
			result.found = true;
			result.point = matcher.getResult();
			result.index = matcher.getHit() + 1;
		}
	}else{
		MultiTemplateMatcher<RGBA_LAYOUT> matcher(bitmap, tplPtrs.data(),
		                                          (int)tplPtrs.size(), budgets.data());
		if(orderFindColor(bitmap, x, y, x2, y2, order, &matcher)){
			result.found = true;
			result.point = matcher.getResult();
			result.index = matcher.getHit() + 1;
		}
	}
	return result;
}

bool isImage(Bitmap* bitmap, int x, int y, const char* templates, double similarity)
{
	if(templates == nullptr || !isInBitmapScope(bitmap, x, y)){
		return false;
	}
	std::vector<CommonBitmap> images;
	if(!loadImages(templates, strlen(templates), images)){
		return false;
	}
	for(auto& image : images){
		if(isImage(bitmap, x, y, &image, similarity)){
			return true;
		}
	}
	return false;
}

int whichImage(Bitmap* bitmap, int x, int y, const char* templates, double similarity)
{
	if(templates == nullptr || !isInBitmapScope(bitmap, x, y)){
		return 0;
	}
	std::vector<CommonBitmap> images;
	if(!loadImages(templates, strlen(templates), images)){
		return 0;
	}
	return whichImage(bitmap, x, y, images, similarity);
}

FindResult findImage(Bitmap* bitmap, int x, int y, int x2, int y2,
                     const char* templates, double similarity, int order)
{
	if(templates == nullptr){
		return FindResult(false);
	}
	normalizeRect(bitmap, x, y, x2, y2);
	std::vector<CommonBitmap> images;
	if(!loadImages(templates, strlen(templates), images)){
		return FindResult(false);
	}
	return findImage(bitmap, x, y, x2, y2, images, similarity, order);
}

} //namespace vision
