#ifndef TEXT_HPP
#define TEXT_HPP

#include "util/math_util.hpp"
#include "util/string_util.hpp"
#include <SDL3_ttf/SDL_ttf.h>

#include "draw_types.hpp"

namespace melv
{

struct Font {
    TTF_Font* font = nullptr;
    float size = 0;
};

bool load_font(Font* font, String_Builder& path, String font_folder, String font_file, float size);
bool load_font_file(Font* font, const char* path, float size);

union UiUserData {
    s64 number;
    void* ptr;
};

struct Text {
    TextureHandle texture = {};
    String string = {};
    melv::Color color = {};

    Text() {}
    Text(TextureHandle p_texture, String p_string, melv::Color col)
        : texture(p_texture), string(p_string), color(col)
    {}

    void clear()
    {
        texture = {};

        string.data = NULL;
        string.size = 0;

        color = {};
    }
};

struct Icon {
    TextureHandle texture = {};
    melv::Color background = {};

    Icon () {}
    Icon (TextureHandle tex, melv::Color bground) : texture(tex), background(bground) {}
};

struct IconButton {
    Icon icon = {};
    UiUserData data = {};

    IconButton() {}
    IconButton(TextureHandle tex, melv::Color background) : icon(tex, background) {}
    IconButton(TextureHandle tex, melv::Color background, s64 n) : icon(tex, background) {
        data.number = n;
    }
    IconButton(TextureHandle tex, melv::Color background, void* ptr) : icon(tex, background) {
        data.ptr = ptr;
    }
};

} // namespace

#endif // TEXT_HPP
