#include "test_data.h"

#include<lodepng.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
// mkdtemp lives in unistd.h on Apple platforms (glibc leaks it via stdlib.h).
#include <unistd.h>

namespace vision_test {

Canvas::Canvas(int width, int height)
	: pixels((size_t)width * height * 4, 0), w(width), h(height)
{
	view.origin_ = pixels.data();
	view.width_ = width;
	view.height_ = height;
	view.pixelStride_ = 4;
	view.rowShift_ = width * 4;
}

Canvas Canvas::toBGRA() const
{
	Canvas out(w, h);
	for(size_t i = 0; i < pixels.size(); i += 4){
		out.pixels[i + 0] = pixels[i + 2];
		out.pixels[i + 1] = pixels[i + 1];
		out.pixels[i + 2] = pixels[i + 0];
		out.pixels[i + 3] = pixels[i + 3];
	}
	out.view.format_ = vision::PIXEL_BGRA;
	return out;
}

void Canvas::fillRect(int x, int y, int rw, int rh, unsigned r, unsigned g, unsigned b)
{
	for(int j = y; j < y + rh && j < h; j++){
		for(int i = x; i < x + rw && i < w; i++){
			size_t idx = ((size_t)j * w + i) * 4;
			pixels[idx + 0] = (unsigned char)r;
			pixels[idx + 1] = (unsigned char)g;
			pixels[idx + 2] = (unsigned char)b;
			pixels[idx + 3] = 255;
		}
	}
}

void Canvas::setPixel(int x, int y, unsigned r, unsigned g, unsigned b)
{
	fillRect(x, y, 1, 1, r, g, b);
}

void Canvas::fill(unsigned r, unsigned g, unsigned b)
{
	fillRect(0, 0, w, h, r, g, b);
}

Canvas makeBoard()
{
	Canvas c(BOARD_W, BOARD_H);
	c.fillRect(0, 0, QUAD, QUAD, 255, 0, 0);       // top-left red
	c.fillRect(QUAD, 0, QUAD, QUAD, 0, 255, 0);    // top-right green
	c.fillRect(0, QUAD, QUAD, QUAD, 0, 0, 255);    // bottom-left blue
	c.fillRect(QUAD, QUAD, QUAD, QUAD, 255, 255, 255); // bottom-right white
	return c;
}

Canvas makeNoisyBoard(int amplitude)
{
	Canvas c = makeBoard();
	// deterministic signed noise in [-amplitude, +amplitude], period 7
	for(int j = 0; j < c.h; j++){
		for(int i = 0; i < c.w; i++){
			int n = ((i * 31 + j * 17) % (2 * amplitude + 1)) - amplitude;
			size_t idx = ((size_t)j * c.w + i) * 4;
			for(int ch = 0; ch < 3; ch++){
				int v = c.pixels[idx + ch] + n;
				if(v < 0) v = 0;
				if(v > 255) v = 255;
				c.pixels[idx + ch] = (unsigned char)v;
			}
		}
	}
	return c;
}

Canvas makeGradient()
{
	Canvas c(BOARD_W, BOARD_H);
	for(int j = 0; j < c.h; j++){
		for(int i = 0; i < c.w; i++){
			unsigned r = (unsigned)(255 * i / (c.w - 1));
			unsigned b = 255 - r;
			c.setPixel(i, j, r, 0, b);
		}
	}
	return c;
}

Canvas makeSparseDots()
{
	Canvas c(BOARD_W, BOARD_H);
	c.fill(0, 0, 0);
	c.setPixel(1, 1, 255, 0, 0);
	c.setPixel(6, 2, 0, 255, 0);
	c.setPixel(3, 5, 0, 0, 255);
	c.setPixel(7, 7, 255, 255, 0);
	return c;
}

CommonBitmap encodeToBitmap(const Canvas& c)
{
	std::vector<unsigned char> png;
	unsigned error = lodepng::encode(png, c.pixels, (unsigned)c.w, (unsigned)c.h);
	CommonBitmap bmp;
	if(error){
		return bmp;
	}
	bmp.load(png.data(), (unsigned)png.size());
	return bmp;
}

bool saveToPng(const Canvas& c, const std::string& path)
{
	unsigned error = lodepng::encode(path, c.pixels, (unsigned)c.w, (unsigned)c.h);
	return error == 0;
}

CommonBitmap loadFromPng(const std::string& path)
{
	CommonBitmap bmp;
	bmp.load(path.c_str());
	return bmp;
}

Canvas makeSolid(int w, int h, unsigned r, unsigned g, unsigned b)
{
	Canvas c(w, h);
	c.fill(r, g, b);
	return c;
}

const std::string& tempDir()
{
	static std::string dir;
	if(dir.empty()){
		const char* tmpl = "/tmp/simple_vision_test_XXXXXX";
		char buf[64];
		snprintf(buf, sizeof(buf), "%s", tmpl);
		char* made = mkdtemp(buf);
		dir = made ? made : "/tmp";
	}
	return dir;
}

} //namespace vision_test
