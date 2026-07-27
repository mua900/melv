#include "asset.hpp"
#include "util/log.hpp"
#include "util/string_util.hpp"
#include "util/file_util.hpp"

#include <SDL3_image/SDL_image.h>

namespace melv
{

bool load_asset(int index, AssetCatalog& catalog);
bool unload_asset(AssetId id, AssetCatalog& catalog, bool reset_generation = true);

SDL_EnumerationResult load_asset_callback(void* userdata, const char* dirname, const char* fname);
SDL_EnumerationResult unload_asset_callback(void* userdata, const char* dirname, const char* fname);

bool load_asset_file(String_Builder& path, Asset& asset, AssetLoadContext& load_context);
void unload_asset_file(Asset& asset, AssetLoadContext& load_context, bool reset_generation = true);

bool parse_image_attribute(String attribute, SDL_Texture*& texture);
bool parse_audio_attribute(String attribute, MIX_Audio*& audio);
bool parse_font_attribute(String attribute, Font& font);
bool parse_shader_attribute(String attribute, Shader& shader);

bool parse_attribute_value(String attribute, const char* key, String& out_value);

#define ASSET_LINE_IS_VALID              BIT(0)
#define ASSET_LINE_IS_COMMENT            BIT(1)
#define ASSET_LINE_IS_EMPTY              BIT(2)
#define ASSET_LINE_HAS_TRAILING_TOKENS   BIT(3)
#define ASSET_LINE_REPEATS_TOKENS        BIT(4)

using AssetParseLineResult = u32;

// return false on failure or comment
// <kind> <scope optional> <name> <path> [optional] [lazy]
AssetParseLineResult asset_parse_line(String file, String line, Asset& pointer)
{
#if LOG_VERBOSE
    SCOPE_STRING(line, cstr);
    log_info("Asset entry: %s\n", cstr);
#endif

	char delimeter = ' ';

    u32 result = 0;
    AssetFlags flags = 0;

    line.trim();

    if (line.size == 0)
    {
        return ASSET_LINE_IS_EMPTY;
    }

    if (string_starts_with(line, make_string("#")))
    {
        return ASSET_LINE_IS_COMMENT;
    }

    String image = make_string("image");
    String audio = make_string("audio");
    String shader = make_string("shader");
    String font = make_string("font");

    int cursor = 0;
    String kind = string_slice_to_character(line, 0, ' ');

    AssetKind asset_kind;

    if (string_compare(kind, image))
    {
        asset_kind = ASSET_KIND_IMAGE;
        cursor += image.size;
    }
    else if (string_compare(kind, audio))
    {
        asset_kind = ASSET_KIND_AUDIO;
        cursor += audio.size;
    }
    else if (string_compare(kind, shader))
    {
        asset_kind = ASSET_KIND_SHADER;
        cursor += shader.size;
    }
    else if (string_compare(kind, font))
    {
        asset_kind = ASSET_KIND_FONT;
        cursor += font.size;
    }
    else {
        return 0;
    }

    // space required after a field
    if (line.data[cursor] != ' ')
    {
        return 0;
    }

    bool is_folder = false;

    String name = {};
    String scope = next_word(line, cursor, delimeter);
    if (string_compare(scope, make_string("file")))
    {
        is_folder = false;
        name = next_word(line, cursor, delimeter);
    }
    else if (string_compare(scope, make_string("folder")))
    {
        is_folder = true;
        name = next_word(line, cursor, delimeter);
    }
    else
    {
        // scope is an optional argument and it's missing
        is_folder = false;

        name = scope;
        scope = {};
    }

    flags |= is_folder ? ASSET_IS_FOLDER : 0;

    String path = next_word(line, cursor, delimeter);

    if (name.size == 0 || path.size == 0)
    {
        return 0;
    }

    String next = next_word(line, cursor, delimeter);
    bool attribute = true;
    while (attribute)
    {
        switch (asset_kind)
        {
            case ASSET_KIND_IMAGE: {
                if (!parse_image_attribute(next, pointer.data.image))
                {
                    attribute = false;
                }
                break;
            }
            case ASSET_KIND_AUDIO: {
                if (!parse_audio_attribute(next, pointer.data.audio))
                {
                    attribute = false;
                }
                break;
            }
            case ASSET_KIND_FONT: {
                if (!parse_font_attribute(next, pointer.data.font))
                {
                    attribute = false;
                }
                break;
            }
            case ASSET_KIND_SHADER: {
                if (!parse_shader_attribute(next, pointer.data.shader))
                {
                    attribute = false;
                }
                break;
            }
            default:
                panic("Invalid asset kind");
                break;
        }

        if (attribute)
        {
            next = next_word(line, cursor, delimeter);
        }
    }

    while (next.size > 0)
    {
        if (string_compare(next, String("lazy")))
        {
            if (flags & ASSET_IS_LAZY) { result |= ASSET_LINE_REPEATS_TOKENS; }
            flags |= ASSET_IS_LAZY;
        }
        else if (string_compare(next, String("optional")))
        {
            if (flags & ASSET_IS_OPTIONAL) { result |= ASSET_LINE_REPEATS_TOKENS; }
            flags |= ASSET_IS_OPTIONAL;
        }
        else {
            break;
        }

        next = next_word(line, cursor, delimeter);
    }

    String trail = string_slice_to_character(line, cursor, '\n');
    trail.trim();
    if (trail.size != 0)
    {
        result |= ASSET_LINE_HAS_TRAILING_TOKENS;
    }

    pointer.kind = asset_kind;
    pointer.flags = flags;
    pointer.name = StringReference(name.data - file.data, name.size);
    pointer.path = StringReference(path.data - file.data, path.size);
    pointer.identifier = NullAssetId;

    result |= ASSET_LINE_IS_VALID;
    return result;
}

bool parse_image_attribute(String attribute, SDL_Texture*& texture)
{
    return false;
}

bool parse_audio_attribute(String attribute, MIX_Audio*& audio)
{
    return false;
}

bool parse_font_attribute(String attribute, Font& font)
{
    String out = {};

    if (parse_attribute_value(attribute, "size", out)) {
        bool success = false;
        float value = string_to_real(out, &success);
        if (!success) {
            return false;
        }

        font.size = value;

        return true;
    }

    return false;
}

bool parse_shader_attribute(String attribute, Shader& shader)
{
    String out = {};

    if (parse_attribute_value(attribute, "stage", out)) {
        if (string_compare(out, String("vertex"))) {
            shader.stage = ShaderStageVertex;
        }
        else if (string_compare(out, String("fragment"))) {
            shader.stage = ShaderStageFragment;
        }
        else {
            return false;
        }

        return true;
    }
    else if (parse_attribute_value(attribute, "nUniforms", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) {
            return false;
        }

        shader.numUniformBuffers = value;
        return true;
    }
    else if (parse_attribute_value(attribute, "nSamplers", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) {
            return false;
        }

        shader.numSamplers = value;
        return true;
    }
    else if (parse_attribute_value(attribute, "nTextures", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) return false;
        shader.numStorageTextures = value;
        return true;
    }
    else if (parse_attribute_value(attribute, "nStorageBuffers", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) return false;
        shader.numStorageBuffers = value;
        return true;
    }

    return false;
}

bool parse_attribute_value(String attribute, const char* key, String& out_value)
{
    String k = make_string(key);
    if (!string_starts_with(attribute, k))
        return false;
    if (!attribute.advance(k.size))
        return false;
    attribute.trim();
    if (attribute.size == 0 || attribute.data[0] != '=')
        return false;
    attribute.advance(1);
    attribute.trim();
    out_value = attribute;
    return true;
}

bool parse_asset_description(const char* description, AssetCatalog& catalog, bool check_unique)
{
    int cursor = 0;
    int line_number = 0;

    String desc = make_string(description);
    while (cursor < desc.size)
    {
        String line = string_slice_to_character(desc, cursor, '\n');
        int next_line_offset = cursor + line.size;
        next_line_offset += string_match_character(desc, next_line_offset, '\n');

        Asset asset = {};
        auto result = asset_parse_line(desc, line, asset);

        line_number += 1;

        if (result & ASSET_LINE_IS_VALID)
        {
            if (check_unique)
            {
                catalog.add_asset_unique(asset);
            }
            else
            {
                catalog.add_asset(asset);
            }

            if (result & ASSET_LINE_HAS_TRAILING_TOKENS)
            {
                log_warning("Asset description %s has trailing tokens on line %d\n", description, line_number);
            }
        }
        else
        {
            if ((result & ASSET_LINE_IS_COMMENT) || (result & ASSET_LINE_IS_EMPTY))
            {
                // do nothing
            }
            else
            {
                SCOPE_STRING(line, line_cstr);
                log_error("Could not parse line %d in asset description. Line: %s", line_number, line_cstr);
                return false;
            }
        }

        cursor = next_line_offset;
    }

    catalog.catalogEntryCount = catalog.assets.count();

    return true;
}

bool parse_assets(const char* path, AssetCatalog& catalog)
{
    catalog.reset();
    bool success = load_file_text(path, catalog.catalog);
    if (!success)
    {
        return false;
    }

    return parse_asset_description(catalog.catalog.c_string(), catalog, false);
}

void AssetCatalog::reset()
{
    catalog.free_buffer();
    path.free_buffer();

    for (auto& asset : assets)
    {
        unload_asset(asset.identifier, *this);
    }

    assets.reset();
    // load_context = {};
}

bool AssetCatalog::reload_asset(AssetId id)
{
    if (!id.is_valid())
    {
        return false;
    }

    auto asset_path = get_asset_path(id);
    log_info("%.*s", asset_path.size, asset_path.data);

    unload_asset(id, *this, false);

    get_to_asset_path_string(path, asset_path);
    return load_asset(id.id, *this);
}

bool AssetCatalog::reload_asset_at_index(int index)
{
    auto asset = assets.get(index);
    auto asset_path = catalog.get_string(asset.path);
    log_info("%.*s", asset_path.size, asset_path.data);

    get_to_asset_path_string(path, asset_path);

    unload_asset(asset.identifier, *this, false);

    return load_asset(asset.identifier.id, *this);
}

AssetId get_asset(String name, AssetCatalog& catalog)
{
    int index = 0;
    for (auto& asset : catalog.assets)
    {
        auto asset_name = catalog.catalog.get_string(asset.name);
        if (string_compare(asset_name, name))
        {
            if (!asset.identifier.is_valid())
            {
                load_asset(index, catalog);
            }

            return catalog.assets[index].identifier;
        }

        index += 1;
    }

    log_error("Couldn't find requested asset with name: %.*s", name.size, name.data);
    return NullAssetId;
}

AssetId get_asset_at_index(int index, AssetCatalog& catalog)
{
    if (!catalog.assets.in_bounds(index))
    {
        log_error("Couldn't load asset: Requested asset index is out of bounds %d", index);
        return NullAssetId;
    }

    if (!catalog.assets[index].identifier.is_valid())
    {
        load_asset(index, catalog);
    }

    return catalog.assets[index].identifier;
}

bool load_asset(int index, AssetCatalog& catalog)
{
    AssetLoadContext& load_context = catalog.load_context;
    auto asset_path = catalog.catalog.get_string(catalog.assets[index].path);
    get_to_asset_path_string(catalog.path, asset_path);

    if (catalog.assets[index].flags & ASSET_IS_FOLDER) {
        catalog.path.append(make_string(PathSeparator));

        if (!SDL_EnumerateDirectory(catalog.path.c_string(), load_asset_callback, &catalog)) {
            log_error("Couldn't load assets in folder: %.*s", asset_path.size, asset_path.data);
            return false;
        }

        catalog.assets[index].identifier.generation += 1;
        return true;
    }
    else {
        bool load = load_asset_file(catalog.path, catalog.assets[index], catalog.load_context);
        if (!load)
        {
            log_error("Couldn't load asset: %.*s", asset_path.size, asset_path.data);
            return false;
        }

        catalog.assets[index].identifier.generation += 1;
        return true;
    }
}

bool load_asset_file(String_Builder& path, Asset& asset, AssetLoadContext& load_context)
{
    switch (asset.kind)
    {
        case ASSET_KIND_IMAGE: {
            SDL_Texture* texture = IMG_LoadTexture(load_context.render->renderer, path.c_string());
            if (!texture)
            {
                asset.identifier.id = -1;
                return false;
            }

            asset.data.image = texture;

            return true;
        }
        case ASSET_KIND_AUDIO: {
            MIX_Audio* audio = MIX_LoadAudio(load_context.audio->mixer, path.c_string(), true);
            if (audio == nullptr)
            {
                asset.identifier.id = -1;
                return false;
            }

            asset.data.audio = audio;

            return true;
        }
        case ASSET_KIND_FONT: {
            bool success = load_font_file(&asset.data.font, path.c_string(), asset.data.font.size);
            if (!success)
            {
                asset.identifier.id = -1;
                return false;
            }

            return true;
        }
        case ASSET_KIND_SHADER: {
            bool success = loadShader(*load_context.render, asset.data.shader, path.c_string());
            if (!success)
            {
                asset.identifier.id = -1;
                return false;
            }

            return true;
        }
        default: {
            return false;
        }
    }
}

SDL_EnumerationResult load_asset_callback(void* userdata, const char* dirname, const char* fname)
{
    AssetCatalog* catalog = (AssetCatalog*)userdata;

    String name = catalog->catalog.put_string(String(fname));

    String ext = string_get_extension(name);
    String file = string_get_file_name(name);
    AssetKind kind = get_asset_kind(ext);
    if (kind == ASSET_KIND_SENTINEL) {
        // we don't recognize the extension of this file
        return SDL_ENUM_CONTINUE;
    }

    Asset asset = {};
    asset.kind = kind;
    asset.flags = ASSET_IS_FROM_FOLDER;
    asset.name = catalog->catalog.get_reference(file);
    asset.path = {};

    int amount = catalog->path.append_path(String(fname));

    // we could make this recursize but maybe not necessary
    if (!load_asset_file(catalog->path, asset, catalog->load_context)) {
        log_error("Couldn't load asset: %s/%s", dirname, fname);
        return SDL_ENUM_FAILURE;
    }

    catalog->path.remove(amount);

    int index = catalog->assets.add(asset);
    catalog->assets.get(index).identifier = { index, 1 };

    return SDL_ENUM_CONTINUE;
}

SDL_EnumerationResult unload_asset_callback(void* userdata, const char* dirname, const char* fname)
{
    AssetCatalog* catalog = (AssetCatalog*) userdata;

    String name = String(fname);
    String file = string_get_file_name(name);

    for (auto it = catalog->assets.begin(); it != catalog->assets.end(); ++it)
    {
        Asset& asset = *it;

        if (!(asset.flags & ASSET_IS_FROM_FOLDER))
        {
            continue;
        }

        if (!string_compare(catalog->catalog.get_string(asset.name), file))
        {
            continue;
        }

        unload_asset_file(asset, catalog->load_context, false);
        catalog->assets.remove(it.index());

        return SDL_ENUM_CONTINUE;
    }

    return SDL_ENUM_CONTINUE;
}

bool unload_asset(AssetId id, AssetCatalog& catalog, bool reset_generation)
{
    Asset& asset = catalog.assets.get(id.id);
    if (asset.flags & ASSET_IS_FOLDER)
    {
        auto asset_path = catalog.catalog.get_string(asset.path);
        get_to_asset_path_string(catalog.path, asset_path);
        catalog.path.append(make_string(PathSeparator));

        if (!SDL_EnumerateDirectory(catalog.path.c_string(), unload_asset_callback, &catalog)) {
            log_error("Couldn't unload assets in folder: %.*s", asset_path.size, asset_path.data);
            return false;
        }

        if (reset_generation)
        {
            asset.identifier.generation = 0;
        }
        return true;
    }
    else
    {
        unload_asset_file(catalog.assets.get(id.id), catalog.load_context, reset_generation);
    }

    return true;
}

void unload_asset_file(Asset& asset, AssetLoadContext& load_context, bool reset_generation)
{
    // we need to be careful with what information we lose.
    // we need to clear out pointers but not lose attribute data.
    switch (asset.kind)
    {
        case ASSET_KIND_IMAGE: {
            SDL_DestroyTexture(asset.data.image);
            asset.data.image = nullptr;
            break;
        }
        case ASSET_KIND_AUDIO: {
            MIX_DestroyAudio(asset.data.audio);
            asset.data.audio = nullptr;
            break;
        }
        case ASSET_KIND_FONT: {
            TTF_CloseFont(asset.data.font.font);
            asset.data.font.font = nullptr;
            break;
        }
        case ASSET_KIND_SHADER: {
            unloadShader(*load_context.render, asset.data.shader);
            asset.data.shader.shader = nullptr;
            break;
        }
        case ASSET_KIND_ZERO: // ???
        default:
        {
            panic("Invalid asset kind");
        }
    }

    if (reset_generation)
    {
        asset.identifier.generation = 0;
    }
}

AssetKind get_asset_kind(String extension) {
    if (string_compare(extension, String("svg"))) {
        return ASSET_KIND_IMAGE;
    }
    else if (string_compare(extension, String("ogg"))) {
        return ASSET_KIND_AUDIO;
    }
    else if (string_compare(extension, String("mp3"))) {
        return ASSET_KIND_AUDIO;
    }
    else if (string_compare(extension, String("wav"))) {
        return ASSET_KIND_AUDIO;
    }
    else if (string_compare(extension, String("ttf"))) {
        return ASSET_KIND_FONT;
    }
    else if (string_compare(extension, String("shader"))) {
        return ASSET_KIND_SHADER;
    }

    return ASSET_KIND_SENTINEL;
}

void get_base_path(String_Builder& builder)
{
    const char* base_path = SDL_GetBasePath();
    builder.clear_and_append(make_string(base_path));
}

void get_pref_path(String_Builder& builder, const char *org, const char *app)
{
    char* pref_path = SDL_GetPrefPath(org, app);
    builder.clear_and_append(make_string(pref_path));
    SDL_free(pref_path);
}

void get_to_run_tree_path(String_Builder& builder, const char* path)
{
    get_base_path(builder);
    builder.append_path(String(path));
}

void get_to_run_tree_path_string(String_Builder& builder, String path)
{
    get_base_path(builder);
    builder.append_path(String(path));
}

void get_to_asset_path(String_Builder& builder, const char* path)
{
    get_base_path(builder);
    builder.append_path(String("asset/"));
    builder.append_path(String(path));
}

void get_to_asset_path_string(String_Builder& builder, String path)
{
    get_base_path(builder);
    builder.append_path(String("asset/"));
    builder.append_path(String(path));
}

const char* get_asset_kind_name(AssetKind kind)
{
    switch (kind)
    {
        case ASSET_KIND_ZERO:       return "AssetKindZero";
        case ASSET_KIND_IMAGE:      return "AssetKindImage";
        case ASSET_KIND_AUDIO:      return "AssetKindAudio";
        case ASSET_KIND_FONT:       return "AssetKindFont";
        case ASSET_KIND_SHADER:     return "AssetKindShader";
        case ASSET_KIND_SENTINEL:   return "AssetKindSentinel";
        case ASSET_KIND_COUNT:  // fallthrough
        default:
            panic("Invalid asset kind");
    }
}

} // namespace
