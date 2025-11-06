#define _USE_MATH_DEFINE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

 
#define REQUIRED_ARGS \
	REQUIRED_STRING_ARG(input_file, "input", "Input file path") \
	REQUIRED_STRING_ARG(mode, "mode", "Mode for quantization, options are: search, equidistant, self, self-sort")
#define OPTIONAL_ARGS \
    OPTIONAL_STRING_ARG(palette, "nord", "-p", "palette", "Palette used for search and equidistant modes") \
    OPTIONAL_UINT_ARG(resolution, 8, "-r", "resolution", "Amount of colors for self and self-sort modes") \
    OPTIONAL_STRING_ARG(output_file, "", "-o", "output", "Output file path") \

#define BOOLEAN_ARGS \
    BOOLEAN_ARG(help, "-h", "Show help") \
    


#ifdef __cplusplus
extern "C" {  
#endif  

#include "easyargs.h"


#ifdef __cplusplus  
}  
#endif

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

#include "stb_image.h"
#include "stb_image_write.h"
#include "kdtree.h"

#include "src/blur.cpp"
#include "src/palette_parser.cpp"
#include "src/reshaping.cpp"

using namespace std;

string out_name(string const path, string const palette){
	size_t pos = path.rfind(".");
	return path.substr(0, pos) + "_" + palette + path.substr(pos);
}


// QUANTIZATION LOGIC

vector<vector<unsigned char>> retrieve_selected_colors(vector<vector<unsigned char>> &list, int const amount, bool by_brightness = false){

	vector<vector<unsigned char>> out;
	size_t size = list.size();
	
	if (by_brightness){
		vector<unsigned char> brightness_list;
		vector<size_t> brightness_positions;
		sort_color_list(list, &brightness_list); // it has to be sorted because we wanto to capture the minimum element, if we do not sort it we will always capture the first pixel instead, and it might not be the minimum
		for (int i = 0; i < amount - 1; i++){
			for (size_t j = (i * size / (amount - 1)); j < size; j++){
				if (brightness_list[j] >= i * 255 / amount){
					brightness_positions.push_back(j);
					break;
				} 
			}
		}
		
		for (const auto& pos : brightness_positions){
			out.push_back(list[pos]);
		}

		out.push_back(list[size - 1]);
	} else {
		sort_color_list(list);
		for (size_t i = 0; i < amount; i++){
			out.push_back(list[i * size / amount]);
		}
		
	}

	

	return out;
}

bool quantize_2d_vector_to_list(vector<vector<unsigned char>> const &mat, vector<vector<unsigned char>> const &list, vector<vector<unsigned char>> * red, vector<vector<unsigned char>> * green, vector<vector<unsigned char>> * blue){
    size_t color_pos;
    size_t size = list.size();

    for (const auto& row : mat){
        vector<unsigned char> cache_red;
        vector<unsigned char> cache_green;
        vector<unsigned char> cache_blue;
        for (auto& val : row){
            color_pos = (static_cast<float>(val) / 255.0f) * static_cast<float>(size - 1) + 0.5f;
            cache_red.push_back(list[color_pos][0]);
            cache_green.push_back(list[color_pos][1]);
            cache_blue.push_back(list[color_pos][2]);
        }
        
        red->push_back(cache_red);
        green->push_back(cache_green);
        blue->push_back(cache_blue);
    }
    
    
    return true;
}

vector<vector<unsigned char>> quantize_2d_vector_to_self(vector<vector<unsigned char>> mat, unsigned char resolution){
    float cache;
    
    for (auto& row : mat){
        for (auto& val : row){
            cache = floor((static_cast<float>(val) / 255.0f) * static_cast<float>(resolution - 1) + 0.5f);
            // cout << cache << "\n";
            val = (cache * 255.0f) / static_cast<float>(resolution - 1);
        }
        // cout << "\n";
    }
    
    return mat;
}


// EDGE DETECTION LOGIC
const vector<vector<int>> sobel_x = {
	{-1, 0, 1},
	{-2, 0, 2},
	{-1, 0, 1}
};

const vector<vector<int>> sobel_y = {
	{-1, -2, -1},
	{ 0,  0,  0},
	{ 1,  2,  1}
};

template<typename T, typename U>
vector<U> vector_converter(vector<T> const &vec){
	vector<U> out;
	for (auto& element : vec){
		out.push_back(static_cast<U>(element));
	}
	return out;
}

