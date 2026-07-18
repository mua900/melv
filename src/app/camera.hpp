#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "util/math_util.hpp"

struct Camera {
    cobot::vec2 position = {};
    float zoom = 0;
    float rotation = 0;
    cobot::vec2 offset = {};

    cobot::vec2 world_to_screen(cobot::vec2 p) const;
    cobot::vec2 screen_to_world(cobot::vec2 p) const;
};

#endif // CAMERA_HPP