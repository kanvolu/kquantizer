#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <limits>

template <typename T>
class Grid {

size_t m_height, m_width;
std::vector<T> m_data;

public:

Grid ()
	: m_height(0), m_width(0), m_data() {}

Grid (size_t const height, size_t const width)
	: m_height(height), m_width(width), m_data(m_height * m_width) {}

Grid (size_t const height, size_t const width, T const &value)
	: m_height(height), m_width(width), m_data(m_height * m_width, value) {}

template <typename U>
Grid (size_t const height, size_t const width, U const *ptr)
	: m_height(height), m_width(width), m_data(ptr, ptr + width * height) {}

template <typename U>
Grid (size_t const height, size_t const width, std::vector<U> data)
	: m_height(height), m_width(width), m_data(data.data().begin(), data.data().end()) {}

template <typename U>
Grid (Grid<U> const &other)
	: m_height (other.height()), m_width (other.width()), m_data (other.data().begin(), other.data().end()) {}

Grid<T>& reshape_raw(size_t const height, size_t const width){ // USED MOSTLY FOR UNINITIALIZED GRIDS
	m_height = height;
	m_width = width;
	m_data.resize(m_height * m_width);
	return *this;
}

inline bool	empty() const { return m_data.empty(); }
inline size_t size() const { return m_data.size(); }
inline size_t width() const { return m_width; }
inline size_t height() const { return m_height; }
inline std::vector<T>& data() { return m_data; }
inline const std::vector<T>& data() const { return m_data; }
inline T* raw() { return m_data.data(); }
inline const T* raw() const { return m_data.data(); }

// FAST ACCESS WITH NO CHECKS
inline T* operator[] (size_t const row_index) {
	return m_data.data() + row_index * m_width;
}

inline const T* operator[] (size_t const row_index) const {
	return m_data.data() + row_index * m_width;
}

// SLOW ACCESS WITH CHECKS
T& operator() (size_t const row_index, size_t const column_index) {
	return m_data.at(row_index * m_width + column_index);
}

const T& operator() (int const row_index, int const column_index) const {
	return m_data.at(row_index * m_width + column_index);
}


template <typename F, typename... Args>
Grid<T>& mutate (F&& func, Args&&... args) {
	size_t const size = this->size();
	T* __restrict data = this->raw();

	for (size_t i = 0; i < size; i++) {
		std::invoke(std::forward<F>(func),
			data[i],
			std::forward<Args>(args)...);
	}

	return *this;
}

template <typename F, typename... Args>
Grid<T> transformed (F&& func, Args&&... args) const {
	Grid<T> new_grid(this->height(), this->width());
	const size_t size = this->size();
	const T* __restrict data = this->raw();
	T* __restrict new_data = new_grid.raw();

	for (size_t i = 0; i < size; i++){
		new_data[i] = std::invoke(std::forward<F>(func),
			data[i],
			std::forward<Args>(args)...);
	}

	return new_grid;
}

T max() const {
	const T* __restrict data = m_data.data();
	size_t const size = m_data.size();

	T max = m_data[0];
	for (size_t i = 1; i < size; i++) {
		if (max < data[i]) max = data[i];
	}

	return max;
}

T abs_max()	const {
	const T* __restrict data = m_data.data();
	size_t const size = m_data.size();

	T max = {};
	for (size_t i = 0; i < size; i++) {
		if (max < std::abs(data[i])) max = std::abs(data[i]);
	}

	return max;
}

Grid<T>& normalize() {
	return *this /= abs_max();
}

Grid<T> pad(size_t const padding, T const &value = T{}) const {
	if (padding == 0) return *this;

	size_t const new_height = m_height + (padding * 2);
	size_t const new_width = m_width + (padding * 2);

	Grid<T> new_grid(new_height, new_width, value);

	for (size_t i = 0; i < m_height; i++) {
		std::copy_n(
			m_data.data() + i * m_width,
			m_width,
			new_grid.raw() + (i + padding) * new_width + padding
		);
	}

	return new_grid;
}


Grid<T> pad_vh(size_t const v_pad, size_t const h_pad, T const &value = T{}) const {
	if (v_pad == 0 && h_pad == 0) return *this;

	size_t const new_height = m_height + (v_pad * 2);
	size_t const new_width = m_width + (h_pad * 2);

	Grid<T> new_grid(new_height, new_width, value);

	for (size_t i = 0; i < m_height; i++) {
		std::copy_n(
			m_data.data() + i * m_width,
			m_width,
			new_grid.raw() + (i + v_pad) * new_width + h_pad
		);
	}

	return new_grid;
}

template<typename U>
Grid<T> convolve(Grid<U> kernel) const {
	return this->convolve_no_transpose(kernel.transpose());
}

template <typename U>
Grid<T> convolve_no_transpose(Grid<U> const &kernel) const {
	size_t const kh = kernel.height();
	size_t const kw = kernel.width();

	if (kh % 2 == 0 || kw % 2 == 0) {
		throw std::out_of_range("Grid to convolve must have odd size.");
	}
	
	Grid<T> const padded = this->pad_vh(kh / 2, kw / 2);
	Grid<T> out(m_height, m_width);


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

template<typename U>
Grid<T> convolve(Grid<U> kernel, Grid<float> const &mask) const {
	return this->convolve_no_transpose(kernel.transpose(), mask);
}

template <typename U>
Grid<T> convolve_no_transpose(Grid<U> const &kernel, Grid<float> const &mask) const {
	size_t const kh = kernel.height();
	size_t const kw = kernel.width();

	if (kh % 2 == 0 || kw % 2 == 0) {
		throw std::out_of_range("Grid to convolve must have odd size.");
	} else if (m_height != mask.height() || m_width != mask.width()) {
		throw std::out_of_range("Maks must be the same size as the grid to convolve.");
	}
	
	Grid<T> const padded = this->pad_vh(kh / 2, kw / 2);
	Grid<T> out(m_height, m_width);

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
Grid<T> convolve(std::vector<U> const &kernel) const {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	}

	using C = std::common_type_t<T, U>;

	Grid<T> const padded = this->pad(pad);
	Grid<C> temp(m_height + 2 * pad, m_width + 2 * pad, U{});
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.data();
	const T* __restrict pd = padded.raw();
	C* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();
	size_t const tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t const start_y = y + pad;

		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad;
			C sum = 0;
					
			for (size_t i = 0; i < ks; i++) {
				sum += pd[start_y * pw + x + i] * kd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad;
			C sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += td[(y + i) * tw + start_x] * kd[i];
			}

			od[y * m_width + x] = sum;
		}
	}

