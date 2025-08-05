#define _USE_MATH_DEFINE
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "lodepng.h"

#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

float gaus(float x, float deviation){
	x = exp(-(x * x) / (2 * deviation * deviation));
	x = x / (sqrt(2 * M_PI) * deviation);
	return x;
}


int main(){
	const char* path = "../lancia.jpeg";
	int width, height, channels;
	unsigned char* raw = stbi_load(path, &width, &height, &channels, 4);
	if (raw == nullptr){
		cout << "failed" << endl;
		return 1;
	}
	cout << width << " x " << height << endl;
	vector<vector<vector<unsigned char>>> img;
	for (int w = 0; w < width * height; w++){
		
	}
	return 0;
}
