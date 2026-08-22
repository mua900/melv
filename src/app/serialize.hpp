#ifndef SERIALIZE_HPP
#define SERIALIZE_HPP

#include "util/hash_table.hpp"
#include "util/value.hpp"

namespace melv
{

// generic serialization for relatively simple state
// when this is enough instead of using a custom format for the task

// and to provide something to start copy pasting from when you want a custom format

// @todo
struct SerializeBlock
{
    DArray<ValueType> types = {};
    HashTable<Value> values = {};
};

struct SerializeState
{
    HashTable<Value> values;

    SerializeState() {}
    SerializeState(int size)
        :
        values(size)
    {}
};

bool serialize_text(const char* outputName, SerializeState& state);
bool serialize_binary(const char* outputName, SerializeState& state);

bool readback_text(const char* fileName, SerializeState* state);
bool readback_binary(const char* fileName, SerializeState* state);

bool readback_text_mem(const char* data, size_t size, SerializeState* state);
bool readback_binary_mem(const u8* data, size_t size, SerializeState* state);

}

#endif // SERIALIZE_HPP
