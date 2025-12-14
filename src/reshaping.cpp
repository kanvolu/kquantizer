#include <iostream>
#include <vector>

#include "grid.h"
#include "reshaping.h"

bool vectorize_to_rgb(
	unsigned char const * data, size_t const height, size_t const width,
	Grid<int> * red,
	Grid<int> * green,
	Grid<int> * blue,
	Grid<int> * alpha
){
	if (red == nullptr || green == nullptr || blue == nullptr){
		std::cerr << "Trying to pass nullptr for RGB arrays" << std::endl;
		return false;
	}
	
	Grid<int> * const grids[4] = {red, green, blue, alpha};
	size_t channels = (alpha == nullptr) ? 3 : 4;

	for (size_t c = 0; c < channels; c++){
		grids[c]->reshape_raw(height, width);
	}

	for (size_t i = 0; i < height; i++){
		for (size_t j = 0; j < width; j++){
			for (size_t c = 0; c < channels; c++){
				(*(grids[c]))[i][j] = data[(i * width + j) * channels + c];
			}
		}
	}

	return true;
}


Grid<int> rgb_to_greyscale(
	Grid<int> const &r,
	Grid<int> const &g,
	Grid<int> const &b
){	
	return (r + g + b) / 3;
}


std::vector<std::vector<int>> vectorize_to_color_list(unsigned char const * data, size_t const size, size_t const channels){
	std::vector<std::vector<int>> out(size / channels, std::vector<int>(channels));
	
	for (size_t i = 0; i < size / channels; i++){
		for (size_t j = 0; j < channels; j++){
			out[i][j] = data[i * channels + j];
		}
	}

	return out;
}


std::vector<unsigned char> flatten(
	Grid<int> const * red,
	Grid<int> const * green,
	Grid<int> const * blue,
	Grid<int> const * alpha
){
    // the output array must be deleted afterwards
    
    size_t channels;
    Grid<int> const * const grids[4] = {red, green, blue, alpha};

	if (green == nullptr){
		channels = 1;
	} else if (blue == nullptr){
		channels = 2;
	} else if (alpha == nullptr){
        channels = 3;
    } else {
        channels = 4;
    }

    for (size_t c = 0; c < channels; c++){
    	if (red->height() != grids[c]->height() || red->width() != grids[c]->width()){
    		std::cerr << "Grids to flatten have different sizes." << std::endl;
    		return {};
    	}
    }

    std::vector<unsigned char> out(red->height() * red->width() * channels);

    for (size_t i = 0; i < red->height(); i++){
		for (size_t j = 0; j < red->width(); j++){
			for (size_t c = 0; c < channels; c++){
				out[(i * red->width() + j) * channels + c] = (*(grids[c]))[i][j];
			}
		}
	}
    
    return out;
}
