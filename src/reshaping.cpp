#include <iostream>
#include <vector>

#include "grid.h"
#include "reshaping.h"

bool vectorize_to_rgb(
	unsigned char const * data, size_t const height, size_t const width,
	grid<int> * red,
	grid<int> * green,
	grid<int> * blue,
	grid<int> * alpha
){
	if (red == nullptr || green == nullptr || blue == nullptr){
		std::cerr << "Trying to pass nullptr for RGB arrays" << std::endl;
		return false;
	}
	
	grid<int> * const grids[4] = {red, green, blue, alpha};
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


grid<int> rgb_to_greyscale(
	grid<int> const &r,
	grid<int> const &g,
	grid<int> const &b
){
	if (
		r.height() != g.height() || r.height() != b.height() ||
		r.width() != g.width() || r.width() != b.width()
	){
		std::cerr << "Grids to convert to greyscale have different sizes." << std::endl;
		return {};
	}
	
	grid<int> out(r.height(), r.width());
	
	for (size_t i = 0; i < out.height(); i++){
		for (size_t j = 0; j < out.width(); j++){
			out[i][j] = (r[i][j] + g[i][j] + b[i][j]) / 3;
		}
	}
	
	return out;
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


unsigned char * flatten(
	grid<int> const * red,
	grid<int> const * green,
	grid<int> const * blue,
	grid<int> const * alpha
){
    // the output array must be deleted afterwards
    
    size_t channels;
    grid<int> const * const grids[4] = {red, green, blue, alpha};

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
    		return nullptr;
    	}
    }

    unsigned char * out = new unsigned char[red->height() * red->width() * channels];

    for (size_t i = 0; i < red->height(); i++){
		for (size_t j = 0; j < red->width(); j++){
			for (size_t c = 0; c < channels; c++){
				out[(i * red->width() + j) * channels + c] = (*(grids[c]))[i][j];
			}
		}
	}
    
    return out;
}
