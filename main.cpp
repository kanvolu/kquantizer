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
    BOOLEAN_ARG(dry, "--dry", "Run the program without saving the processed image") \

#include <thread>


#ifdef __cplusplus
extern "C" {  
#endif  

#include "easyargs.h"
#include "stb_image.h"
#include "stb_image_write.h"

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
#include <array>

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

vector<unsigned char> resize_image(vector<unsigned char> const &data, size_t const old_height, size_t const old_width, size_t const new_height, size_t const new_width, size_t const channels) {
	size_t const new_size = new_height * new_width * channels;
	float const v_proportion = static_cast<float>(old_height) / static_cast<float>(new_height);
	float const h_proportion = static_cast<float>(old_width) / static_cast<float>(new_width);
	vector<unsigned char> output(new_size);

	for (size_t i = 0; i < new_height; i++) {
		size_t new_i = i * new_width;
		size_t old_i = i * old_width * v_proportion;
		for (size_t j = 0; j < new_width; j++) {
			size_t old_j = j * h_proportion;
			for (size_t c = 0; c < channels; c++) {
				output[(new_i + j) * channels + c] = data[(old_i + old_j) * channels + c];
			}
		}
	}

	return output;
}

void print_image(int const height, int const width, int const channels, vector<unsigned char> const &data) {
	//resizing in case the original image is too big
	size_t constexpr MAX_HEIGHT = 720;
	size_t constexpr MAX_WIDTH = 1280;

	size_t new_height, new_width;
	vector<unsigned char> new_data;

	if (data.size() > MAX_HEIGHT * MAX_WIDTH * channels) {
		float const proportion = min(static_cast<float>(MAX_HEIGHT) / static_cast<float>(height), static_cast<float>(MAX_WIDTH) / static_cast<float>(width));
		new_height = height * proportion;
		new_width = width * proportion;
		new_data = resize_image(data, height, width, height * proportion, width * proportion, channels);
	} else {
		new_height = height;
		new_width = width;
		new_data = data;
	}

	//encoding in base64
	char constexpr b64_table[65] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

	size_t const encoded_length = ((new_data.size() + 2) / 3) * 4;

	string encoded;
	encoded.reserve(encoded_length);

	size_t i;
	for (i = 0; i + 2  < new_data.size(); i += 3) {
		encoded.push_back(
			b64_table[new_data[i] >> 2]);
		encoded.push_back(
			b64_table[(new_data[i] & 0x03) << 4 | new_data[i+1] >> 4]);
		encoded.push_back(
			b64_table[(new_data[i+1] & 0x0F) << 2 | new_data[i+2] >> 6]);
		encoded.push_back(
			b64_table[new_data[i+2] & 0x3F]);
	}

	if (i + 1 < new_data.size()) {
		encoded.push_back(
			b64_table[new_data[i] >> 2]);
		encoded.push_back(
			b64_table[(new_data[i] & 0x03) << 4 | new_data[i+1] >> 4]);
		encoded.push_back(
			b64_table[(new_data[i+1] & 0x0F) << 2]);
		encoded.push_back('=');
	} else if (i < new_data.size()) {
		encoded.push_back(
			b64_table[new_data[i] >> 2]);
		encoded.push_back(
			b64_table[(new_data[i] & 0x03) << 4]);
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

    if (new_height < new_width) {
    	p_height = new_height * p_width / (2 * new_width);  // it must be multiplied by two since rows are double the height of columns
    } else {
    	p_width = 2 * new_width * p_height / new_height;
    }

	// transmitting one chunk at a time
	size_t const total = encoded.size();
	size_t offset = 0;

	while (offset < total) {
		size_t constexpr chunk_size = 4000;
		size_t const count = min(chunk_size, total - offset);
		string chunk = encoded.substr(offset, count);

		cout << "\033_Gq=1,a=T,i=1,m=" << (count == chunk_size ? 1 : 0)
			<< ",f=" << channels * 8  //number of bits for storing a pixel in RGB8 or RBA8 format
			<< ",s=" << new_width
			<< ",v=" << new_height
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

vector<array<int, 3>> retrieve_selected_colors(vector<array<int, 3>> &list, size_t const amount, bool const by_brightness = false){

	vector<array<int, 3>> out;
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

void quantize_search(
	KDTree<int, 3> &palette,
	int * const red,
	int * const green,
	int * const blue,
	size_t const length
){
	if (length == 0) return;

	for (size_t i = 0; i < length; i++){
		array<int, 3> cache = palette.nearest({red[i], green[i], blue[i]});

		red[i] = cache[0];
		green[i] = cache[1];
		blue[i] = cache[2];
	}
}

bool quantize_2d_vector_to_list(Grid<int> const &mat,
	vector<array<int,3>> const &list,
	Grid<int> * red,
	Grid<int> * green,
	Grid<int> * blue
){
    size_t const size = list.size();

    for (size_t i = 0; i < red->height(); i++){
    	for (size_t j = 0; j < red->width(); j++){
    		size_t const color_pos = (mat[i][j] * (size - 1) + 127) / 255;
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

	string mode(args.mode);

	// IMPORT
	
	int width, height, channels;
 	filesystem::path output_file;

	unsigned char const * data = stbi_load(filesystem::path(args.input_file).c_str(), &width, &height, &channels, 0);
	
	if (!data) {
	    cerr << "Failed to load image: " << stbi_failure_reason() << endl;
	    return 1;
	}
	

	Grid<int> red, green, blue, alpha;
	vector<array<int, 3>> palette;

	// PREPROCESSING

	if (channels == 3){
		if (!vectorize_to_rgb(data, height, width, &red, &green, &blue)) {
			cout << "Could not process image" << endl;
			return 1;
		}
	} else if (channels == 4){
		if (!vectorize_to_rgb(data, height, width, &red, &green, &blue, &alpha)) {
			cout << "Could not process image" << endl;
			return 1;
		}
	} else {
		cerr << "Image is neither RGB nor RGBA" << endl;
		return 1;
	}
	
	if (args.blur > 0){
		Grid<float> edges = 1 - detect_edges_sobel(rgb_to_greyscale(red, green, blue));
		Grid<float> kernel = g_kernel(2 * args.blur + 1, static_cast<float>(args.blur) / 1.5f);
		
		red = red.convolve(kernel, edges);
		green = green.convolve(kernel, edges);
		blue = blue.convolve(kernel, edges);

		if (!alpha.empty()){
			alpha = alpha.convolve(kernel);
		}
	}


	// PROCESSING
	if (mode == "search"){ // TODO make resolution work by finding the farthest points apart from each other in the 3D set that is the palette
	
		palette = import_palette(args.palette);
		if (palette.empty()) return 1;
		KDTree<int, 3> palette_tree(palette);

		constexpr size_t MAX_SIZE_PER_THREAD = 720 * 1280;
		if (red.size() > MAX_SIZE_PER_THREAD * 3 / 2) {
			size_t const thread_count = red.size() / MAX_SIZE_PER_THREAD;
			size_t const thread_size = red.size() / thread_count;
			vector<thread> pool;
			pool.reserve(thread_count);

			for (size_t i = 0; i < thread_count - 1; i++) {
				pool.emplace_back(
					quantize_search,
					ref(palette_tree),
					red.raw() + thread_size * i,
					green.raw() + thread_size * i,
					blue.raw() + thread_size * i,
					thread_size);
			}

			size_t last_index = thread_size * (thread_count - 1);
			quantize_search(palette_tree,
				red.raw() + last_index,
				green.raw() + last_index,
				blue.raw() + last_index,
				red.size() - last_index);

			for (auto &floaty : pool) { //called it floaty, but I mean a thread, just could not use the symbol
				floaty.join();
			}

		} else {
			quantize_search(palette_tree, red.raw(), green.raw(), blue.raw(), red.size());
		}
		
		output_file = out_name(input_file, "search_" + string(args.palette));

		
	} else if (mode == "equidistant"){
	
		Grid<int> grey = rgb_to_greyscale(red, green, blue);
		palette = import_palette(args.palette);
		if (palette.empty()) return 1;
		
		sort_color_list(palette);
		quantize_2d_vector_to_list(grey, palette, &red, &green, &blue);
		
		output_file = out_name(input_file, "equidistant_" + string(args.palette));

		
	} else if (mode == "self"){
	
		vectorize_to_rgb(data, width, height, &red, &green, &blue);

		red = quantize_2d_vector_to_self(red, args.resolution);
		green = quantize_2d_vector_to_self(green, args.resolution);
		blue = quantize_2d_vector_to_self(blue, args.resolution);

		output_file = out_name(input_file, "self");

		
	} else if (mode == "self-sort"){
	
		vector<array<int, 3>> color_list = vectorize_to_color_list(data, width * height * channels, channels);
		color_list = retrieve_selected_colors(color_list, args.resolution, true);
		
		Grid<int> grey = rgb_to_greyscale(red, green, blue);
		quantize_2d_vector_to_list(grey, color_list, &red, &green, &blue);
		
		output_file = out_name(input_file, "self_sort");

		
	} else if (mode == "bw") {

		Grid<int> grey = quantize_2d_vector_to_self(rgb_to_greyscale(red, green, blue), args.resolution);

		red = grey;
		green = grey;
		blue = grey;

		output_file = out_name(input_file, "bw");

		
// 	} else if (mode == "blur"){
// 		grid<float> kernel = g_kernel(3, 1);
// 		
// 		red = red.convolve(kernel);
// 		green = green.convolve(kernel);
// 		blue = blue.convolve(kernel);
// 
// 		output_file = out_name(input_file, "blur");
// 	} else if (mode == "edges") {
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
		Grid<int> grey = rgb_to_greyscale(red, green, blue);
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

	if (args.print) print_image(height, width, channels, output);
	if (args.dry) return 0;

	if (!args.print || !string(args.output_file).empty()) {

		int error;

		if (filesystem::path extension = output_file.extension(); extension == ".png") {
			error = stbi_write_png(output_file.c_str(), width, height, channels, output.data(), 0);
		} else if (extension == ".bmp" || extension == ".dib") {
			error = stbi_write_bmp(output_file.c_str(), width, height, channels, output.data());
		} else if (extension == ".tga" || extension == ".icb" || extension == ".vda") {
			error = stbi_write_tga(output_file.c_str(), width, height, channels, output.data());
		} else if (extension == ".jpg" || extension == ".jpeg" || extension == ".jpe" || extension == ".jif" || extension == ".jfif" || extension == ".jfi") {
			error = stbi_write_jpg(output_file.c_str(), width, height, channels, output.data(), static_cast<int>(args.quality));
		} else if (extension == ".hdr") {
			vector<float> output_hdr;
			for (auto const &value : output) {
				output_hdr.emplace_back(static_cast<float>(value) / 255.0f);
			}
			error = stbi_write_hdr(output_file.c_str(), width, height, channels, output_hdr.data());
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
