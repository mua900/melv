#include "camera.hpp"

namespace melv
{

melv::vec2 Camera::world_to_screen(melv::vec2 p) const
{
    p = (p - position).rotated(rotation) * zoom;
    return melv::vec2(p.x, -p.y);
}

melv::vec2 Camera::screen_to_world(melv::vec2 p) const
{
    return (melv::vec2(p.x, -p.y) / zoom).rotated(-rotation) + position;
}

} // namespace
