#pragma once

#include "util/common.hpp"
#include "util/string_util.hpp"
#include "util/log.hpp"

template <typename T>
struct DArray {
private:
	T* m_data = NULL;
	int m_size = 0;
	int m_cap = 0;

public:
	const T * data() const { return m_data; }
	int size() const { return m_size; }

	DArray() {}
	DArray(int cap) {
		m_data = new T[cap];
		m_cap = cap;
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

	int add(T& elem)	{
		int ret_index = m_size;
		if (m_size + 1 > m_cap)
		{
			grow();
		}

		m_data[m_size] = std::move(elem);
		m_size += 1;
		return ret_index;
	}

	int add(T&& elem)	{
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

// compare is true if a is greater
template<typename T>
void sort_array(DArray<T>& array, bool (*CompareFunc)(T& a, T& b))
{
	for (int i = 0; i < array.size(); i++)
	{
		int minIndex = i;

		for (int j = i + 1; j < array.size(); j++)
		{
			if (CompareFunc(array.get_ref(minIndex), array.get_ref(j)))
			{
				minIndex = j;
			}
		}

		array.switch_items(i, minIndex);
	}
}

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

template<typename T>
struct BucketList {
	using FlagType = u32;

	static const int BUCKET_SIZE = 32;
	static const u32 BUCKET_FLAGS_FULL = 0xFFFFFFFF;  // match the bucket size in the number of bits
	struct Bucket {
		T elements[BUCKET_SIZE];
		FlagType occupied_flags = 0;  // at least as many bits as bucket size
	};

	// due to the fact that the dynamic array can relocate, the pointers are not stable but slot ids are
	DArray<Bucket> buckets;

	int count() const
	{
		int total = 0;
		for (const auto& bucket : buckets)
		{
			total += pop_count(bucket.occupied_flags);
		}

		return total;
	}

	int add(const T& elem)
	{
		int bucket_index = 0;
		for (auto& bucket : buckets)
		{
			auto flags = bucket.occupied_flags;
			if (flags == BUCKET_FLAGS_FULL)
			{
				bucket_index += 1;
				continue;
			}

			FlagType inverse_flags = ~flags;
			int index = lsb_index(inverse_flags);
			bucket.elements[index] = std::move(elem);
			bucket.occupied_flags |= BIT(index);

			return bucket_index * BUCKET_SIZE + index;
		}

		int new_bucket_index = buckets.add(Bucket());
		buckets.get_ref(new_bucket_index).elements[0] = std::move(elem);
		buckets.get_ref(new_bucket_index).occupied_flags |= BIT(0);
		return new_bucket_index * BUCKET_SIZE + 0;
	}

	int add(T&& elem)
	{
		int bucket_index = 0;
		for (auto& bucket : buckets)
		{
			auto flags = bucket.occupied_flags;
			if (flags == BUCKET_FLAGS_FULL)
			{
				bucket_index += 1;
				continue;
			}

			FlagType inverse_flags = ~flags;
			int index = lsb_index(inverse_flags);
			bucket.elements[index] = std::move(elem);
			bucket.occupied_flags |= BIT(index);

			return bucket_index * BUCKET_SIZE + index;
		}

		int new_bucket_index = buckets.add(Bucket());
		buckets.get_ref(new_bucket_index).elements[0] = std::move(elem);
		buckets.get_ref(new_bucket_index).occupied_flags |= BIT(0);
		return new_bucket_index * BUCKET_SIZE + 0;
	}

	void remove(int elem_index)
	{
		int bucket_index = elem_index / BUCKET_SIZE;
		int index = elem_index % BUCKET_SIZE;

		if (!(buckets[bucket_index].occupied_flags & BIT(index)))
		{
			panic("Trying to remove an empty slot in bucket list");
		}
		buckets.get_ref(bucket_index).occupied_flags &= ~BIT(index);
	}

	bool in_bounds(int elem_index) const {
		int bucket_index = elem_index / BUCKET_SIZE;
		int index = elem_index % BUCKET_SIZE;

		return buckets.in_bounds(bucket_index) && (index >= 0 && index < BUCKET_SIZE);
	}

	T& get(int elem_index) const
	{
		int bucket_index = elem_index / BUCKET_SIZE;
		int index = elem_index % BUCKET_SIZE;

		if (!in_bounds(elem_index)) panic("Out of bounds array access");
		if (!(buckets[bucket_index].occupied_flags & BIT(index)))
		{
			panic("Accessing an empty slot in bucket list");
		}
		return buckets[bucket_index].elements[index];
	}

	T& operator[](int elem_index) const
	{
		int bucket_index = elem_index / BUCKET_SIZE;
		int index = elem_index % BUCKET_SIZE;

		return buckets[bucket_index].elements[index];
	}

	T get_or_default(int elem_index) const {
		int bucket_index = elem_index / BUCKET_SIZE;
		int index = elem_index % BUCKET_SIZE;

		if (!in_bounds(elem_index)) return T();
		return buckets.get_ref(bucket_index).elements[index];
	}

	T* get_ptr(int elem_index) const {
		int bucket_index = elem_index / BUCKET_SIZE;
		int index = elem_index % BUCKET_SIZE;

		if (!in_bounds(elem_index)) panic("Out of bounds array access");
		return &buckets[bucket_index].elements[index];
	}

	bool get_next(int* bucket_index, int* slot_index) const
	{
		for (int i = *bucket_index; i < buckets.size(); i++)
		{
			Bucket& bucket = buckets.get_ref(i);
			auto flags = bucket.occupied_flags;
			flags &= BUCKET_FLAGS_FULL << *slot_index;  // ignore flags before the point we are looking
			if (flags != 0)
			{
				int index = lsb_index(flags);

				*bucket_index = i;
				*slot_index = index;
				return true;
			}
			else
			{
				*slot_index = 0;
			}
		}

		*bucket_index = buckets.size();
		*slot_index = 0;
		return false;
	}

	void reset()
	{
		buckets.reset();
	}

	struct Iterator {
		BucketList<T>* list = {};
		int bucket_index = 0;
		int slot_index = 0;

		int index() const
		{
			return bucket_index * BUCKET_SIZE + slot_index;
		}

		void next()
		{
			list->get_next(&bucket_index, &slot_index);
		}

		Iterator& operator++() {
			slot_index += 1;
			if (slot_index == BUCKET_SIZE)
			{
				bucket_index += 1;
				slot_index = 0;
			}
			next();
			return *this;
		}

		T& operator*() {
			return list->buckets.get_ref(bucket_index).elements[slot_index];
		}

		bool operator!=(const Iterator& other) const {
			return bucket_index != other.bucket_index || slot_index != other.slot_index;
		}
	};

	Iterator begin()
	{
		Iterator it = { this, 0, 0 };
		it.next();
		return it;
	}

	Iterator end()
	{
		return { this, buckets.size(), 0 };
	}

	struct ConstIterator {
		const BucketList<T>* list = {};
		int bucket_index = 0;
		int slot_index = 0;

		int index() const
		{
			return bucket_index * BUCKET_SIZE + slot_index;
		}

		void next()
		{
			list->get_next(&bucket_index, &slot_index);
		}

		ConstIterator& operator++() {
			slot_index += 1;
			if (slot_index == BUCKET_SIZE)
			{
				bucket_index += 1;
				slot_index = 0;
			}
			next();
			return *this;
		}

		const T& operator*() const {
			return list->buckets.get_ref(bucket_index).elements[slot_index];
		}

		bool operator!=(const ConstIterator& other) const {
			return bucket_index != other.bucket_index || slot_index != other.slot_index;
		}
	};

	ConstIterator begin() const
	{
		ConstIterator it = { this, 0, 0 };
		it.next();
		return it;
	}

	ConstIterator end() const
	{
		return { this, buckets.size(), 0 };
	}
};

template<typename T>
struct List
{
	struct Entry {
		T element;
		Entry* previous = nullptr;
		Entry* next = nullptr;

		Entry(const T& elem) : element(elem) {}
		Entry(const T& elem, Entry* prev) : element(elem), previous(prev) {}
	};

	Entry* head = nullptr;
	Entry* last = nullptr;

	T* get_start() const
	{
		return head ? &head->element : nullptr;
	}

	T* get_last() const
	{
		return last ? &last->element : nullptr;
	}

	T* get_element(int index) const
	{
		if (index < 0) panic("Negative index trying to access list");

		int i = 0;
		Entry * iter = head;
		while (iter)
		{
			if (i == index)
			{
				return &iter->element;
			}

			iter = iter->next;
			i += 1;
		}

		panic("Out of bounds list access");
	}

	List() {}
	~List() { reset(); }

	List(const List& other) = delete;
	void operator=(const List& other) = delete;

	List(List&& other)
		:
		head(other.head), last(other.last)
	{
		other.head = nullptr;
		other.last = nullptr;
	}

	void operator=(List&& other)
	{
		if (head == other.head)
		{
			return;
		}

		reset();

		head = other.head;
		last = other.last;

		other.head = nullptr;
		other.last = nullptr;
	}

	void reset()
	{
		while (head)
		{
			remove_start();
		}
	}

	bool empty() const { return head ? false : true; }

	void add(const T& elem)
	{
		if (!head)
		{
			head = new Entry(elem);
			last = head;
			return;
		}

		last->next = new Entry(elem, last);
		last = last->next;
	}

	void remove_start()
	{
		if (!head)
		{
			return;
		}

		bool is_single = (head == last);

		Entry* next = head->next;
		delete head;
		if (is_single) last = nullptr;
		head = next;
		if (head)
		{
			head->previous = nullptr;
		}
	}

	T remove_and_get_start()
	{
		if (!head)
		{
			return T();
		}

		T elem = std::move(head->element);
		remove_start();

		return elem;
	}

	void remove_last()
	{
		if (!head)
		{
			return;
		}

		bool is_single = (last == head);

		Entry* prev = last->previous;
		delete last;
		if (is_single) head = nullptr;
		last = prev;
		if (last)
		{
			last->next = nullptr;
		}
	}

	T remove_and_get_last()
	{
		if (!head)
		{
			return T();
		}

		T elem = std::move(last->element);
		remove_last();

		return elem;
	}
};
