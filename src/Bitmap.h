#ifndef __VISION_BITMAP_H__
#define __VISION_BITMAP_H__

namespace vision {

// In-memory channel order of the pixel buffer.
//
// PIXEL_RGBA: bytes laid out [R,G,B,(A)] — lodepng decode output,
//             MediaProjection/ImageReader (RGBA_8888) screenshots.
// PIXEL_BGRA: bytes laid out [B,G,R,(A)] — Android framebuffer /
//             screencap on many devices, ANativeWindow buffers.
enum PixelFormat {
	PIXEL_RGBA = 0,
	PIXEL_BGRA = 1,
};

class Bitmap{
public:
	unsigned char* origin_;
	unsigned int width_;
	unsigned int height_;
	int rowShift_;
	int pixelStride_;
	PixelFormat format_ = PIXEL_RGBA;
};

bool isImage(Bitmap*bitmap,int x,int y,Bitmap* templateImage,int shiftSum);

} //namespace vision


#endif// __VISION_BITMAP_H__
