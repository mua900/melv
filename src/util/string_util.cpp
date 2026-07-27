#include "string_util.hpp"

namespace melv
{

int string_length(const char* cstr) {
    return (int)strlen(cstr);
}

String make_string(const char* s)
{
    int len = (int)strlen(s);
    return { s, len };
}

bool string_compare(String s1, String s2)
{
    if (s1.size != s2.size) return false;
    for (int i = 0; i < s1.size; i++)
    {
        if (s1.data[i] != s2.data[i]) return false;
    }
    return true;
}


String next_word(String source, int& offset, char delimeter)
{
    offset += string_match_character(source, offset, delimeter);
    String word = string_slice_to_character(source, offset, delimeter);
    offset += word.size;
    return word;
}

String string_slice(String s, int start, int end)
{
    return String { s.data + start, end - start };
}

String string_slice_to_character(String s, int start, char c) {
    int cursor = start;
    while (cursor < s.size && s.data[cursor] != c) {
        cursor += 1;
    }

    return String(s.data + start, cursor - start);
}

int string_match_start(String s1, String s2)
{
    int cursor = 0;
    while (cursor < s1.size && cursor < s2.size)
    {
        if (s1.data[cursor] != s2.data[cursor])
            break;
        cursor += 1;
    }

    return cursor;
}

bool string_starts_with(String s, String start)
{
    return string_match_start(s, start) == start.size;
}

int string_match_character(const String s, int offset, char c)
{
    if (offset >= s.size)
    {
        return 0;
    }

    int count = 0;
    while (s.size - (offset + count) > 0 && s.data[offset + count] == c)
    {
        count += 1;
    }

    return count;
}

int string_find_character(String s, int offset, char c)
{
    if (offset >= s.size)
        return -1;

    int cursor = 0;
    while (s.size - (offset + cursor) > 0 && s.data[offset + cursor] != c)
    {
        cursor += 1;
    }

    return (s.data[offset + cursor] == c) ? (offset + cursor) : -1;
}

String string_get_extension(String s)
{
    for (int i = s.size - 1; i >= 0; i--)
    {
        if (s.data[i] == '.')
        {
            // without the '.'
            return string_slice(s, i + 1, s.size);
        }
    }

    return String{NULL,0};
}

String string_get_file_name(String s) {
    for (int i = s.size - 1; i >= 0; i--) {
        if (s.data[i] == '.') {
            return string_slice(s, 0, i);
        }
    }

    return String{NULL, 0};
}

u64 string_hash(String s)
{
    u64 hash = 5383;

    for (int i = 0; i < s.size; i++)
    {
        int c = s.data[i];
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

int string_to_integer(String s, bool* success)
{
    int accum = 0;
    int power = 1;
    for (int i = s.size - 1; i >= 0; i--)
    {
        if (!(s.data[i] >= '0' && s.data[i] <= '9'))
        {
            *success = false;
            return 0;
        }

        accum += (s.data[i] - '0') * power;

        power *= 10;
    }

    *success = true;
    return accum;
}

double string_to_real(String s, bool* success)
{
    char* end_ptr = NULL;
    SCOPE_STRING(s, cstr);
    double res = std::strtod(cstr, &end_ptr);
    if (end_ptr == cstr)
    {
        if (success)
            *success = false;
        return 0.0;
    }

    if (success)
        *success = true;
    return res;
}

void String_Builder::create(int initial_capacity)
{
    ASSERT(initial_capacity > 0);
    buffer = (char*)malloc(initial_capacity);
    if (!buffer) panic("Malloc fail");
    buffer_capacity = initial_capacity;
    cursor = 0;
}

String_Builder::String_Builder(int initial_capacity) {
    create(initial_capacity);
}

void String_Builder::remove(int amount)
{
    cursor = MAX(0, cursor - amount);
}

void String_Builder::remove_slice(int start, int end)
{
    if (start >= cursor || start >= end)
        return;

    if (end >= cursor)
        end = cursor;

    int amount = cursor - end;
    std::memmove(buffer + start, buffer + end, amount);

    cursor -= (end - start);
}

void String_Builder::resize() {
    int ncap = buffer_capacity ? buffer_capacity * 2 : 32;
    char* nbuff = (char*) malloc(ncap);
    if (!nbuff) panic("Malloc fail");
    if (buffer)
    {
        std::memcpy(nbuff, buffer, cursor);
        std::free(buffer);
    }
    buffer = nbuff;
    buffer_capacity = ncap;
}

int String_Builder::ensure_size(int size) {
    int count = 0;
    while (size >= buffer_capacity) {
        resize();
        count++;
        if (count > 10) {
            fprintf(stderr, "String builder buffer resize failed repeatedly: Possible memory allocation issue or corrupted buffer state.\n"
                "Relevant: buffer_capacity: %d, cursor: %d, provided string size: %d",
                buffer_capacity, cursor, size);
            return 1;
        }
    }

    return 0;
}

int String_Builder::append(String string) {
    ensure_size(cursor + string.size);

    std::memcpy(buffer + cursor, string.data, string.size);
    cursor += string.size;
    return string.size;
}

int String_Builder::append_path(String string)
{
    int total = 0;

    ensure_size(cursor + string.size);
    for (int i = 0; i < string.size; i++) {
        if (string.data[i] == '/')
        {
            total += append(make_string(PathSeparator));
        }
        else
        {
            total += append_char(string.data[i]);
        }
    }

    return total;
}

bool String_Builder::ends_with(String s) const {
    return string_compare(s, String(buffer + cursor - s.size, s.size));
}


int String_Builder::append_char(char ch) {
    ensure_size(cursor + 1);

    buffer[cursor] = ch;
    cursor += 1;
    return 1;
}

int String_Builder::append_integer(int n)
{
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer), "%d", n);
    append(make_string(buffer));
    return len;
}

int String_Builder::append_hex(int n) {
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer), "%x", n);
    append(make_string(buffer));
    return len;
}

