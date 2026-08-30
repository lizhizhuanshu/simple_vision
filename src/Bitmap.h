#ifndef __VISION_BITMAP_H__
#define __VISION_BITMAP_H__

#include <vector>

namespace vision {

class Bitmap;

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

// Template matching. Screen and template layouts may differ (BGRA screen x
// RGBA template decoded from PNG is the classic Android mix); the pair is
// dispatched once per call, not per candidate position.
bool isImage(Bitmap*bitmap,int x,int y,Bitmap* templateImage,int shiftSum);

// Multi-template scan support: try every template at a candidate screen
// position, first match wins. Defined in vision_util.h (MultiTemplateMatcher)
// so the finder can be constructed on the stack and fully inlined into the
// scan loop.

} //namespace vision


#endif// __VISION_BITMAP_H__
