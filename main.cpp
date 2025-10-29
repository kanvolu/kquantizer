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
#include <fstream>
#include <string>

#include "stb_image.h"
#include "stb_image_write.h"
#include "kdtree.h"

#include "src/blur.cpp"

using namespace std;


// PALETTE PARSING LOGIC

vector<unsigned char> parse_colors(const string& line){
	vector<unsigned char> color;
	size_t index1 = 0;
	size_t index2 = 0;
	string value_s;
	unsigned char value;

	// TODO make error checking to check if there are any non-numeric values on the line

	for (int i = 0; i < 3; i++){
		index1 = line.find_first_of("1234567890", index2);
		index2 = line.find_first_of("	 ", index1);
		if (index1 != string::npos){
			value_s = line.substr(index1, index2-index1);
			value = stoi(value_s);
			color.push_back(value);
		} else {
			cerr << "COULD NOT PARSE COLORS." << endl;
			return {};
		}
	}
	return color;
}


string clean_line(string line){
	size_t comment = line.find("#");
	if (comment != string::npos){
		line = line.substr(0, comment);
	}

	size_t start = line.find_first_not_of(" ");
	if (start != string::npos){
		line = line.substr(start, line.size());
	}

	size_t end = line.find_last_not_of(" ");
	if (end != string::npos){
		line = line.substr(0, end + 1);
	}
	
	return line;
}


vector<vector<unsigned char>> import_palette(const string file, const string name){
	vector<vector<unsigned char>> palette;
	vector<unsigned char> color;
 	ifstream raw(file);
	string line;
	size_t block;
	
	while (getline(raw, line)) {
		line = clean_line(line);
    	block = line.find("[" + name + "]"); // find the palette that we are looking for
    	if (block != string::npos){ // if palette is found break out and start parsing the colors
    		break;
    	}
    }

    if (block == string::npos){
    	cout << "Palette was not found." << endl;
    	return {};
    }
    
   	while (getline(raw, line)){
   		line = clean_line(line);
   		size_t end = line.find("["); // find the end of the block
   		if (line.empty()){
   		} else if (end != string::npos){
   			break;
   		} else {
   			color = parse_colors(line);
   			palette.push_back(color);
   		}
   	}    	
	return palette;
}

bool sort_color_list(vector<vector<unsigned char>> &list, vector<unsigned char> * brightness_list = nullptr){
    sort(list.begin(), list.end(), [](vector<unsigned char> a, vector<unsigned char> b){
        return ((a[0] + a[1] + a[2]) / 3) < ((b[0] + b[1] + b[2]) / 3);
    });

	if (brightness_list){
	    for (const auto& color : list){
	    	brightness_list->push_back((color[0] + color[1] + color [2]) / 3);
		
		}
    }

    return true;
}

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

// QUANTIZATION LOGIC

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


// IMPORTING AND EXPORTING LOGIC

bool vectorize_to_rgb(unsigned char const * data, int const width, int const height, vector<vector<unsigned char>> * red, vector<vector<unsigned char>> * green, vector<vector<unsigned char>> * blue, vector<vector<unsigned char>> * alpha = nullptr){
    if (alpha == nullptr){
        int const channels = 3;
        for (int y = 0; y < height; y++){
            vector<unsigned char> cache_red;
            vector<unsigned char> cache_green;
            vector<unsigned char> cache_blue;
            for (int x = 0; x < width; x++){
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
        for (int y = 0; y < height; y++){
            vector<unsigned char> cache_red;
            vector<unsigned char> cache_green;
            vector<unsigned char> cache_blue;
            vector<unsigned char> cache_alpha;
            for (int x = 0; x < width; x++){
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


bool vectorize_to_greyscale(unsigned char const * data, int const width, int const height, vector<vector<unsigned char>> * grey, vector<vector<unsigned char>> * alpha = nullptr){
    int max = 0;
    char cur;
    size_t cur_pos;

    if (alpha == nullptr){
    	int channels = 3;
	    for (int y = 0; y < height; y++){
	        vector<unsigned char> cache_grey;
	        for (int x = 0; x < width; x++){
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
    	for (int y = 0; y < height; y++){
	        vector<unsigned char> cache_grey;
	        vector<unsigned char> cache_alpha;
	        for (int x = 0; x < width; x++){
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

vector<vector<unsigned char>> vectorize_to_color_list(unsigned char const * data, int const width, int const height, int const channels){
	vector<vector<unsigned char>> out;
	for (size_t i = 0; i < width * height * channels; i += channels){
		out.push_back({
			data[i + 0],
			data[i + 1],
			data[i + 2]
			});
	}
	

	return out;
}


unsigned char * flatten(vector<vector<unsigned char>> const * red, vector<vector<unsigned char>> const * green = nullptr, vector<vector<unsigned char>> const * blue = nullptr, vector<vector<unsigned char>> const * alpha = nullptr){
    // the output array must be deleted afterwards
    
    size_t height = (*red).size();
    size_t width = (*red)[0].size();
    int channels;
    vector<vector<unsigned char>> const * channel[4] = {red, green, blue, alpha};

	if (green == nullptr){
		channels = 1;
	} else if (blue == nullptr){
		channels = 2;
	} else if (alpha == nullptr){
        channels = 3;
    } else {
        channels = 4;
    }


    unsigned char * out = new unsigned char[width * height * channels];

    for (size_t y = 0; y < height; y++){
        for (size_t x = 0; x < width; x++){
        	for (size_t c = 0; c < channels; c++){
            	out[(y * width + x) * channels + c] = (*(channel[c]))[y][x];
        	}
        }
    }
    
    return out;
}

string out_name(string const path, string const palette){
	size_t pos = path.rfind(".");
	return path.substr(0, pos) + "_" + palette + path.substr(pos);
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

	if (string(args.mode) == "equidistant"){
		vectorize_to_greyscale(data, width, height, &grey);
		palette = import_palette("../palettes.txt", args.palette);
		sort_color_list(palette);
		quantize_2d_vector_to_list(grey, palette, &red, &green, &blue);
		output_file = out_name(string(args.input_file), "equidistant_" + string(args.palette));
	}
	if (string(args.mode) == "self"){
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
	}


	if (!(string(args.output_file).empty())){
		output_file = string(args.output_file);
		// cout << "writing image to " + output_file << endl;
	}
	
	unsigned char * output = flatten(&red, &green, &blue);
	
	int error = stbi_write_bmp(output_file.c_str(), width, height, channels, output);

	
	
	    
	return 0;
}