	return out;
}

template <typename U>
Grid<T> convolve(std::vector<U> const &kernel, Grid<float> const &mask) const {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	} else if (m_height != mask.height() || m_width != mask.width()) {
		throw std::out_of_range("Maks must be the same size as the grid to convolve.");
	}

	using C = std::common_type_t<T, U, float>;

	Grid<T> const padded = this->pad(pad);
	Grid<C> temp(m_height + 2 * pad, m_width + 2 * pad, U{});
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.data();
	const T* __restrict pd = padded.raw();
	const T* __restrict id = m_data.data();
	const float* __restrict md = mask.raw();
	C* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();
	size_t const tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t const start_y = y + pad;

		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad;
			C sum = 0;
					
			for (size_t i = 0; i < ks; i++) {
				sum += pd[start_y * pw + x + i] * kd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad;
			C sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += td[(y + i) * tw + start_x] * kd[i];
			}

			od[y * m_width + x] = sum * md[y * m_width + x] + id[y * m_width + x] * (1 - md[y * m_width + x]);
		}
	}

	return out;
}

template <typename U>
Grid<T> convolve(std::vector<U> const &kernel_v, std::vector<U> const &kernel_h) const {
	size_t const kvs = kernel_v.size();
	size_t const pad_v = kvs / 2;
	size_t const khs = kernel_h.size();
	size_t const pad_h = khs / 2;

	if (kvs % 2 == 0 || khs % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	}

	using C = std::common_type_t<T, U>;

	Grid<T> const padded = this->pad_vh(pad_v, pad_h);
	Grid<C> temp(m_height + 2 * pad_v, m_width + 2 * pad_h, U{});
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kvd = kernel_v.data();
	const U* __restrict khd = kernel_h.data();
	const T* __restrict pd = padded.raw();
	C* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();
	size_t const tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t const start_y = y + pad_v;

		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad_h;
			C sum = 0;
					
			for (size_t i = 0; i < khs; i++) {
				sum += pd[start_y * pw + x + i] * khd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad_h;
			C sum = 0;

			for (size_t i = 0; i < kvs; i++) {
				sum += td[(y + i) * tw + start_x] * kvd[i];
			}

			od[y * m_width + x] = sum;
		}
	}

	return out;
}

