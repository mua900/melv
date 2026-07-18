#include "input.hpp"

bool KeyboardState::key_pressed(Scancode code) const
{
    return keys[code] && do_input;
}