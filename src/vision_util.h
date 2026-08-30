#ifndef __VISION_UTIL_H__
#define __VISION_UTIL_H__

#include"Bitmap.h"
#include <cmath>

namespace vision {

// Pixel-vs-pixel and pixel-vs-color comparisons are templates on the
// channel layout, specialized for RGBA and BGRA at compile time. See
// int-encoded layouts below (RGBA_LAYOUT / BGRA_LAYOUT).

#ifdef USE_JEMALLOC
#include <jemalloc/jemalloc.h>
#define MY_MALLOC(size) je_malloc(size);
#define MY_FREE(ptr) je_free(ptr);
#else
#define MY_MALLOC(size) malloc(size)
#define MY_FREE(ptr) free(ptr)
#endif


struct Point
{
	int x;
	int y;
	Point()
		:x(-1), y(-1)
	{}
	Point(int x, int y)
		:x(x), y(y)
	{}
};

enum READ_ORDER {
	UP_DOWN_LEFT_RIGHT,
	UP_DOWN_RIGHT_LEFT,
	DOWN_UP_LEFT_RIGHT,
	DOWN_UP_RIGHT_LEFT,
	LEFT_RIGHT_UP_DOWN,
	LEFT_RIGHT_DOWN_UP,
	RIGHT_LEFT_UP_DOWN,
	RIGHT_LEFT_DOWN_UP,
};



template<class T1>
static bool upDownLeftRightReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool upDownRightLeftReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool downUpLeftRightReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool downUpRightLeftReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool leftRightUpDownReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool leftRightUpDownReadColor(Bitmap* bitmap, int x, int y, int x1, int y1,int size, T1* comparator);
template<class T1>
static bool rightLeftUpDownReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool leftRightDownUpReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool rightLeftDownUpReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator);
template<class T1>
static bool orderFindColor(Bitmap* bitmap, int x, int y, int x1, int y1, int readOrder, T1* comparator);


static bool isInBitmapScope(Bitmap* bitmap, int x, int y);
static bool isInBitmapScope(Bitmap* bitmap, int x, int y, int x1, int y1);

inline void checkRect(Bitmap* bitmap, int& x, int& y, int& x1, int& y1)
{
	if (x < 0)
		x = 0;
	else if (x >= bitmap->width_)
		x = bitmap->width_ - 1;
	if (x1 < x)
		x1 = x;
	if (y < 0)
		y = 0;
	else if (y >= bitmap->height_)
		y = bitmap->height_ - 1;
	if (y1 < y)
		y1 = y;
}

inline unsigned char* computeCoordColor(Bitmap* bitmap, int x, int y)
{
	return bitmap->origin_ + y * bitmap->rowShift_ + x * bitmap->pixelStride_;
}



// Channel layout as a C++20 class-type non-type template parameter
// (P0732R2). Each field is the byte offset of that logical channel inside
// one pixel: RGBA is 0/1/2, BGRA is 2/1/0. Specializing on this keeps the
// hot loops free of any runtime lookup.
struct PixelChannels {
	int r;
	int g;
	int b;
};

constexpr PixelChannels RGBA_LAYOUT{0, 1, 2};
constexpr PixelChannels BGRA_LAYOUT{2, 1, 0};

// Two raw pixel buffers, each with its own compile-time layout: a BGRA
// screen can be matched against an RGBA template (and vice versa) without
// converting either buffer.
template <PixelChannels PM, PixelChannels PMM>
inline int computeColorShiftSum(const unsigned char* color, const unsigned char* color1)
{
	return (abs(color1[PMM.r] - color[PM.r])
	      + abs(color1[PMM.g] - color[PM.g])
	      + abs(color1[PMM.b] - color[PM.b]));
}

template <PixelChannels PM, PixelChannels PMM>
inline int compareColor(const unsigned char* color, const unsigned char* color1, int colorShiftSum)
{
	return colorShiftSum >= computeColorShiftSum<PM, PMM>(color, color1);
}

