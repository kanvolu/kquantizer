#include <iostream>
#include <vector>

using namespace std;

class node{
private:
	int parent;
	int left;
	int right;
	vector<int> color;
public:
	node(vector<int> data){
		color = data;
	}

	void set_children(int l_child, int r_child){
		left = l_child;
		right = r_child;
	}

	int get_left(){
		return left;
	}

	int get_right(){
		return right;
	}

	void print(){
		for (int value : color){
			cout << value << " ";
		}
		cout << endl;
	}

	int r(){
		return color[0];
	}

	int g(){
		return color[1];
	}

	int b(){
		return color[2];
	}
};

class k3dtree{
private:
	vector<node> tree;
public:
	k3dtree(vector<vector<int>> palette){
		for (vector<int> color : palette){
				
		}
	}	

	void print(){
		for (node color : tree){
			color.print();
		}
	}
};

int main(){
	vector<vector<int>> palette;
	
	for (int i = 0; i < 5; ++i) {
		vector<int> color;
		for (int j = 0; j < 3; ++j) {
			color.push_back(rand() % 256);
	    }     
	    palette.push_back(color);  
	}

	k3dtree tree_p(palette);
	tree_p.print();
	
	return 0;
}
