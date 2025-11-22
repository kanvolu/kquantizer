#define _USE_MATH_DEFINE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

 
#define REQUIRED_ARGS \
	REQUIRED_STRING_ARG(input_file, "input", "Input file path") \
	REQUIRED_STRING_ARG(mode, "mode", "Mode for quantization, options are: search, equidistant, self, self-sort")
#define OPTIONAL_ARGS \
    OPTIONAL_STRING_ARG(palette, "nord", "-p", "palette", "Palette used for search and equidistant modes") \
    OPTIONAL_UINT_ARG(resolution, 8, "-r", "resolution", "Amount of colors for self and self-sort modes") \
    OPTIONAL_UINT_ARG(blur, 0, "-b", "blur", "Blur radius for edge detection based blur before quantization") \
    OPTIONAL_UINT_ARG(antialiasing, 0, "-a", "antialiasing", "Smooth edges after processing") \
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
#include <future>

#include "stb_image.h"
#include "stb_image_write.h"

#include "kdtree.h"
#include "grid.h"

#include "blur.h"
#include "reshaping.h"
#include "palette-parsing.h"

using namespace std;

string out_name(string const path, string const palette){
	size_t pos = path.rfind(".");
	return path.substr(0, pos) + "_" + palette + path.substr(pos);
}


// QUANTIZATION LOGIC

