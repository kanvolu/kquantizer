#pragma once

#include <iostream>
#include <vector>
#include <algorithm>


template <typename T>
class grid {

size_t m_height, m_width;
std::vector<T> m_data;

public:

grid ()
	: m_height(0), m_width(0), m_data() {}

grid (size_t height, size_t width)
	: m_height(height), m_width(width), m_data(m_height * m_width) {}

grid (size_t height, size_t width, T const &value)
	: m_height(height), m_width(width), m_data(m_height * m_width, value) {}

grid (size_t height, size_t width, std::vector<T> data)
	: m_height(height), m_width(width), m_data(data) {}

template <typename U>
grid(const grid<U>& other)
	: m_height (other.height()), m_width (other.width()), m_data (m_height * m_width)
{
	if constexpr (std::is_same_v<T, U>) {
		std::copy(other.m_data.begin(), other.m_data.end(), m_data.begin());
	} else {
		for (size_t i = 0; i < other.size(); i++){
			m_data[i] = static_cast<T>(other.data()[i]);
		}	
	}
}

void reshape_raw(size_t height, size_t width){ // USED MOSTLY FOR UNINITIALIZED GRIDS
	m_height = height;
	m_width = width;
	m_data.resize(m_height * m_width);
}

inline size_t size() const { return m_data.size(); }
inline size_t width() const { return m_width; }
inline size_t height() const { return m_height; }
inline std::vector<T>& data() { return m_data; }
inline const std::vector<T>& data() const { return m_data; }
inline T* raw() { return m_data.data(); }
inline const T* raw() const { return m_data.data(); }

// FAST ACCESS WITH NO CHECKS
inline T* operator[] (size_t row_index) {
	return m_data.data() + row_index * m_width;
}

inline const T* operator[] (size_t row_index) const {
	return m_data.data() + row_index * m_width;
}

// SLOW ACCESS WITH CHECKS
T& operator() (size_t row_index, size_t column_index) {
	return m_data.at(row_index * m_width + column_index);
}

const T& operator() (int row_index, int column_index) const {
	return m_data.at(row_index * m_width + column_index);
}


template <typename F, typename... Args>
void mutate (F&& func, Args&&... args) {
	size_t size = this->size();
	T* __restrict data = this->raw();

	for (size_t i = 0; i < size; i++) {
		func(data[i], std::forward<Args>(args)...);
	}
}

template <typename F, typename... Args>
grid<T> transformed (F&& func, Args&&... args) const {
	grid<T> new_grid(this->height(), this->width());
	const size_t size = this->size();
	const T* __restrict data = this->raw();
	T* __restrict new_data = new_grid.raw();

	for (size_t i = 0; i < size; i++){
		new_data[i] = func(data[i], std::forward<Args>(args)...);
	}

	return new_grid;
}


grid<T> pad(size_t const padding, T const &value = T{}) const {
	if (padding == 0) return *this;

	size_t const new_height = m_height + (padding * 2);
	size_t const new_width = m_width + (padding * 2);

	grid<T> new_grid(new_height, new_width, value);

	for (size_t i = 0; i < m_height; i++) {
		std::copy_n(
			m_data.data() + i * m_width,
			m_width,
			new_grid.raw() + (i + padding) * new_width + padding
		);
	}

	return new_grid;
}


grid<T> pad_vh(size_t const v_pad, size_t const h_pad, T const &value = T{}) const {
	if (v_pad == 0 && h_pad == 0) return *this;

	size_t const new_height = m_height + (v_pad * 2);
	size_t const new_width = m_width + (h_pad * 2);

	grid<T> new_grid(new_height, new_width, value);

	for (size_t i = 0; i < m_height; i++) {
		std::copy_n(
			m_data.data() + i * m_width,
			m_width,
			new_grid.raw() + (i + v_pad) * new_width + h_pad
		);
	}

	return new_grid;
}


template <typename U>
grid<T> convolve(grid<U> const &kernel) const {
	size_t const kh = kernel.height();
	size_t const kw = kernel.width();

	if (kh % 2 == 0 || kw % 2 == 0) {
		throw std::out_of_range("Grid to convolve must have odd size.");
	}
	
	grid<T> const padded = this->pad_vh(kh / 2, kw / 2);
	grid<T> out(m_height, m_width);

	for (size_t start_y = 0; start_y < m_height; start_y++) {
		for (size_t start_x = 0; start_x < m_width; start_x++) {
			std::common_type_t<T, U> sum = 0;

			for (size_t i = 0; i < kh; i++) {
				for (size_t j = 0; j < kw; j++) {
					sum += padded[start_y + i][start_x + j] * kernel[i][j];
				}
			}

			out[start_y][start_x] = sum;
		}
	}

	return out;
}

template <typename U>
grid<T> convolve(grid<U> const &kernel, grid<float> const &mask) const {
	size_t const kh = kernel.height();
	size_t const kw = kernel.width();

	if (kh % 2 == 0 || kw % 2 == 0) {
		throw std::out_of_range("Grid to convolve must have odd size.");
	} else if (m_height != mask.height() || m_width != mask.width()) {
		throw std::out_of_range("Maks must be the same size as the grid to convolve.");
	}
	
	grid<T> const padded = this->pad_vh(kh / 2, kw / 2);
	grid<T> out(m_height, m_width);

	for (size_t start_y = 0; start_y < m_height; start_y++) {
		for (size_t start_x = 0; start_x < m_width; start_x++) {
			std::common_type_t<T, U, float> sum = 0;

			for (size_t i = 0; i < kh; i++) {
				for (size_t j = 0; j < kw; j++) {
					sum += padded[start_y + i][start_x + j] * kernel[i][j];
				}
			}

			out[start_y][start_x] = sum * mask[start_y][start_x] + (*this)[start_y][start_x] * (1 - mask[start_y][start_x]);
		}
	}

	return out;
}

template <typename U>
grid<T> convolve(std::vector<U> const &kernel) const {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	}

