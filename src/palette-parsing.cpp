#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <array>
#include <cassert>

#include "palette-parsing.h"

std::array<int, 3> parse_colors(const std::string& line){
	size_t index1 = 0;
	size_t index2 = 0;
	std::array<int, 3> color{};

	// TODO make error checking to check if there are any non-numeric values on the line

	for (int i = 0; i < 3; i++){
		index1 = line.find_first_of("1234567890", index2);
		index2 = line.find_first_of("	 ", index1);
		if (index1 != std::string::npos){
			color[i] = stoi(line.substr(index1, index2-index1));
		} else {
			std::cerr << "Could not parse colors" << std::endl;
			return {};
		}
	}

	return color;
}


std::string clean_line(std::string line){
	size_t const comment = line.find("#");
	if (comment != std::string::npos){
		line = line.substr(0, comment);
	}

	size_t const start = line.find_first_not_of(" ");
	if (start != std::string::npos){
		line = line.substr(start, line.size());
	}

	size_t const end = line.find_last_not_of(" ");
	if (end != std::string::npos){
		line = line.substr(0, end + 1);
	}
	
	return line;
}

std::filesystem::path find_palette_path() {
	std::filesystem::path const config_path = std::filesystem::path(getenv("HOME")) / ".config/kquantizer/palettes.txt";
	std::filesystem::path const up_path = "../palettes.txt";

	assert(std::filesystem::exists(config_path) || std::filesystem::exists(config_path));

	if (std::filesystem::exists(up_path)) return up_path;
	if (std::filesystem::exists(config_path)) return config_path;

	return {};
} 


std::vector<std::array<int, 3>> import_palette(std::string const name){
	std::filesystem::path filepath = find_palette_path();
	if (filepath.empty()) {
		std::cout << "Could not locate file path.\n" <<
		"It must be either ../palettes.txt or ~/.config/kquantizer/palettes.txt" << std::endl;
		return {};
	}
	std::vector<std::array<int, 3>> palette;
 	std::ifstream raw(filepath);
	std::string line;
	size_t block;
	
	while (getline(raw, line)) {
		line = clean_line(line);
    	block = line.find("[" + name + "]"); // find the palette that we are looking for
    	if (block != std::string::npos) break; // if palette is found break out and start parsing the colors
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
   			palette.push_back(parse_colors(line));
   		}
   	}
   	// sort_color_list(palette);
	return palette;
}


void sort_color_list(std::vector<std::array<int, 3>> &list, std::vector<int> * brightness_list) {
    std::sort(list.begin(), list.end(), [](auto a, auto b){
        return ((a[0] + a[1] + a[2]) / 3) < ((b[0] + b[1] + b[2]) / 3);
    });

	if (brightness_list){
	    for (auto const &color : list){
	    	brightness_list->push_back((color[0] + color[1] + color [2]) / 3);
		
		}
    }

}
