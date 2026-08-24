#include "animation.hpp"

namespace melv {

    void SpriteAnimation::step(float delta)
    {
        float total = total_duration();
        elapsed += delta;
        if (elapsed >= total)
        {
            if (flags & AnimationLoop)
            {
                elapsed = 0;
                current_frame = 0;
            }
        }

        current_frame = float(num_frames) * (elapsed / total);
    }

    void SpriteAnimation::draw(RenderContext& render, vec3 position, float rotation, vec2 scale)
    {
        InstanceData data = {};
        data.x = position.x;
        data.y = position.y;
        data.z = position.z;
        data.rotation = rotation;
        data.scale = pack_scale(scale);
        data.color = 0xffffffff;
        data.sourceOffset = pack_unorm16x2(atlas.calculate_position_index(current_frame));
        data.sourceScale = pack_unorm16x2(atlas.get_element_scale());

        queue_draw_group(render, data, group);
    }

} // namespace