	grid<T> const padded = this->pad(pad);
	grid<U> temp(m_height + 2 * pad, m_width + 2 * pad, U{});
	grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.raw();
	const T* __restrict pd = padded.raw();
	U* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t pw = padded.width();
	size_t tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t start_y = y + pad;

		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad;
			std::common_type_t<T, U> sum = 0;
					
			for (size_t i = 0; i < ks; i++) {
				sum += pd[start_y * pw + x + i] * kd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad;
			std::common_type_t<T, U> sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += td[(y + i) * tw + start_x] * kd[i];
			}

			od[y * m_width + x] = sum;
		}
	}

	return out;
}

template <typename U>
grid<T> convolve(std::vector<U> const &kernel, grid<float> const &mask) const {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	} else if (m_height != mask.height() || m_width != mask.width()) {
		throw std::out_of_range("Maks must be the same size as the grid to convolve.");
	}

	grid<T> const padded = this->pad(pad);
	grid<U> temp(m_height + 2 * pad, m_width + 2 * pad, U{});
	grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.raw();
	const T* __restrict pd = padded.raw();
	const T* __restrict id = m_data.data();
	const float* __restrict md = mask.raw();
	U* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t pw = padded.width();
	size_t tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t start_y = y + pad;

		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad;
			std::common_type_t<T, U, float> sum = 0;
					
			for (size_t i = 0; i < ks; i++) {
				sum += pd[start_y * pw + x + i] * kd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad;
			std::common_type_t<T, U, float> sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += td[(y + i) * tw + start_x] * kd[i];
			}

			od[y * m_width + x] = sum * md[y * m_width + x] + id[y * m_width + x] * (1 - md[y * m_width + x]);
		}
	}

	return out;
}

template <typename U>
grid<T> convolve(std::vector<U> const &kernel_v, std::vector<U> const &kernel_h) const {
	size_t const kvs = kernel_v.size();
	size_t const pad_v = kvs / 2;
	size_t const khs = kernel_h.size();
	size_t const pad_h = khs / 2;

	if (kvs % 2 == 0 || khs % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	}

	grid<T> const padded = this->pad_vh(pad_v, pad_h);
	grid<U> temp(m_height + 2 * pad_v, m_width + 2 * pad_h, U{});
	grid<T> out(m_height, m_width, T{});

	const U* __restrict kvd = kernel_v.data();
	const U* __restrict khd = kernel_h.data();
	const T* __restrict pd = padded.raw();
	U* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t pw = padded.width();
	size_t tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t start_y = y + pad_v;

		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad_h;
			std::common_type_t<T, U> sum = 0;
					
			for (size_t i = 0; i < khs; i++) {
				sum += pd[start_y * pw + x + i] * khd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad_h;
			std::common_type_t<T, U> sum = 0;

			for (size_t i = 0; i < kvs; i++) {
				sum += td[(y + i) * tw + start_x] * kvd[i];
			}

			od[y * m_width + x] = sum;
		}
	}

	return out;
}

