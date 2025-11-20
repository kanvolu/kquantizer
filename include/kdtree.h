#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

template <typename T>
constexpr bool is_char_type_v =
    std::is_same_v<T, char> ||
    std::is_same_v<T, signed char> ||
    std::is_same_v<T, unsigned char> ||
    std::is_same_v<T, wchar_t> ||
    // std::is_same_v<T, char8_t> ||
    std::is_same_v<T, char16_t> ||
    std::is_same_v<T, char32_t>;


template <typename T, typename U = long long>
class kdtree {

struct node {

int m_left = -1;
int m_right = -1;
size_t m_depth;
std::vector<T> m_data;

node(std::vector<T> data, std::size_t depth = 0)
	: m_depth (depth), m_data (data) {}

void print(){
	for (T value : m_data){
		if constexpr (is_char_type_v<T>){
			std::cout << static_cast<int>(value) << " ";
		} else {
			std::cout << value << " ";
		}
	}
	std::cout << std::endl;
}

// making a node iterable
auto begin() {return m_data.begin();}
auto end() {return m_data.end();}

auto begin() const {return m_data.begin();}
auto end() const {return m_data.end();}

// for making the [] notation work with the node
T& operator[](size_t index) {return m_data[index];}
const T& operator[](size_t index) const {return m_data[index];}

size_t size() { return m_data.size(); }

};

std::vector<node> m_tree;

	// recursive function to build the tree
void next_node(std::vector<std::vector<T>> data, size_t depth = 0){

	if (data.size() == 0){
		std::cerr << "empty array" << std::endl;
		return;
	}
	
	size_t k;
	
	if (depth == 0){
		k = 0;
	} else {
		k = depth % data[0].size();
	}
	
	if (data.size() == 1)
	{
		m_tree.emplace_back(node(data[0], depth));
	} 
	else if (data.size() == 2)
	{
		std::sort(data.begin(), data.end(), [k] (const auto& a, const auto& b) {
			return a[k] < b[k];
			});
			
		size_t cur = m_tree.size();
		m_tree.emplace_back(node(data[1], depth));
		m_tree[cur].m_left = m_tree.size();
		next_node({data[0]}, depth + 1);
	} 
	else 
	{
		size_t m = data.size() / 2;
		std::sort(data.begin(), data.end(), [k] (const auto& a, const auto& b) {
			return a[k] < b[k];});
		size_t cur = m_tree.size();
		
		m_tree.emplace_back(node(data[m], depth));
		m_tree[cur].m_left = m_tree.size();
		next_node(std::vector<std::vector<T>>(data.begin(), data.begin() + m), depth + 1);
		m_tree[cur].m_right = m_tree.size();
		next_node(std::vector<std::vector<T>>(data.begin() + m + 1, data.end()), depth + 1);
	}
}

U dist_sqrd(std::vector<T> target, std::vector<T> cur){
	U dist = 0;
	for (std::size_t i = 0; i < target.size(); i++){
		dist += (target[i] - cur[i]) * (target[i] - cur[i]);
	}
	return dist;
}


	// find the closest of 2 nodes to a point, used in the "nearest()" function
node closest(std::vector<T> target, node temp, node cur){
	if (dist_sqrd(target, cur.m_data) < dist_sqrd(target, temp.m_data)){
		return cur;
	} else {
		return temp;
	}
}

node farthest(std::vector<T> target, node temp, node cur){
	if (dist_sqrd(target, temp.m_data) < dist_sqrd(target, cur.m_data)){
		return cur;
	} else {
		return temp;
	}
}

public:

kdtree(std::vector<std::vector<T>> data){
	next_node(data);
}

size_t minimum(){
	size_t minimum_pos;
	T minimum_sum = 0;
	for (const auto& value : m_tree[0]){
		minimum_sum += value * value;
	}
	
	for (size_t i = 0; i < m_tree.size(); i++){
		T cache_sum = 0;
		for (size_t j = 0; j < m_tree[i].size(); j++){
			cache_sum += m_tree[i][j] * m_tree[i][j];
		}
		if (cache_sum < minimum_sum){
			minimum_sum = cache_sum;
			minimum_pos = i;
		}
	}
	return minimum_pos;
}

std::vector<T> nearest(std::vector<T> target, size_t cur_pos = 0){
	node cur = m_tree[cur_pos];
	size_t k = cur.m_depth % cur.size();
	size_t next, other;

	// handle cases possible cases of leaf, single branch and split branch
	if (cur.m_left < 0 && cur.m_right < 0){
		return cur.m_data;
	} else if (cur.m_right < 0){

		next = cur.m_left;
		other = cur_pos;

		node temp = nearest(target, next);
		node best = closest(target, temp, cur);
		
		return best.m_data;
	} else {
		if (target[k] <= cur[k]){
			next = cur.m_left;
			other = cur.m_right;
		} else {
			next = cur.m_right;
			other = cur.m_left;
		}

		node temp = nearest(target, next);
		node best = closest(target, temp, cur);

		U r_sqrd = dist_sqrd(target, best.m_data);
		U dist = (target[k] - cur[k]) * (target[k] - cur[k]);

		if (r_sqrd >= dist){
			temp = nearest(target, other);
			best = closest(target, temp, best);
		}
		return best.m_data;
	}
}

void print(int k = -1, bool print_depth = false){
	if (k < 0){
		for (node point : m_tree){
			for (auto value : point){
				if constexpr (is_char_type_v<T>){
					std::cout << static_cast<int>(value) << " ";
				} else {	
					std::cout << value << " ";
				}
			}
			
			if (print_depth == true){
				std::cout << point.depth << "\n";
			} else {
				std::cout << "\n";
			}
		}
	} else {
		for (node point : m_tree){
			k = k % point.size();
			if constexpr (is_char_type_v<T>){
				std::cout << static_cast<int>(point[k]);
			} else {
				std::cout << point[k];
			}
			
			if (print_depth == true){
				std::cout << " " << point.depth << "\n";
			} else {
				std::cout << "\n";
			}
		}
	}
}

	// making tree iterable
auto begin() {return m_tree.begin();}
auto end() {return m_tree.end();}

auto begin() const {return m_tree.begin();}
auto end() const {return m_tree.end();}


// for making the [] notation work with tree
std::vector<T>& operator[] (size_t index) { return m_tree[index].m_data; }
const std::vector<T>& operator[] (size_t index) const { return m_tree[index].m_data; }

node& at(size_t index) { return m_tree[index]; }
const node& at(size_t index) const { return m_tree[index]; }

size_t size() { return m_tree.size(); }
	
};
