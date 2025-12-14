#define _USE_MATH_DEFINE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

 
#define REQUIRED_ARGS \
	REQUIRED_STRING_ARG(input_file, "input", "Input file path") \
	REQUIRED_STRING_ARG(mode, "mode", "Mode for quantization, options are: search, equidistant, self, self-sort, bw")
#define OPTIONAL_ARGS \
    OPTIONAL_STRING_ARG(palette, "nord", "-p", "palette", "Palette used for search and equidistant modes") \
    OPTIONAL_UINT_ARG(resolution, 8, "-r", "resolution", "Amount of colors for self and self-sort modes") \
    OPTIONAL_UINT_ARG(blur, 0, "-b", "blur", "Blur radius for edge detection based blur before quantization") \
    OPTIONAL_UINT_ARG(antialiasing, 0, "-a", "antialiasing", "Smooth edges after processing") \
    OPTIONAL_UINT_ARG(quality, 80, "-q", "quality", "Number between 1 and 100 for quality to export .jpg images") \
    OPTIONAL_STRING_ARG(output_file, "", "-o", "output", "Output file path") \

#define BOOLEAN_ARGS \
    BOOLEAN_ARG(help, "-h", "Show help") \
    BOOLEAN_ARG(print, "--print", "Print processed image to the console without saving it unless '-o' is also passed") \
    


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
#include <filesystem>
#include <sys/ioctl.h>

#include "stb_image.h"
#include "stb_image_write.h"

#include "kdtree.h"
#include "grid.h"

#include "blur.h"
#include "reshaping.h"
#include "palette-parsing.h"

using namespace std;


filesystem::path out_name(filesystem::path path, string const &append){
	filesystem::path filename = path.stem();
	return path.replace_filename(path.stem().string() + "_" + append + path.extension().string());
}

void print_image(int const height, int const width, int const channels, unsigned char const * data) {
	//encoding in base64
	char constexpr b64_table[65] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
        
	size_t const data_length = height * width * channels;
	size_t const encoded_length = ((data_length + 2) / 3) * 4;

	string encoded;
	encoded.reserve(encoded_length);

	size_t i;
	for (i = 0; i + 2  < data_length; i += 3) {
		encoded.push_back(
			b64_table[data[i] >> 2]);
		encoded.push_back(
			b64_table[(data[i] & 0x03) << 4 | data[i+1] >> 4]);
		encoded.push_back(
			b64_table[(data[i+1] & 0x0F) << 2 | data[i+2] >> 6]);
		encoded.push_back(
			b64_table[data[i+2] & 0x3F]);
	}

	if (i + 1 < data_length) {
		encoded.push_back(
			b64_table[data[i] >> 2]);
		encoded.push_back(
			b64_table[(data[i] & 0x03) << 4 | data[i+1] >> 4]);
		encoded.push_back(
			b64_table[(data[i+1] & 0x0F) << 2]);
		encoded.push_back('=');
	} else if (i < data_length) {
		encoded.push_back(
			b64_table[data[i] >> 2]);
		encoded.push_back(
			b64_table[(data[i] & 0x03) << 4]);
		encoded.push_back('=');
		encoded.push_back('=');
	} 

	// getting window dimensions

	winsize window;

	if (ioctl(0, TIOCGWINSZ, &window) == -1) {
        perror("Could not retrieve terminal dimensions to print image");
        return;
    }

    size_t p_height = window.ws_row;
    size_t p_width = window.ws_col;

    if (height < width) {
    	p_height = height * p_width / (2 * width);  // it must be multiplied by two since rows are double the height of columns
    } else {
    	p_width = 2 * width * p_height / height;
    }

	// transmitting one chunk at a time
	size_t const total = encoded.size();
	size_t offset = 0;

	while (offset < total) {
		size_t constexpr chunk_size = 4000;
		size_t const count = min(chunk_size, total - offset);
		string chunk = encoded.substr(offset, count);

		cout << "\033_Gq=1,a=T,i=1,m=" << (count == chunk_size ? 1 : 0)
			<< ",f=" << channels * 8
			<< ",s=" << width
			<< ",v=" << height
			<< ",c=" << p_width
			<< ",r=" << p_height
			<< ";" << chunk
			<< "\033\\"
			<< flush;

		offset += count;
	}

	cout << endl;

}

