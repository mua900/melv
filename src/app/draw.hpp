#ifndef DRAW_HPP
#define DRAW_HPP

#include <SDL3/SDL.h>
#include "text.hpp"
#include "camera.hpp"

#include "util/common.hpp"
#include "util/math_util.hpp"
#include "util/template.hpp"

namespace melv
{

#define GRAPHICS_DEBUG 0

static const auto DEBUG_COLOR = melv::ColorF(0.6, 0.5, 0.4, 1.0);

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

// @todo maybe a function to validate this
struct GraphicsPipelineParameters
{
    // @todo
    // vertex description
    // and anything else as we need them
    SDL_GPUTextureFormat format = {};
};

struct GraphicsPipeline
{
    // remember what parameters we created this with so that we can recreate it
    GraphicsPipelineParameters parameters = {};
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
};

struct MeshData
{
    DArray<Vertex> vertices;
    DArray<u16> indices;
};

struct MeshReference
{
    int vertex_count = 0;
    int index_count = 0;
    int vertex_offset = 0;
    int index_offset = 0;

    int vertex_buffer = 0;
    int index_buffer = 0;
};

struct MeshDataSize
{
    int vertex_count = 0;
    int index_count = 0;

    MeshDataSize() {}
    MeshDataSize(int vc, int ic) : vertex_count(vc), index_count(ic) {}
};

struct TransferData {
    DArray<MeshDataSize> meshes = {};

    TransferData() {}
    TransferData(DArray<MeshDataSize> meshInfo) : meshes(meshInfo) {}
};

struct GPUTexture {
    SDL_GPUTexture* texture = nullptr;
    u32 width = 0;
    u32 height = 0;
};


enum GPUBufferUsage {
    GPUBufferVertex = SDL_GPU_BUFFERUSAGE_VERTEX,
    GPUBufferIndex = SDL_GPU_BUFFERUSAGE_INDEX,
};

struct GPUBuffer {
    SDL_GPUBuffer* buffer = nullptr;
    GPUBufferUsage usage = {};
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
// typically you would set this in the top draw loop to switch between things that you want to draw in the coordinate space you want
// and also set it and back in place for exceptions
enum CoordinateSpace
{
	// top left of the screen is 0, 0. y is down, x is right
    Screen,

	// 0, 0 is at the center of the screen if camera is at 0, 0.
	// y is up, x is right
    World,
};

struct RenderContext {
    melv::vec2 render_size = {};
    // @todo remove
    SDL_Renderer* renderer = nullptr;

    CoordinateSpace space = {};  // what coordinate space input vertices are in
	const Camera* camera = {};

    ColorF clear_color = {};

    // @todo switch to sdl gpu
    // all below belongs to incomplete code
    SDL_GPUDevice* device = nullptr;

    melv::mat4x4 mvp = {};

    DArray<GPUBuffer> buffers = {};
    int active_vertex_buffer = 0;
    int active_index_buffer = 0;

    SDL_GPUSampler* sampler = nullptr;

    // @todo
    // GraphicsPipeline user_pipeline = {};
    GraphicsPipeline graphics = {};

    SDL_GPUTexture* render_target = nullptr;

    TransferBuffer transfer_buffer = {};

    FrameContext frame = {};

    DArray<SDL_Vertex> vertex_scratch = {};
    DArray<int> index_scratch = {};

    bool gpu_inited() const
    {
        return device && graphics.pipeline && render_target;
    }

    melv::vec2 get_center() const { return render_size / 2; }

    bool start_render_pass();
    void end_render_pass();

    bool start_copy_pass();
    void end_copy_pass();

    int allocate_gpu_buffer(GPUBufferUsage usage, int size);
    bool set_vertex_buffer(int buffer);
    bool set_index_buffer(int buffer);

    bool get_command_buffer();
    void submit_command_buffer();
    void cancel_command_buffer(); // @todo

    void set_viewport(Viewport viewport);

    bool set_shaders(SDL_GPUShader* vertex, SDL_GPUShader* fragment);

    void copy_to_swapchain();

