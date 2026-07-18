#ifndef DRAW_HPP
#define DRAW_HPP

#include <SDL3/SDL.h>
#include "text.hpp"
#include "camera.hpp"

#include "util/common.hpp"
#include "util/math_util.hpp"
#include "util/template.hpp"

// for users: there's some noise down here
// you only need to worry about draw_* or render_* functions
// the rest is about how it's implemented
// you can also use your own fragment shaders

#define GRAPHICS_DEBUG 0

static const auto DEBUG_COLOR = cobot::ColorF(0.6, 0.5, 0.4, 1.0);

using Texture = SDL_Texture;
using Viewport = SDL_GPUViewport;

struct Vertex {
    float x = 0;
    float y = 0;
    float uvx = 0;
    float uvy = 0;
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;

    Vertex() {}
    Vertex(float x, float y, float uvx, float uvy, float r, float g, float b, float a)
        :
        x(x), y(y), uvx(uvx), uvy(uvy), r(r), g(g), b(b), a(a)
    {}
};

struct GPUTexture {
    SDL_GPUTexture* texture = nullptr;
    u32 width = 0;
    u32 height = 0;
};

enum MeshType {
	Quad,
	Count,
};

struct MeshData {
    DArray<Vertex> vertices = {};
    DArray<u16> indices = {};
};

struct MeshReference {
    u32 numVertices = 0;
    u32 numIndices = 0;
    u32 vertex_offset = 0;
    u32 index_offset = 0;
};

struct Mesh {
    MeshData data = {};
    MeshReference ref = {};
};

struct GPUBuffer {
    SDL_GPUBuffer* buffer = nullptr;
    u32 size = 0;
    u32 used = 0;
};

struct TransferBuffer {
    SDL_GPUTransferBuffer* buffer = nullptr;
    u32 size = 0;
};

struct FrameContext {
    SDL_GPUCommandBuffer* command_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
    SDL_GPUCopyPass* copy_pass = nullptr;
    GPUTexture swapchain = {};
};

// you should set this to the correct value before drawing something
enum CoordinateSpace
{
	// top left of the screen is 0, 0. y is down, x is right
    Screen,

	// 0, 0 is at the center of the screen if camera is at 0, 0.
	// y is up, x is right
    World,
};

struct RenderContext {
    cobot::vec2 render_size = {};
    SDL_Renderer* renderer = nullptr;
    DArray<SDL_GPURenderState*> render_states = {};

    CoordinateSpace space = {};  // what coordinate space input vertices are in
    cobot::vec2 zoomTarget = {};
	const Camera* camera = {};
	
    // @todo switch to sdl gpu
    // all below belongs to incomplete code
    SDL_Texture* target_texture = nullptr;
    SDL_GPUTexture* render_target = nullptr;

    SDL_GPUDevice* device = nullptr;

    cobot::mat4x4 mvp = {};

    GPUBuffer vertex_buffer = {};
    GPUBuffer index_buffer = {};
    SDL_GPUSampler* sampler = nullptr;
    SDL_GPUGraphicsPipeline* graphics = nullptr;

    TransferBuffer transfer_buffer = {};

    FrameContext frame = {};

    DArray<SDL_Vertex> vertex_scratch = {};
    DArray<int> index_scratch = {};

    cobot::vec2 get_center() const { return render_size / 2; }

    // camera transforms
    cobot::vec2 transformWorld(cobot::vec2 p) const;
    cobot::vec2 transformScreen(cobot::vec2 p) const;
    cobot::Rectangle transform_rectangle(cobot::Rectangle r) const;
    SDL_FPoint transform_sdl_point(SDL_FPoint p) const;
    SDL_FRect transform_sdl_rectangle(SDL_FRect r) const;
    SDL_Vertex transform_sdl_vertex(SDL_Vertex v) const;

    // you do a copy pass to update positions etc. first and then draw those every frame
    bool start_render_pass();
    void end_render_pass();
    bool start_copy_pass();
    void end_copy_pass();

    void set_viewport(Viewport viewport);

    bool add_mesh(MeshData& meshData, MeshReference& mesh);
    bool update_mesh(MeshData& meshData, MeshReference& mesh);

    void draw_mesh(MeshReference mesh);
};

enum ShaderStage {
    ShaderStageVertex = SDL_GPU_SHADERSTAGE_VERTEX,
    ShaderStageFragment = SDL_GPU_SHADERSTAGE_FRAGMENT,
};

