#include "kdtree.h"

node::node(std::vector<int> data, int d){
	values = data;
	depth = d;
}

void node::print(){
	for (int value : values){
		std::cout << value << " ";
	}
	std::cout << "\n";
}

// making a node iterable
auto node::begin() {return values.begin();}
auto node::end() {return values.end();}

auto node::begin() const {return values.begin();}
auto node::end() const {return values.end();}

// for making the [] notation work with the node
int& node::operator[](std::size_t index) {return values[index];}
const int& node::operator[](std::size_t index) const {return values[index];}

std::size_t node::size() {return values.size();}




	// recursive function to build the tree
void kdtree::next_node(std::vector<std::vector<int>> data, int depth){
	if (data.size() == 0){
		std::cerr << "empty array" << "\n";
		return;
	}
	int k;
	if (depth == 0){
		k = 0;
	} else {
		k = depth % data[0].size();
	}
	if (data.size() == 1){
		tree.push_back(node(data[0], depth));
		// std::cout << tree.back().values[0] <<" leaf " << tree.back().depth << "\n";
	} else if (data.size() == 2){
		std::sort(data.begin(), data.end(), [k] (const auto& a, const auto& b) {
			return a[k] < b[k];});
		int cur = tree.size();
		tree.push_back(node(data[1], depth));
		// std::cout << tree.back().values[0] <<" single " << tree.back().depth << "\n";
		tree[cur].left = tree.size();
		next_node({data[0]}, depth + 1);
	} else {
		int m = data.size() / 2;
		std::sort(data.begin(), data.end(), [k] (const auto& a, const auto& b) {
			return a[k] < b[k];});
		int cur = tree.size();
		
		tree.push_back(node(data[m], depth));
		// std::cout << tree.back().values[0] <<" double " << tree.back().depth << "\n";
		tree[cur].left = tree.size();
		next_node(std::vector<std::vector<int>>(data.begin(), data.begin() + m), depth + 1);
		tree[cur].right = tree.size();
		next_node(std::vector<std::vector<int>>(data.begin() + m + 1, data.end()), depth + 1);
	}
}

	
long kdtree::dist_sqrd(std::vector<int> target, std::vector<int> cur){
	long dist = 0;
	for (std::size_t i = 0; i < target.size(); i++){
		dist += (target[i] - cur[i]) * (target[i] - cur[i]);
	}
	return dist;
}

	// find the closest of 2 nodes to a point, used in the "nearest()" function
node kdtree::closest(std::vector<int> target, node temp, node cur){
	if (dist_sqrd(target, cur.values) < dist_sqrd(target, temp.values)){
		return cur;
	} else {
		return temp;
	}
}


kdtree::kdtree(std::vector<std::vector<int>> data){
	next_node(data);
}

node kdtree::nearest(std::vector<int> target, int cur_pos){
	node cur = tree[cur_pos];
	int k = cur.depth % cur.size();
	int next;
	int other;

	// handle cases possible cases of leaf, single branch and split branch
	if (cur.left < 0 && cur.right < 0){
		return cur;
	} else if (cur.right < 0){

		next = cur.left;
		other = cur_pos;

		// std::cout << "single in search" << \n;
		node temp = nearest(target, next);
		node best = closest(target, temp, cur);

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

		node temp = nearest(target, next);
		node best = closest(target, temp, cur);

		long r_sqrd = dist_sqrd(target, best.values);
		long dist = (target[k] - cur[k]) * (target[k] - cur[k]);

		if (r_sqrd >= dist){
			temp = nearest(target, other);
			best = closest(target, temp, best);
		}
		return best;
	}
}

	// if you want to print a single value of the nodes in the tree you have to also pass print_depth
void kdtree::print(bool print_depth, int k){
	if (k < 0){
		for (node point : tree){
			for (int value : point){
				std::cout << value << " ";
			}
			if (print_depth == true){
				std::cout << point.depth << "\n";
			} else {
				std::cout << "\n";
			}
		}
	} else {
		for (node point : tree){
			k = k % point.size();
			std::cout << point[k];
			if (print_depth == true){
				std::cout << point.depth << "\n";
			} else {
				std::cout << "\n";
			}
		}
	}
}

	
	// making tree iterable
auto kdtree::begin() {return tree.begin();}
auto kdtree::end() {return tree.end();}

auto kdtree::begin() const {return tree.begin();}
auto kdtree::end() const {return tree.end();}

// for making the [] notation work with tree
node& kdtree::operator[](std::size_t index) {return tree[index];}
const node& kdtree::operator[](std::size_t index) const {return tree[index];}

std::size_t kdtree::size() {return tree.size();}


// int main(){
// 	int rows = 12;
// 	int cols = 3;
//     int min_val = 0;
//     int max_val = 255;
// 
//     // Initialize random number generator
//     random_device rd;  // seed
//     mt19937 gen(rd()); // Mersenne Twister engine
//     uniform_int_distribution<> dist(min_val, max_val);
// 
//     // Create and fill 2D std::vector
//     std::vector<std::vector<int>> matrix(rows, std::vector<int>(cols));
//     for (int i = 0; i < rows; ++i){
//         for (int j = 0; j < cols; ++j){
//             matrix[i][j] = dist(gen);
//         }
//     }
// 	kdtree sus(matrix);
// 
// 	std::cout << \n;
// 	sus.print(true);
// 
// 	node si = sus.nearest({2,214,145});
// 	si.print();
// 
// 	return 0;
// }
