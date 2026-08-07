#ifndef DRAW_HPP
#define DRAW_HPP

#include "draw_types.hpp"
#include "text.hpp"
#include "camera.hpp"

#include "util/common.hpp"
#include "util/math_util.hpp"
#include "util/template.hpp"

namespace melv
{

#define GRAPHICS_DEBUG 1

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
    Vertex(vec2 pos, vec2 uv, ColorF col)
        :
        x(pos.x), y(pos.y), uvx(uv.x), uvy(uv.y), r(col.r), g(col.g), b(col.b), a(col.a)
    {}

    vec2 position() const {
        return vec2(x, y);
    }

    vec2 uv() const {
        return vec2(uvx, uvy);
    }

    ColorF color() const {
        return ColorF(r, g, b, a);
    }
};

struct VertexInstance
{
    float x = 0;
    float y = 0;
    float uvx = 0;
    float uvy = 0;

    VertexInstance() {}
    VertexInstance(float px, float py, float u, float v)
        :
        x(px), y(py), uvx(u), uvy(v)
    {}
};

struct InstanceData
{
    float x = 0;
    float y = 0;
    float rotation = 0;
    float scalex = 0;
    float scaley = 0;
    u32 color = 0;
};

struct InstanceDraw
{
    InstanceData data = {};
    TextureHandle texture = {};

    InstanceDraw() {}
    InstanceDraw(InstanceData d, TextureHandle t)
        :
        data(d),
        texture(t)
    {}
};

enum VertexInputType
{
    InputVertex,
    InputInstance,
};

struct GraphicsPipelineParameters
{
    VertexInputType input = {};
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

struct MeshDraw
{
    MeshReference mesh = {};
    TextureHandle texture = {};
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

struct GPUTexture
{
    SDL_GPUTexture* texture = nullptr;
    u32 width = 0;
    u32 height = 0;

    TextureFormat format = {};
    u32 sampler = 0;

    GPUTexture() {}
    GPUTexture(SDL_GPUTexture *tex, TextureFormat fm, u32 w, u32 h)
        :
        texture(tex), format(fm), width(w), height(h)
    {}
};

struct TextureUpload
{
    TextureHandle target = 0;
    SDL_Surface *src = nullptr;
    bool done = false;
};

struct DrawGroup
{
    TextureHandle texture = {};
    int offset = 0;
    int capacity = 0;
    int used = 0;  // reset every frame
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
    u32 used = 0;
};

struct FrameContext {
    SDL_GPUCommandBuffer* command_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
    SDL_GPUCopyPass* copy_pass = nullptr;
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

struct DefaultShaders
{
    SDL_GPUShader* vertex = {};
    SDL_GPUShader* vertex_instance = {};
    SDL_GPUShader* fragment = {};
    SDL_GPUShader* fragmentTexture = {};
};

enum MeshCommon
{
    MeshQuad,
    MeshCircle,

    MeshCount
};

struct RenderContext {
    melv::vec2 render_size = {};

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

    GraphicsPipeline graphics = {};
    GraphicsPipeline graphics_instance = {};
    GraphicsPipeline graphics_texture = {};
    GraphicsPipeline graphics_instance_texture = {};

    SDL_GPUTexture* render_target = nullptr;

    TransferBuffer transfer_buffer = {};
    TransferBuffer group_transfer_buffer = {};

    FrameContext frame = {};

    // for predefined meshes
    MeshReference mesh_common[MeshCount] = {};
    GPUBuffer instance_buffer = {};
    GPUBuffer group_instance_buffer = {};
    GPUBuffer vertex_buffer = {};
    GPUBuffer index_buffer = {};

    // @todo rename
    DArray<MeshReference> frameMeshDraw = {};
    DArray<MeshDraw> frameMeshDrawTex = {};

    DArray<InstanceDraw> frameInstanceDraw = {};
    DArray<InstanceData> groupDraw = {};
    DArray<DrawGroup> drawGroups = {};

    DArray<GPUTexture> textures = {};

    bool gpu_inited() const
    {
        return device && graphics.pipeline && render_target;
    }

    bool make_texture_upload(SDL_Surface* surface, TextureUpload* upload);
    TextureHandle create_texture(TextureFormat format, u32 width, u32 height);
    TextureHandle create_texture_verbose(TextureFormat format, TextureUsage usage, u32 width, u32 height, int mip_levels, SampleCount sampleCount);
    void destroy_texture(TextureHandle handle);
    TextureHandle load_gpu_texture(const char* path);
    bool is_texture_handle_valid(TextureHandle handle);

    GPUTexture get_texture(TextureHandle handle);

