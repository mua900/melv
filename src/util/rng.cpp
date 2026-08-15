#include "rng.hpp"

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
