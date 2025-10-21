#include <iostream>
#include <vector>
#include <cmath>



using namespace std;

float gaus(float x, float deviation){
	x = exp(-(x * x) / (2 * deviation * deviation));
	x = x / (sqrt(2 * M_PI) * deviation);
	return x;
}


vector<vector<float>> g_kernel(int size, float deviation){
	vector<vector<float>> kernel(size, vector<float>(size, 0));
	float dist;
	float sum = 0;
	int cen = (size - 1) / 2;
	
	for (int i = 0; i < size; i++){
		for (int j = 0; j < size; j++){
			dist = sqrt(pow(i - cen, 2) + pow(j - cen, 2));
			kernel[i][j] = gaus(dist, deviation);
			sum += kernel[i][j];
		}
	}

	for (auto& row : kernel){ // normalize kernel
		for (auto& val : row){
			val /= sum;
		}
	}
	
	return kernel;
}

int colapse(vector<vector<int>> const mat, vector<vector<float>> kernel){
	float sum = 0;
	
	if (mat.size() != kernel.size() || mat[0].size() != kernel[0].size()){
		cerr << "kernel and matrix to colapse are of different size\n";
		return 0;
	}
	
	for (int i = 0; i < mat.size(); i++){
		for (int j = 0; j < mat[0].size(); j++){
			sum += mat[i][j] * kernel[i][j];
		}
	}
	return static_cast<int>(sum);
}

vector<vector<int>> pad(vector<vector<int>> mat, int padding){
	for (auto& row : mat){
		row.insert(row.begin(), padding, 0);
		row.insert(row.end(), padding, 0);
	}

	mat.insert(mat.begin(), padding, vector<int>(mat[0].size(), 0));
	mat.insert(mat.end(), padding, vector<int>(mat[0].size(), 0));
	
	return mat;
}

vector<vector<int>> slice(vector<vector<int>> const *mat, int start_x, int start_y, int size){
	vector<vector<int>> out(size, vector<int>(size));
	
	for (int i = 0; i < size; i++){
		for (int j = 0; j < size; j++){
			out[i][j] = (*mat)[i + start_x][j + start_y];
		}
	}

	return out;
}

vector<vector<int>> convolve(vector<vector<int>> const mat, vector<vector<float>> const kernel){
	vector<vector<int>> padded = pad(mat, kernel.size()/2);
	vector<vector<int>> out = mat;
	
	for (int i = 0; i < out.size(); i++){
		for (int j = 0; j < out[0].size(); j++){
			out[i][j] = colapse(slice(&padded, i, j, kernel.size()), kernel);
		}
	}

// 	cout << "Convolution:\n";
// 
// 	for (int i = 200; i < 210; i++){
// 		for (int j = 200; j < 210; j++){
// 			cout << out[i][j] << " ";
// 		}
// 		cout << "\n";
// 	}
	
	return out;
}

vector<vector<int>> dog(vector<vector<int>> const mat, float s_sigma){
	float b_sigma = 1.6 * s_sigma;
	int kernel_radius = 3 * b_sigma;
	vector<vector<int>> out(mat.size(), vector<int>(mat[0].size()));

	// cout << "Small sigma: " << s_sigma << "\nBig sigma: " << b_sigma << "\nRadius: " << kernel_radius << endl;

	vector<vector<int>> blurred_big_sigma = convolve(mat, g_kernel(2 * kernel_radius + 1, b_sigma));
	vector<vector<int>> blurred_small_sigma = convolve(mat, g_kernel(2 * kernel_radius + 1, s_sigma));

	int max = 0;
	
	for (int i = 0; i < out.size(); i++){
		for (int j = 0; j < out[0].size(); j++){
			out[i][j] = abs(blurred_big_sigma[i][j] - blurred_small_sigma[i][j]);
			if (max < out[i][j]){max = out[i][j];}
		}
	}


	for (auto& row : out){
		for (auto& val : row){
			val = (val * 255) / max;
		}
	}

	return out;
	
}