bool is_extension_supported(filesystem::path const &path) {
	filesystem::path const extension = path.extension();
	return extension == ".png" ||
		extension == ".bmp" ||
		extension == ".dib" ||
		extension == ".tga" ||
		extension == ".icb" ||
		extension == ".vda" ||
		extension == ".jpg" ||
		extension == ".jpeg" ||
		extension == ".jpe" ||
		extension == ".jif" ||
		extension == ".jfif" ||
		extension == ".jfi";
}


// QUANTIZATION LOGIC

vector<vector<int>> retrieve_selected_colors(vector<vector<int>> &list, size_t const amount, bool const by_brightness = false){

	vector<vector<int>> out;
	size_t const size = list.size();
	
	if (by_brightness){
		vector<int> brightness_list;
		vector<size_t> brightness_positions;
		sort_color_list(list, &brightness_list); // it has to be sorted because we want to capture the minimum element, if we do not sort it we will always capture the first pixel instead, and it might not be the minimum
		for (size_t i = 0; i < amount - 1; i++){
			for (size_t j = i * size / (amount - 1); j < size; j++){
				if (brightness_list[j] >= static_cast<int>(i) * 255 / static_cast<int>(amount)){
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


bool quantize_2d_vector_to_list(Grid<int> const &mat,
	vector<vector<int>> const &list, 
	Grid<int> * red,
	Grid<int> * green,
	Grid<int> * blue
){
    size_t const size = list.size();

    for (size_t i = 0; i < red->height(); i++){
    	for (size_t j = 0; j < red->width(); j++){
    		size_t const color_pos = static_cast<float>(mat[i][j]) / 255.0f * static_cast<float>(size - 1) + 0.5f;
    		(*red)[i][j] = list[color_pos][0];
    		(*green)[i][j] = list[color_pos][1];
    		(*blue)[i][j] = list[color_pos][2];
    	}
    }
    
    return true;
}


Grid<int> quantize_2d_vector_to_self(Grid<int> mat, size_t const resolution){

    for (size_t i = 0; i < mat.height(); i++){
    	for (size_t j = 0; j < mat.width(); j++){
    		float const cache = floor(static_cast<float>(mat[i][j]) / 255.0f * static_cast<float>(resolution - 1) + 0.5f);
            mat[i][j] = cache * 255.0f / static_cast<float>(resolution - 1);
		}
    }	
    
    return mat;
}


int main(int argc, char* argv[]){

	// PARSING ARGS
	args_t args = make_default_args();

	if (!parse_args(argc, argv, &args) || args.help) {
        print_help(argv[0]);
        return 1;
    }

	if (args.resolution < 2){
		cerr << "Resolution must be equal or greater than 2" << endl;
		return 1;
	}

	filesystem::path const input_file = args.input_file;
	if (!is_extension_supported(input_file)) {
		cout << "Image format not supported" << endl;
		return 1;
	}

	// IMPORT
	
	int width, height, channels;
 	filesystem::path output_file;

	unsigned char const * data = stbi_load(filesystem::path(args.input_file).c_str(), &width, &height, &channels, 0);
	
	if (!data) {
	    cerr << "Failed to load image: " << stbi_failure_reason() << endl;
	    return 1;
	}
	

	Grid<int> red, green, blue, alpha, grey;
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
		Grid<float> edges = 1 - detect_edges_sobel(grey);
		Grid<float> kernel = g_kernel(2 * args.blur + 1, static_cast<float>(args.blur) / 1.5f);
		
		red = red.convolve(kernel, edges);
		green = green.convolve(kernel, edges);
		blue = blue.convolve(kernel, edges);

		if (!alpha.empty()){
			alpha = alpha.convolve(kernel);
		}
	}


	// PROCESSING
	
	if (string(args.mode) == "search"){ // TODO make resolution work by finding the farthest points apart from eachother in the 3D set that is the palette
	
		palette = import_palette(args.palette);
		if (palette.empty()) return 1;
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
		
		output_file = out_name(input_file, "search_" + string(args.palette));

		
	} else if (string(args.mode) == "equidistant"){
	
		grey = rgb_to_greyscale(red, green, blue);
		palette = import_palette(args.palette);
		if (palette.empty()) return 1;
		
		sort_color_list(palette);
		quantize_2d_vector_to_list(grey, palette, &red, &green, &blue);
		
		output_file = out_name(input_file, "equidistant_" + string(args.palette));

		
	} else if (string(args.mode) == "self"){
	
		vectorize_to_rgb(data, width, height, &red, &green, &blue);

		red = quantize_2d_vector_to_self(red, args.resolution);
		green = quantize_2d_vector_to_self(green, args.resolution);
		blue = quantize_2d_vector_to_self(blue, args.resolution);

		output_file = out_name(input_file, "self");

		
	} else if (string(args.mode) == "self-sort"){
	
		vector<vector<int>> color_list = vectorize_to_color_list(data, width * height * channels, channels);
		color_list = retrieve_selected_colors(color_list, args.resolution, true);
		
		grey = rgb_to_greyscale(red, green, blue);
		quantize_2d_vector_to_list(grey, color_list, &red, &green, &blue);
		
		output_file = out_name(input_file, "self_sort");

		
	} else if (string(args.mode) == "bw") {
	
		grey = rgb_to_greyscale(red, green, blue);
		grey = quantize_2d_vector_to_self(grey, args.resolution);

		red = grey;
		green = grey;
		blue = grey;

		output_file = out_name(input_file, "bw");

		
// 	} else if (string(args.mode) == "blur"){
// 		grid<float> kernel = g_kernel(3, 1);
// 		
// 		red = red.convolve(kernel);
// 		green = green.convolve(kernel);
// 		blue = blue.convolve(kernel);
// 
// 		output_file = out_name(input_file, "blur");
// 	} else if (string(args.mode) == "edges") {
// 		grey = rgb_to_greyscale(red, green, blue);
// 		edges = detect_edges_sobel(grey);
// 		edges *= 255;
// 
// 		red = edges;
// 		green = edges;
// 		blue = edges;
// 
// 		output_file = out_name(input_file, "edges");
		
	} else {
		print_help(argv[0]);
        return 1;
	}

	delete[] data;
	
	// POSTPROCESSING

	if (args.antialiasing > 0){ // TODO make actual antialiasing
		grey = rgb_to_greyscale(red, green, blue);
		Grid<float> edges_h = detect_edges_horizontal(grey);
		Grid<float> edges_v = detect_edges_vertical(grey);
	}

	
	if (args.output_file[0] != '\0'){
		output_file = filesystem::path(args.output_file);
	}

	// EXPORTING

	vector<unsigned char> output;

	if (channels == 4) {
		output = flatten(&red, &green, &blue, &alpha);
	} else {
		output = flatten(&red, &green, &blue);
	}

	if (args.print) print_image(height, width, channels, output.data());

	if (!args.print || !string(args.output_file).empty()) {

		int error;

		// TODO maybe use enums for mode selection and export selection

		if (filesystem::path extension = output_file.extension(); extension == ".png") {
			error = stbi_write_png(output_file.c_str(), width, height, channels, output.data(), 0);
		} else if (extension == ".bmp" || extension == ".dib") {
			error = stbi_write_bmp(output_file.c_str(), width, height, channels, output.data());
		} else if (extension == ".tga" || extension == ".icb" || extension == ".vda") {
			error = stbi_write_tga(output_file.c_str(), width, height, channels, output.data());
		} else if (extension == ".jpg" || extension == ".jpeg" || extension == ".jpe" || extension == ".jif" || extension == ".jfif" || extension == ".jfi") {
			error = stbi_write_jpg(output_file.c_str(), width, height, channels, output.data(), static_cast<int>(args.quality));
		} else if (extension == ".hdr") {
			// TODO Make exporting to hdr work
			cout << "Exporting to HDR does not work yet" << endl;
			error = 1;
		} else {
			cout << "Format of the image to export could not be recognized\n" << "Exporting as png..." << endl;
			error = stbi_write_png(output_file.replace_extension(".png").c_str(), width, height, channels, output.data(), 0);
		}
		
		if (!error) {
			cerr << "Could not export image." << endl;
			return error;
		}

	}
	    
	return 0;
}