template <typename U>
Grid<T> convolve(std::vector<U> const &kernel_v, std::vector<U> const &kernel_h, Grid<float> const &mask) const {
	size_t const kvs = kernel_v.size();
	size_t const pad_v = kvs / 2;
	size_t const khs = kernel_h.size();
	size_t const pad_h = khs / 2;

	if (kvs % 2 == 0 || khs % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	} else if (m_height != mask.height() || m_width != mask.width()) {
		throw std::out_of_range("Maks must be the same size as the grid to convolve.");
	}

	using C = std::common_type_t<T, U>;

	Grid<T> const padded = this->pad_vh(pad_v, pad_h);
	Grid<C> temp(m_height + 2 * pad_v, m_width + 2 * pad_h, U{});
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kvd = kernel_v.data();
	const U* __restrict khd = kernel_h.data();
	const T* __restrict pd = padded.raw();
	const T* __restrict id = m_data.data();
	const float* __restrict md = mask.raw();
	C* __restrict td = temp.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();
	size_t const tw = temp.width();

	//horizontal pass
	for (size_t y = 0; y < m_height; y++) {
		size_t const start_y = y + pad_v;

		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad_h;
			C sum = 0;
					
			for (size_t i = 0; i < khs; i++) {
				sum += pd[start_y * pw + x + i] * khd[i];
			}

			td[start_y * pw + start_x] = sum;
		}	
	}

	//vertical pass
	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			size_t const start_x = x + pad_h;
			C sum = 0;

			for (size_t i = 0; i < kvs; i++) {
				sum += td[(y + i) * tw + start_x] * kvd[i];
			}

			od[y * m_width + x] = sum * md[y * m_width + x] + id[y * m_width + x] * (1 - md[y * m_width + x]);
		}
	}

	return out;
}

template <typename U>
Grid<T> convolve_horizontal(std::vector<U> const &kernel) {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	}

	Grid<T> const padded = this->pad_vh(0, pad);
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.data();
	const T* __restrict pd = padded.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();

	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			std::common_type_t<T, U> sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += pd[y * pw + x + i] * kd[i];
			}

			out[y * pw + x] = sum;
		}
	}

	return out;
}

template <typename U>
Grid<T> convolve_horizontal(std::vector<U> const &kernel, Grid<float> const &mask) {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	} else if (m_height != mask.height() || m_width != mask.width()) {
		throw std::out_of_range("Maks must be the same size as the grid to convolve.");
	}

	Grid<T> const padded = this->pad_vh(0, pad);
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.data();
	const T* __restrict pd = padded.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();

	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			std::common_type_t<T, U> sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += pd[y * pw + x + i] * kd[i];
			}

			out[y * pw + x] = sum * mask[y * m_width + x] + (*this)[y][x] * (1 - mask[y][x]); ;
		}
	}

	return out;
}

template <typename U>
Grid<T> convolve_vertical(std::vector<U> const &kernel) {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	}

	Grid<T> const padded = this->pad_vh(pad, 0);
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.data();
	const T* __restrict pd = padded.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();

	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			std::common_type_t<T, U> sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += pd[(y + i) * pw + x] * kd[i];
			}

			out[y * pw + x] = sum;
		}
	}

	return out;
}

