#pragma once

#include <iostream>
#include <vector>

template <typename T>
bool vectorize_to_rgb(T const * data, size_t const width, size_t const height, std::vector<std::vector<T>> * red, std::vector<std::vector<T>> * green, std::vector<std::vector<T>> * blue, std::vector<std::vector<T>> * alpha = nullptr){
    if (alpha == nullptr){
        int const channels = 3;
        for (size_t y = 0; y < height; y++){
            std::vector<T> cache_red;
            std::vector<T> cache_green;
            std::vector<T> cache_blue;
            for (size_t x = 0; x < width; x++){
                cache_red.push_back(data[(y * width + x) * channels + 0]);
                cache_green.push_back(data[(y * width + x) * channels + 1]);
                cache_blue.push_back(data[(y * width + x) * channels + 2]);
            }
            red->push_back(cache_red);
            green->push_back(cache_green);
            blue->push_back(cache_blue);
        }
    } else {
        int const channels = 4;
        for (size_t y = 0; y < height; y++){
            std::vector<T> cache_red;
            std::vector<T> cache_green;
            std::vector<T> cache_blue;
            std::vector<T> cache_alpha;
            for (size_t x = 0; x < width; x++){
                cache_red.push_back(data[(y * width + x) * channels + 0]);
                cache_green.push_back(data[(y * width + x) * channels + 1]);
                cache_blue.push_back(data[(y * width + x) * channels + 2]);
                cache_alpha.push_back(data[(y * width + x) * channels + 3]);
            }
            red->push_back(cache_red);
            green->push_back(cache_green);
            blue->push_back(cache_blue);
            alpha->push_back(cache_alpha);
        }
    }

    
    
    return true;
}

template <typename T>
bool vectorize_to_greyscale(T const * data, size_t const width, size_t const height, std::vector<std::vector<T>> * grey, std::vector<std::vector<T>> * alpha = nullptr){
    int max = 0;
    char cur;
    size_t cur_pos;

    if (alpha == nullptr){
    	int channels = 3;
	    for (size_t y = 0; y < height; y++){
	        std::vector<T> cache_grey;
	        for (size_t x = 0; x < width; x++){
	            cur_pos = (y * width + x) * channels;
	            cur = (data[cur_pos] + data[cur_pos + 1] + data[cur_pos + 2]) / 3;
	            if (max < cur){
	                max = cur;
	            }
	            cache_grey.push_back(cur);
	        }
	        grey->push_back(cache_grey);
	    }
    } else {
    	int channels = 4;
    	for (size_t y = 0; y < height; y++){
	        std::vector<T> cache_grey;
	        std::vector<T> cache_alpha;
	        for (size_t x = 0; x < width; x++){
	            cur_pos = (y * width + x) * channels;
	            cur = (data[cur_pos] + data[cur_pos + 1] + data[cur_pos + 2]) / 3;
	            if (max < cur){
	                max = cur;
	            }
	            cache_grey.push_back(cur);
	            cache_alpha.push_back(data[cur_pos + 3]);
	        }
	        grey->push_back(cache_grey);
	        alpha->push_back(cache_alpha);
	    }
    	
    }
    
    return true;
}


template <typename T>
std::vector<std::vector<T>> vectorize_to_color_list(T const * data, size_t const width, size_t const height, int const channels){
	std::vector<std::vector<T>> out;
	for (size_t i = 0; i < width * height * channels; i += channels){
		out.push_back({
			data[i + 0],
			data[i + 1],
			data[i + 2]
			});
	}
	

	return out;
}

template <typename T>
T * flatten(std::vector<std::vector<T>> const * red, std::vector<std::vector<T>> const * green = nullptr, std::vector<std::vector<T>> const * blue = nullptr, std::vector<std::vector<T>> const * alpha = nullptr){
    // the output array must be deleted afterwards
    
    size_t height = (*red).size();
    size_t width = (*red)[0].size();
    int channels;
    std::vector<std::vector<unsigned char>> const * channel[4] = {red, green, blue, alpha};

	if (green == nullptr){
		channels = 1;
	} else if (blue == nullptr){
		channels = 2;
	} else if (alpha == nullptr){
        channels = 3;
    } else {
        channels = 4;
    }


    T * out = new T[width * height * channels];

    for (size_t y = 0; y < height; y++){
        for (size_t x = 0; x < width; x++){
        	for (size_t c = 0; c < channels; c++){
            	out[(y * width + x) * channels + c] = (*(channel[c]))[y][x];
        	}
        }
    }
    
    return out;
}
