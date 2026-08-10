#ifndef BITSET_HPP
#define BITSET_HPP

#include "common.hpp"

template<int Size = 8>
struct Bitset
{
    u8 bits[Size];

    bool in_bounds(int index)
    {
        return (index > 0) && (index < Size * 8);
    }

    void set_bit(int index, bool value)
    {
        if (!in_bounds(index))
        {
            panic("Out of bounds access to bitset");
        }

        int mod = index % 8;
        if (value)
        {
            bits[index / 8] |= BIT(mod);
        }
        else
        {
            bits[index / 8] &= ~BIT(mod);
        }
    }

    bool read_bit(int index)
    {
        if (!in_bounds(index))
        {
            panic("Out of bounds access to bitset");
        }

        return bool(bits[(index / 8)] & BIT(index % 8));
    }
};

#endif // BITSET_HPP