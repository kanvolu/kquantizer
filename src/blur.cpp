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


// EDGE DETECTION LOGIC

grid<int> dog(grid<int> const mat, float s_sigma){
	float b_sigma = 1.6 * s_sigma;
	int kernel_radius = 3 * b_sigma;
	grid<int> out(mat.height(), mat.width());

	grid<int> blurred_big_sigma = mat.convolve(g_kernel(2 * kernel_radius + 1, b_sigma));
	grid<int> blurred_small_sigma = mat.convolve(g_kernel(2 * kernel_radius + 1, s_sigma));

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
	grid<float> float_mat(mat);

	grid<float> horizontal = float_mat.convolve(sobel_xv, sobel_xh);
	grid<float> vertical = float_mat.convolve(sobel_yv, sobel_yh);

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
