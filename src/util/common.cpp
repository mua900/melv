#include "common.hpp"

#include <cmath>
#include <array>


u64 pop_count(u64 x)
{
    x = (x & (u64)0x5555555555555555) + ((x >> 1)  & (u64)0x5555555555555555);
    x = (x & (u64)0x3333333333333333) + ((x >> 2)  & (u64)0x3333333333333333);
    x = (x & (u64)0x0F0F0F0F0F0F0F0F) + ((x >> 4)  & (u64)0x0F0F0F0F0F0F0F0F);
    x = (x & (u64)0x00FF00FF00FF00FF) + ((x >> 8)  & (u64)0x00FF00FF00FF00FF);
    x = (x & (u64)0x0000FFFF0000FFFF) + ((x >> 16) & (u64)0x0000FFFF0000FFFF);
    x = (x & (u64)0x00000000FFFFFFFF) + ((x >> 32) & (u64)0x00000000FFFFFFFF);
    return x;
}

NORETURN
void panic(char const* const msg)
{
    fprintf(stderr, "[PANIC]: %s\n", msg);
    exit(1);
}

int pop_lsb(u64* x) {
    int index = TRAILING_ZEROS(*x);
    *x &= *x - 1;
    return index;
}

int pop_msb(u64* x) {
    int index = LEADING_ZEROS(*x);
    *x &= ~BIT(index);
    return index;
}

int lsb_index(u64 x) {
    return TRAILING_ZEROS(x);
}

int msb_index(u64 x) {
    return 63 - LEADING_ZEROS(x);
}
