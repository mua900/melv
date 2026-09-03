#ifndef ARRAY_HPP
#define ARRAY_HPP

#include "util/common.hpp"
#include "util/log.hpp"

template<typename T>
struct Array {
	T* data = nullptr;
	size_t count = 0;

	bool in_bounds(int index) {
		return index >= 0 && index < count;
	}

	T& get(int index) {
		if (!in_bounds(index))
		{
			panic("Out of bounds array access");
		}
		return data[index];
	}
	T& operator[](int index) { return data[index]; }

	Array() {}
	Array(T* d, size_t c) : data(d), count(c) {}

	T* begin()
	{
		return data;
	}

	T* end()
	{
		return data + count;
	}

	const T* begin() const
	{
		return data;
	}

	const T* end() const
	{
		return data + count;
	}
};

template <typename T>
struct DArray {
private:
	T* m_data = NULL;
	int m_size = 0;
	int m_cap = 0;

public:
	const T * data() const { return m_data; }
	int size() const { return m_size; }

	Array<T> to_array()
	{
		return Array<T>(m_data, m_size);
	}

	DArray() {}
	DArray(int cap) {
		m_data = new T[cap];
		m_cap = cap;
	}

	DArray(int cap, bool mark)
	{
		m_data = new T[cap];
		m_cap = cap;
		if (mark)
		{
			m_size = cap;
		}
	}

	// for special use case when you are using this more as an allocator
	void mark_full() {
		m_size = m_cap;
	}

	void discard_data() {
		m_size = 0;
	}

	void reset() {
		if (m_data)
		{
			delete[](m_data);
			m_data = nullptr;

			m_size = 0;
			m_cap = 0;
		}
	}

	bool in_bounds(int index) const {
		return index < m_size && index >= 0;
	}

	T& operator[](int index) const {
		return m_data[index];
	}

	T get(int index) const {
		if (index >= m_size) panic("Out of bounds array access");
		return m_data[index];
	}

	T get_or_default(int index) const {
		if (index >= m_size) return T();
		return m_data[index];
	}

	T& get_ref(int index) const {
		if (index >= m_size) panic("Out of bounds array access");
		return m_data[index];
	}

	T* get_ptr(int index) const {
		if (index >= m_size) panic("Out of bounds array access");
		return &m_data[index];
	}

	T* get_last() const
	{
		if (m_size < 1)
		{
			return nullptr;
		}

		return &m_data[m_size - 1];
	}

	int add(const T& elem) {
		int ret_index = m_size;
		if (m_size + 1 > m_cap)
		{
			grow();
		}

		m_data[m_size] = std::move(elem);
		m_size += 1;
		return ret_index;
	}

	int add(T&& elem) {
		int ret_index = m_size;
		if (m_size + 1 > m_cap)
		{
			grow();
		}

		m_data[m_size] = std::move(elem);
		m_size += 1;
		return ret_index;
	}

	int add_array(T* elems, int count)
	{
		ensure_size(m_size + count);

		for (int i = 0; i < count; i++)
		{
			m_data[m_size + i] = elems[i];
		}

		int start = m_size;
		m_size += count;

		return start;
	}

	void remove_shift(int index) {
		if (!in_bounds(index)) {
			panic("Out of bounds array access");
		}

		for (int i = index; i < m_size-1; i++) {
			m_data[i] = m_data[i+1];
		}

		m_size -= 1;
	}

	void remove(int index) {
		if (!in_bounds(index)) {
			panic("Out of bounds array access");
		}

		m_size -= 1;

		m_data[index] = m_data[m_size-1];
	}

	void replace(T elem, int index)
	{
		if (!in_bounds(index)) {
			panic("Out of bounds array access");
		}

		m_data[index] = elem;
	}

	void insert(T elem, int index)
	{
		if (m_size + 1 >= m_cap)
		{
			grow();
		}

		for (int i = m_size; i > index; i -= 1)
		{
			m_data[i] = m_data[i - 1];
		}

		m_data[index] = elem;

		m_size += 1;
	}

	int add_unique(T elem) {
		Find_Result find_result = find_in_array(*this, elem);
		if (find_result.found)
		{
			return find_result.index;
		}

		return add(elem);
	}

	bool is_empty()	const {
		return m_size == 0;
	}

	void switch_items(int a, int b)
	{
		if (!(in_bounds(a) && in_bounds(b))) panic("Out of bounds array access trying to switch items");
		if (a == b) return;
		T tmp = std::move(m_data[a]);
		m_data[a] = std::move(m_data[b]);
		m_data[b] = std::move(tmp);
	}

	T pop()	{
		if (is_empty())
		{
			panic("Trying to pop from empty array");
		}

		m_size -= 1;
		return m_data[m_size];
	}

	void free()	{
		reset();
	}

	void ensure_size(int capacity) {
		if (m_cap < capacity) {
			resize(capacity);
		}
	}

	void resize(int size) {
		T* new_buffer = new T[size];

		int new_size = (size < m_size) ? size : m_size;

		for (int i = 0; i < new_size; i++) {
			new_buffer[i] = m_data[i];
		}

		if (m_data) {
			delete[] m_data;
		}

		m_data = new_buffer;
		m_cap = size;
		m_size = new_size;
	}

	T* begin() {
		return m_data;
	}

	T* end() {
		return m_data + m_size;
	}

	const T* begin() const {
		return m_data;
	}

	const T* end() const {
		return m_data + m_size;
	}

private:
	void grow() {
		int ncap = m_cap ? (m_cap * 2) : 8;
		T* ndata = new T[ncap];
		if (m_size > m_cap) panic("Invalid dynamic array");
		for (int i = 0; i < m_size; i++)
		{
			ndata[i] = std::move(m_data[i]);
		}
		delete[](m_data);
		m_data = ndata;
		m_cap = ncap;
	}
};

template<typename T>
Find_Result find_in_array(DArray<T>& array, T& elem) {
	for (int i = 0; i < array.size(); i++)
	{
		if (array[i] == elem)
		{
			return Find_Result{ i, true };
		}
	}

	return Find_Result{ 0, false };
}

// simple
// compare is true if a is greater
template<typename T>
void sort_array(DArray<T>& arr, bool (*CompareFunc)(T& a, T& b))
{
	for (int i = 0; i < arr.size(); i++)
	{
		int minIndex = i;

		for (int j = i + 1; j < arr.size(); j++)
		{
			if (CompareFunc(arr.get_ref(minIndex), arr.get_ref(j)))
			{
				minIndex = j;
			}
		}

		arr.switch_items(i, minIndex);
	}
}

#endif // ARRAY_HPP