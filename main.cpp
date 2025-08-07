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

vector<int> parse_colors(const string& line){
	vector<int> color;
	size_t index1 = 0;
	size_t index2 = 0;
	string value_s;
	int value;
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
		cout << "COULD NOT PARSE COLORS" << endl;
		return {};
	}
	return color;
}



vector<vector<int>> import_palette(const string& file, const string& name){
	vector<vector<int>> palette(1, vector<int>(0, 0));
	vector<int> color;
	ifstream raw(file);
	std::string line;
	int line_num = 0;
	while (getline(raw, line)) {
    	size_t block = line.find("["+name+"]"); // find the block that we are looking for
    	size_t comment = line.find("#");
    	if (line.empty() || comment != string::npos){ //TODO make the parser recognize a partially commented line
    		// if line is empty or commented just skip to the next line
    	} else if (block != string::npos){
			while (getline(raw, line)){
				size_t end = line.find("[");
				if (line.empty() || comment != string::npos){
					// if line is empty or commented just skip to the next line
				} else if (end != string::npos){
					break;
				} else {
					color = parse_colors(line);
					palette.push_back(color);
					line_num++;
				}
			}    	
    		break;
    	}
    	line_num++;
    }
	return palette;
}

// TODO make a kernel generator using the "gaus" function already defined

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
