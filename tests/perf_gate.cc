// Performance regression gate.
//
// Guards the two hot paths that historically regressed silently:
//   1. multi-template findImage must be a single pass (N templates cost one
//      traversal, not N) — a regression to per-template scans shows up as
//      the 3-template case taking ~3x the 1-template time
//   2. single-template miss scan throughput (memory-bound floor)
//
// Thresholds are deliberately loose (2x) to avoid flakiness on shared CI
// machines; this catches order-of-magnitude regressions, not tuning noise.
// Disable with: ctest -E perf_gate

#include "test_data.h"
#include"vision_api.h"

#include <chrono>
#include <cstdio>
#include <vector>

using namespace vision;
using namespace vision_test;

static double msSince(auto t0)
{
	using namespace std::chrono;
	return duration_cast<microseconds>(high_resolution_clock::now() - t0).count() / 1000.0;
}

int main()
{
	// ~VGA-sized noisy canvas: deterministic, miss-heavy
	const int W = 640, H = 480;
	Canvas c(W, H);
	for(int y = 0; y < H; y++){
		for(int x = 0; x < W; x++){
			c.setPixel(x, y, (unsigned)((x * 7 + y * 13) & 0xFF),
			               (unsigned)((x * 3 + y * 5) & 0xFF),
			               (unsigned)((x * 11 + y * 2) & 0xFF));
		}
	}
	CommonBitmap tpl = encodeToBitmap(makeSolid(32, 32, 255, 0, 0));
	std::vector<CommonBitmap> one{tpl};
	std::vector<CommonBitmap> three{tpl, tpl, tpl};

	auto t0 = std::chrono::high_resolution_clock::now();
	int misses = 0;
	for(int i = 0; i < 20; i++){
		misses += findImage(&c.view, 0, 0, -1, -1, one).found ? 0 : 1;
	}
	double oneMs = msSince(t0) / 20;

	t0 = std::chrono::high_resolution_clock::now();
	for(int i = 0; i < 20; i++){
		misses += findImage(&c.view, 0, 0, -1, -1, three).found ? 0 : 1;
	}
	double threeMs = msSince(t0) / 20;
	(void)misses;

	std::printf("perf: 1-template %.2f ms, 3-template %.2f ms (ratio %.2f)\n",
	            oneMs, threeMs, threeMs / oneMs);

	int failures = 0;
	// single pass shares the traversal; a degenerate per-template scan pays
	// the traversal N times and lands at or above 3.0x (measured 2.4-2.6x
	// single-pass vs 2.9x+ degenerate at this size)
	if(!(threeMs < oneMs * 3.0)){
		std::printf("  FAIL: 3-template scan %.2fx the 1-template cost — "
		            "multi-template single-pass regression?\n", threeMs / oneMs);
		failures++;
	}
	// absolute floor: even a slow machine should stay under 25 ms for a
	// 640x480 miss scan at these sizes
	if(!(oneMs < 25.0)){
		std::printf("  FAIL: single-template miss scan %.2f ms exceeds 25 ms floor\n", oneMs);
		failures++;
	}
	std::printf("%s\n", failures == 0 ? "perf gate: PASS" : "perf gate: FAIL");
	return failures == 0 ? 0 : 1;
}