template <typename T>
vector<vector<T>> retrieve_selected_colors(vector<vector<T>> &list, size_t const amount, bool by_brightness = false){

	vector<vector<T>> out;
	size_t size = list.size();
	
	if (by_brightness){
		vector<T> brightness_list;
		vector<size_t> brightness_positions;
		sort_color_list(list, &brightness_list); // it has to be sorted because we want to capture the minimum element, if we do not sort it we will always capture the first pixel instead, and it might not be the minimum
		for (size_t i = 0; i < amount - 1; i++){
			for (size_t j = (i * size / (amount - 1)); j < size; j++){
				if (brightness_list[j] >= int(i) * 255 / int(amount)){
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

template <typename T>
bool quantize_2d_vector_to_list(grid<T> const &mat, 
	vector<vector<T>> const &list, 
	grid<T> * red, 
	grid<T> * green, 
	grid<T> * blue
){
    size_t color_pos;
    size_t size = list.size();

    for (size_t i = 0; i < red->height(); i++){
    	for (size_t j = 0; j < red->width(); j++){
    		color_pos = (static_cast<float>(mat[i][j]) / 255.0f) * static_cast<float>(size - 1) + 0.5f;
    		(*red)[i][j] = list[color_pos][0];
    		(*green)[i][j] = list[color_pos][1];
    		(*blue)[i][j] = list[color_pos][2];
    	}
    }
    
    return true;
}

template <typename T>
grid<T> quantize_2d_vector_to_self(grid<T> mat, size_t resolution){
    float cache;

    for (size_t i = 0; i < mat.height(); i++){
    	for (size_t j = 0; j < mat.width(); j++){
    		cache = floor((static_cast<float>(mat[i][j]) / 255.0f) * static_cast<float>(resolution - 1) + 0.5f);
            mat[i][j] = (cache * 255.0f) / static_cast<float>(resolution - 1);
		}
    }	
    
    return mat;
}




template <typename T>
vector<vector<size_t>> count_repetitions(vector<vector<T>> const &red, 
	vector<vector<T>> const &green, 
	vector<vector<T>> const &blue, 
	vector<vector<T>> const &palette){
	vector<vector<size_t>> counter(palette.size(), vector<size_t>(2, 0));
	for (size_t i = 0; i < palette.size(); i++){
		counter[i][0] = i;
	}
	
	for (size_t i = 0; i < red.size(); i++){
		for (size_t j = 0; j < red[0].size(); j++){
			for (size_t p = 0; p < palette.size(); p++){
				if (red[i][j] == palette[p][0] &&
					green[i][j] == palette[p][1] &&
					blue[i][j] == palette[p][2])
					{
					counter[p][1] += 1;
					break;
				}
			}
		}
	}

	sort(counter.begin(), counter.end(), [](auto a, auto b){
		return a[1] > b[1];
	});

	return counter;
}



int main(int argc, char* argv[]){

	// PARSING ARGS
	args_t args = make_default_args();

	if (!parse_args(argc, argv, &args) || args.help) {
        print_help(argv[0]);
        return 1;
    }

	if (args.resolution < 2){
		cerr << "Resolution must be equal or greater than 2." << endl;
		return 1;
	}


	// IMPORT
	
	int width, height, channels;
 	string output_file;

	unsigned char const * data = stbi_load(args.input_file, &width, &height, &channels, 0);
	
	if (!data) {
	    cerr << "Failed to load image: " << stbi_failure_reason() << endl;
	    return 1; // or handle error
	}
	

	grid<int> red, green, blue, alpha, grey;
	grid<float> edges;
	vector<vector<int>> palette;

	// PREPROCESSING

	if (channels == 3){
		vectorize_to_rgb(data, height, width, &red, &green, &blue);
	} else if (channels == 4){
		vectorize_to_rgb(data, height, width, &red, &green, &blue, &alpha);
	} else {
		cerr << "Image is neither RGB nor RGBA" << endl;
		return 1;
	}
	
	if (args.blur > 0){
		grey = rgb_to_greyscale(red, green, blue);
		edges = detect_edges_sobel(grey);
		grid<float> kernel = g_kernel(2 * args.blur + 1, static_cast<float>(args.blur) / 3.0f);
		
		red = convolve(red, kernel, &edges);
		green = convolve(green, kernel, &edges);
		blue = convolve(blue, kernel, &edges);

		if (!alpha.empty()){
			alpha = convolve(alpha, kernel);
		}
	}

	// PROCESSING
	
	if (string(args.mode) == "search"){
	
		palette = import_palette("../palettes.txt", args.palette);
		kdtree<int> palette_tree(palette);
		//invariants for accessing raw data in the grids since the same process is applied to all pixels
		size_t size = red.size();
		int* __restrict r_raw = red.raw();
		int* __restrict g_raw = green.raw();
		int* __restrict b_raw = blue.raw();
		vector<int> cache(3);

		for (size_t i = 0; i < size; i++){
			cache = palette_tree.nearest({r_raw[i], g_raw[i], b_raw[i]});

			r_raw[i] = cache[0];
			g_raw[i] = cache[1];
			b_raw[i] = cache[2];
		}
		
		output_file = out_name(string(args.input_file), "search_" + string(args.palette));

		
	} else if (string(args.mode) == "equidistant"){
	
		grey = rgb_to_greyscale(red, green, blue);
		palette = import_palette("../palettes.txt", args.palette);
		
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
	
		vector<vector<int>> color_list = vectorize_to_color_list(data, width * height * channels, channels);
		color_list = retrieve_selected_colors(color_list, args.resolution, true);
		
		grey = rgb_to_greyscale(red, green, blue);
		quantize_2d_vector_to_list(grey, color_list, &red, &green, &blue);
		
		output_file = out_name(string(args.input_file), "self_sort");

		
	} else if (string(args.mode) == "bw") {
	
		grey = rgb_to_greyscale(red, green, blue);
		grey = quantize_2d_vector_to_self(grey, args.resolution);

		red = grey;
		green = grey;
		blue = grey;

		output_file = out_name(string(args.input_file), "bw");

		
	} else if (string(args.mode) == "blur"){
		grid<float> kernel = g_kernel(3, 1);
		
		red = convolve(red, kernel);
		green = convolve(green, kernel);
		blue = convolve(blue, kernel);

		output_file = out_name(string(args.input_file), "blur");
	} else if (string(args.mode) == "edges") {
		grey = rgb_to_greyscale(red, green, blue);
		edges = detect_edges_sobel(grey);
		edges *= 255;

		red = edges;
		green = edges;
		blue = edges;

		output_file = out_name(string(args.input_file), "edges");
		
	} else {
	
		print_help(argv[0]);
        return 1;
	}

	// POSTPROCESSING

	if (args.antialiasing > 0){
	
		grey = rgb_to_greyscale(red, green, blue);
		edges = detect_edges_sobel(grey);
		
		grid<float> kernel = g_kernel(2 * args.antialiasing + 1, static_cast<float>(args.antialiasing) / 3.0f);

		red = convolve(red, kernel, &edges);
		green = convolve(green, kernel, &edges);
		blue = convolve(blue, kernel, &edges);

		if (!alpha.empty()){
			alpha = convolve(alpha, kernel, &edges);
		}
	}

	if (!(string(args.output_file).empty())){
		output_file = string(args.output_file);
		// cout << "writing image to " + output_file << endl;
	}

	// EXPORTING
	
	unsigned char * output = flatten(&red, &green, &blue);
	
	int error = stbi_write_bmp(output_file.c_str(), width, height, channels, output);

	if (!error) {
		cerr << "Could not export image." << endl;
		return error;
	}
	    
	return 0;
}
