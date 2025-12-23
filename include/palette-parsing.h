#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <array>

std::array<int, 3> parse_colors(
	std::string const &line
);

std::string clean_line(
	std::string line
);

std::vector<std::array<int, 3>> import_palette(
	std::string const name
);

std::filesystem::path find_palette_path();

void sort_color_list(
	std::vector<std::array<int, 3>> &list,
	std::vector<int> * brightness_list = nullptr
);