template <typename U>
grid<T> convolve(std::vector<U> const &kernel_v, std::vector<U> const &kernel_h, grid<float> const &mask) const {
	size_t const kvs = kernel_v.size();
	size_t const pad_v = kvs / 2;
	size_t const khs = kernel_h.size();
	size_t const pad_h = khs / 2;

	if (kvs % 2 == 0 || khs % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	} else if (m_height != mask.height() || m_width != mask.width()) {
		throw std::out_of_range("Maks must be the same size as the grid to convolve.");
	}

	grid<T> const padded = this->pad_vh(pad_v, pad_h);
	grid<U> temp(m_height + 2 * pad_v, m_width + 2 * pad_h, U{});
	grid<T> out(m_height, m_width, T{});

	const U* __restrict kvd = kernel_v.data();
	const U* __restrict khd = kernel_h.data();
	const T* __restrict pd = padded.raw();
	const T* __restrict id = m_data.data();
	const float* __restrict md = mask.raw();
	U* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t pw = padded.width();
	size_t tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t start_y = y + pad_v;

		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad_h;
			std::common_type_t<T, U, float> sum = 0;
					
			for (size_t i = 0; i < khs; i++) {
				sum += pd[start_y * pw + x + i] * khd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t start_x = x + pad_h;
			std::common_type_t<T, U, float> sum = 0;

			for (size_t i = 0; i < kvs; i++) {
				sum += td[(y + i) * tw + start_x] * kvd[i];
			}

			od[y * m_width + x] = sum * md[y * m_width + x] + id[y * m_width + x] * (1 - md[y * m_width + x]);
		}
	}

	return out;
}

// SHAPING
void resize (size_t height, size_t width, T const &value = T{}) {
	std::vector<T> new_data(height * width, value);
	// invariants
	size_t min_height = std::min(height, m_height);
	size_t min_width = std::min(width, m_width);
	T* __restrict raw = m_data.data();
	T* __restrict new_raw = new_data.data();
	
	for (size_t i = 0; i < min_height; i++){
		std::copy_n(
			raw + i * m_width,
			min_width,
			new_raw + i * width
		);
	}

	m_data.swap(new_data);
	m_height = height;
	m_width = width;
}

void transpose() {
	std::vector<T> new_data(m_height * m_width);

	T* __restrict raw = m_data.data();
	T* __restrict new_raw = new_data.data();

	for (size_t i = 0; i < m_height; i++) {
		size_t start_i = i * m_width;
		for (size_t j = 0; j < m_width; j++) {
			new_raw[j * m_height + i] = raw[start_i + j];
		}
	}

	m_data.swap(new_data);
	std::swap(m_height, m_width);
}

inline void append_rows (size_t amount, T const &value = T{}) {
	resize(m_height + amount, m_width, value);
}

inline void append_columns (size_t amount, T const &value = T{}) {
	resize(m_height, m_width + amount, value);
}

void insert_rows (size_t position, size_t amount, T const &value = T{}) {
	position = std::min(position, m_width);

	m_data.insert(
		m_data.begin() + position * m_width,
		amount * m_width,
		value
	);

	m_height += amount;
}

void insert_columns (size_t position, size_t amount, T const &value = T{}) {
	position = std::min(position, m_width);
	size_t new_width = m_width + amount;

	std::vector<T> new_data(m_height * new_width);

	for (size_t i = 0; i < m_height; i++){
		T* old_row = m_data.data() + i * m_width;
		T* new_row = new_data.data() + i * new_width;
		
		std::copy_n(
			old_row,
			position,
			new_row
		);

		std::fill_n(
			new_row + position,
			amount,
			value
		);

		std::copy_n(
			old_row + position,
			m_width - position,
			new_row + position + amount
		);
	}

	m_width = new_width;
	m_data.swap(new_data);
}


grid<T> slice (
	size_t start_y,
	size_t start_x,
	size_t size_y,
	size_t size_x
) const {
	if (start_y + size_y > m_height || start_x + size_x > m_width)
		throw std::out_of_range("Slice exceeds grid bounds.");
		
	grid<T> out(size_y, size_x);

	for (size_t i = 0; i < size_y; i++){
		std::copy_n(
			m_data.data() + (start_y + i) * m_width + start_x,
			size_x,
			out.m_data.data() + i * out.m_width
		);
	}

	return out;
}

bool empty() {
	return (m_data.size() == 0);
}

