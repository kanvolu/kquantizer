#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

template <typename T, size_t C, typename U = long>
class KDTree {

struct Node {

	int m_left = -1;
	int m_right = -1;
	size_t m_depth;
	std::array<T, C> m_data;

	Node(std::array<T, C> data, std::size_t depth = 0)
		: m_depth (depth), m_data (data) {}

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

std::vector<Node> m_tree;

	// recursive function to build the tree
void next_node(std::vector<std::array<T, C>> data, size_t const depth = 0){

	if (data.empty()){
		std::cerr << "empty array" << std::endl;
		return;
	}

	size_t k = depth % data[0].size();

	if (data.size() == 1)
	{
		m_tree.emplace_back(Node(data[0], depth));
	}
	else if (data.size() == 2)
	{
		std::sort(data.begin(), data.end(), [k] (auto const &a, auto const &b) {
			return a[k] < b[k];
			});

		size_t cur = m_tree.size();
		m_tree.emplace_back(Node(data[1], depth));
		m_tree[cur].m_left = m_tree.size();
		next_node({data[0]}, depth + 1);
	}
	else
	{
		size_t m = data.size() / 2;
		std::sort(data.begin(), data.end(), [k] (auto const &a, auto const &b) {
			return a[k] < b[k];});
		size_t cur = m_tree.size();

		m_tree.emplace_back(Node(data[m], depth));
		m_tree[cur].m_left = m_tree.size();
		next_node(std::vector<std::array<T, C>>(data.begin(), data.begin() + m), depth + 1);
		m_tree[cur].m_right = m_tree.size();
		next_node(std::vector<std::array<T, C>>(data.begin() + m + 1, data.end()), depth + 1);
	}
}

U dist_sqrd(std::array<T, C> const &target, std::array<T, C> const &cur) const {
	U dist = 0;
	for (std::size_t i = 0; i < target.size(); i++){
		dist += (target[i] - cur[i]) * (target[i] - cur[i]);
	}
	return dist;
}


	// find the closest of 2 nodes to a point, used in the "nearest()" function
Node closest(std::array<T, C> const &target, Node const &temp, Node const &cur) const {
	if (dist_sqrd(target, cur.m_data) < dist_sqrd(target, temp.m_data)){
		return cur;
	} else {
		return temp;
	}
}

Node farthest(std::array<T, C> const &target, Node const &temp, Node const &cur) const {
	if (dist_sqrd(target, temp.m_data) < dist_sqrd(target, cur.m_data)){
		return cur;
	} else {
		return temp;
	}
}

public:

KDTree(std::vector<std::array<T, C>> data){
	next_node(data);
}

size_t minimum_position() const {
	size_t minimum_pos = 0;
	T minimum_sum = 0;
	for (const auto& value : m_tree[0]){
		minimum_sum += value * value;
	}

	for (size_t i = 1; i < m_tree.size(); i++){
		T cache_sum = 0;
		for (size_t j = 0; j < C; j++){
			cache_sum += m_tree[i][j] * m_tree[i][j];
		}
		if (cache_sum < minimum_sum){
			minimum_sum = cache_sum;
			minimum_pos = i;
		}
	}
	return minimum_pos;
}

std::array<T, C> nearest(std::array<T, C> const &target, size_t const cur_pos = 0) const {
	Node cur(m_tree[cur_pos]);
	size_t k = cur.m_depth % cur.size();
	size_t next, other;

	// handle cases possible cases of leaf, single branch and split branch
	if (cur.m_left < 0 && cur.m_right < 0){
		return cur.m_data;
	} else if (cur.m_right < 0){

		next = cur.m_left;

		Node const temp = nearest(target, next);
		Node const best = closest(target, temp, cur);

		return best.m_data;
	} else {
		if (target[k] <= cur[k]){
			next = cur.m_left;
			other = cur.m_right;
		} else {
			next = cur.m_right;
			other = cur.m_left;
		}

		Node temp = nearest(target, next);
		Node best = closest(target, temp, cur);

		U r_sqrd = dist_sqrd(target, best.m_data);
		U dist = (target[k] - cur[k]) * (target[k] - cur[k]);

		if (r_sqrd >= dist){
			temp = nearest(target, other);
			best = closest(target, temp, best);
		}
		return best.m_data;
	}
}


	// making tree iterable
auto begin() {return m_tree.begin();}
auto end() {return m_tree.end();}

auto begin() const {return m_tree.begin();}
auto end() const {return m_tree.end();}


// for making the [] notation work with tree
std::array<T, C>& operator[] (size_t index) { return m_tree[index].m_data; }
const std::array<T, C>& operator[] (size_t index) const { return m_tree[index].m_data; }

Node& at(size_t index) { return m_tree[index]; }
const Node& at(size_t index) const { return m_tree[index]; }

size_t size() { return m_tree.size(); }
	
};