template <typename U>
Grid<T> convolve_vertical(std::vector<U> const &kernel, Grid<float> const &mask) {
	size_t const ks = kernel.size();
	size_t const pad = ks / 2;

	if (ks % 2 == 0) {
		throw std::out_of_range("Vector to convolve must have odd size.");
	}

	Grid<T> const padded = this->pad_vh(pad, 0);
	Grid<T> out(m_height, m_width, T{});

	const U* __restrict kd = kernel.data();
	const T* __restrict pd = padded.raw();
	T* __restrict od = out.raw();

	size_t const pw = padded.width();

	for (size_t y = 0; y < m_height; y++) {
		for (size_t x = 0; x < m_width; x++) {
			std::common_type_t<T, U> sum = 0;

			for (size_t i = 0; i < ks; i++) {
				sum += pd[(y + i) * pw + x] * kd[i];
			}

			out[y * pw + x] = sum * mask[y * m_width + x] + (*this)[y][x] * (1 - mask[y][x]); ;;
		}
	}

	return out;
}


// SHAPING
Grid<T>& resize (size_t const height, size_t const width, T const &value = T{}) {
	std::vector<T> new_data(height * width, value);
	// invariants
	size_t const min_height = std::min(height, m_height);
	size_t const min_width = std::min(width, m_width);
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

	return *this;
}

Grid<T>& transpose() {
	std::vector<T> new_data(m_height * m_width);

	T* __restrict raw = m_data.data();
	T* __restrict new_raw = new_data.data();

	for (size_t i = 0; i < m_height; i++) {
		size_t const start_i = i * m_width;
		for (size_t j = 0; j < m_width; j++) {
			new_raw[j * m_height + i] = raw[start_i + j];
		}
	}

	m_data.swap(new_data);
	std::swap(m_height, m_width);

	return *this;
}

inline Grid<T>& append_rows (size_t const amount, T const &value = T{}) {
	return resize(m_height + amount, m_width, value);
}

inline Grid<T>& append_columns (size_t const amount, T const &value = T{}) {
	return resize(m_height, m_width + amount, value);
}

Grid<T>& insert_rows (size_t position, size_t const amount, T const &value = T{}) {
	position = std::min(position, m_width);

	m_data.insert(
		m_data.begin() + position * m_width,
		amount * m_width,
		value
	);

	m_height += amount;
	return *this;
}

Grid<T>& insert_columns (size_t position, size_t amount, T const &value = T{}) {
	position = std::min(position, m_width);
	size_t const new_width = m_width + amount;

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

	return *this;
}


Grid<T> slice (
	size_t const start_y,
	size_t const start_x,
	size_t const size_y,
	size_t const size_x
) const {
	if (start_y + size_y > m_height || start_x + size_x > m_width)
		throw std::out_of_range("Slice exceeds grid bounds.");
		
	Grid<T> out(size_y, size_x);

	for (size_t i = 0; i < size_y; i++){
		std::copy_n(
			m_data.data() + (start_y + i) * m_width + start_x,
			size_x,
			out.m_data.data() + i * out.m_width
		);
	}

	return out;
}

// Only defined to be able to get slices without creating new grids
class View {
	size_t m_y, m_x, m_height, m_width, m_big_width;
	T* m_data;
public:
	View(size_t const y, size_t const x, size_t const height, size_t const width, Grid<T> const &data) {
		assert(y + height < data.height() && x + width < data.width());
		m_y = y;
		m_x = x;
		m_width = width;
		m_height = height;
		m_big_width = data.width();
		m_data = data.raw();
	}

	// Fast access with no checks
	inline T* operator[] (size_t const row_index) {
		return m_data + row_index * m_big_width;
	}

	inline const T* operator[] (size_t const row_index) const {
		return m_data + row_index * m_big_width;
	}

	inline size_t width() const { return m_width; }
	inline size_t height() const { return m_height; }
	inline size_t y() const { return m_y; }
	inline size_t x() const { return m_x; }
};

View view(size_t const y, size_t const x, size_t const height, size_t const width) {
	return View(y, x, height, width, *this);
}

void print() {
	std::cout << "[";
	for (size_t i = 0; i < m_height; i++){
		std::cout << "[";
		for (size_t j = 0; j < m_width; j++){
			std::cout << (*this)[i][j] << " ";
		}
		std::cout << "]\n";
	}
	std::cout << "]" << std::endl;
}

