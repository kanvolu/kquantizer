#pragma once

#include <vector>
#include "grid.h"

bool vectorize_to_rgb(
	unsigned char const * data, 
	size_t const height, 
	size_t const width,
	Grid<int> * red,
	Grid<int> * green,
	Grid<int> * blue,
	Grid<int> * alpha = nullptr
);

Grid<int> rgb_to_greyscale(
	Grid<int> const &r,
	Grid<int> const &g,
	Grid<int> const &b
);

std::vector<std::vector<int>> vectorize_to_color_list(
	unsigned char const * data, 
	size_t const size, 
	size_t const channels
);

std::vector<unsigned char> flatten(
	Grid<int> const * red,
	Grid<int> const * green = nullptr,
	Grid<int> const * blue = nullptr,
	Grid<int> const * alpha = nullptr
);
