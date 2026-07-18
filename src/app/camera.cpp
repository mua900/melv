#include "camera.hpp"

cobot::vec2 Camera::world_to_screen(cobot::vec2 p) const
{
    p = (p - position).rotated(rotation) * zoom;
    return cobot::vec2(p.x, -p.y) + offset;
}

cobot::vec2 Camera::screen_to_world(cobot::vec2 p) const
{
    p = p - offset;
    return (cobot::vec2(p.x, -p.y) / zoom).rotated(-rotation) + position;
}
