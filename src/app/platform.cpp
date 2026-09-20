#include "platform.hpp"

// @todo separate these into separate source files

#if SDL_PLATFORM_WINDOWS
    #pragma comment(lib, "dxgi.lib")
#endif

bool init_platform_context(PlatformContext* platform)
{
#if SDL_PLATFORM_WINDOWS
    IDXGIFactory4 *dxgi_factory = nullptr;
    IDXGIAdapter1 *dxgi_adapter1 = nullptr;
    IDXGIAdapter3 *dxgi_adapter3 = nullptr;

    if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory))))
    {
        return false;
    }

    if (!SUCCEEDED(dxgi_factory->EnumAdapters1(0, &dxgi_adapter1)))
    {
        dxgi_factory->Release();
        return false;
    }

    if (!SUCCEEDED(dxgi_adapter1->QueryInterface(IID_PPV_ARGS(&dxgi_adapter3))))
    {
        dxgi_factory->Release();
        dxgi_adapter1->Release();
        return false;
    }

    dxgi_factory->Release();
    dxgi_adapter1->Release();

    platform->dxgi_adapter3 = dxgi_adapter3;

    return true;
#elif SDL_PLATFORM_LINUX
    return false;
#elif SDL_PLATFORM_MACOS
    return false;
#endif
}

void cleanup_platform_context(PlatformContext *platform)
{
#if SDL_PLATFORM_WINDOWS
    platform->dxgi_adapter3->Release();
#elif SDL_PLATFORM_LINUX
#elif SDL_PLATFORM_MACOS
#endif
}

void get_process_video_memory_info(PlatformContext* platform, VideoMemoryInfo *info)
{
#if SDL_PLATFORM_WINDOWS
    DXGI_QUERY_VIDEO_MEMORY_INFO mem_info = {};
    if (SUCCEEDED(platform->dxgi_adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &mem_info)))
    {
        info->budget = mem_info.Budget;
        info->usage = mem_info.CurrentUsage;
    }
#elif SDL_PLATFORM_LINUX
#elif SDL_PLATFORM_MACOS
#endif
}