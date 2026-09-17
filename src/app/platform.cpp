#include "platform.hpp"

#if SDL_PLATFORM_WINDOWS
    #include <dxgi1_4.h>
#elif SDL_PLATFORM_LINUX
    // @todo
#elif SDL_PLATFORM_MACOS
    // @todo
#else
    #error Platform not supported
#endif

// @todo
size_t get_process_video_memory_usage()
{
#if SDL_PLATFORM_WINDOWS
    return 0;
#elif SDL_PLATFORM_LINUX
    return 0;
#elif SDL_PLATFORM_MACOS
    return 0;
#else
    return 0;
#endif
}