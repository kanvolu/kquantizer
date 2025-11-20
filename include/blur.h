#pragma once

#include "grid.h"

inline const grid<int> sobel_x(3, 3,
	{
	-1, 0, 1,
	-2, 0, 2,
	-1, 0, 1
	}
);

inline const grid<int> sobel_y(3, 3,
	{
	-1, -2, -1,
	 0,  0,  0,
	 1,  2,  1
	}
);

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