template<class T1>
bool upDownLeftRightReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveVerticalPointer;
	const unsigned char* moveLinePointer = bitmap->origin_ + y * bitmap->rowShift_ + x * bitmap->pixelStride_;
	for (int intx = x; intx < x1; intx++)
	{
		moveVerticalPointer = moveLinePointer;
		for (int inty = y; inty < y1; inty++)
		{
			if (comparator->compare(intx, inty, moveVerticalPointer))
				return true;
			moveVerticalPointer += bitmap->rowShift_;
		}
		moveLinePointer += bitmap->pixelStride_;
	}
	return false;
}

template<class T1>
bool upDownRightLeftReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveVerticalPointer;
	const unsigned char* moveLinePointer = bitmap->origin_ + y * bitmap->rowShift_ + (x1-1) * bitmap->pixelStride_;
	for (int intx = x1-1; intx >= x; intx--)
	{
		moveVerticalPointer = moveLinePointer;
		for (int inty = y; inty < y1; inty++)
		{
			if (comparator->compare(intx, inty, moveVerticalPointer))
				return true;
			moveVerticalPointer += bitmap->rowShift_;
		}
		moveLinePointer -= bitmap->pixelStride_;
	}
	return false;
}

template<class T1>
bool downUpLeftRightReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveVerticalPointer;
	const unsigned char* moveLinePointer = bitmap->origin_ + (y1-1) * bitmap->rowShift_ + x * bitmap->pixelStride_;
	for (int intx = x; intx < x1; intx++)
	{
		moveVerticalPointer = moveLinePointer;
		for (int inty = y1-1; inty >= y; inty--)
		{
			if (comparator->compare(intx, inty, moveVerticalPointer))
				return true;
			moveVerticalPointer -= bitmap->rowShift_;
		}
		moveLinePointer += bitmap->pixelStride_;
	}
	return false;
}

template<class T1>
bool downUpRightLeftReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveVerticalPointer;
	const unsigned char* moveLinePointer = bitmap->origin_ + (y1-1) * bitmap->rowShift_ + (x1-1) * bitmap->pixelStride_;
	for (int intx = x1-1; intx >= x; intx--)
	{
		moveVerticalPointer = moveLinePointer;
		for (int inty = y1-1; inty >= y; inty--)
		{
			if (comparator->compare(intx, inty, moveVerticalPointer))
				return true;
			moveVerticalPointer -= bitmap->rowShift_;
		}
		moveLinePointer -= bitmap->pixelStride_;
	}
	return false;
}

template<class T1>
bool leftRightUpDownReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveLinePointer;
	const unsigned char* moveVerticalPointer = bitmap->origin_ + y * bitmap->rowShift_ + x * bitmap->pixelStride_;
	for (int inty = y; inty < y1; inty++)
	{
		moveLinePointer = moveVerticalPointer;
		for (int intx = x; intx < x1; intx++)
		{
			if (comparator->compare(intx, inty, moveLinePointer))
				return true;
			moveLinePointer += bitmap->pixelStride_;
		}
		moveVerticalPointer += bitmap->rowShift_;
	}
	return false;
}

template<class T1>
inline bool leftRightUpDownReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, int size, T1* comparator)
{
	const unsigned char* moveLinePointer;
	const unsigned char* moveVerticalPointer = bitmap->origin_ + y * bitmap->rowShift_ + x * bitmap->pixelStride_;
	int xSize = bitmap->pixelStride_ * size;
	int ySize = bitmap->rowShift_ * size;
	for (int inty = y; inty < y1; inty+=size)
	{
		moveLinePointer = moveVerticalPointer;
		for (int intx = x; intx < x1; intx+= size)
		{
			if (comparator->compare(intx, inty, moveLinePointer))
				return true;
			moveLinePointer += xSize;
		}
		moveVerticalPointer += ySize;
	}
	return false;
}



template<class T1>
bool rightLeftUpDownReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveLinePointer;
	const unsigned char* moveVerticalPointer = bitmap->origin_ + y * bitmap->rowShift_ + (x1-1) * bitmap->pixelStride_;
	for (int inty = y; inty < y1; inty++)
	{
		moveLinePointer = moveVerticalPointer;
		for (int intx = x1-1; intx >= x; intx--)
		{
			if (comparator->compare(intx, inty, moveLinePointer))
				return true;
			moveLinePointer -= bitmap->pixelStride_;
		}
		moveVerticalPointer += bitmap->rowShift_;
	}
	return false;
}

