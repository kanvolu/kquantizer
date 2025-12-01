#pragma once

#include <vector>
#include <string>

std::vector<int> parse_colors(
	std::string const &line
);

std::string clean_line(
	std::string line
);

std::vector<std::vector<int>> import_palette(
	std::string const name
);

std::string find_palette_path();

bool sort_color_list(
	std::vector<std::vector<int>> &list, 
	std::vector<int> * brightness_list = nullptr
);