template<typename T, typename U = int>
vector<vector<T>> detect_edges_sobel(vector<vector<T>> const &mat, int normalize_to = 0){
	vector<vector<U>> transformed;
	for (const auto& row : mat){
		transformed.push_back(vector_converter<T, U>(row));
	}

	vector<vector<U>> horizontal = convolve(transformed, sobel_x);
	transformed = convolve(transformed, sobel_y);

	vector<vector<T>> output = mat;

	U max = 0;
	for (size_t i = 0; i < output.size(); i++){
		for (size_t j = 0; j < output[i].size(); j++){
			transformed[i][j] = abs(transformed[i][j]) + abs(horizontal[i][j]);
			if (normalize_to > 0 && transformed[i][j] > max){
				max = transformed[i][j];
			}
		}
	}

	if (normalize_to > 0){
		for (size_t i = 0; i < output.size(); i++){
			for (size_t j = 0; j < output[i].size(); j++){
				output[i][j] = normalize_to * transformed[i][j] / max;
			}
		}
		
	}

	return output;
}



int main(int argc, char* argv[]){
	// parse arguments

	args_t args = make_default_args();
	// parse_args(argc, argv, &args);

	if (!parse_args(argc, argv, &args) || args.help) {
        print_help(argv[0]);
        return 1;
    }

 	string output_file;

 	
		
	// IMPORT
	// vector<vector<unsigned char>> palette_raw = import_palette("../palettes.txt", palette_name);
	// kdtree palette(palette_raw);
	
	int width, height, channels;
	unsigned char const * data = stbi_load(args.input_file, &width, &height, &channels, 0);
	
	if (!data) {
	    cerr << "Failed to load image: " << stbi_failure_reason() << endl;
	    return 1; // or handle error
	}


	// cout << "Size: " << img.size() << "x" << img[0].size() << "x" << img[0][0].size() << endl;

	
	
	vector<vector<unsigned char>> red, green, blue, alpha, grey, out, palette;


	if (string(args.mode) == "search"){
		palette = import_palette<unsigned char>("../palettes.txt", args.palette);
		kdtree<unsigned char> palette_tree(palette);
		vectorize_to_rgb(data, width, height, &red, &green, &blue);
		for (size_t i = 0; i < red.size(); i++){
			for (size_t j = 0; j < red[0].size(); j++){
				vector<unsigned char> cache(3);
				cache[0] = red[i][j];
				cache[1] = green[i][j];
				cache[2] = blue[i][j];

				node<unsigned char> cache_node = palette_tree.nearest(cache);

				red[i][j] = cache_node[0];
				green[i][j] = cache_node[1];
				blue[i][j] = cache_node[2];
			}
		}
		quantize_2d_vector_to_list(grey, palette, &red, &green, &blue);
		output_file = out_name(string(args.input_file), "search_" + string(args.palette));
		
	} else if (string(args.mode) == "equidistant"){
		vectorize_to_greyscale(data, width, height, &grey);
		palette = import_palette<unsigned char>("../palettes.txt", args.palette);
		sort_color_list(palette);
		quantize_2d_vector_to_list(grey, palette, &red, &green, &blue);
		output_file = out_name(string(args.input_file), "equidistant_" + string(args.palette));
	} else if (string(args.mode) == "self"){
		vectorize_to_rgb(data, width, height, &red, &green, &blue);
		red = quantize_2d_vector_to_self(red, args.resolution);
		green = quantize_2d_vector_to_self(green, args.resolution);
		blue = quantize_2d_vector_to_self(blue, args.resolution);
		output_file = out_name(string(args.input_file), "self");
	} else if (string(args.mode) == "self-sort"){
		vector<vector<unsigned char>> color_list = vectorize_to_color_list(data, width, height, channels);
		color_list = retrieve_selected_colors(color_list, args.resolution, true);
		vectorize_to_greyscale(data, width, height, &grey);
		quantize_2d_vector_to_list(grey, color_list, &red, &green, &blue);
		output_file = out_name(string(args.input_file), "self_sort");
	} else if (string(args.mode) == "edge") {
		vectorize_to_rgb(data, width, height, &red, &green, &blue);
		red = detect_edges_sobel(red, 255);
		green = detect_edges_sobel(green, 255);
		blue = detect_edges_sobel(blue, 255);
		output_file = out_name(string(args.input_file), "edges");
	} else {
		print_help(argv[0]);
        return 1;
	}


	if (!(string(args.output_file).empty())){
		output_file = string(args.output_file);
		// cout << "writing image to " + output_file << endl;
	}
	
	unsigned char * output = flatten(&red, &green, &blue);
	
	int error = stbi_write_bmp(output_file.c_str(), width, height, channels, output);

	
	
	    
	return 0;
}
