#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "util/math_util.hpp"

struct Camera {
    melv::vec2 position = {};
    float zoom = 0;
    float rotation = 0;
    melv::vec2 offset = {};

    melv::vec2 world_to_screen(melv::vec2 p) const;
    melv::vec2 screen_to_world(melv::vec2 p) const;
};

#endif // CAMERA_HPP
