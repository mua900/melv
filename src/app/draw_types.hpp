#ifndef DRAW_TYPES_HPP
#define DRAW_TYPES_HPP

#include <SDL3/SDL.h>
#include "util/common.hpp"

namespace melv {

struct Texture
{
    u32 index = 0;
    u32 mip_levels = 0;

    Texture() {}
    Texture(u32 idx)
        : index(idx)
    {}

    bool is_valid()
    {
        return index != -1;
    }
};

using BufferHandle = u32;
using DrawGroupId = u32;

#define TEXTURE_INVALID Texture(-1)
#define DRAW_GROUPID_INVALID   u32(-1)

using TextureFormat = SDL_GPUTextureFormat;

enum SampleCount
{
    SampleCount1 = SDL_GPU_SAMPLECOUNT_1,
    SampleCount2 = SDL_GPU_SAMPLECOUNT_2,
    SampleCount4 = SDL_GPU_SAMPLECOUNT_4,
    SampleCount8 = SDL_GPU_SAMPLECOUNT_8,
};

enum TextureUsage
{
    TextureUsageSampler = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    TextureUsageColorTarget = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
    TextureUsageDepthStencilTarget = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
};

} // namespace

#endif // DRAW_TYPES_HPP