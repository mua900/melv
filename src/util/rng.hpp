#ifndef RNG_HPP
#define RNG_HPP

#include "common.hpp"

struct Xor32
{
    u32 state = 0;

    Xor32() {}
    Xor32(u32 seed)
        : state(seed)
    {}
    void init(u32 seed)
    {
        state = seed;
    }
    u32 next();
};
u32 xor32(u32 x);

struct PCG
{
    u64 state = 0;

    void init(u64 seed);
    u32 next();
};

int random_within_range(Xor32* x, int low, int high);

#endif // RNG_HPP