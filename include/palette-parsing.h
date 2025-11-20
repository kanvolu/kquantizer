#pragma once

#include <vector>

std::vector<int> parse_colors(
	std::string const &line
);

std::string clean_line(
	std::string line
);

std::vector<std::vector<int>> import_palette(
	std::string const file, 
	std::string const name
);

bool sort_color_list(
	std::vector<std::vector<int>> &list, 
	std::vector<int> * brightness_list = nullptr
);
