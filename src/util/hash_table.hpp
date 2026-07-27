#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#include "template.hpp"
#include "util/string_util.hpp"

template<typename T>
struct HashTable {
	enum TableEntryFlag
	{
		EMPTY = 0,
		OCCUPIED = 1,
		DELETED = 2,
	};

	static constexpr int step = 1;

	struct Entry {
		TableEntryFlag flag = TableEntryFlag::EMPTY;
		melv::StringReference key = {};
		T value;

		Entry() {}
		Entry(melv::StringReference k, const T& v) : key(k), value(v) {}
	};

	HashTable() { make(31); }
	HashTable(int size) { make(size); }

	void reset()
	{
		buffer.free_buffer();
		entries.reset();
	}

	void make(int size) {
		entries.resize(size);
		entries.mark_full();
	}

	bool add(melv::String key, const T& value)
	{
		// to detect and replace duplicates if possible
		Entry* first_deleted = nullptr;

		int num_slot = entries.size();
		u64 hash = melv::string_hash(key);
		int slot = hash % num_slot;
		Entry* entry = entries.get_ptr(slot);
		int total = 0;
		while (entry && total < num_slot)
		{
			if (!first_deleted && entry->flag == TableEntryFlag::DELETED)
			{
				first_deleted = entry;
			}
			else if (entry->flag == TableEntryFlag::EMPTY)
			{
				if (first_deleted)
				{
					entry = first_deleted;
				}

				entry->key = buffer.put_string_reference(key);
				entry->value = value;
				entry->flag = TableEntryFlag::OCCUPIED;

				count += 1;

				return true;
			}
			else if (entry->flag == TableEntryFlag::OCCUPIED &&
					melv::string_compare(buffer.get_string(entry->key), key))
			{
				log_error("Non unique insertions to hash table");
				entry->value = value;
				return true;
			}

			slot = (slot + step) % num_slot;
			total += step;
			entry = entries.get_ptr(slot);
		}

		if (first_deleted)
		{
			first_deleted->key = buffer.put_string_reference(key);
			first_deleted->value = value;
			first_deleted->flag = TableEntryFlag::OCCUPIED;

			count += 1;

			return true;
		}

		return false;
	}

	Entry* find_entry(melv::String key)
	{
		u64 hash = melv::string_hash(key);
		int num_slot = entries.size();
		int slot = hash % num_slot;
		Entry* entry = entries.get_ptr(slot);

		int total = 0;
		while (entry && total < num_slot)
		{
			if (entry->flag == TableEntryFlag::EMPTY)
			{
				return nullptr;
			}
			else if (entry->flag == TableEntryFlag::OCCUPIED &&
					melv::string_compare(buffer.get_string(entry->key), key))
			{
				return entry;
			}

			slot = (slot + step) % num_slot;
			total += step;
			entry = entries.get_ptr(slot);
		}

		return nullptr;
	}

	T* find(melv::String key)
	{
		Entry* entry = find_entry(key);
		if (entry)
		{
			return &entry->value;
		}
		else
		{
			return nullptr;
		}
	}

	int capacity() const
	{
		return entries.size();
	}

	bool remove(melv::String key)
	{
		Entry* entry = find_entry(key);
		if (entry)
		{
            ASSERT(count > 0);
            count -= 1;
			entry->flag = TableEntryFlag::DELETED;
		}

		return entry ? true : false;
	}

	void resize(int new_size)
	{
		ASSERT(new_size >= count);
		HashTable<T> newTable(new_size);

		for (auto& entry : entries)
		{
			if (entry.flag == TableEntryFlag::OCCUPIED)
			{
				newTable.add(buffer.get_string(entry.key), entry.value);
			}
		}

		reset();

		buffer = std::move(newTable.buffer);
		entries = newTable.entries;
	}

    struct KeyValue
    {
        melv::String key;
        T *value;

        KeyValue(melv::String k, T* v) : key(k), value(v) {}
    };

	struct Iterator {
		HashTable<T>* table = {};
		int index = 0;

		Iterator& operator++() {
            index += 1;

            while (index < table->entries.size() && table->entries[index].flag != TableEntryFlag::OCCUPIED)
            {
                index += 1;
            }

			return *this;
		}

		KeyValue operator*() {
            Entry *entry = table->entries.get_ptr(index);
			return KeyValue(table->buffer.get_string(entry->key), &entry->value);
		}

		bool operator!=(const Iterator& other) const {
			return table != other.table || index != other.index;
		}
	};

	Iterator begin()
	{
        int index = 0;
		while (index < entries.size() && entries[index].flag != TableEntryFlag::OCCUPIED)
		{
            index += 1;
		}

        return { this, index };
	}

	Iterator end()
	{
		return { this, entries.size() };
	}

	int count = 0;
	melv::String_Builder buffer = {};
	DArray<Entry> entries = {};
};

#endif // HASH_TABLE_HPP