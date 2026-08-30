#ifndef __VISION_TEST_DATA_H__
#define __VISION_TEST_DATA_H__

// Deterministic in-memory test images.
//
// The reference board is an 8x8 RGBA canvas with four 4x4 quadrants:
//
//     +----+----+
//     | RED|GRN |   red   = (255, 0, 0)
//     +----+----+   green = (0, 255, 0)
//     | BLU |WHT|   blue  = (0, 0, 255)
//     +----+----+   white = (255, 255, 255)
//
// Variants (noisy, gradient, sparse-dots) are derived deterministically from
// it so every test run sees byte-identical data.

#include"Bitmap.h"
#include"CommonBitmap.h"

#include <string>
#include <vector>

namespace vision_test {

using vision::Bitmap;
using vision::CommonBitmap;

constexpr int BOARD_W = 8;
constexpr int BOARD_H = 8;
constexpr int QUAD = 4;

// Owns the pixel buffer for a Bitmap view. Bitmap points into data().
struct Canvas {
	std::vector<unsigned char> pixels;   // RGBA, w*h*4
	Bitmap view{};
	int w = 0;
	int h = 0;

	Canvas() = default;
	Canvas(int width, int height);
	// Reinterpret the same image in BGRA byte order (swaps R and B of
	// every pixel) and flags the view as PIXEL_BGRA.
	Canvas toBGRA() const;
	void fillRect(int x, int y, int w, int h, unsigned r, unsigned g, unsigned b);
	void setPixel(int x, int y, unsigned r, unsigned g, unsigned b);
	void fill(unsigned r, unsigned g, unsigned b);
};

// The reference quadrant board.
Canvas makeBoard();

// Board with `amplitude` of deterministic per-pixel noise added (i-j seeded).
Canvas makeNoisyBoard(int amplitude);

// Red-to-blue horizontal gradient over the full canvas.
Canvas makeGradient();

// 8x8 black canvas with single colored pixels at deterministic positions.
Canvas makeSparseDots();

// Encode/decode round-trip through PNG into a CommonBitmap.
// Returns an unloaded bitmap on failure (check via size).
CommonBitmap encodeToBitmap(const Canvas& c);
bool saveToPng(const Canvas& c, const std::string& path);
CommonBitmap loadFromPng(const std::string& path);

// Tiny fully-colored template (w x h solid color).
Canvas makeSolid(int w, int h, unsigned r, unsigned g, unsigned b);

// Path of a temp directory for file-based tests (created on first use).
const std::string& tempDir();

} //namespace vision_test

#endif // __VISION_TEST_DATA_H__
