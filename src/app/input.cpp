#include "input.hpp"

namespace melv {

ValueType get_input_value_type(InputKind kind)
{
    switch (kind)
    {
        case InputMouseButton:      return ValueType::BOOLEAN;
        case InputMouseWheel:       return ValueType::VECTOR2;
        case InputMouseMotion:      return ValueType::VECTOR2;
        case InputKeyboardKey:      return ValueType::BOOLEAN;
        case InputGamepadButton:    return ValueType::BOOLEAN;
        case InputGamepadAxis:      return ValueType::REAL;
        case InputGamepadVector:    return ValueType::VECTOR2;
        default:                    return ValueType::ZERO;
    }
}

bool KeyboardState::key_pressed(Scancode code) const
{
    // do_input is false when a text input has the keyboard focus (in library)
    return keys[code] && do_input;
}

bool GamepadState::is_connected() const
{
    return SDL_GamepadConnected(gamepad);
}

bool GamepadState::has_axis(SDL_GamepadAxis axis) const
{
    return SDL_GamepadHasAxis(gamepad, axis);
}

s16 GamepadState::get_axis_raw(SDL_GamepadAxis axis) const
{
    return SDL_GetGamepadAxis(gamepad, axis);
}

float GamepadState::get_axis_normalized(SDL_GamepadAxis axis, float range) const
{
    int maxRaw = MAX_SIGNED_16_BIT; //32767;
    float value = float(SDL_GetGamepadAxis(gamepad, axis)) / float(maxRaw);
    if (std::fabsf(value) < epsilon)
    {
        return 0;
    }

    value *= range;
    return clamp(-range, range, value);
}

float GamepadState::get_axis(SDL_GamepadAxis axis) const
{
    return get_axis_normalized(axis, 1);
}

ivec2 GamepadState::get_left_raw() const
{
    return ivec2(get_axis_raw(SDL_GAMEPAD_AXIS_LEFTX), get_axis_raw(SDL_GAMEPAD_AXIS_LEFTY));
}

ivec2 GamepadState::get_right_raw() const
{
    return ivec2(get_axis_raw(SDL_GAMEPAD_AXIS_RIGHTX), get_axis_raw(SDL_GAMEPAD_AXIS_RIGHTY));
}

vec2 GamepadState::get_left_normalized(float range) const
{
    return vec2(get_axis_normalized(SDL_GAMEPAD_AXIS_LEFTX, range), get_axis_normalized(SDL_GAMEPAD_AXIS_LEFTY, range));
}

vec2 GamepadState::get_right_normalized(float range) const
{
    return vec2(get_axis_normalized(SDL_GAMEPAD_AXIS_RIGHTX, range), get_axis_normalized(SDL_GAMEPAD_AXIS_RIGHTY, range));
}

vec2 GamepadState::get_left() const
{
    return vec2(get_axis(SDL_GAMEPAD_AXIS_LEFTX), get_axis(SDL_GAMEPAD_AXIS_LEFTY));
}

vec2 GamepadState::get_right() const
{
    return vec2(get_axis(SDL_GAMEPAD_AXIS_RIGHTX), get_axis(SDL_GAMEPAD_AXIS_RIGHTY));
}

bool GamepadState::has_button(SDL_GamepadButton button) const
{
    return SDL_GamepadHasButton(gamepad, button);
}

bool GamepadState::get_button(SDL_GamepadButton button) const
{
    return SDL_GetGamepadButton(gamepad, button);
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

bool GamepadState::dpad_up() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
}

bool GamepadState::dpad_down() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
}

bool GamepadState::dpad_left() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
}

bool GamepadState::dpad_right() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
}

bool GamepadState::back() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK);
}

bool GamepadState::start() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START);
}

bool GamepadState::left_stick() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK);
}

bool GamepadState::right_stick() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
}

bool GamepadState::guide() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_GUIDE);
}

bool GamepadState::left_shoulder() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
}

bool GamepadState::right_shoulder() const
{
    return SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
}

const char* get_gamepad_button_name(SDL_GamepadButton button)
{
    switch (button)
    {
        case SDL_GAMEPAD_BUTTON_SOUTH:          return "SOUTH";
        case SDL_GAMEPAD_BUTTON_EAST:           return "EAST";
        case SDL_GAMEPAD_BUTTON_WEST:           return "WEST";
        case SDL_GAMEPAD_BUTTON_NORTH:          return "NORTH";
        case SDL_GAMEPAD_BUTTON_BACK:           return "BACK";
        case SDL_GAMEPAD_BUTTON_GUIDE:          return "GUIDE";
        case SDL_GAMEPAD_BUTTON_START:          return "START";
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:     return "LEFT_STICK";
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    return "RIGHT_STICK";
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  return "LEFT_SHOULDER";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "RIGHT_SHOULDER";
        case SDL_GAMEPAD_BUTTON_DPAD_UP:        return "DPAD_UP";
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      return "DPAD_DOWN";
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      return "DPAD_LEFT";
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     return "DPAD_RIGHT";
        case SDL_GAMEPAD_BUTTON_MISC1:          return "MISC1";
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1:  return "RIGHT_PADDLE1";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1:   return "LEFT_PADDLE1";
        case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2:  return "RIGHT_PADDLE2";
        case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2:   return "LEFT_PADDLE2";
        case SDL_GAMEPAD_BUTTON_TOUCHPAD:       return "TOUCHPAD";
        case SDL_GAMEPAD_BUTTON_MISC2:          return "MISC2";
        case SDL_GAMEPAD_BUTTON_MISC3:          return "MISC3";
        case SDL_GAMEPAD_BUTTON_MISC4:          return "MISC4";
        case SDL_GAMEPAD_BUTTON_MISC5:          return "MISC5";
        case SDL_GAMEPAD_BUTTON_MISC6:          return "MISC6";
        default:                                return "INVALID";
    }
}

} // namespace