    // camera transforms on the cpu
    melv::vec2 transformWorld(melv::vec2 p) const;
    melv::vec2 transformScreen(melv::vec2 p) const;
    melv::Rectangle transform_rectangle(melv::Rectangle r) const;
    SDL_FPoint transform_sdl_point(SDL_FPoint p) const;
    SDL_FRect transform_sdl_rectangle(SDL_FRect r) const;
    SDL_Vertex transform_sdl_vertex(SDL_Vertex v) const;
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

bool get_default_graphics_pipeline_parameters(GraphicsPipelineParameters* parameters, SDL_GPUDevice* device, SDL_Window* window);
SDL_GPUGraphicsPipeline *create_gpu_graphics_pipeline(GraphicsPipelineParameters* parameters, RenderContext* render, SDL_GPUShader* vertex, SDL_GPUShader* fragment);

bool initialize_render_context(RenderContext* render, SDL_Window* window, bool enableGpuDebug);
bool init_gpu_renderer(RenderContext* render, SDL_Window* window, SDL_GPUShader* vertex, SDL_GPUShader* fragment);

bool start_frame(RenderContext& context, SDL_Window* window);
void end_frame(RenderContext& context);

GPUBuffer allocate_gpu_buffer(RenderContext& context);

TransferData add_to_transfer_buffer(RenderContext& context, DArray<MeshData>& data);
DArray<MeshReference> upload_mesh_data(RenderContext& context, TransferData& data);

void draw_mesh(RenderContext& render, MeshReference mesh);

bool loadShader(RenderContext& context, Shader& shader, const char* path);
bool unloadShader(RenderContext& context, Shader& shader);

// immediate mode drawing
void draw_triangle(const RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, ColorF color);
void draw_triangle_texture(const RenderContext& context, SDL_Texture* texture, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, ColorF color);
void draw_rectangle(const RenderContext& context, melv::Rectangle area, melv::ColorF color);
void draw_rectangle_rotated(const RenderContext& context, melv::Rectangle area, float rotation, melv::ColorF color);
void draw_segment(const RenderContext& context, melv::vec2 start, melv::vec2 end, float thick, melv::ColorF color);
void draw_arrow(const RenderContext& context, melv::vec2 start, melv::vec2 end, float thick, float head_ratio, melv::ColorF color);
void draw_circle(const RenderContext& context, melv::vec2 position, float radius, melv::ColorF color);
void draw_circle_empty(const RenderContext& context, melv::vec2 position, float radius, float thick, melv::ColorF color);
void draw_circle_with_texture(const RenderContext& context, melv::vec2 position, float radius, SDL_Texture* texture, melv::ColorF color);
void draw_circle_segment(const RenderContext& context, melv::vec2 position, float radius, float start_angle, float angle, melv::ColorF color);
void draw_circle_segment_with_texture(const RenderContext& context, melv::vec2 position, float radius, float start_angle, float angle, SDL_Texture* texture, melv::ColorF color);
void draw_arc(const RenderContext& context, melv::vec2 center, float inner_radius, float outer_radius, float start_angle, float arc, melv::ColorF color);
void draw_capsule(const RenderContext& context, melv::vec2 center0, melv::vec2 center1, float radius, melv::ColorF color);
void draw_polygon(RenderContext& context, melv::vec2 points[], int numPoints, melv::ColorF color);
void draw_path(RenderContext& context, melv::vec2 points[], int numPoints, float thick, melv::ColorF color);
void draw_closed_path(RenderContext& context, melv::vec2 points[], int numPoints, float thick, melv::ColorF color);
void draw_quadratic_bezier(const RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, float thick, melv::ColorF color);
void draw_cubic_bezier(const RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, melv::vec2 p3, float thick, melv::ColorF color);
void draw_quad(const RenderContext& context, melv::RectPoints quad, melv::ColorF color);
void draw_quad_with_texture(const RenderContext& context, melv::RectPoints quad, SDL_Texture* texture, melv::ColorF color);

void draw_texture(const RenderContext& context, melv::Rectangle area, SDL_Texture* texture);

SDL_Texture* render_text(SDL_Renderer* renderer, String text, Font font, melv::Color color);
Text create_text(SDL_Renderer* renderer, String text, Font font, melv::Color color);

void render_texture(const RenderContext& render, melv::Rectangle area, SDL_Texture* texture, bool strech = false);
void render_texture_rotate(const RenderContext& render, melv::Rectangle area, SDL_Texture* texture, float angle, Flip flip, bool strech = false);
void render_textured_rectangle(const RenderContext& render, melv::Rectangle rect, SDL_Texture* texture, melv::Color color, bool strech = false, bool center = true);
void render_texture_with_tint(const RenderContext& render, melv::Rectangle area, SDL_Texture* texture, melv::ColorF tint, bool strech = false);

void render_text_size(SDL_Renderer* renderer, Text text, melv::vec2 where, melv::vec2 absolute_scale = melv::vec2(0, 0));
void render_text_scale(SDL_Renderer* renderer, Text text, melv::vec2 where, melv::vec2 scale_factor = melv::vec2(0,0));

} // namespace

#endif // DRAW_HPP