// UNARY OPERATORS
Grid<T>& operator++ () {
	this->mutate([](T &x){
		return x++;
	});
	return *this;
}

Grid<T>& operator-- () {
	this->mutate([](T &x){
		return x--;
	});
	return *this;
}

// SCALAR OPERATORS
template <typename U>
Grid<T> operator+ (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x + s;
	}, scalar);
}


template <typename U>
Grid<T>& operator+= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		x += s;
	}, scalar);
	return *this;
}

template <typename U>
Grid<T> operator- (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x - s;
	}, scalar);
}


template <typename U>
Grid<T>& operator-= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		x -= s;
	}, scalar);
	return *this;
}

template <typename U>
Grid<T> operator* (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x * s;
	}, scalar);
}

template <typename U>
Grid<T>& operator*= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		x *= s;
	}, scalar);
	return *this;
}

template <typename U>
Grid<T> operator/ (U const &scalar) const {
	return this->transformed([](T const &x, U const &s){
		return x / s;
	}, scalar);
}

template <typename U>
Grid<T>& operator/= (U const &scalar) {
	this->mutate([](T &x, U const &s){
		x /= s;
	}, scalar);
	return *this;
}

// GRID OPERATORS
template <typename U>
Grid<T> operator+ (Grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to sum have different dimensions.");
	}

	Grid<T> out(this->height(), this->width());
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
Grid<T>& operator+= (Grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t const size = this->size();
	T* __restrict data = this->raw();
	U* __restrict input_data = input.raw();
	
	for (size_t i = 0; i < size; i++){
		data[i] += input_data[i];
	}
	return *this;
}

template <typename U>
Grid<T> operator- (Grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to sum have different dimensions.");
	}

	Grid<T> out(this->height(), this->width());
	// invariants
	size_t const size = this->size();
	T* __restrict out_data = out.raw();
	const T* __restrict this_data = this->raw();
	const U* __restrict input_data = input.raw();

	for (size_t i = 0; i < size; i++){
		out_data[i] = this_data[i] - input_data[i];
	}

	return out;
}

template <typename U>
Grid<T>& operator-= (Grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t const size = this->size();
	T* __restrict data = this->raw();
	U* __restrict input_data = input.raw();
	
	for (size_t i = 0; i < size; i++){
		data[i] -= input_data[i];
	}
	return *this;
}

template <typename U>
Grid<T> operator* (Grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}

	Grid<T> out(this->height(), this->width());
	// invariants
	size_t const size = this->size();
	T* __restrict out_data = out.raw();
	const T* __restrict this_data = this->raw();
	const U* __restrict input_data = input.raw();

	for (size_t i = 0; i < size; i++){
		out_data[i] = this_data[i] * input_data[i];
	}

	return out;
}

template <typename U>
Grid<T>& operator*= (Grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t const size = this->size();
	T* __restrict data = this->raw();
	U* __restrict input_data = input.raw();
	
	for (size_t i = 0; i < size; i++){
		data[i] *= input_data[i];
	}
	return *this;
}

template <typename U>
Grid<T> operator/ (Grid<U> const &input) const {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}

	Grid<T> out(this->height(), this->width());
	// invariants
	size_t const size = this->size();
	T* __restrict out_data = out.raw();
	const T* __restrict this_data = this->raw();
	const U* __restrict input_data = input.raw();

	for (size_t i = 0; i < size; i++){
		out_data[i] = this_data[i] / input_data[i];
	}

	return out;
}

template <typename U>
Grid<T>& operator/= (Grid<U> const &input) {
	if (this->height() != input.height() || this->width() != input.width()){
		throw std::out_of_range("Grids to multiply have different dimensions.");
	}
	
	size_t const size = this->size();
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
Grid<T> operator- (U const &scalar, Grid<T> const &mat) {
	return mat.transformed([](T const &x, U const &s){
		return static_cast<T>(s - x);
	}, scalar);
}

template <typename T, typename U>
Grid<T> operator/ (U const &scalar, Grid<T> const &mat) {
	return mat.transformed([](T const &x, U const &s){
		return static_cast<T>(s / x);
	}, scalar);
}
