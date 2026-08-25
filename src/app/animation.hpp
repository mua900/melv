#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "draw.hpp"

namespace melv {

	enum AnimationFlags {
		AnimationZero = 0,
		AnimationLoop = BIT(0),
	};

	struct SpriteAnimation
	{
		TextureAtlas atlas = {};

		float elapsed = 0;
		int current_frame = 0;

		float frame_duration = 0;
		int num_frames = 0;

		AnimationFlags flags = {};

		float total_duration()
		{
			return num_frames * frame_duration;
		}

		void step(float delta);
		InstanceData get_frame(vec3 position, float rotation, vec2 scale);
	};

	SpriteAnimation make_sprite_animation(TextureAtlas& atlas, int num_frames, float frame_duration, AnimationFlags flags);

} // namespace

#endif // ANIMATION_HPP