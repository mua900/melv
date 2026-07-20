#include "input.hpp"

namespace melv {

bool KeyboardState::key_pressed(Scancode code) const
{
    return keys[code] && do_input;
}

} // namespace
