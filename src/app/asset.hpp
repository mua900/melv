#ifndef ASSET_HPP
#define ASSET_HPP

#include "text.hpp"
#include "audio.hpp"
#include "draw.hpp"
#include "util/common.hpp"
#include "util/template.hpp"
#include "util/log.hpp"

namespace melv
{

enum AssetKind {
    ASSET_KIND_ZERO   = 0,
    ASSET_KIND_IMAGE  = 1,
    ASSET_KIND_AUDIO  = 2,
    ASSET_KIND_FONT   = 3,
    ASSET_KIND_SHADER = 4,
    // ASSET_KIND_TEXTURE_ATLAS = 5,
    // ASSET_KIND_ANIMATION = 6,
    ASSET_KIND_COUNT,
    ASSET_KIND_SENTINEL,
};

const char* get_asset_kind_name(AssetKind kind);

struct AssetId {
    int id;
    int generation;

    bool is_valid() const { return id != -1 && generation != 0; }
};

using ImageId = AssetId;
using AudioId = AssetId;
using FotnId = AssetId;
using ShaderId = AssetId;

static constexpr AssetId NullAssetId = AssetId {-1, 0};

struct AssetLoadContext {
    RenderContext* render;
    AudioPlayer* audio;
};

using AssetFlags = u8;

// by default 0
#define ASSET_IS_FOLDER      BIT(0)
#define ASSET_IS_OPTIONAL    BIT(1)
#define ASSET_IS_LAZY        BIT(2)
#define ASSET_IS_FROM_FOLDER BIT(3)

struct Asset {
    AssetKind kind;
    StringReference name = {};
    StringReference path = {};
    AssetFlags flags = 0;

    AssetId identifier = {};
    union {
        Font font;
        SDL_Texture* image;
        MIX_Audio* audio;
        Shader shader;
    } data = {};

    Asset() : kind(ASSET_KIND_ZERO), identifier(NullAssetId), data{} {}
    Asset(AssetKind kind) : kind(kind), identifier(NullAssetId), data{}
    {}
};

struct AssetCatalog {
    // This holds the asset description we loaded.
    // Asset names and paths reference this buffer so don't mess with it unless you know what you are doing.
    String_Builder catalog;

    // the number of original entries we read from the description file
    // excluding the ones dynamically added from enumarating folders
    int catalogEntryCount = 0;

    AssetLoadContext load_context;
    BucketList<Asset> assets;

    // used as scracth space to build paths
    String_Builder path;

    void add_asset(Asset& asset)
    {
        int index = assets.add(asset);
        assets.get(index).identifier.id = index;
        assets.get(index).identifier.generation = 0;
    }

    bool add_asset_unique(Asset& asset)
    {
        for (auto x : assets)
        {
            if (x.kind == asset.kind &&
                string_compare(catalog.get_string(x.path), catalog.get_string(asset.path)))
            {
                log_info("Asset already exists");
                return false;
            }
        }

        log_info("Adding new asset");
        add_asset(asset);
        return true;
    }

    String get_asset_name_at_index(int index) const {
        return catalog.get_string(assets.get(index).name);
    }

    String get_asset_path_at_index(int index) const {
        return catalog.get_string(assets.get(index).path);
    }

    String get_asset_name(AssetId id) const {
        if (!id.is_valid()) return String();

        const Asset& asset = assets.get(id.id);
        return catalog.get_string(asset.name);
    }

    String get_asset_path(AssetId id) const {
        if (!id.is_valid()) return String();

        const Asset& asset = assets.get(id.id);
        return catalog.get_string(asset.path);
    }

    bool compare_asset_kind(AssetKind expected, AssetKind got) const
    {
        if (expected != got)
        {
            log_error("Asset type mismatch: Expected %s, got %s", get_asset_kind_name(expected), get_asset_kind_name(got));
            return false;
        }

        return true;
    }

    bool compare_asset_generation(int expected, int got) const
    {
        if (expected != got)
        {
            log_error("Stale asset handle: Expected %d, got %d", expected, got);
            return false;
        }

        return true;
    }

    SDL_Texture* get_image(AssetId id) const
    {
        if (!id.is_valid())
        {
            return nullptr;
        }

        const Asset& asset = assets.get(id.id);
        if (!compare_asset_kind(ASSET_KIND_IMAGE, asset.kind))
        {
            return nullptr;
        }

        if (!compare_asset_generation(asset.identifier.generation, id.generation))
        {
            return nullptr;
        }

        return asset.data.image;
    }

    Font get_font(AssetId id) const
    {
        if (!id.is_valid())
        {
            return Font();
        }

        const Asset& asset = assets.get(id.id);
        if (!compare_asset_kind(ASSET_KIND_FONT, asset.kind))
        {
            return Font();
        }

        if (!compare_asset_generation(asset.identifier.generation, id.generation))
        {
            return Font();
        }

        return asset.data.font;
    }

    MIX_Audio* get_audio(AssetId id) const
    {
        if (!id.is_valid())
        {
            return nullptr;
        }

        const Asset& asset = assets.get(id.id);
        if (!compare_asset_kind(ASSET_KIND_AUDIO, asset.kind))
        {
            return nullptr;
        }

        if (!compare_asset_generation(asset.identifier.generation, id.generation))
        {
            return nullptr;
        }

        return asset.data.audio;
    }

    SDL_GPUShader* get_shader(AssetId id) const
    {
        if (!id.is_valid())
        {
            return nullptr;
        }

        const Asset& asset = assets.get(id.id);
        if (!compare_asset_kind(ASSET_KIND_SHADER, asset.kind))
        {
            return nullptr;
        }

        if (!compare_asset_generation(asset.identifier.generation, id.generation))
        {
            return nullptr;
        }

        return asset.data.shader.shader;
    }

    bool reload_asset(AssetId id);
    bool reload_asset_at_index(int index);

    void reset();
};

// parse asset catalog file and add assets listed in it to the catalog
bool parse_asset_description(const char* description, AssetCatalog& catalog, bool check_unique);
// load the asset description pointed on path and parse it
bool parse_assets(const char* path, AssetCatalog& catalog);

// returns the existing handle if the asset is already loaded otherwise loads the asset on the fly and returns the handle
// returns null id if no asset with the given name is found or the asset load fails
AssetId get_asset(String name, AssetCatalog& catalog);
// useful when iterating through the assets and you already know the index
AssetId get_asset_at_index(int index, AssetCatalog& catalog);

// @todo maybe add an intermediate step that loads the file to memory but doesn't process it yet like load svg text or font but don't yet rasterize it.
// if memory usage becomes a problem

AssetKind get_asset_kind(String file_extension);

void get_base_path(String_Builder& builder);

void get_to_run_tree_path(String_Builder& builder, const char* path);
void get_to_run_tree_path_string(String_Builder& builder, String path);

void get_to_asset_path(String_Builder& builder, const char* path);
void get_to_asset_path_string(String_Builder& builder, String path);

} // namespace

#endif // ASSET_HPP
