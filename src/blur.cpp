#include <iostream>
#include <vector>
#include <cmath>

#include "grid.h"
#include "blur.h"

float gaus(float x, float deviation){
	x = exp(-(x * x) / (2 * deviation * deviation));
	x = x / (sqrt(2 * M_PI) * deviation);
	return x;
}

grid<float> g_kernel(size_t size, float deviation){
	grid<float> kernel(size, size);
	float dist;
	float sum = 0;
	float cen = floor(size / 2);
	
	for (size_t i = 0; i < size; i++){
		for (size_t j = 0; j < size; j++){
			dist = sqrt(pow(i - cen, 2) + pow(j - cen, 2));
			kernel[i][j] = gaus(dist, deviation);
			sum += kernel[i][j];
		}
	}

	kernel /= sum;
	
	return kernel;
}

grid<int> convolve (grid<int> mat, grid<float> const &kernel, grid<float> const * mask){
	grid<int> padded = mat;
	padded.pad(kernel.height() / 2);

	//invariants
	size_t const mh = mat.height();
	size_t const mw = mat.width();
	size_t const kh = kernel.height();
	size_t const kw = kernel.width();


	if (!mask) {

	for (size_t start_y = 0; start_y < mh; start_y++){
		for (size_t start_x = 0; start_x < mw; start_x++){
			float sum = 0;
			for (size_t i = 0; i < kh; i++){
				for (size_t j = 0; j < kw; j++){
					sum += padded[start_y + i][start_x + j] * kernel[i][j];
				}
			}

			mat[start_y][start_x] = sum;
		}
	}
	
	} else {

	for (size_t start_y = 0; start_y < mh; start_y++){
		for (size_t start_x = 0; start_x < mw; start_x++){
			float sum = 0;
			for (size_t i = 0; i < kh; i++){
				for (size_t j = 0; j < kw; j++){
					sum += padded[start_y + i][start_x + j] * kernel[i][j];
				}
			}

			mat[start_y][start_x] = mat[start_y][start_x] * (*mask)[start_y][start_x] + sum * (1 - (*mask)[start_y][start_x]);
		}
	}
		
	}

	return mat;
}

// EDGE DETECTION LOGIC

grid<int> dog(grid<int> const mat, float s_sigma){
	float b_sigma = 1.6 * s_sigma;
	int kernel_radius = 3 * b_sigma;
	grid<int> out(mat.height(), mat.width());

	grid<int> blurred_big_sigma = convolve(mat, g_kernel(2 * kernel_radius + 1, b_sigma));
	grid<int> blurred_small_sigma = convolve(mat, g_kernel(2 * kernel_radius + 1, s_sigma));

	int max = 0;
	
	for (size_t i = 0; i < out.height(); i++){
		for (size_t j = 0; j < out.width(); j++){
			out[i][j] = abs(blurred_big_sigma[i][j] - blurred_small_sigma[i][j]);
			if (max < out[i][j]){max = out[i][j];}
		}
	}

	out *= float(255) / float(max);

	return out;
	
}


grid<float> detect_edges_sobel(grid<int> const &mat){
	grid<float> new_mat(mat);

	grid<float> horizontal = convolve(new_mat, sobel_x);
	grid<float> vertical = convolve(new_mat, sobel_y);

	float max = 0;
	for (size_t i = 0; i < horizontal.height(); i++){
		for (size_t j = 0; j < horizontal.width(); j++){
			horizontal[i][j] = sqrt(horizontal[i][j] * horizontal[i][j] + vertical[i][j] * vertical[i][j]);
			if (max < horizontal[i][j]){
				max = horizontal[i][j];
			}
		}
	}

	horizontal /= max;

	return horizontal;
}
