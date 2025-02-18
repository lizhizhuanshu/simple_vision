#ifndef __VISION_H__
#define __VISION_H__
#include"vision_color.h"
#include "vision_feature.h"

namespace vision {
template<class TColor,class TShift>
class ColorCounter
{
	TColor mColor;
	TShift mShift;
	int count;
public:
	ColorCounter(TColor color, TShift shift);
	bool compare(int x, int y, const unsigned char* color);
	int getResult();
};

template<class TColor,class TShift>
class ColorFinder
{
	TColor mColor;
	TShift mShift;
	Point mPoint;
public:
	ColorFinder(TColor color, TShift shift);
	bool compare(int x, int y, const unsigned char* color);
	Point& getResult();
};

template<class TFeature,class TShift>
class FeatureFinder
{
	Bitmap* mBitmap;
  TFeature mFeature;
	TShift mShift;
	Point mPoint;
public:
	FeatureFinder(Bitmap* bitmap,TFeature feature,TShift shift);
	bool compare(int x, int y, const unsigned char* color);
	Point& getResult();
};


inline Color getColor(Bitmap* bitmap, int x, int y)
{
	Color color;
	Pixel* pixel = (Pixel*)&color;
	const unsigned char* c = computeCoordColor(bitmap, x, y);
	pixel->g = c[1];
#if UNORDERED_PIXEL
	pixel->r = c[2];
	pixel->b = c[0];
#else
	pixel->r = c[0];
	pixel->b = c[2];
#endif
	return color;
}


template<class TColor,class TShift>
int getColorCount(Bitmap* bitmap, int x, int y, int x1, int y1, TColor color, TShift shift);

template<class T>
int compareColor(Bitmap* bitmap, int x, int y, T color, int colorShiftSum);




template<class TColor,class TShift>
inline ColorCounter<TColor,TShift>::ColorCounter(TColor color, TShift shift)
	:mColor(color), mShift(shift), count(0)
{
}

template<class TColor,class TShift>
bool inline ColorCounter<TColor,TShift>::compare(int x, int y, const unsigned char* color)
{
	if (compareColor(color,mColor, mShift))
		count++;
	return false;
}

template<class TColor,class TShift>
inline int ColorCounter<TColor,TShift>::getResult()
{
	return count;
}




template<class TColor,class TShift>
inline ColorFinder<TColor,TShift>::ColorFinder(TColor color, TShift shift)
	:mColor(color), mShift(shift)
{
}

template<class TColor,class TShift>
inline bool ColorFinder<TColor,TShift>::compare(int x, int y, const unsigned char* color)
{
	if (compareColor(color,mColor, mShift)) {
		mPoint.x = x;
		mPoint.y = y;
		return true;
	}
	return false;
}

template<class TColor,class TShift>
inline Point& ColorFinder<TColor,TShift>::getResult()
{
	return mPoint;
}



template<class TFeature,class TShift>
inline FeatureFinder<TFeature,TShift>::FeatureFinder(
	Bitmap* bitmap,TFeature feature, TShift shift)
	:mBitmap(bitmap),mFeature(feature), mShift(shift)
{
}

template<class TFeature,class TShift>
inline bool FeatureFinder<TFeature,TShift>::compare(int x, int y, const unsigned char* color)
{
	if(isFeature(mBitmap, x, y, mFeature, mShift)){
		mPoint.x = x;
		mPoint.y = y;
		return true;
	}
	return false;
}

template<class TFeature,class TShift>
inline Point& FeatureFinder<TFeature,TShift>::getResult()
{
	return mPoint;
}






template<class TColor,class TShift>
int getColorCount(Bitmap* bitmap, int x, int y, int x1, int y1, TColor color, TShift shift)
{
	ColorCounter<TColor,TShift> counter(color, shift);
	upDownLeftRightReadColor(bitmap, x, y, x1, y1, &counter);
	return counter.getResult();
}

template<class T>
int compareColor(Bitmap* bitmap, int x, int y, T color, int colorShiftSum)
{
	return compareColor(computeCoordColor(bitmap, x, y), color, colorShiftSum);
}

template<class TColor,class TShift>
bool findColor(Bitmap* bitmap, int x, int y, int x1, int y1,TColor color, TShift shift,int order, Point* out)
{
	ColorFinder<TColor,TShift> finder(color, shift);
	bool result = orderFindColor(bitmap, x, y, x1, y1, order, &finder);
	if (result && out)
	{
		Point& point = finder.getResult();
		out->x = point.x;
		out->y = point.y;
	}
	return result;
}


template<class TFeature, class TShift>
bool findFeature(Bitmap* bitmap, int x, int y, int x1, int y1,TFeature feature, TShift shift,int direction,Point* out)
{
	FeatureFinder<TFeature,TShift> finder(bitmap,feature, shift);
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