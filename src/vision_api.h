#ifndef __VISION_API_H__
#define __VISION_API_H__

// Public C++ API for simple_vision.
//
// All scanning functions take a screenshot as `Bitmap*` (the caller owns it —
// typically a screen buffer provided by the host application) and a template
// region or a string-encoded color/feature/image description.
//
// Color string grammar (see decodeColor in vision_color.h):
//   "rrggbb"            single color, exact channels
//   "!rrggbb"           anything but this color
//   "rrggbb-rrggbb"     gamut: color plus per-channel tolerance
//   "!rrggbb-rrggbb"    anything outside the gamut
//   "a|b|c"             composition: any of a, b, c (whichX reports 1-based index)
//
// Feature string grammar (see decodeFeature in vision_feature.h):
//   "x|y|color,x|y|color,..."   a sparse set of anchor points matched with a
//                               shared total shift budget
//
// Image template argument is a path list separated by '|'; each entry is
// loaded through CommonBitmap (honoring the installed ResourceLoader).
//
// Similarity: 1.0 means exact match, 0.0 accepts anything. It is converted to
// an internal shift budget; for multi-point matching the budget is the sum
// over all points, so the same similarity value is stricter for bigger
// templates.

#include"Bitmap.h"
#include"CommonBitmap.h"
#include"vision_color.h"
#include"vision_feature.h"

#include <string>
#include <vector>

namespace vision {

struct FindResult {
	Point point;    // (-1,-1) when nothing matched
	int index;      // 1-based template index for whichImage/findImage, else 1
	bool found;

	explicit FindResult(bool found_ = false)
		: point(-1, -1), index(0), found(found_) {}
};

// ---- single pixel ----------------------------------------------------------

// Channel-packed color (0xRRGGBB) at (x,y). Coordinates must be in scope.
Color getColor(Bitmap* bitmap, int x, int y);

// ---- color predicates ------------------------------------------------------

// True when the pixel at (x,y) matches `color` within the similarity budget.
bool isColor(Bitmap* bitmap, int x, int y, const char* color, double similarity = 1.0);

// 1-based index of the first alternative in the composition that matches,
// 0 when none does.
int whichColor(Bitmap* bitmap, int x, int y, const char* color, double similarity = 1.0);

// Count pixels inside the rectangle that match. (x2,y2)==(-1,-1) means
// "to the bottom-right corner of the bitmap".
int getColorCount(Bitmap* bitmap, int x, int y, int x2, int y2,
                  const char* color, double similarity = 1.0);

// Scan the rectangle in `order` (READ_ORDER) for the first matching pixel.
FindResult findColor(Bitmap* bitmap, int x, int y, int x2, int y2,
                     const char* color, double similarity = 1.0,
                     int order = UP_DOWN_LEFT_RIGHT);

// ---- features --------------------------------------------------------------

// True when every anchor of the feature matches within the total budget.
bool isFeature(Bitmap* bitmap, int anchorX, int anchorY,
               const char* feature, double similarity = 1.0);

FindResult findFeature(Bitmap* bitmap, int x, int y, int x2, int y2,
                       const char* feature, double similarity = 1.0,
                       int order = UP_DOWN_LEFT_RIGHT);

// ---- image templates -------------------------------------------------------

// `templates` holds one or more '|'-separated image paths. They are loaded
// once per call; for repeated scanning, load CommonBitmap objects yourself
// and pass the overloads below.

bool isImage(Bitmap* bitmap, int x, int y,
             const char* templates, double similarity = 1.0);

// 1-based index of the first template that matches at (x,y), 0 when none.
int whichImage(Bitmap* bitmap, int x, int y,
               const char* templates, double similarity = 1.0);

FindResult findImage(Bitmap* bitmap, int x, int y, int x2, int y2,
                     const char* templates, double similarity = 1.0,
                     int order = UP_DOWN_LEFT_RIGHT);

// Pre-loaded variants — no path parsing, no filesystem access.
bool isImage(Bitmap* bitmap, int x, int y, Bitmap* templateImage,
             double similarity = 1.0);

int whichImage(Bitmap* bitmap, int x, int y,
               const std::vector<CommonBitmap>& templates,
               double similarity = 1.0);

FindResult findImage(Bitmap* bitmap, int x, int y, int x2, int y2,
                     const std::vector<CommonBitmap>& templates,
                     double similarity = 1.0, int order = UP_DOWN_LEFT_RIGHT);

// ---- helpers ---------------------------------------------------------------

// Load every '|'-separated entry of `paths`. Returns false and leaves
// `out` with whatever loaded successfully when an entry fails.
bool loadImages(const char* paths, size_t size, std::vector<CommonBitmap>& out);
inline bool loadImages(const std::string& paths, std::vector<CommonBitmap>& out) {
	return loadImages(paths.data(), paths.size(), out);
}

// Clamp a similarity into [0,1]; values outside the range return -1 so
// callers can distinguish "invalid" from "clamped".
int similarityToShift(double similarity);

} //namespace vision

#endif // __VISION_API_H__