template<class T1>
bool leftRightDownUpReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveLinePointer;
	const unsigned char* moveVerticalPointer = bitmap->origin_ + (y1-1) * bitmap->rowShift_ + x * bitmap->pixelStride_;
	for (int inty = y1-1; inty >= y; inty--)
	{
		moveLinePointer = moveVerticalPointer;
		for (int intx = x; intx < x1; intx++)
		{
			if (comparator->compare(intx, inty, moveLinePointer))
				return true;
			moveLinePointer += bitmap->pixelStride_;
		}
		moveVerticalPointer -= bitmap->rowShift_;
	}
	return false;
}

template<class T1>
bool rightLeftDownUpReadColor(Bitmap* bitmap, int x, int y, int x1, int y1, T1* comparator)
{
	const unsigned char* moveLinePointer;
	const unsigned char* moveVerticalPointer = bitmap->origin_ + (y1-1) * bitmap->rowShift_ + (x1-1) * bitmap->pixelStride_;
	for (int inty = y1-1; inty >= y; inty--)
	{
		moveLinePointer = moveVerticalPointer;
		for (int intx = x1-1; intx >= x; intx--)
		{
			if (comparator->compare(intx, inty, moveLinePointer))
				return true;
			moveLinePointer -= bitmap->pixelStride_;
		}
		moveVerticalPointer -= bitmap->rowShift_;
	}
	return false;
}

template<class T1>
bool orderFindColor(Bitmap* bitmap, int x, int y, int x1, int y1, int readOrder, T1* comparator)
{
	switch (readOrder)
	{
	case UP_DOWN_LEFT_RIGHT:return upDownLeftRightReadColor(bitmap, x, y, x1, y1, comparator);
	case UP_DOWN_RIGHT_LEFT:return upDownRightLeftReadColor(bitmap, x, y, x1, y1, comparator);
	case DOWN_UP_LEFT_RIGHT:return downUpLeftRightReadColor(bitmap, x, y, x1, y1, comparator);
	case DOWN_UP_RIGHT_LEFT:return downUpRightLeftReadColor(bitmap, x, y, x1, y1, comparator);
	case LEFT_RIGHT_UP_DOWN:return leftRightUpDownReadColor(bitmap, x, y, x1, y1, comparator);
	case RIGHT_LEFT_UP_DOWN:return rightLeftUpDownReadColor(bitmap, x, y, x1, y1, comparator);
	case LEFT_RIGHT_DOWN_UP:return leftRightDownUpReadColor(bitmap, x, y, x1, y1, comparator);
	case RIGHT_LEFT_DOWN_UP:return rightLeftDownUpReadColor(bitmap, x, y, x1, y1, comparator);
	}
	return false;
}



inline bool isInBitmapScope(Bitmap* bitmap, int x, int y)
{
	return x >= 0 && x < bitmap->width_ && y >= 0 && y < bitmap->height_;;
}

inline bool isInBitmapScope(Bitmap* bitmap, int x, int y, int x1, int y1)
{
	return x1 > x && y1 > y && x >= 0 && y >= 0 && x1 <= bitmap->width_ && y1 <= bitmap->height_;
}

inline void catCoord(Bitmap* bitmap, int& x, int& y)
{
	if (x < 0)
		x = 0;
	else if (x > bitmap->width_)
		x = bitmap->width_ - 1;
	if (y < 0)
		y = 0;
	else if (y > bitmap->height_)
		y = bitmap->height_ - 1;
}

inline void catScope(Bitmap* bitmap, int& x, int& y, int& x1, int& y1)
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if(x1>bitmap->width_ || x1 == -1)
		x1= bitmap->width_;
	if (y1 >= bitmap->height_ || y1 == -1)
		y1 = bitmap->height_ ;
}


inline int computeCharBit(int c)
{
    if (c <= '9')
        c -= '0';
    else if (c <= 'F')
        c -= 55;
    else
        c -= 87;
    return c;
}

inline int toIntColor(const char* s)
{
    int r = 0;
    r |= (computeCharBit(s[0]) << 20);
    r |= (computeCharBit(s[1]) << 16);
    r |= (computeCharBit(s[2]) << 12);
    r |= (computeCharBit(s[3]) << 8);
    r |= (computeCharBit(s[4]) << 4);
    r |= (computeCharBit(s[5]) << 0);
    return r;
}
}

#endif //__VISION_UTIL_H__
