#pragma once

#include "array.hpp"

template<typename T>
struct LinearBuffer
{
	enum LinearBufferFlags : u32
	{
		BufferFlagsNone = 0,
		BufferIsEmpty = BIT(0),
	};

private:
	T* m_data = nullptr;
	u32 m_capacity = 0;
public:
	u32 flags = 0;

	const T * data() const { return m_data; }
	int size() const { return m_capacity; }

	Array<T> to_array()
	{
		return Array<T>(m_data, m_capacity);
	}

	void mark_full()
	{
		flags |= BufferIsEmpty;
	}

	void mark_empty()
	{
		flags &= ~ BufferIsEmpty;
	}

	void reset()
	{
		if (m_data)
		{
			delete[] m_data;
			m_data = nullptr;
			m_capacity = 0;
		}
	}

	void ensure_size(int capacity) {
		if (m_capacity < capacity) {
			resize(capacity);
		}
	}

	void resize(int size) {
		T* new_buffer = new T[size];

		int new_size = (size < m_capacity) ? size : m_capacity;

		for (int i = 0; i < new_size; i++) {
			new_buffer[i] = m_data[i];
		}

		if (m_data) {
			delete[] m_data;
		}

		m_data = new_buffer;
		m_capacity = size;
	}

	~LinearBuffer()
	{
		if (m_data)
		{
			delete[] m_data;
		}
	}

	bool in_bounds(int index) const {
		return index < m_capacity && index >= 0;
	}

	T& get(int index)
	{
		if (!in_bounds(index))
		{
			panic("Out of bounds access to linear buffer");
		}

		return m_data[index];
	}

	T& operator[](int index) const {
		return m_data[index];
	}

	T* begin() {
		return m_data;
	}

	T* end() {
		if (flags & BufferIsEmpty)
		{
			return begin();
		}

		return m_data + m_size;
	}

	const T* begin() const {
		return m_data;
	}

	const T* end() const {
		if (flags & BufferIsEmpty)
		{
			return begin();
		}

		return m_data + m_size;
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
