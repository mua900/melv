#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "util/common.hpp"
#include <SDL3/SDL_platform_defines.h>

#if SDL_PLATFORM_WINDOWS
    #include <dxgi1_4.h>
#elif SDL_PLATFORM_LINUX
    // @todo
#elif SDL_PLATFORM_MACOS
    // @todo
#else
    #error Platform not supported
#endif

struct PlatformContext
{
#if SDL_PLATFORM_WINDOWS
    IDXGIAdapter3 *dxgi_adapter3 = nullptr;
#elif SDL_PLATFORM_LINUX
    // @todo
#elif SDL_PLATFORM_MACOS
    // @todo
#endif
};

struct VideoMemoryInfo
{
    size_t budget = 0;
    size_t usage = 0;
};

bool init_platform_context(PlatformContext* platform);
void cleanup_platform_context(PlatformContext* platform);

void get_process_video_memory_info(PlatformContext* platform, VideoMemoryInfo* info);

#endif // PLATFORM_HPP