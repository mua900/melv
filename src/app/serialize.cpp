#include "serialize.hpp"

#include "util/file_util.hpp"

namespace melv {

const int versionNumber = 1;

bool serialize_text(const char* outputName, SerializeState& state)
{
    String_Builder builder(state.values.count * 16);

    // header
    builder.append_char('[');
    builder.append_integer(versionNumber);
    builder.append_char(']');
    builder.append_char('\n');

    for (auto kv : state.values)
    {
        builder.append(kv.key);
        builder.append_char(' ');
        switch (kv.value->type)
        {
            case Value::STRING: {
                builder.append(kv.value->data.string);
                break;
            }
            case Value::INTEGER: {
                builder.append_integer(kv.value->data.integer);
                break;
            }
            case Value::REAL: {
                builder.append_float(kv.value->data.real);
                break;
            }
            default:
                break;
        }
        builder.append_char('\n');
    }

    return true;
}

bool serialize_binary(const char* outputName, SerializeState& state)
{
    panic("Not implemented");
    return false;
}

bool readback_text(const char* fileName, SerializeState* state)
{
    BinaryData data;
    if (!load_file(fileName, data))
    {
        return false;
    }

    return readback_text_mem(data.data, data.size, state);
}

bool readback_binary(const char* fileName, SerializeState* state)
{
    BinaryData data;
    if (!load_file(fileName, data))
    {
        return false;
    }

    return readback_binary_mem(data.data, data.size, state);
}

bool readback_text_mem(const u8* data, size_t size, SerializeState* state)
{
    

    return true;
}

bool readback_binary_mem(const u8* data, size_t size, SerializeState* state)
{
    panic("Not implemented");
    return false;
}

} // namespace