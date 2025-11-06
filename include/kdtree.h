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
    

template <typename T>
struct node{
	int left = -1;
	int right = -1;
	std::size_t depth;
	std::vector<T> values;

	node(std::vector<T> data, std::size_t d = 0){
		values = data;
		depth = d;
	}

	void print(){
		for (T value : values){
			if constexpr (is_char_type_v<T>){
				std::cout << static_cast<int>(value) << " ";
			} else {
				std::cout << value << " ";
			}
		}
		std::cout << "\n";
	}
	
	// making a node iterable
	auto begin() {return values.begin();}
	auto end() {return values.end();}

	auto begin() const {return values.begin();}
	auto end() const {return values.end();}

	// for making the [] notation work with the node
	T& operator[](std::size_t index) {return values[index];}
	const T& operator[](std::size_t index) const {return values[index];}

	std::size_t size() {return values.size();}
};


template <typename T, typename U = long long>
class kdtree{

		// recursive function to build the tree
	void next_node(std::vector<std::vector<T>> data, size_t depth = 0){
		if (data.size() == 0){
			std::cerr << "empty array" << "\n";
			return;
		}
		size_t k;
		if (depth == 0){
			k = 0;
		} else {
			k = depth % data[0].size();
		}
		if (data.size() == 1){
			tree.push_back(node<T>(data[0], depth));
			// std::cout << tree.back().values[0] <<" leaf " << tree.back().depth << "\n";
		} else if (data.size() == 2){
			std::sort(data.begin(), data.end(), [k] (const auto& a, const auto& b) {
				return a[k] < b[k];});
			size_t cur = tree.size();
			tree.push_back(node<T>(data[1], depth));
			// std::cout << tree.back().values[0] <<" single " << tree.back().depth << "\n";
			tree[cur].left = tree.size();
			next_node({data[0]}, depth + 1);
		} else {
			size_t m = data.size() / 2;
			std::sort(data.begin(), data.end(), [k] (const auto& a, const auto& b) {
				return a[k] < b[k];});
			size_t cur = tree.size();
			
			tree.push_back(node<T>(data[m], depth));
			// std::cout << tree.back().values[0] <<" double " << tree.back().depth << "\n";
			tree[cur].left = tree.size();
			next_node(std::vector<std::vector<T>>(data.begin(), data.begin() + m), depth + 1);
			tree[cur].right = tree.size();
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
	node<T> closest(std::vector<T> target, node<T> temp, node<T> cur){
		if (dist_sqrd(target, cur.values) < dist_sqrd(target, temp.values)){
			return cur;
		} else {
			return temp;
		}
	}

public:

	std::vector<node<T>> tree;

	kdtree(std::vector<std::vector<T>> data){
		next_node(data);
	}
	
	node<T> nearest(std::vector<T> target, size_t cur_pos = 0){
		node<T> cur = tree[cur_pos];
		size_t k = cur.depth % cur.size();
		size_t next, other;

		// handle cases possible cases of leaf, single branch and split branch
		if (cur.left < 0 && cur.right < 0){
			return cur;
		} else if (cur.right < 0){

			next = cur.left;
			other = cur_pos;

			// std::cout << "single in search" << \n;
			node<T> temp = nearest(target, next);
			node<T> best = closest(target, temp, cur);

	// 			long r_sqrd = dist_sqrd(target, best.values);
	// 			long dist = (target[k] - cur[k]) * (target[k] - cur[k]);
	// 
	// 			if (r_sqrd >= dist){
	// 				temp = nearest(target, other);
	// 				best = closest(target, temp, best);
	// 			}
			
			return best;
		} else {
			if (target[k] <= cur[k]){
				next = cur.left;
				other = cur.right;
			} else {
				next = cur.right;
				other = cur.left;
			}

			node<T> temp = nearest(target, next);
			node<T> best = closest(target, temp, cur);

			U r_sqrd = dist_sqrd(target, best.values);
			U dist = (target[k] - cur[k]) * (target[k] - cur[k]);

			if (r_sqrd >= dist){
				temp = nearest(target, other);
				best = closest(target, temp, best);
			}
			return best;
		}
	}
	
		// if you want to print a single value of the nodes in the tree you have to also pass print_depth
	void print(int k = -1, bool print_depth = false){
		if (k < 0){
			for (node<T> point : tree){
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
			for (node<T> point : tree){
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
	auto begin() {return tree.begin();}
	auto end() {return tree.end();}

	auto begin() const {return tree.begin();}
	auto end() const {return tree.end();}


	// for making the [] notation work with tree
	node<T>& operator[](std::size_t index) {return tree[index];}
	const node<T>& operator[](std::size_t index) const {return tree[index];}

	std::size_t size() {return tree.size();}
	
};
