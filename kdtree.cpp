#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

struct kdtree{
	kdtree* parent = nullptr;
	kdtree* left = nullptr;
	kdtree* right = nullptr;
	vector<int> point;
	int depth;

	kdtree(vector<vector<int>> data, int depth = 0) : depth(depth) {
		if (data.size() == 0){
			cerr << "Cannot make a tree out of an empty array." << endl;
		}
		int k;
		if (depth == 0){
			k = 0;
		} else {
			k = depth % data[0].size();
		}
		
		if (data.size() == 1){
			point = data[0];
		} else if (data.size() == 2){
			sort(data.begin(), data.end(), [k](const auto& a, const auto& b) {
				return a[k] < b[k];});
			point = data[0];
			right = new kdtree({data[1]}, depth + 1);
		} else{
			sort(data.begin(), data.end(), [k](const auto& a, const auto& b) {
				return a[k] < b[k];});
			int m = data.size()/2;
			point = data[m];
			left = new kdtree(vector<vector<int>>(data.begin(), data.begin() + m), depth + 1);
			right = new kdtree(vector<vector<int>>(data.begin() + m + 1, data.end()), depth + 1);
		}
	}
	
};


int main(){
	kdtree sus({{1,2,3},{4,5,6},{7,8,9}}, 0);
	return 0;
}
