#pragma once

#include "util/math_util.hpp"

#include <SDL3/SDL.h>

#define MOUSE_LEFT   SDL_BUTTON_LEFT
#define MOUSE_MIDDLE SDL_BUTTON_MIDDLE
#define MOUSE_RIGHT  SDL_BUTTON_RIGHT

#define MOUSE_LEFT_MASK   SDL_BUTTON_LMASK
#define MOUSE_MIDDLE_MASK SDL_BUTTON_MMASK
#define MOUSE_RIGHT_MASK  SDL_BUTTON_RMASK

#define KEY_W       SDL_SCANCODE_W
#define KEY_D       SDL_SCANCODE_D
#define KEY_S       SDL_SCANCODE_S
#define KEY_A       SDL_SCANCODE_A
#define KEY_Q       SDL_SCANCODE_Q
#define KEY_E       SDL_SCANCODE_E
#define KEY_X       SDL_SCANCODE_X
#define KEY_Z       SDL_SCANCODE_Z
#define KEY_C       SDL_SCANCODE_C
#define KEY_SPACE   SDL_SCANCODE_SPACE
#define KEY_RETURN  SDL_SCANCODE_RETURN
#define KEY_UP      SDL_SCANCODE_UP
#define KEY_DOWN    SDL_SCANCODE_DOWN
#define KEY_RIGHT   SDL_SCANCODE_RIGHT
#define KEY_LEFT    SDL_SCANCODE_LEFT

#define KEYMOD_LEFT_SHIFT SDL_KMOD_LSHIFT

using Scancode = SDL_Scancode;

using KeyboardEvent = SDL_KeyboardEvent;

struct KeyboardState {
    const bool* keys = {};
    int num_keys = {};
    SDL_Keymod mod_state = {};

    bool do_input = false;

    bool key_pressed(Scancode code) const;
};

struct MouseCursor {
    SDL_Cursor* normal = nullptr;
    SDL_Cursor* text = nullptr;
    SDL_Cursor* resize_ns = nullptr;
    SDL_Cursor* resize_ew = nullptr;
};

struct MouseState {
    cobot::vec2 pos = {};
    SDL_MouseButtonFlags buttonFlags = {};
    MouseCursor cursor = {};
    bool down = false;

    cobot::vec2 dragPosition = {};
    bool drag = false;
};

struct Input {
    KeyboardState keyboard;
    MouseState mouse;
};
