#ifndef __VISION_BITMAP_H__
#define __VISION_BITMAP_H__
namespace vision {
class Bitmap{
public:
	unsigned char* origin_;
	unsigned int width_;
	unsigned int height_;
	int rowShift_;
	int pixelStride_;
};

bool isImage(Bitmap*bitmap,int x,int y,Bitmap* templateImage,int shiftSum);

} //namespace vision


#endif// __VISION_BITMAP_H__

