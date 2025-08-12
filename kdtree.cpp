#include <iostream>
#include <vector>

using namespace std;

struct node{
	vector<int> data;
	int parent;
	int left;
	int right;

	node(vector<int> input){
		data = input;
	}

	// auto begin(){return data.begin();}
	// auto end(){return data.end();}
	// auto begin() const {return data.begin();}
	// auto end() const {return data.end();}

	void set_left(int input){left = input;}
	void set_right(int input){right = input;}
	void set_parent(int input){parent = input;}
};

struct kdtree{
private:
	vector<node> av_nodes;

public:
	vector<node> tree;
	int dimension;

	kdtree(vector<vector<int>> input){
		dimension = input[0].size();
		for (vector point : input){
			av_nodes.push_back(point);
		}

		tree.push_back(av_nodes[0]);

		for (int i = 0; i < av_nodes.size(); i++){
			// left side
			for (int j = 0; j < av_nodes.size(); j++){
				if (av_nodes[j].data[0] < tree[i].data[0]){
					tree.push_back(av_nodes[j]);
					tree[i].set_left(tree.size() - 1);
					av_nodes.erase(av_nodes.begin() + j);
				}
			}
			// right side
			for (int j = 0; j < av_nodes.size(); j++){
				if (av_nodes[j].data[0] >= tree[i].data[0]){
					tree.push_back(av_nodes[j]);
					tree[i].set_right(tree.size() - 1);
					av_nodes.erase(av_nodes.begin() + j);
				}
			}

		}
	}
};

int main(){
	return 0;
}
