#include "camera.hpp"

namespace melv
{

Camera init_camera()
{
    Camera camera = {};
    camera.position = { 0, 0 };
    camera.zoom = 1;
    camera.rotation = 0;

    return camera;
}

melv::vec2 Camera::world_to_screen(melv::vec2 p) const
{
    p = (p - position).rotated(rotation);
    p *= zoom;
    return melv::vec2(p.x, -p.y);
}

melv::vec2 Camera::screen_to_world(melv::vec2 p) const
{
    auto q = melv::vec2(p.x, -p.y);
    q /= zoom;
    q = q.rotated(-rotation) + position;
    return q;
}

} // namespace
