#pragma once

#include "grid.h"

inline const std::vector<int> sobel_xv = {
	1,
	2,
	1
};

inline const std::vector<int> sobel_xh = {
	-1, 0, 1
};

inline const std::vector<int> &sobel_yv = sobel_xh;

inline const std::vector<int> &sobel_yh = sobel_xv;


float gaus(
	float x, 
	float deviation
);

grid<float> g_kernel(
	size_t size, 
	float deviation
);

grid<int> convolve (
	grid<int> mat, 
	grid<float> const &kernel, 
	grid<float> const * mask = nullptr
);

grid<int> dog(
	grid<int> const mat, 
	float s_sigma
);

grid<float> detect_edges_sobel(
	grid<int> const &mat
);
