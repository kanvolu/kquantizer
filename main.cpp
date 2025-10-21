#define _USE_MATH_DEFINE
#define STB_IMAGE_IMPLEMENTATION

 
#define REQUIRED_ARGS \
	REQUIRED_STRING_ARG(input_file, "input", "Input file path") \
	REQUIRED_FLOAT_ARG(blur_sigma, "blur_sigma", "Value of sigma for the small sigma used in DOG edge detection")

#define BOOLEAN_ARGS \
    BOOLEAN_ARG(help, "-h", "Show help")


// #ifdef __cplusplus
// extern "C" {  
// #endif  

#include "easyargs.h"

// 
// #ifdef __cplusplus  
// }  
// #endif

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>

#include "stb_image.h"
#include "lodepng.h"
#include "kdtree.h"

#include "src/blur.cpp"

using namespace std;


// PARSING LOGIC

vector<vector<int>> greyscale(vector<vector<vector<int>>> const mat){
	vector<vector<int>> out(mat.size(), vector<int>(mat[0].size(), 0));


	for (int i = 0; i < out.size(); i++){
		for (int j = 0; j < out[0].size(); j++){
			int cache = 0;
			for (int c = 0; c < mat[0][0].size() - 1; c++){
				cache += mat[i][j][c];
			}

			out[i][j] = cache / (mat[0][0].size() - 1);

		}
	}


	// cout << "Greyscale: \n";
	// for (int i = 200; i < 210; i++){
	// 	for (int j = 200; j < 210; j++){
	// 		cout << out[i][j] << " ";
	// 		
	// 	}
	// 	cout << "\n";
	// }

	return out;
}

vector<int> parse_colors(const string& line){
	vector<int> color;
	size_t index1 = 0;
	size_t index2 = 0;
	string value_s;
	int value;

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


vector<vector<int>> import_palette(const string& file, const string name){
	vector<vector<int>> palette;
	vector<int> color;
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

// IMPORTING AND EXPORTING LOGIC

vector<vector<vector<int>>> vectorize_img(unsigned char* data, int width, int height){
	vector<vector<vector<int>>> img;
	for (int h = 0; h < height; h++){
		vector<vector<int>> w_cache;
		for (int w = 0; w < width; w++){
			vector<int> c_cache;
			for (int c = 0; c < 4; c++){
				c_cache.push_back(static_cast<int>(data[c + (4 * (h * width + w))]));
			}
			w_cache.push_back(c_cache);
		}
		img.push_back(w_cache);
	}
	return img;
}

void splitting(vector<vector<vector<int>>> const img, vector<vector<int>> * r, vector<vector<int>> * g, vector<vector<int>> * b){
	for (int i = 0; i < img.size(); i++){
		for (int j = 0; j < img[i].size(); j++){
			(*r)[i][j] = img[i][j][0];
			(*g)[i][j] = img[i][j][1];
			(*b)[i][j] = img[i][j][2];
		}
	}
}

vector<vector<vector<int>>> combine(vector<vector<int>> const *r, vector<vector<int>> const *g, vector<vector<int>> const *b){
	vector<vector<vector<int>>> out((*r).size(), vector<vector<int>>((*r)[0].size(), vector<int>(4, 255)));
	for (int i = 0; i < out.size(); i++){
		for (int j = 0; j < out[i].size(); j++){
			 out[i][j][0] = (*r)[i][j];
			 out[i][j][1] = (*g)[i][j];
			 out[i][j][2] = (*b)[i][j];
		}
	}
	return out;
}

vector<unsigned char> flatten_img(vector<vector<vector<int>>> img){
	vector<unsigned char> data;
	for (vector row : img){
		for (vector color : row){
			for (int value : color){
				data.push_back(static_cast<unsigned char>(value));
			}
		}
	}
	return data;
}

string out_name(string const path, string const palette){
	size_t pos = path.rfind(".");
	return path.substr(0, pos) + "_" + palette + path.substr(pos);
}


int main(int argc, char* argv[]){
	// parse arguments

	args_t args = make_default_args();
	parse_args(argc, argv, &args);

	// if (!parse_args(argc, argv, &args) || args.help) {
 //        print_help(argv[0]);
 //        return 1;
 //    }

	float s_sigma = args.blur_sigma;
	float b_sigma = 1.6 * s_sigma;
	int kernel_radius = 3 * b_sigma;
 	string output_file = out_name(string(args.input_file), "dog");

 	
		
	// IMPORT
	// vector<vector<int>> palette_raw = import_palette("../palettes.txt", palette_name);
	// kdtree palette(palette_raw);
	
	int width, height, channels;
	unsigned char* data = stbi_load(args.input_file, &width, &height, &channels, 4);
	
	if (!data) {
	    cerr << "Failed to load image: " << stbi_failure_reason() << endl;
	    return 1; // or handle error
	}
	
	vector<vector<vector<int>>> const img = vectorize_img(data, width, height);

	// cout << "Size: " << img.size() << "x" << img[0].size() << "x" << img[0][0].size() << endl;

	
	
	vector<vector<int>> red (img.size(), vector<int>(img[0].size()));
	vector<vector<int>> blue (img.size(), vector<int>(img[0].size()));
	vector<vector<int>> green (img.size(), vector<int>(img[0].size()));

	splitting(img, &red, &green, &blue);

	vector<vector<float>> kernel = g_kernel(2 * kernel_radius + 1, s_sigma);


	vector<vector<int>> dog_img = dog(greyscale(img), s_sigma);
	// 
	// red = convolve(red, kernel);
	// green = convolve(green, kernel);
	// blue = convolve(blue, kernel);
	

	// vector<vector<vector<int>>> dog_out = combine(&red, &green, &blue);
	vector<vector<vector<int>>> dog_out = img;

	for (int i = 0; i < dog_out.size(); i++){
		for (int j = 0; j < dog_out[0].size(); j++){
			for (int c = 0; c < dog_out[0][0].size() - 1; c++){
				dog_out[i][j][c] = dog_img[i][j];
			}
		}
	}

	unsigned error = lodepng::encode(output_file, flatten_img(dog_out), width, height);

	if(error) {
		cerr << "encoder error " << error << ": "<< lodepng_error_text(error) << endl;
	}
	    
	return 0;
}
