#ifndef __VISION_H__
#define __VISION_H__
#include"vision_color.h"
#include "vision_feature.h"

namespace vision {

// Core scanners. PC is the pixel layout of the searched bitmap; it flows
// into every pixel-vs-color comparison so RGBA and BGRA screens both run
// through compile-time-specialized code paths.

template<PixelChannels PC,class TColor>
class ColorCounter
{
	TColor mColor;
	int mShift;
	int count;
public:
	ColorCounter(TColor color, int shift)
		:mColor(color), mShift(shift), count(0)
	{
	}
	bool compare(int x, int y, const unsigned char* color)
	{
		if (compareColor<PC>(color,mColor, mShift))
			count++;
		return false;
	}
	int getResult()
	{
		return count;
	}
};

template<PixelChannels PC,class TColor>
class ColorFinder
{
	TColor mColor;
	int mShift;
	Point mPoint;
public:
	ColorFinder(TColor color, int shift)
		:mColor(color), mShift(shift)
	{
	}
	bool compare(int x, int y, const unsigned char* color)
	{
		if (compareColor<PC>(color,mColor, mShift)) {
			mPoint.x = x;
			mPoint.y = y;
			return true;
		}
		return false;
	}
	Point& getResult()
	{
		return mPoint;
	}
};

template <PixelChannels PC>
class FeatureFinder
{
	Bitmap* mBitmap;
	FeatureCompositionRoot* mFeature;
	int mShift;
	Point mPoint;
public:
	FeatureFinder(Bitmap* bitmap,FeatureCompositionRoot* feature, int shift)
		:mBitmap(bitmap),mFeature(feature), mShift(shift)
	{
	}
	bool compare(int x, int y, const unsigned char* color)
	{
		if(isFeature<PC>(mBitmap, x, y, mFeature, mShift)){
			mPoint.x = x;
			mPoint.y = y;
			return true;
		}
		return false;
	}
	Point& getResult()
	{
		return mPoint;
	}
};

Color getColor(Bitmap* bitmap, int x, int y);

template<PixelChannels PC,class TColor>
int getColorCount(Bitmap* bitmap, int x, int y, int x1, int y1, TColor color, int shift)
{
	ColorCounter<PC,TColor> counter(color, shift);
	upDownLeftRightReadColor(bitmap, x, y, x1, y1, &counter);
	return counter.getResult();
}

template<PixelChannels PC,class TColor>
bool findColor(Bitmap* bitmap, int x, int y, int x1, int y1,TColor color, int shift,int order, Point* out)
{
	ColorFinder<PC,TColor> finder(color, shift);
	bool result = orderFindColor(bitmap, x, y, x1, y1, order, &finder);
	if (result && out)
	{
		Point& point = finder.getResult();
		out->x = point.x;
		out->y = point.y;
	}
	return result;
}

template <PixelChannels PC>
bool findFeature(Bitmap* bitmap, int x, int y, int x1, int y1,FeatureCompositionRoot* feature, int shift,int direction,Point* out)
{
	FeatureFinder<PC> finder(bitmap,feature, shift);
	bool result = orderFindColor(bitmap, x, y, x1, y1, direction, &finder);
	if (result && out)
	{
		Point& point = finder.getResult();
		out->x = point.x;
		out->y = point.y;
	}
	return result;
}

}

#endif // __VISION_H__
