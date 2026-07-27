#include "file_util.hpp"

namespace melv
{

long get_file_size(FILE* file) {
	long pos = std::ftell(file);
    std::fseek(file, 0, SEEK_END);
	long len = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
	return len;
}

bool load_file(const char* filepath, BinaryData& data) {
	FILE* handle = std::fopen(filepath, "rb");
    if (!handle)
    {
        return false;
    }

	auto filesize = get_file_size(handle);

	u8* mem = (u8*) std::malloc(filesize);
	if (!mem) {
		panic("malloc fail");
	}

	size_t written = std::fread(mem, sizeof(u8), filesize, handle);
	if (filesize != written) {
        std::free(mem);
		return false;
	}

    std::fclose(handle);

	data.data = mem;
	data.size = filesize;

	return true;
}

bool load_file_text(const char* filepath, String_Builder& builder)
{
	FILE* handle = fopen(filepath, "rb");
    if (!handle)
    {
        return false;
    }

	auto filesize = get_file_size(handle);

    builder.clear();
    builder.ensure_size(filesize);

	size_t written = fread(builder.buffer, sizeof(u8), filesize, handle);
    if (filesize != written) {
        return false;
	}

    builder.cursor = written;
    fclose(handle);

	return true;
}

size_t write_to_file(const char* filepath, String_Builder& builder, const char* access)
{
	FILE* handle = fopen(filepath, access);

	size_t written = fwrite(builder.buffer, 1, builder.cursor, handle);

	fclose(handle);

	return written;
}

void File::write_string(String s) {
	fwrite(&s.size, 1, sizeof(s.size), handle);
	fwrite(s.data, s.size, sizeof(s.data[0]), handle);
}

void File::write_number(double n) {
	fwrite(&n, sizeof(n), 1, handle);
}

void File::write_integer(u64 n) {
	fwrite(&n, sizeof(n), 1, handle);
}

String File::read_string() {
	u32 size = 0;  // type must match String.size
	fread(&size, sizeof(size), 1, handle);

	char* data = (char*) malloc(size + 1);

    if (!data)
    {
        panic("Malloc fail");
    }

	fread(data, sizeof(data[0]), size, handle);
	data[size] = '\0';

	return String(data, size);
}

double File::read_number() {
	double n = 0;
	fread(&n, sizeof(n), 1, handle);
	return n;
}

u64 File::read_integer() {
	u64 n = 0;
	fread(&n, sizeof(n), 1, handle);
	return n;
}

} // namespace
