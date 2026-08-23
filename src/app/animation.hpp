#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "draw.hpp"

namespace melv {

	enum AnimationFlags {
		AnimationLoop = BIT(0),
	};

	struct SpriteAnimation
	{
		DrawGroupId group = {};
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
		void draw(RenderContext& render, vec3 position, float rotation, vec2 scale);
	};

} // namespace

#endif // ANIMATION_HPP