#include "camera.hpp"

melv::vec2 Camera::world_to_screen(melv::vec2 p) const
{
    p = (p - position).rotated(rotation) * zoom;
    return melv::vec2(p.x, -p.y) + offset;
}

melv::vec2 Camera::screen_to_world(melv::vec2 p) const
{
    p = p - offset;
    return (melv::vec2(p.x, -p.y) / zoom).rotated(-rotation) + position;
}
