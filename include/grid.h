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
	: m_height (0), m_width (0), m_data() {}

grid (size_t height, size_t width)
	: m_height (height), m_width (width), m_data (m_height * m_width) {}

grid (size_t height, size_t width, const T& value)
	: m_height (height), m_width (width), m_data (m_height * m_width, value) {}

grid (size_t height, size_t width, std::vector<T> data)
	: m_height (height), m_width (width), m_data (data) {}

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

size_t size() const { return m_data.size(); }
size_t width() const { return m_width; }
size_t height() const { return m_height; }
std::vector<T> data() const { return m_data; }

// FAST ACCESS WITH NO CHECKS
T* operator[] (size_t row_index) {
	return m_data.data() + row_index * m_width;
}

const T* operator[] (size_t row_index) const {
	return m_data.data() + row_index * m_width;
}

// SLOW ACCESS WITH CHECKS
T& operator() (size_t row_index, size_t column_index) {
	return m_data.at(row_index * m_width + column_index);
}

const T& operator() (int row_index, int column_index) const {
	return m_data.at(row_index * m_width + column_index);
}

void resize (size_t height, size_t width, const T& value = T{}) {
	std::vector<T> new_data(height * width, value);

	size_t min_height = std::min(height, m_height);
	size_t min_width = std::min(width, m_width);
	
	for (size_t i = 0; i < min_height; i++){
		std::copy_n(
			m_data.data() + i * m_width,
			min_width,
			new_data.data() + i * width
		);
	}

	m_data.swap(new_data);
	m_height = height;
	m_width = width;
}

void pad(size_t padding, const T& value = T{}) {
	if (padding == 0) return;
	
	size_t new_height = m_height + (padding * 2);
	size_t new_width = m_width + (padding * 2);
	
	std::vector<T> new_data(new_height * new_width);

	std::fill_n(
		new_data.data(),
		new_width * padding,
		value
	);

	for (size_t i = 0; i < m_height; i++){
		T* row_start = new_data.data() + (i + padding) * new_width;
		
		std::fill_n(
			row_start,
			padding,
			value
		);
		
		std::copy_n(
			m_data.data() + i * m_width,
			m_width,
			row_start + padding
		);

		std::fill_n(
			row_start + padding + m_width,
			padding,
			value
		);
	}

	std::fill_n(
		new_data.data() + (padding + m_height) * new_width,
		new_width * padding,
		value
	);

	m_data.swap(new_data);
	m_height = new_height;
	m_width = new_width;
}

inline void append_rows (size_t amount, const T& value = T{}) {
	resize(m_height + amount, m_width, value);
}

inline void append_columns (size_t amount, const T& value = T{}) {
	resize(m_height, m_width + amount, value);
}

void insert_rows (size_t position, size_t amount, const T& value = T{}) {
	position = std::min(position, m_width);

	m_data.insert(
		m_data.begin() + position * m_width,
		amount * m_width,
		value
	);

	m_height += amount;
}

void insert_columns (size_t position, size_t amount, const T& value = T{}) {
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

};

