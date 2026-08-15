#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <array>

#define BIT(x) ((uint64_t)1 << (x))

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

constexpr s16 MAX_SIGNED_16_BIT = s16(0x7FFF);
constexpr s32 MAX_SIGNED_32_BIT = s32(0x7FFFFFFF);
constexpr s64 MAX_SIGNED_64_BIT = s64(0x7FFFFFFFFFFFFFFF);

constexpr int MAX_INTEGER = int(-1) ^ (1 << (sizeof(int) * 8 - 1));

u64 pop_count(u64 x);

#ifdef _MSC_VER

#include <intrin.h>

#define NORETURN __declspec(noreturn)

static inline unsigned int msvc_trailing_zeros(u64 x)
{
    unsigned long pos = 0;
    unsigned char is_zero = _BitScanForward64(&pos, x);
    // @note no checking for zero here since we assume non-zero input.
    return pos;
}

static inline unsigned int msvc_leading_zeros(u64 x)
{
    unsigned long pos = 0;
    unsigned char is_zero = _BitScanReverse64(&pos, x);
    // @note no checking for zero here since we assume non-zero input.
    return pos;
}

#define POP_COUNT(x)      pop_count(x)
#define LEADING_ZEROS(x)  msvc_leading_zeros(x)
#define TRAILING_ZEROS(x) msvc_trailing_zeros(x)

#else // _MSC_VER

#define NORETURN __attribute__((noreturn))

#define POP_COUNT(x)      __builtin_popcountll(x)
#define LEADING_ZEROS(x)  __builtin_clzll(x)
#define TRAILING_ZEROS(x) __builtin_ctzll(x)

#endif

#define ASSERT(x)   do {    \
        if (!(x)) {             \
            fprintf(stderr, "-----*****----- Assertion failed at %s:%d   %s\n", __FILE__, __LINE__, #x); \
            exit(1);    \
        }               \
    } while(0)


NORETURN
void panic(char const* const msg);

#define NOT_IMPLEMENTED(x) panic(x " not implemeneted");

int pop_lsb(u64* x);
int pop_msb(u64* x);

int lsb_index(u64 x);
int msb_index(u64 x);

struct BinaryData {
	u8* data = nullptr;
	size_t size = 0;

    ~BinaryData() {
        if (data) {
            free(data);

            data = nullptr;
            size = 0;
        }
    }

	void release() {
		if (data) {
			free(data);
			data = nullptr;
		}
	}
};

struct Find_Result {
	int index = 0;
	bool found = false;

    Find_Result(int index, bool found) : index(index), found(found) {}
    Find_Result() {}
};

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define MIN(x,y) (((x) > (y)) ? (y) : (x))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define CLAMP(x, lower, upper) (MIN(upper, MAX(x, lower)))

#ifdef _WIN32
    static const char* PathSeparator = "\\";
    static const char* NewLine = "\r\n";
#else
    static const char* PathSeparator = "/";
    static const char* NewLine = "\n";
#endif

static inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static inline bool is_alpha_lower(char c)
{
    return c >= 'a' && c <= 'z';
}

static inline bool is_alpha_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static inline bool is_alpha(char c)
{
    return is_alpha_lower(c) || is_alpha_upper(c);
}

static inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline char to_lower_ascii(char c)
{
    return is_alpha_upper(c) ? (c - 'A' + 'a') : (c);
}

static inline char to_upper_ascii(char c)
{
    return is_alpha_lower(c) ? (c - 'a' + 'A') : (c);
}

#define BOOL_STRING(b) ((b) ? ("true") : ("false"))
