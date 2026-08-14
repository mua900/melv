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

// https://stackoverflow.com/questions/2509679/how-to-generate-a-random-integer-number-from-within-a-range
int random_within_range(Xor32* x, int low, int high)
{
    // ASSERT(high >= low);

    u32 max = 0xffffffff;

    u32 range = 1 + high - low;
    u32 buckets = max / range;
    u32 limit = buckets * range;

    int result = 0;

    do {
        result = x->next();
    } while(result >= limit);

    result /= buckets;
    return result + low;
}

u32 Xor32::next()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

// https://en.wikipedia.org/wiki/Permuted_congruential_generator
static const u64 PCGMultiplier = 6364136223846793005u;

u32 PCG::next()
{
    u64 x = state;
    u32 count = (u32)(x >> 61);

    state = x * PCGMultiplier;
    x ^= x >> 22;
    return (u32) (x >> (22 + count));
}
