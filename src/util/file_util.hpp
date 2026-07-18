#ifndef FILE_UTIL_HPP
#define FILE_UTIL_HPP

#include "string_util.hpp"

bool load_file(const char* filepath, BinaryData& bdata);
bool load_file_text(const char* filepath, String_Builder& s);

long get_file_size(FILE* file);

struct File {
	FILE* handle = nullptr;

    File(FILE* handle) : handle(handle) {}
	File(String filepath, const char* access) {
		SCOPE_STRING(filepath, buffer);

		handle = fopen(buffer, access);
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


#endif // FILE_UTIL_HPP