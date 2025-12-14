#pragma once

#include "grid.h"

float gaus(
	float x, 
	float deviation
);

Grid<float> g_kernel(
	size_t size, 
	float deviation
);

Grid<int> convolve (
	Grid<int> mat, 
	Grid<float> const &kernel, 
	Grid<float> const * mask = nullptr
);

Grid<int> dog(
	Grid<int> const mat, 
	float s_sigma
);

Grid<float> detect_edges_sobel(
	Grid<float> const &mat
);

Grid<float> detect_edges_horizontal(
	Grid<float> const &mat
);

Grid<float> detect_edges_vertical(
	Grid<float> const &mat
);
