#define _USE_MATH_DEFINE
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "lodepng.h"
#include "kdtree.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>

using namespace std;

// BLURRING LOGIC

float gaus(float x, float deviation){
	x = exp(-(x * x) / (2 * deviation * deviation));
	x = x / (sqrt(2 * M_PI) * deviation);
	return x;
}


vector<vector<float>> g_kernel(int size, float deviation){
	vector<vector<float>> kernel(size, vector<float>(size, 0));
	float dist;
	float sum = 0;
	int cen = (size - 1) / 2;
	for (int i = 0; i < size; i++){
		for (int j = 0; j < size; j++){
			dist = sqrt(pow(i - cen, 2) + pow(j - cen, 2));
			kernel[i][j] = gaus(dist, deviation);
			sum += kernel[i][j];
		}
	}
	
	for (int i = 0; i < size; i++){ // normalize kernel
		for (int j = 0; j < size; j++){
			kernel[i][j] /= sum;
		}
	}
	return kernel;
}

// TODO implement convolution of 2d vectors

// PARSING LOGIC

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

// TODO implement kdtree class so we can do nearest neighbor search

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
	char const * file_path = argv[1];
	string palette_name = argv[2];
	string output_path = out_name(string(file_path), palette_name);
		
	// IMPORT
	vector<vector<int>> palette_raw = import_palette("../palettes.txt", palette_name);
	kdtree palette(palette_raw);
	
	// char const* path = "../lancia.jpeg";
	// cin >> path;
	int width, height, channels;
	unsigned char* data = stbi_load(file_path, &width, &height, &channels, 4);
	
	if (!data) {
	    cerr << "Failed to load image: " << stbi_failure_reason() << endl;
	    return 1; // or handle error
	}
	
	// vector<vector<vector<int>>> img = vectorize_img(data, width, height);

	vector<int> cur_pixel(3);
	vector<unsigned char> out_data;
	for (int i = 0; i < (width * height * 4); i += 4){
		for (int j = 0; j < 3; j++){
			cur_pixel[j] = data[i+j];
		}
		node nearest = palette.nearest(cur_pixel);
		for (int j = 0; j < 3; j++){
			out_data.push_back(nearest[j]);
		}
		out_data.push_back(data[i+3]);
	}

	// EXPORT
	// vector<unsigned char> out_data = flatten_img(img);

	unsigned error = lodepng::encode(output_path, out_data, width, height);
	if (error) {
        cerr << "Encoder error " << error << ": " << lodepng_error_text(error) << endl;
        return 1;
	}
	    
	return 0;
}
