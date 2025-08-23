#include <iostream>
#include <algorithm>
#include <vector>
#include <random>

using namespace std;

// struct kdtree{
// 	kdtree* parent = nullptr;
// 	kdtree* left = nullptr;
// 	kdtree* right = nullptr;
// 	vector<int> point;
// 	int depth;
// 
// 	kdtree(vector<vector<int>> data, int depth = 0) : depth(depth) {
// 		if (data.size() == 0){
// 			cerr << "Cannot make a tree out of an empty array." << endl;
// 		}
// 		int k;
// 		if (depth == 0){
// 			k = 0;
// 		} else {
// 			k = depth % data[0].size();
// 		}
// 		
// 		if (data.size() == 1){
// 			point = data[0];
// 		} else if (data.size() == 2){
// 			sort(data.begin(), data.end(), [k](const auto& a, const auto& b) {
// 				return a[k] < b[k];});
// 			point = data[0];
// 			right = new kdtree({data[1]}, depth + 1);
// 		} else{
// 			sort(data.begin(), data.end(), [k](const auto& a, const auto& b) {
// 				return a[k] < b[k];});
// 			int m = data.size()/2;
// 			point = data[m];
// 			left = new kdtree(vector<vector<int>>(data.begin(), data.begin() + m), depth + 1);
// 			right = new kdtree(vector<vector<int>>(data.begin() + m + 1, data.end()), depth + 1);
// 		}
// 	}
// 
// 	vector<int> nearest(vector<int> data){
// 		int k;
// 		if (depth == 0){
// 			k = 0;
// 		} else {
// 			k = depth % data.size();
// 		}
// 
// 		int cache_cur = data[k] - point[k];
// 		int cache_l;
// 		int cache_r;
// 		if (left != nullptr){
// 			cache_l = data[k] - left->point[k];
// 			cache_l = (cache_cur - cache_l) * (cache_cur - cache_l);
// 		}
// 		if (right != nullptr){
// 			cache_r = data[k] - right->point[k];
// 			cache_r = (cache_cur - cache_r) * (cache_cur - cache_l);
// 		}
// 
// 		if (cache_l < cache_r){
// 			left->nearest(data);
// 		} else if (cache_r < cache_l){
// 			right->nearest(data);
// 		} else if (left == nullptr && right != nullptr){
// 			right->nearest(data);
// 		} else {
// 			cout << "leaf reached" << endl;
// 			return point;
// 		}
// 	}
// };

class kdtree{
	void next_node(vector<vector<int>> data, int depth = 0){
		if (data.size() == 0){
			cerr << "empty array" << endl;
			return;
		}
		int k;
		if (depth == 0){
			k = 0;
		} else {
			k = depth % data[0].size();
		}
		if (data.size() == 1){
			node cur(data[0], depth++);
			
		}
	}
public:
	vector<node> tree;
	struct node{
		int left = -1;
		int right = -1;
		// int parent = -1;
		vector<int> value;

		node(vector<int> data, int depth){
			value = data;
		}
	}
};

int main(){
	int rows = 12;
	int cols = 3;
    int min_val = 0;
    int max_val = 255;

    // Initialize random number generator
    random_device rd;  // seed
    mt19937 gen(rd()); // Mersenne Twister engine
    uniform_int_distribution<> dist(min_val, max_val);

    // Create and fill 2D vector
    vector<vector<int>> matrix(rows, vector<int>(cols));
    for (int i = 0; i < rows; ++i){
        for (int j = 0; j < cols; ++j){
            matrix[i][j] = dist(gen);
        }
    }
	kdtree sus(matrix);
	return 0;
}