void print() {
	for (size_t i = 0; i < m_height; i++){
		for (size_t j = 0; j < m_width; j++){
			std::cout << (*this)[i][j] << " ";
		}
		std::cout << "\n";
	}
	std::cout << std::endl;
}

// UNARY OPERATORS
grid<T>& operator++ () {
	this->mutate([](T &x){
		return x++;
	});
	return *this;
}

grid<T>& operator-- () {
	this->mutate([](T &x){
		return x--;
	});
	return *this;
}

// SCALAR OPERATORS
template <typename U>
grid<T> operator+ (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x + s;
	}, scalar);
}


template <typename U>
grid<T>& operator+= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		return x += s;
	}, scalar);
	return *this;
}

template <typename U>
grid<T> operator- (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x - s;
	}, scalar);
}


template <typename U>
grid<T>& operator-= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		return x -= s;
	}, scalar);
	return *this;
}

template <typename U>
grid<T> operator* (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x * s;
	}, scalar);
}

template <typename U>
grid<T>& operator*= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		return x *= s;
	}, scalar);
	return *this;
}

template <typename U>
grid<T> operator/ (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x / s;
	}, scalar);
}

template <typename U>
grid<T>& operator/= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		return x /= s;
	}, scalar);
	return *this;
}

// GRID OPERATORS
template <typename U>
grid<T> operator+ (grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to sum have different dimensions.");
	}

	grid<T> out(this->height(), this->width());
	// invariants
	size_t size = this->size();
	T* __restrict out_data = out.raw();
	const T* __restrict this_data = this->raw();
	const U* __restrict input_data = input.raw();

	for (size_t i = 0; i < size; i++){
		out_data[i] = this_data[i] + input_data[i];
	}

	return out;
}

template <typename U>
grid<T>& operator+= (grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t size = this->size();
	T* __restrict data = this->raw();
	U* __restrict input_data = input.raw();
	
	for (size_t i = 0; i < size; i++){
		data[i] += input_data[i];
	}
	return *this;
}

template <typename U>
grid<T> operator- (grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to sum have different dimensions.");
	}

	grid<T> out(this->height(), this->width());
	// invariants
	size_t size = this->size();
	T* __restrict out_data = out.raw();
	const T* __restrict this_data = this->raw();
	const U* __restrict input_data = input.raw();

	for (size_t i = 0; i < size; i++){
		out_data[i] = this_data[i] - input_data[i];
	}

	return out;
}

template <typename U>
grid<T>& operator-= (grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t size = this->size();
	T* __restrict data = this->raw();
	U* __restrict input_data = input.raw();
	
	for (size_t i = 0; i < size; i++){
		data[i] -= input_data[i];
	}
	return *this;
}

template <typename U>
grid<T> operator* (grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}

	grid<T> out(this->height(), this->width());
	// invariants
	size_t size = this->size();
	T* __restrict out_data = out.raw();
	const T* __restrict this_data = this->raw();
	const U* __restrict input_data = input.raw();

	for (size_t i = 0; i < size; i++){
		out_data[i] = this_data[i] * input_data[i];
	}

	return out;
}

template <typename U>
grid<T>& operator*= (grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t size = this->size();
	T* __restrict data = this->raw();
	U* __restrict input_data = input.raw();
	
	for (size_t i = 0; i < size; i++){
		data[i] *= input_data[i];
	}
	return *this;
}

template <typename U>
grid<T> operator/ (grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}

	grid<T> out(this->height(), this->width());
	// invariants
	size_t size = this->size();
	T* __restrict out_data = out.raw();
	const T* __restrict this_data = this->raw();
	const U* __restrict input_data = input.raw();

	for (size_t i = 0; i < size; i++){
		out_data[i] = this_data[i] / input_data[i];
	}

	return out;
}

template <typename U>
grid<T>& operator/= (grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t size = this->size();
	T* __restrict data = this->raw();
	U* __restrict input_data = input.raw();
	
	for (size_t i = 0; i < size; i++){
		data[i] /= input_data[i];
	}
	return *this;
}

};


// NON-MEMBER BINARY OPERATORS
template <typename T, typename U>
grid<T> operator- (U const &scalar, grid<T> const &mat) {
	return mat.transformed([](T const &x, U const &s){
		return s - x;
	}, scalar);
}

template <typename T, typename U>
grid<T> operator/ (U const &scalar, grid<T> const &mat) {
	return mat.transformed([](T const &x, U const &s){
		return s / x;
	}, scalar);
}
