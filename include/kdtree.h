#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
// #include <random>
// #include <optional>

struct node{
	int left = -1;
	int right = -1;
	int depth;
	std::vector<int> values;
	
	node(std::vector<int> data, int d = 0);

	void print();
	
	auto begin();
	auto end();
	
	auto begin() const;
	auto end() const;

	int& operator[](std::size_t index);
	const int& operator[](std::size_t index) const;

	std::size_t size();
};

class kdtree{
	void next_node(std::vector<std::vector<int>> data, int depth = 0);

	long dist_sqrd(std::vector<int> target, std::vector<int> cur);
	node closest(std::vector<int> target, node temp, node cur);

public:

	std::vector<node> tree;

	kdtree(std::vector<std::vector<int>> data);

	node nearest(std::vector<int> target, int cur_pos = 0);

	void print(bool print_depth = false, int k = -1);

	auto begin();
	auto end();
	
	auto begin() const;
	auto end() const;

	node& operator[](std::size_t index);
	const node& operator[](std::size_t index) const;

	std::size_t size();
};
