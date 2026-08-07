#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "util/common.hpp"
#include "util/math_util.hpp"

namespace melv
{

using CameraId = u32;

struct Camera {
    melv::vec2 position = {};
    float zoom = {};
    float rotation = 0;

    melv::vec2 world_to_screen(melv::vec2 p) const;
    melv::vec2 screen_to_world(melv::vec2 p) const;
};

// get an camera with identity transform
Camera init_camera();

} // namespace

#endif // CAMERA_HPP
