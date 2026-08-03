#include "input.hpp"

namespace melv {

bool KeyboardState::key_pressed(Scancode code) const
{
    // do_input is false when a text input has the keyboard focus (in library)
    return keys[code] && do_input;
}

bool GamepadState::south() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
}

bool GamepadState::north() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH);

}

bool GamepadState::west() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST);
}

bool GamepadState::east() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST);
}

} // namespace
