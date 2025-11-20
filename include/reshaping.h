#pragma once

#include <vector>
#include "grid.h"

bool vectorize_to_rgb(
	unsigned char const * data, 
	size_t const height, 
	size_t const width,
	grid<int> * red,
	grid<int> * green,
	grid<int> * blue,
	grid<int> * alpha = nullptr
);

grid<int> rgb_to_greyscale(
	grid<int> const &r,
	grid<int> const &g,
	grid<int> const &b
);

std::vector<std::vector<int>> vectorize_to_color_list(
	unsigned char const * data, 
	size_t const size, 
	size_t const channels
);

unsigned char * flatten(
	grid<int> const * red,
	grid<int> const * green = nullptr,
	grid<int> const * blue = nullptr,
	grid<int> const * alpha = nullptr
);