int String_Builder::append_float(float n) {
	char buffer[128];
	int len = snprintf(buffer, sizeof(buffer), "%1.3f", n);
	append(make_string(buffer));
	return len;
}

String String_Builder::put_string(String s) {
    int c = cursor;
    append(s);
    return String(buffer + c, s.size);
}

String String_Builder::put_path(String path) {
    int c = cursor;
    append_path(path);
    return String(buffer + c, path.size);
}

int String_Builder::clear_and_append(String s) {
    cursor = 0;
    append(s);
    return s.size;
}

int String_Builder::clear_and_append_float(float n) {
    clear();
    return append_float(n);
}


int String_Builder::append_many(String* strings, int n) {
    int total_length = 0;
    for (int i = 0; i < n; i++) {
        total_length += strings[i].size;
    }

    ensure_size(this->cursor + total_length);
    for (int i = 0; i < n; i++) {
        std::memcpy(this->buffer + this->cursor, strings[i].data, strings[i].size);
        cursor += strings[i].size;
    }

    return total_length;
}

const char* String_Builder::c_string() {
    ensure_size(this->cursor + 1);

    this->buffer[this->cursor] = '\0';
    return this->buffer;
}

void String_Builder::free_buffer() {
    if (buffer) {
        std::free(this->buffer);
    }
    cursor = 0;
    buffer_capacity = 0;
    buffer = NULL;
}

void String_Builder::clear() {
    cursor = 0;
    if (buffer && buffer_capacity > 0)
    {
        buffer[0] = '\0';
    }
}

String String_Builder::to_string()
{
    return String(buffer, cursor);
}

String String_Builder::slice(int start, int length) const
{
    return String(buffer + start, length);
}

bool String::advance(int amount) {
    if (amount >= size)
    {
        return false;
    }
    data += amount; size -= amount;
    return true;
}

void String::trim() {
	while (size > 0 && is_space(data[size-1])) {
		size--;
	}

	while (size > 0 && is_space(data[0])) {
		size--;
		data++;
	}
}

bool String::operator==(const String& other) const
{
    return string_compare(*this, other);
}


int utf8_handle_start_character(u8 c)
{
    if (!(c & 0x80))                return 1;
    if ((c & 0xE0) == 0xC0)         return 2;
    if ((c & 0xF0) == 0xE0)         return 3;
    if ((c & 0xF8) == 0xF0)         return 4;
    return 0;
}

bool utf8_is_continuation(u8 c)
{
    return (c & 0xC0) == 0x80;
}

// 0 for error
int utf8_next(String s, int offset)
{
    if (offset >= s.size)
    {
        return 0;
    }

    u8 c = s.data[offset];
    int len = utf8_handle_start_character(c);

    if (offset + len - 1 >= s.size)
    {
        return 0;
    }

    int i = 1;
    while (i < len)
    {
        c = s.data[offset + i];

        if (!utf8_is_continuation(c))
        {
            return 0;
        }

        i += 1;
    }

    return len;
}

int utf8_previous(String s, int offset)
{
    if (offset > s.size || offset < 0)
    {
        return 0;
    }

    u8 c = s.data[offset - 1];
    int i = 1;
    while (utf8_is_continuation(c))
    {
        i += 1;

        if (i > 4 || offset < i)
        {
            return 0;
        }

        c = s.data[offset - i];
    }

    int len = utf8_handle_start_character(c);

    if (len != i)
    {
        return 0;
    }

    return len;
}

int string_length_utf8(String s)
{
    int count = 0;
    int pos = 0;
    while (pos < s.size)
    {
        int next = utf8_next(s, pos);
        if (next == 0)
        {
            return 0;
        }

        pos += next;
        count += 1;
    }

    if (pos != s.size)
    {
        return 0;
    }

    return count;
}

} // namespace
