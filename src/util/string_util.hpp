#ifndef STRING_UTIL_HPP
#define STRING_UTIL_HPP

#include "common.hpp"

namespace melv
{

int string_length(const char* cstr);

struct String {
    const char* data = NULL;
    int size = 0;

    String () {}
	explicit String (const char* d) : data(d), size(string_length(d)) {}
    String (const char* d, int s) : data(d), size(s) {}
	String (const BinaryData& b) : data((const char*)b.data), size(b.size) {}

    bool advance(int amount);

    char operator[](int index) const { return data[index]; }
    bool operator==(const String& other) const;
	void trim();
};

#define STRING_EMPTY ((String){.data=NULL,.size=0})
#define CSTRING_LENGTH(s) (sizeof(s)-1)

#define SCOPE_STRING_EXP(p_s, p_name, p_size)				\
	char p_name[p_size];  \
	memcpy(p_name, p_s.data, p_s.size);			\
	p_name[p_s.size] = '\0';

#define SCOPE_STRING(str, name) SCOPE_STRING_EXP(str, name, 256)

String make_string(const char* s);
bool string_compare(String s1, String s2);
bool string_starts_with(String s, String start);
int string_match_start(String s1, String s2);
int string_match_character(const String s, int offset, char c);
int string_find_character(String s, int offset, char c);
String string_slice(String s, int start, int end);
String string_slice_to_character(String s, int start, char c);
String string_get_extension(String s);  // the extension
String string_get_file_name(String s);  // the string except the extension
u64 string_hash(String s);

int string_to_integer(String s, bool* success);
double string_to_real(String s, bool* success);

struct MutableString {
    char* data = nullptr;
    int size = 0;
    int cap = 0;

    void clear_values() { data = nullptr; size = 0; cap = 0; }

    MutableString() {
        create(128);
    }

    MutableString(int init_cap) {
        create(init_cap);
    }

	MutableString(const char* s)
	{
		int len = strlen(s);
		create(len);
		memcpy(data, s, len * sizeof(char));
		size = len;
	}
	
    MutableString(String s) {
        create(s.size);
        memcpy(data, s.data, s.size * sizeof(char));
        size = s.size;
    }

    MutableString(MutableString& other) = delete;
    void operator=(MutableString& other) = delete;
    MutableString(MutableString&& other) noexcept {
        data = other.data;
        size = other.size;
        cap = other.cap;
        other.clear_values();
    }
    void operator=(MutableString&& other) noexcept {
        data = other.data;
        size = other.size;
        cap = other.cap;
        other.clear_values();
    }

    ~MutableString()
    {
        delete[] data;
    }

    void create(int init_cap)
    {
        data = new char[init_cap];
        cap = init_cap;
    }

    void set_str(const char* str)
    {
        int len = (int) strlen(str);
        ensure_size(len);
        memcpy(data, str, len * sizeof(char));
        size = len;
    }

    void set_string(String string)
    {
        ensure_size(string.size);
        memcpy(data, string.data, string.size * sizeof(char));
        size = string.size;
    }

    String to_string()
    {
        return String(data, size);
    }

	void ensure_size(int capacity) {
		if (cap < capacity) {
			resize(capacity);
		}
	}

	void resize(int ncap) {
		char* new_buffer = new char[ncap];

		int new_size = (ncap < size) ? ncap : size;

		for (int i = 0; i < new_size; i++) {
			new_buffer[i] = data[i];
		}

		delete[] data;

		data = new_buffer;
		cap = ncap;
		size = new_size;
	}
};

struct StringReference {
    s64 offset = 0;
    s64 length = 0;

    StringReference() {}
    StringReference(s64 off, s64 len) : offset(off), length(len) {}
};

struct String_Builder {
    char* buffer = nullptr;
    int buffer_capacity = 0;
    int cursor = 0;

    String_Builder() {
        create(128);
    }

    String_Builder(int initial_capacity);

    ~String_Builder() {
        if (buffer) {
            free(buffer);
            buffer = nullptr;
        }
    }

    String_Builder(const String_Builder& other) {
        free_buffer();

        ensure_size(other.buffer_capacity);
        ASSERT(buffer && other.buffer);
        memcpy(buffer, other.buffer, other.cursor);
        cursor = other.cursor;
    }

    void operator=(const String_Builder& other) {
        free_buffer();

        ensure_size(other.buffer_capacity);
        ASSERT(buffer && other.buffer);
        memcpy(buffer, other.buffer, other.cursor);
        cursor = other.cursor;
    }

    String_Builder(String_Builder&& other) noexcept {
        free_buffer();

        buffer = other.buffer;
        cursor = other.cursor;
        buffer_capacity = other.buffer_capacity;
        other.clear_values();
    }

    void operator=(String_Builder&& other) noexcept {
        free_buffer();

        buffer = other.buffer;
        cursor = other.cursor;
        buffer_capacity = other.buffer_capacity;
        other.clear_values();
    }

    bool is_empty() const { return cursor == 0; }
    const char* get_end() const { return buffer + cursor; }
    void create(int initial_capacity);
    int append(String string);
    int append_path(String string);  // expect / as the separator and replace it with \\ on windows
    int append_char(char ch);
    int append_integer(int n);
    int append_hex(int n);
	int append_float(float n);
    String put_string(String s);
    String put_path(String path);
    const char* c_string();
    bool ends_with(String s) const;
    void remove(int amount);  // remove the last n characters from the buffer
    void remove_slice(int start, int end);
    int clear_and_append(String s);
    int clear_and_append_float(float n);
    int append_many(String* strings, int n);
    void free_buffer();
    void clear();
    String to_string();
    String slice(int start, int length) const;
    String get_string(StringReference ref) const { return slice(ref.offset, ref.length); }
    StringReference get_reference(String s) const {
        s64 offset = std::ptrdiff_t(s.data - buffer);
        if (offset > 100000 || offset < 0) {
            panic("Probably wrong pointers");
        }
        return StringReference{ offset, s.size };
    }
    int ensure_size(int size);
private:
    void resize();
    void clear_values() { buffer = nullptr; buffer_capacity = 0; cursor = 0; }
};


int utf8_handle_start_character(u8 c);
bool utf8_is_continuation(u8 c);

int utf8_next(String s, int offset);
int utf8_previous(String s, int offset);

int string_length_utf8(String s);

} // namespace
	
#endif // STRING_UTIL_HPP
