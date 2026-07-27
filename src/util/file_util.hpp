#ifndef FILE_UTIL_HPP
#define FILE_UTIL_HPP

#include "string_util.hpp"

namespace melv
{

bool load_file(const char* filepath, BinaryData& bdata);
bool load_file_text(const char* filepath, String_Builder& s);

// access is directly passed to fopen
size_t write_to_file(const char* filepath, String_Builder& s, const char* access);

long get_file_size(FILE* file);

struct File {
	FILE* handle = nullptr;

    File(FILE* handle) : handle(handle) {}
	File(String filepath, const char* access) {
		char* tmp = (char*) malloc(filepath.size);

		memcpy(tmp, filepath.data, filepath.size);
		tmp[filepath.size] = '\0';

		handle = fopen(tmp, access);

		free(tmp);
	}
	~File() {
		fclose(handle);
	}

	void write_string(String s);
	void write_number(double n);
	void write_integer(u64 n);

	String read_string();
	double read_number();
	u64 read_integer();
};

} // namespace

#endif // FILE_UTIL_HPP