    melv::vec2 get_center() const { return render_size / 2; }

    bool start_render_pass();
    void end_render_pass();

    bool start_copy_pass();
    void end_copy_pass();

    u32 allocate_gpu_buffer(GPUBufferUsage usage, u32 size);
    bool set_vertex_buffer(u32 buffer);
    bool set_index_buffer(u32 buffer);

    DrawGroupId make_draw_group(TextureHandle texture, int size);
    void add_to_draw_group(DrawGroupId group, InstanceData data);

    bool upload_common_mesh_data();

    bool get_command_buffer();
    void submit_command_buffer();
    SDL_GPUFence* submit_command_buffer_and_get_fence();
    void cancel_command_buffer();

    void wait_on_fence(SDL_GPUFence* fence);
    void release_fence(SDL_GPUFence* fence);

    void set_viewport(Viewport viewport);

    bool set_shaders(GraphicsPipeline* gp, SDL_GPUShader* vertex, SDL_GPUShader* fragment);

    void copy_to_swapchain(SDL_GPUTexture* swapchain, u32 swapchain_width, u32 swapchain_height);

    // camera transforms on the cpu
    melv::vec2 transformWorld(melv::vec2 p) const;
    melv::vec2 transformScreen(melv::vec2 p) const;
    melv::Rectangle transform_rectangle(melv::Rectangle r) const;
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

bool create_default_shaders(SDL_GPUDevice* device, DefaultShaders* info);

bool get_default_graphics_pipeline_parameters(GraphicsPipelineParameters* parameters, SDL_GPUDevice* device, SDL_Window* window);
SDL_GPUGraphicsPipeline *create_gpu_graphics_pipeline(GraphicsPipelineParameters* parameters, RenderContext* render, SDL_GPUShader* vertex, SDL_GPUShader* fragment);

bool initialize_render_context(RenderContext* render, SDL_Window* window, bool enableGpuDebug);
bool init_gpu_renderer(RenderContext* render, SDL_Window* window);

bool start_frame(RenderContext& context, SDL_Window* window);
void end_frame(RenderContext& context, SDL_Window* window);

GPUBuffer allocate_gpu_buffer(RenderContext& context);

TransferData add_to_transfer_buffer(RenderContext& context, DArray<MeshData>& data);

bool copy_frame_instance_data(RenderContext& render);

DArray<MeshReference> upload_mesh_data(RenderContext& context, TransferData& data);
DArray<MeshReference> upload_mesh_data_buffers(RenderContext& context, TransferData& data, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer);

void upload_frame_instance_data(RenderContext& render);

// draw calls
// do not call these from user code
void draw_mesh(RenderContext& render, MeshReference mesh);
void draw_mesh_buffers(RenderContext& render, MeshReference mesh, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer);
void draw_mesh_texture(RenderContext& render, MeshDraw draw);
void draw_mesh_texture_buffers(RenderContext& render, MeshDraw draw, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer);

void draw_quads(RenderContext& render);
void draw_quads_texture(RenderContext& render, DrawGroup draw);

// user functions
void queue_draw_quad(RenderContext& render, InstanceData instance);
void queue_draw_mesh(RenderContext& render, MeshReference mesh);
void queue_draw_mesh_texture(RenderContext &render, MeshReference mesh, TextureHandle texture);
// returns false if there is no space left in the group buffer
bool queue_draw_group(RenderContext& render, InstanceData data, DrawGroupId groupId);

bool loadShader(RenderContext& context, Shader& shader, const char* path);
bool unloadShader(RenderContext& context, Shader& shader);

void destroy_texture(GPUTexture *texture);

TextureHandle render_text(RenderContext& render, String text, Font font, melv::Color color);
Text create_text(RenderContext& render, String text, Font font, melv::Color color);

void render_texture(const RenderContext& render, melv::Rectangle area, GPUTexture texture, bool strech = false);
void render_texture_rotate(const RenderContext& render, melv::Rectangle area, GPUTexture texture, float angle, Flip flip, bool strech = false);
void render_textured_rectangle(const RenderContext& render, melv::Rectangle rect, GPUTexture texture, melv::Color color, bool strech = false, bool center = true);
void render_texture_with_tint(const RenderContext& render, melv::Rectangle area, GPUTexture texture, melv::ColorF tint, bool strech = false);

// failure if the size of the arrays don't match what's expected
void generate_quad_mesh(DArray<VertexInstance>& out_vertex, DArray<u16>& out_index);
void generate_circle_mesh(DArray<VertexInstance>& out_vertex, DArray<u16>& out_index);

} // namespace

#endif // DRAW_HPP
