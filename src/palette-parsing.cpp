#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include "palette-parsing.h"

std::vector<int> parse_colors(const std::string& line){
	size_t index1 = 0;
	size_t index2 = 0;
	std::string value_s;
	int value;
	
	std::vector<int> color;
	color.reserve(3);

	// TODO make error checking to check if there are any non-numeric values on the line

	for (int i = 0; i < 3; i++){
		index1 = line.find_first_of("1234567890", index2);
		index2 = line.find_first_of("	 ", index1);
		if (index1 != std::string::npos){
			value_s = line.substr(index1, index2-index1);
			value = stoi(value_s);
			color.push_back(value);
		} else {
			std::cerr << "Could not parse colors" << std::endl;
			return {};
		}
	}
	return color;
}


std::string clean_line(std::string line){
	size_t comment = line.find("#");
	if (comment != std::string::npos){
		line = line.substr(0, comment);
	}

	size_t start = line.find_first_not_of(" ");
	if (start != std::string::npos){
		line = line.substr(start, line.size());
	}

	size_t end = line.find_last_not_of(" ");
	if (end != std::string::npos){
		line = line.substr(0, end + 1);
	}
	
	return line;
}


std::vector<std::vector<int>> import_palette(const std::string file, const std::string name){
	std::vector<std::vector<int>> palette;
	std::vector<int> color;
 	std::ifstream raw(file);
	std::string line;
	size_t block;
	
	while (getline(raw, line)) {
		line = clean_line(line);
    	block = line.find("[" + name + "]"); // find the palette that we are looking for
    	if (block != std::string::npos){ // if palette is found break out and start parsing the colors
    		break;
    	}
    }

    if (block == std::string::npos){
    	std::cerr << "Could not find palette" << std::endl;
    	return {};
    }
    
   	while (getline(raw, line)){
   		line = clean_line(line);
   		size_t end = line.find("["); // find the end of the block
   		if (line.empty()){
   		} else if (end != std::string::npos){
   			break;
   		} else {
   			color = parse_colors(line);
   			palette.push_back(color);
   		}
   	}
   	// sort_color_list(palette);
	return palette;
}


bool sort_color_list(std::vector<std::vector<int>> &list, std::vector<int> * brightness_list){
    std::sort(list.begin(), list.end(), [](std::vector<int> a, std::vector<int> b){
        return ((a[0] + a[1] + a[2]) / 3) < ((b[0] + b[1] + b[2]) / 3);
    });

	if (brightness_list){
	    for (const auto& color : list){
	    	brightness_list->push_back((color[0] + color[1] + color [2]) / 3);
		
		}
    }

    return true;
}
