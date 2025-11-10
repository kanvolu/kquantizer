#pragma once

#include <iostream>
#include <vector>
#include <cmath>

#include "reshaping.cpp"

float gaus(float x, float deviation){
	x = exp(-(x * x) / (2 * deviation * deviation));
	x = x / (sqrt(2 * M_PI) * deviation);
	return x;
}


std::vector<std::vector<float>> g_kernel(int size, float deviation){
	std::vector<std::vector<float>> kernel(size, std::vector<float>(size));
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

template <typename T, typename U = float>
T colapse(std::vector<std::vector<T>> const &mat, std::vector<std::vector<U>> const &kernel){
	U sum = 0;
	
	if (mat.size() != kernel.size() || mat[0].size() != kernel[0].size()){
		std::cerr << "kernel and matrix to colapse are of different size" << std::endl;
		return 0;
	}
	
	for (size_t i = 0; i < mat.size(); i++){
		for (size_t j = 0; j < mat[0].size(); j++){
			sum += static_cast<U>(mat[i][j]) * kernel[i][j];
		}
	}
	return static_cast<T>(sum);
}

template <typename T>
std::vector<std::vector<T>> pad(std::vector<std::vector<T>> mat, int padding){
	for (auto& row : mat){
		row.insert(row.begin(), padding, 0);
		row.insert(row.end(), padding, 0);
	}

	mat.insert(mat.begin(), padding, std::vector<T>(mat[0].size(), 0));
	mat.insert(mat.end(), padding, std::vector<T>(mat[0].size(), 0));
	
	return mat;
}

template <typename T>
std::vector<std::vector<T>> slice(std::vector<std::vector<T>> const &mat, size_t const start_y, size_t const start_x, size_t const size){
	std::vector<std::vector<T>> out(size, std::vector<T>(size));
	
	for (size_t i = 0; i < size; i++){
		for (size_t j = 0; j < size; j++){
			out[i][j] = mat[i + start_y][j + start_x];
		}
	}

	return out;
}

template <typename T, typename U = float>
std::vector<std::vector<T>> convolve(std::vector<std::vector<T>> const mat, std::vector<std::vector<U>> const kernel){
	std::vector<std::vector<T>> padded = pad(mat, kernel.size()/2);
	std::vector<std::vector<T>> out = mat;
	
	for (size_t i = 0; i < out.size(); i++){
		for (size_t j = 0; j < out[0].size(); j++){
			out[i][j] = colapse(slice(padded, i, j, kernel.size()), kernel);
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


template <typename T>
std::vector<std::vector<T>> apply_blur(std::vector<std::vector<T>> mat, std::vector<std::vector<float>> const &kernel){
	std::vector<std::vector<T>> padded = pad(mat, kernel.size() / 2);

	for (size_t i = 0; i < mat.size(); i++){
		for (size_t j = 0; j < mat[i].size(); j++){
			mat[i][j] = colapse(slice<T>(padded, i, j, kernel.size()), kernel);
		}
	}

	return mat;
}

template <typename T>
std::vector<std::vector<T>> dog(std::vector<std::vector<T>> const mat, float s_sigma){
	float b_sigma = 1.6 * s_sigma;
	int kernel_radius = 3 * b_sigma;
	std::vector<std::vector<T>> out(mat.size(), std::vector<T>(mat[0].size()));

	// cout << "Small sigma: " << s_sigma << "\nBig sigma: " << b_sigma << "\nRadius: " << kernel_radius << endl;

	std::vector<std::vector<T>> blurred_big_sigma = convolve(mat, g_kernel(2 * kernel_radius + 1, b_sigma));
	std::vector<std::vector<T>> blurred_small_sigma = convolve(mat, g_kernel(2 * kernel_radius + 1, s_sigma));

	T max = 0;
	
	for (size_t i = 0; i < out.size(); i++){
		for (size_t j = 0; j < out[0].size(); j++){
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

// EDGE DETECTION LOGIC
const std::vector<std::vector<int>> sobel_x = {
	{-1, 0, 1},
	{-2, 0, 2},
	{-1, 0, 1}
};

const std::vector<std::vector<int>> sobel_y = {
	{-1, -2, -1},
	{ 0,  0,  0},
	{ 1,  2,  1}
};


template<typename T = int>
std::vector<std::vector<float>> detect_edges_sobel(std::vector<std::vector<T>> const &mat){
	std::vector<std::vector<float>> new_mat(mat.size());
	if constexpr (!std::is_same_v<T, float>){
		for (size_t i = 0; i < mat.size(); i++){
			new_mat[i] = vector_converter<T, float>(mat[i]);
		}
	}

	std::vector<std::vector<float>> horizontal = convolve(new_mat, sobel_x);
	std::vector<std::vector<float>> vertical = convolve(new_mat, sobel_y);

	float max = 0;
	for (size_t i = 0; i < horizontal.size(); i++){
		for (size_t j = 0; j < horizontal[0].size(); j++){
			horizontal[i][j] = sqrt(horizontal[i][j] * horizontal[i][j] + vertical[i][j] * vertical[i][j]);
			if (max < horizontal[i][j]){
				max = horizontal[i][j];
			}
		}
	}

	for (auto& row : horizontal){
		for (auto& val : row){
			val = val / max;
		}
	}

	return horizontal;
}
