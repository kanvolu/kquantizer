#define _USE_MATH_DEFINE
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "lodepng.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>

using namespace std;

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

// TODO implement convolution of matrices


vector<int> parse_colors(const string& line){
	vector<int> color;
	size_t index1 = 0;
	size_t index2 = 0;
	string value_s;
	int value;

	// TODO make error checking to check if there are any non-numeric values on the line
	
	index1 = line.find_first_of("1234567890", index2);
	index2 = line.find(" ", index1);
	if (index1 != string::npos && index2 != string::npos){
		for (int i = 0; i < 3; i++){	
			value_s = line.substr(index1, index2-index1);
			value = stoi(value_s);
			color.push_back(value);
			index1 = line.find_first_of("1234567890", index2);
			index2 = line.find(" ", index1);
			}	
	} else {
		cout << "COULD NOT PARSE COLORS." << endl;
		return {};
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


vector<vector<int>> import_palette(const string& file, const string& name){
	vector<vector<int>> palette;
	vector<int> color;
	ifstream raw(file);
	std::string line;
	size_t block;
	
	while (getline(raw, line)) {
		line = clean_line(line);
    	block = line.find("["+name+"]"); // find the palette that we are looking for
    	if (line.empty()){ //if the line is empty just skip to the next one
    	} else if (block != string::npos){ // if palette is found break out and start parsing the colors
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

// TODO implement logic to import images properly
// TODO implement kdtree class so we can do nearest neighbor search

int main(){
	vector<vector<int>> palette = import_palette("../palettes.txt", "debug");
	for (const auto& i : palette){
		for (int j : i){
			cout << j << " ";
		}
		cout << endl;
	}

	return 0;
}