struct Shader {
    SDL_GPUShader* shader = nullptr;
    ShaderStage stage = {};
    int numSamplers = 0;
    int numStorageTextures = 0;
    int numStorageBuffers = 0;
    int numUniformBuffers = 0;

    bool is_valid() const {
        return shader != nullptr;
    }
};

enum Flip {
    FlipNone = SDL_FLIP_NONE,
    FlipHorizontal = SDL_FLIP_HORIZONTAL,
    FlipVertical = SDL_FLIP_VERTICAL,
    FlipHorizontalAndVertical = SDL_FLIP_HORIZONTAL_AND_VERTICAL,
};

bool initialize_render_context(RenderContext* render, SDL_Window* window);
bool init_gpu_renderer(RenderContext* render, SDL_Window* window, SDL_GPUShader* vertex, SDL_GPUShader* fragment);

void start_frame(RenderContext& context, SDL_Window* window);
void end_frame(RenderContext& context);

bool loadShader(RenderContext& context, Shader& shader, const char* path);
bool unloadShader(RenderContext& context, Shader& shader);

void draw_rectangle(const RenderContext& context, cobot::Rectangle area, cobot::ColorF color);
void draw_segment(const RenderContext& context, cobot::vec2 start, cobot::vec2 end, float thick, cobot::ColorF color);
void draw_arrow(const RenderContext& context, cobot::vec2 start, cobot::vec2 end, float thick, float head_ratio, cobot::ColorF color);
void draw_circle(const RenderContext& context, cobot::vec2 position, float radius, cobot::ColorF color);
void draw_circle_empty(const RenderContext& context, cobot::vec2 position, float radius, float thick, cobot::ColorF color);
void draw_circle_with_texture(const RenderContext& context, cobot::vec2 position, float radius, SDL_Texture* texture, cobot::ColorF color);
void draw_circle_segment(const RenderContext& context, cobot::vec2 position, float radius, float start_angle, float angle, cobot::ColorF color);
void draw_circle_segment_with_texture(const RenderContext& context, cobot::vec2 position, float radius, float start_angle, float angle, SDL_Texture* texture, cobot::ColorF color);
void draw_arc(const RenderContext& context, cobot::vec2 center, float inner_radius, float outer_radius, float start_angle, float arc, cobot::ColorF color);
void draw_capsule(const RenderContext& context, cobot::vec2 center0, cobot::vec2 center1, float radius, cobot::ColorF color);
void draw_polygon(RenderContext& context, cobot::vec2 points[], int numPoints, cobot::ColorF color);
void draw_path(RenderContext& context, cobot::vec2 points[], int numPoints, float thick, cobot::ColorF color);
void draw_closed_path(RenderContext& context, cobot::vec2 points[], int numPoints, float thick, cobot::ColorF color);
void draw_quadratic_bezier(const RenderContext& context, cobot::vec2 p0, cobot::vec2 p1, cobot::vec2 p2, float thick, cobot::ColorF color);
void draw_cubic_bezier(const RenderContext& context, cobot::vec2 p0, cobot::vec2 p1, cobot::vec2 p2, cobot::vec2 p3, float thick, cobot::ColorF color);
void draw_quad(const RenderContext& context, cobot::Quad quad, cobot::ColorF color);
void draw_quad_with_texture(const RenderContext& context, cobot::Quad quad, SDL_Texture* texture, cobot::ColorF color);

void draw_texture(const RenderContext& context, cobot::Rectangle area, SDL_Texture* texture);

SDL_Texture* render_text(SDL_Renderer* renderer, String text, Font font, cobot::Color color);
Text create_text(SDL_Renderer* renderer, String text, Font font, cobot::Color color);

void render_texture(const RenderContext& render, cobot::Rectangle area, Texture* texture, bool strech = false);
void render_texture_rotate(const RenderContext& render, cobot::Rectangle area, Texture* texture, float angle, Flip flip, bool strech = false);
void render_textured_rectangle(const RenderContext& render, cobot::Rectangle rect, Texture* texture, cobot::Color color, bool strech = false, bool center = true);
void render_texture_with_tint(const RenderContext& render, cobot::Rectangle area, Texture* texture, cobot::ColorF tint, bool strech = false);


void render_text_size(SDL_Renderer* renderer, Text text, cobot::vec2 where, cobot::vec2 absolute_scale = cobot::vec2(0, 0));
void render_text_scale(SDL_Renderer* renderer, Text text, cobot::vec2 where, cobot::vec2 scale_factor = cobot::vec2(0,0));

#endif // DRAW_HPP
