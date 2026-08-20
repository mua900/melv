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

// @todo
// normal maps
// lighting

struct RenderInitConfig
{
    bool doLights = false;
    bool gpuDebug = false;
};

// change if you want
const int InitTransferBufferSize = 16 * 1024;
const int InitVertexBufferSize = 1024;
const int InitIndexBufferSize = 16 * 1024;
const int InitInstanceBufferSize = 16 * 1024;

const TextureFormat RenderFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
const TextureFormat DepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

#define GRAPHICS_DEBUG 0

static const auto DEBUG_COLOR = melv::Colorf(0.6, 0.5, 0.4, 1.0);

using Viewport = SDL_GPUViewport;

enum DrawMatrixUsage
{
    MatrixDontUse = 0, // only use view and projection matrices (default)
    MatrixIsModel = 1, // multiply with view and projection matrices to get mvp
    MatrixIsMVP   = 2, // override mvp
};

const float MaxInstanceScale = 256;

const int VBufferDescriptionCountVertex = 1;
const int VBufferDescriptionCountInstance = 2;
const int VBufferDescriptionCountMax = melv::max(VBufferDescriptionCountVertex, VBufferDescriptionCountInstance);

const int InputAttributeCountVertex = 3;
const int InputAttributeCountInstance = 8;
const int InputAttributeCountMax = melv::max(InputAttributeCountVertex, InputAttributeCountInstance);

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
    Vertex(vec2 pos, vec2 uv, Colorf col)
        :
        x(pos.x), y(pos.y), uvx(uv.x), uvy(uv.y), r(col.r), g(col.g), b(col.b), a(col.a)
    {}

    vec2 position() const {
        return vec2(x, y);
    }

    vec2 uv() const {
        return vec2(uvx, uvy);
    }

    Colorf color() const {
        return Colorf(r, g, b, a);
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
    float z = 0;

    float rotation = 0;

    // x, y
    u32 scale = 0;

    // rgba
    u32 color = 0;

    // uv = vertex_uv * source_scale + source_offset
    u32 sourceOffset = 0; // x, y
    u32 sourceScale = 0;  // x, y
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

struct Draw
{
    vec3 position = {};
    float rotation = 0;
    vec2 scale = {}; // clamped to 0..MaxInstanceScale
    Colorf color = {};
    vec2 sourceOffset = {};
    vec2 sourceScale = {};
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
    TextureHandle texture = TEXTURE_HANDLE_INVALID;

    mat4x4 matrix = {};
    DrawMatrixUsage matrix_usage = {};
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

    mat4x4 matrix = {};
    DrawMatrixUsage matrix_usage = {};
};

enum GPUBufferUsage : u32 {
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
    SDL_GPUShader* vertexLight = {};
    SDL_GPUShader* fragmentLight = {};
};

enum MeshCommon
{
    MeshQuad,
    // @todo not used
    MeshCircle,
    // @todo more builtin shapes

    MeshCount
};

struct RenderContext {
    melv::vec2 render_size = {};

    CoordinateSpace space = {};  // what coordinate space input vertices are in
    // a view matrix is derived from this if this pointer is not null
	const Camera* camera = {};

    Colorf clear_color = {};

    SDL_GPUDevice* device = nullptr;

    // view matrix is orthographic projection from the render coordinates to the ndc
    // view matrix is calculated from the camera
    // you can set a model matrix for a draw command
    // or just set the mvp
    melv::mat4x4 mvp = {};

    DArray<GPUBuffer> buffers = {};
    int active_vertex_buffer = 0;
    int active_index_buffer = 0;

    SDL_GPUSampler* sampler = nullptr;

    GraphicsPipeline graphics = {};
    GraphicsPipeline graphics_texture = {};
    GraphicsPipeline graphics_instance_texture = {};
    GraphicsPipeline graphics_light = {};
    GraphicsPipeline graphics_composition = {};

    SDL_GPUTexture* render_target = nullptr;
    SDL_GPUTexture* light_target = nullptr;
    SDL_GPUTexture* depth_target = nullptr;

    TransferBuffer transfer_buffer = {};
    TransferBuffer group_transfer_buffer = {};

    FrameContext frame = {};

    // predefined mesh
    MeshReference mesh_common[MeshCount] = {};

    GPUBuffer instance_buffer = {}; // used with draw groups
    GPUBuffer vertex_buffer = {};
    GPUBuffer index_buffer = {};

    DArray<MeshDraw> frameMeshDraw = {};
    DArray<MeshDraw> frameMeshDrawTex = {};

    // @todo draw groups and instancing for user defined meshes

    DArray<InstanceData> groupDraw = {};
    DArray<DrawGroup> drawGroups = {};

    BucketList<GPUTexture> textures = {};

    bool resize_transfer_buffer(TransferBuffer& buffer, u32 nsize);
    bool resize_gpu_buffer(GPUBuffer& buffer, u32 nsize);

    bool gpu_inited() const
    {
        return device && graphics.pipeline && render_target;
    }

    bool make_texture_upload(SDL_Surface* surface, TextureUpload* upload);
    TextureHandle create_texture(TextureFormat format, u32 width, u32 height);
    TextureHandle create_texture_verbose(TextureFormat format, TextureUsage usage, u32 width, u32 height, int mip_levels, SampleCount sampleCount);
    void destroy_texture(TextureHandle handle);
    TextureHandle load_gpu_texture(const char* path);

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

    void set_mvp(mat4x4* mat, DrawMatrixUsage usage);

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
bool init_gpu_renderer(RenderContext* render, RenderInitConfig* conf, SDL_Window* window);

void render_present(RenderContext& context, SDL_Window* window);

GPUBuffer allocate_gpu_buffer(RenderContext& context);

TransferData add_to_transfer_buffer(RenderContext& context, DArray<MeshData>& data);

DArray<MeshReference> upload_mesh_data(RenderContext& context, TransferData& data);
DArray<MeshReference> upload_mesh_data_buffers(RenderContext& context, TransferData& data, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer);

// user functions
void queue_draw_mesh(RenderContext& render, MeshDraw& draw);
// returns false if there is no space left in the group buffer
bool queue_draw_group(RenderContext& render, InstanceData data, DrawGroupId groupId);

bool loadShader(RenderContext& context, Shader& shader, const char* path);
bool unloadShader(RenderContext& context, Shader& shader);

void destroy_texture(GPUTexture *texture);

TextureHandle render_text(RenderContext& render, String text, Font font, melv::Color color);
Text create_text(RenderContext& render, String text, Font font, melv::Color color);

bool draw(RenderContext& render, Draw& d, DrawGroupId groupId);

u16 pack_unorm16(float x, float range);
float unpack_unorm16(u16 x, float range);

u32 pack_unorm16x2(vec2 v);
vec2 unpack_unorm16x2(u32 v);

u32 pack_scale(vec2 v);
vec2 unpack_scale(u32 v);

} // namespace

#endif // DRAW_HPP
