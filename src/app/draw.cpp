#include "draw.hpp"
#include "util/math_util.hpp"
#include "util/common.hpp"
#include "util/log.hpp"
#include "util/file_util.hpp"

namespace melv
{

const int RenderTargetWidth = 1440;
const int RenderTargetHeight = 810;

bool start_frame(RenderContext& context, SDL_Window* window) {
    context.frame.command_buffer = nullptr;

    if (!context.get_command_buffer())
    {
        return false;
    }

    return true;
}

void end_frame(RenderContext& context) {
    if (context.frame.command_buffer)
    {
        SDL_SubmitGPUCommandBuffer(context.frame.command_buffer);
        context.frame.command_buffer = nullptr;
    }

    context.frame_vertex.discard_data();
    context.frame_index.discard_data();
}


bool RenderContext::get_command_buffer()
{
    frame.command_buffer = nullptr;

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    frame.command_buffer = command_buffer;
    return command_buffer ? true : false;
}

void RenderContext::submit_command_buffer()
{
    if (frame.command_buffer)
    {
        SDL_SubmitGPUCommandBuffer(frame.command_buffer);
        frame.command_buffer = nullptr;
    }
}

bool RenderContext::start_render_pass() {
    SDL_GPURenderPass* render_pass = nullptr;

    SDL_GPUColorTargetInfo color_targets[1] = {};
    color_targets[0].texture = render_target;
    color_targets[0].mip_level = 0;
    color_targets[0].layer_or_depth_plane = 0;
    color_targets[0].clear_color = SDL_FColor { COLOR_ARG(clear_color) };
    color_targets[0].load_op = SDL_GPU_LOADOP_CLEAR;
    color_targets[0].store_op = SDL_GPU_STOREOP_STORE;
    color_targets[0].resolve_texture = nullptr;
    color_targets[0].resolve_mip_level = 0;
    color_targets[0].resolve_layer = 0;
    color_targets[0].cycle = true;
    color_targets[0].cycle_resolve_texture = false;

    render_pass = SDL_BeginGPURenderPass(frame.command_buffer, color_targets, 1, nullptr);

    SDL_BindGPUGraphicsPipeline(render_pass, graphics.pipeline);

    float half_width = RenderTargetWidth/2;
    float half_height = RenderTargetHeight/2;
    melv::mat4x4 orthographic = melv::orthographic_projection_matrix(-half_width, half_width,
                                                                     -half_height, half_height,
                                                                      0.0, 1.0);

    // @todo actually read this from the camera
    melv::mat4x4 camera = melv::camera_matrix(melv::vec2(0, 0), melv::vec2(1,1));

    mat4mul(&mvp, &orthographic, &camera);
    SDL_PushGPUVertexUniformData(frame.command_buffer, 0, &mvp, sizeof(melv::mat4x4));

    frame.render_pass = render_pass;
    return render_pass ? true : false;
}

void RenderContext::end_render_pass(SDL_Window* window) {
    // this or record render commands and do all the rendering here

    ASSERT(frame.render_pass);
    SDL_EndGPURenderPass(frame.render_pass);
    frame.render_pass = nullptr;

    /*
    if (!start_copy_pass())
    {
        log_error("Couldn't begin copy pass at the end of frame %s", SDL_GetError());
        return;
    }

    auto frame_refs = copy_frame_geometry();

    end_copy_pass();

    if (!start_render_pass())
    {
        log_error("Couldn't start render pass at the end of frame");
        return;
    }

    for (auto mesh : frame_refs)
    {
        draw_mesh_buffers(*this, mesh, vertex_buffer, index_buffer);
    }

    SDL_EndGPURenderPass(frame.render_pass);
    frame.render_pass = nullptr;

    frame_refs.reset();
    */

    copy_to_swapchain(window);
}

DArray<MeshReference> RenderContext::copy_frame_geometry()
{
    TransferData transfer = add_to_transfer_buffer_ref(*this, frameGeometry);
    DArray<MeshReference> frame_refs = upload_mesh_data_buffers(*this, transfer, vertex_buffer, index_buffer);
    transfer.meshes.reset();
    return frame_refs;
}

void RenderContext::copy_to_swapchain(SDL_Window* window)
{
    SDL_GPUTexture* swapchain = nullptr;
    u32 swapchain_width = 0;
    u32 swapchain_height = 0;
    SDL_WaitAndAcquireGPUSwapchainTexture(frame.command_buffer, window, &swapchain, &swapchain_width, &swapchain_height);

    if (!swapchain)
    {
        return;
    }

    SDL_GPUBlitRegion rt_region = {};
    rt_region.texture = render_target;
    rt_region.x = 0;
    rt_region.y = 0;
    rt_region.w = RenderTargetWidth;
    rt_region.h = RenderTargetHeight;

    SDL_GPUBlitRegion sc_region = {};
    sc_region.texture = swapchain;
    sc_region.x = 0;
    sc_region.y = 0;
    sc_region.w = swapchain_width;
    sc_region.h = swapchain_height;

    SDL_GPUBlitInfo blit_info = {};
    blit_info.source = rt_region;       /**< The source region for the blit. */
    blit_info.destination = sc_region;  /**< The destination region for the blit. */
    blit_info.load_op = SDL_GPU_LOADOP_CLEAR;          /**< What is done with the contents of the destination before the blit. */
    blit_info.clear_color = SDL_FColor {COLOR_ARG(clear_color) };         /**< The color to clear the destination region to before the blit. Ignored if load_op is not SDL_GPU_LOADOP_CLEAR. */
    blit_info.flip_mode = SDL_FLIP_NONE;         /**< The flip mode for the source region. */
    blit_info.filter = SDL_GPU_FILTER_LINEAR;           /**< The filter mode used when blitting. */

    SDL_BlitGPUTexture(frame.command_buffer, &blit_info);
}

bool RenderContext::start_copy_pass() {
    frame.copy_pass = SDL_BeginGPUCopyPass(frame.command_buffer);
    return frame.copy_pass ? true : false;
}

void RenderContext::end_copy_pass() {
    ASSERT(frame.copy_pass);
    SDL_EndGPUCopyPass(frame.copy_pass);
    frame.copy_pass = nullptr;
}

bool initialize_render_context(RenderContext* render, SDL_Window* window, bool enableGpuDebug)
{
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, enableGpuDebug, nullptr);
    if (!device)
    {
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        return false;
    }

    int render_size_x, render_size_y;
    SDL_GetWindowSize(window, &render_size_x, &render_size_y);

    render->device = device;
    render->render_size = melv::vec2(render_size_x, render_size_y);

    melv::mat4x4 orthographic = melv::orthographic_projection_matrix(-1.0, 1.0, -1.0, 1.0, 0.0, 1.0);
    melv::mat4x4 camera = melv::camera_matrix(melv::vec2(0, 0), melv::vec2(1,1));
    mat4mul(&render->mvp, &orthographic, &camera);

    return true;
}

bool get_default_graphics_pipeline_parameters(GraphicsPipelineParameters *parameters, SDL_GPUDevice* device, SDL_Window *window)
{
    auto texture_format = SDL_GetGPUSwapchainTextureFormat(device, window);
    if (texture_format == SDL_GPU_TEXTUREFORMAT_INVALID)
    {
        log_error("Couldn't get swapchain texture format: %s", SDL_GetError());
        return false;
    }

    parameters->format = texture_format;

    return true;
}

SDL_GPUGraphicsPipeline* create_gpu_graphics_pipeline(GraphicsPipelineParameters* parameters, RenderContext* render, SDL_GPUShader* vertex, SDL_GPUShader* fragment)
{
    SDL_GPUVertexBufferDescription vertex_buffer_description[1] = {};
    SDL_GPUVertexAttribute vertex_attributes[3] = {};
    vertex_buffer_description[0].slot = 0;                        /**< The binding slot of the vertex buffer. */
    vertex_buffer_description[0].pitch = sizeof(Vertex);                       /**< The size of a single element + the offset between elements. */
    vertex_buffer_description[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;  /**< Whether attribute addressing is a function of the vertex index or instance index. */
    vertex_buffer_description[0].instance_step_rate = 0;          /**< Reserved for future use. Must be set to 0. */

    vertex_attributes[0].location = 0;                    /**< The shader input location index. */
    vertex_attributes[0].buffer_slot = 0;                 /**< The binding slot of the associated vertex buffer. */
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;  /**< The size and type of the attribute data. */
    vertex_attributes[0].offset = 0;                      /**< The byte offset of this attribute relative to the start of the vertex element. */

    vertex_attributes[1].location = 1;                    /**< The shader input location index. */
    vertex_attributes[1].buffer_slot = 0;                 /**< The binding slot of the associated vertex buffer. */
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;  /**< The size and type of the attribute data. */
    vertex_attributes[1].offset = sizeof(float) * 2;                      /**< The byte offset of this attribute relative to the start of the vertex element. */

    vertex_attributes[2].location = 2;                    /**< The shader input location index. */
    vertex_attributes[2].buffer_slot = 0;                 /**< The binding slot of the associated vertex buffer. */
    vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;  /**< The size and type of the attribute data. */
    vertex_attributes[2].offset = sizeof(float) * 4;                      /**< The byte offset of this attribute relative to the start of the vertex element. */

    SDL_GPUVertexInputState vertex_input = {
        vertex_buffer_description,  /**< A pointer to an array of vertex buffer descriptions. */
        ARRAY_SIZE(vertex_buffer_description),                          /**< The number of vertex buffer descriptions in the above array. */
        vertex_attributes,                   /**< A pointer to an array of vertex attribute descriptions. */
        ARRAY_SIZE(vertex_attributes)                          /**< The number of vertex attribute descriptions in the above array. */
    };
    SDL_GPURasterizerState rasterizer = {};
    rasterizer.fill_mode = SDL_GPU_FILLMODE_FILL;         /**< Whether polygons will be filled in or drawn as lines. */
    rasterizer.cull_mode = SDL_GPU_CULLMODE_NONE;         /**< The facing direction in which triangles will be culled. */
    rasterizer.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;       /**< The vertex winding that will cause a triangle to be determined as front-facing. */
    // rasterizer.depth_bias_constant_factor;  /**< A scalar factor controlling the depth value added to each fragment. */
    // rasterizer.depth_bias_clamp;            /**< The maximum depth bias of a fragment. */
    // rasterizer.depth_bias_slope_factor;     /**< A scalar factor applied to a fragment's slope in depth calculations. */
    // rasterizer.enable_depth_bias;            /**< true to bias fragment depth values. */
    // rasterizer.enable_depth_clip;            /**< true to enable depth clip, false to enable depth clamp. */

    SDL_GPUMultisampleState multisample = {};
    multisample.sample_count = SDL_GPU_SAMPLECOUNT_1;  /**< The number of samples to be used in rasterization. */
    multisample.sample_mask = 0;               /**< Reserved for future use. Must be set to 0. */
    multisample.enable_mask = false;                 /**< Reserved for future use. Must be set to false. */
    multisample.enable_alpha_to_coverage = false;    /**< true enables the alpha-to-coverage feature. */

    SDL_GPUDepthStencilState stencil = {};
    stencil.enable_depth_test = false;                     /**< true enables the depth test. */
    stencil.enable_depth_write = false;                    /**< true enables depth writes. Depth writes are always disabled when enable_depth_test is false. */
    stencil.enable_stencil_test = false;                   /**< true enables the stencil test. */

    SDL_GPUColorTargetBlendState blend_state = {};
    blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;     /**< The value to be multiplied by the source RGB value. */
    blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;     /**< The value to be multiplied by the destination RGB value. */
    blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;                /**< The blend operation for the RGB components. */
    blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;     /**< The value to be multiplied by the source alpha. */
    blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;     /**< The value to be multiplied by the destination alpha. */
    blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;                /**< The blend operation for the alpha component. */
    // blend_state.color_write_mask = 0;  /**< A bitmask specifying which of the RGBA components are enabled for writing. Writes to all channels if enable_color_write_mask is false. */
    blend_state.enable_blend = true;                            /**< Whether blending is enabled for the color target. */
    blend_state.enable_color_write_mask = false;                 /**< Whether the color write mask is enabled. */

    SDL_GPUColorTargetDescription color_target_description[1] = {};

    color_target_description[0].format = parameters->format;
    color_target_description[0].blend_state = blend_state;  /**< The blend state to be used for the color target. */

    SDL_GPUGraphicsPipelineTargetInfo target_info = {};
    target_info.color_target_descriptions = color_target_description;  /**< A pointer to an array of color target descriptions. */
    target_info.num_color_targets = ARRAY_SIZE(color_target_description);                                        /**< The number of color target descriptions in the above array. */
    target_info.depth_stencil_format = {};                       /**< The pixel format of the depth-stencil target. Ignored if has_depth_stencil_target is false. */
    target_info.has_depth_stencil_target = false;                                   /**< true specifies that the pipeline uses a depth-stencil target. */

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vertex;
    pipelineInfo.fragment_shader = fragment;
    pipelineInfo.vertex_input_state = vertex_input;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizer;
    pipelineInfo.multisample_state = multisample;
    pipelineInfo.depth_stencil_state = stencil;
    pipelineInfo.target_info = target_info;

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(render->device, &pipelineInfo);
    return pipeline;
}

bool init_gpu_renderer(RenderContext* render, SDL_Window* window, SDL_GPUShader* vertex, SDL_GPUShader* fragment)
{
    GraphicsPipelineParameters pipeline_parameters;
    if (!get_default_graphics_pipeline_parameters(&pipeline_parameters, render->device, window))
    {
        return false;
    }

    SDL_GPUGraphicsPipeline* pipeline = create_gpu_graphics_pipeline(&pipeline_parameters, render, vertex, fragment);
    if (!pipeline) {
        log_error("Failed to create graphics pipeline: %s", SDL_GetError());
        return false;
    }

    // @todo a way to allocate and reallocate gpu buffers in the api

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = 1024;  // @todo
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(render->device, &transferInfo);

    const int buffer_size = 2048;
    SDL_GPUBufferCreateInfo vertexBufferCI = { SDL_GPU_BUFFERUSAGE_VERTEX, buffer_size };
    SDL_GPUBufferCreateInfo indexBufferCI = { SDL_GPU_BUFFERUSAGE_INDEX, buffer_size };

    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(render->device, &vertexBufferCI);
    SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(render->device, &indexBufferCI);

    if (!(vertex_buffer && index_buffer))
    {
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerCI = {};
    samplerCI.min_filter = SDL_GPU_FILTER_LINEAR;                  /**< The minification filter to apply to lookups. */
    samplerCI.mag_filter = SDL_GPU_FILTER_LINEAR;                  /**< The magnification filter to apply to lookups. */
    samplerCI.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;      /**< The mipmap filter to apply to lookups. */
    samplerCI.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;  /**< The addressing mode for U coordinates outside [0, 1). */
    samplerCI.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;  /**< The addressing mode for V coordinates outside [0, 1). */
    samplerCI.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;  /**< The addressing mode for W coordinates outside [0, 1). */
    // samplerCI.mip_lod_bias;                        /**< The bias to be added to mipmap LOD calculation. */
    // samplerCI.max_anisotropy;                      /**< The anisotropy value clamp used by the sampler. If enable_anisotropy is false, this is ignored. */
    // samplerCI.compare_op = {};               /**< The comparison operator to apply to fetched data before filtering. */
    samplerCI.min_lod = 0;                             /**< Clamps the minimum of the computed LOD value. */
    samplerCI.max_lod = 8;                             /**< Clamps the maximum of the computed LOD value. */
    // samplerCI.enable_anisotropy = false;                    /**< true to enable anisotropic filtering. */
    // samplerCI.enable_compare = false;                       /**< true to enable comparison against a reference value during lookups. */
    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(render->device, &samplerCI);

    if (!sampler)
    {
        log_error("Couldn't create gpu sampler");
        return false;
    }

    SDL_GPUTextureCreateInfo renderTargetCI = {};
    renderTargetCI.type = SDL_GPU_TEXTURETYPE_2D;
    // @todo this needs to change if swapchain changes
    // or create it in a known format
    renderTargetCI.format = SDL_GetGPUSwapchainTextureFormat(render->device, window);
    renderTargetCI.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    renderTargetCI.width = RenderTargetWidth;
    renderTargetCI.height = RenderTargetHeight;
    renderTargetCI.layer_count_or_depth = 1;
    renderTargetCI.num_levels = 1;
    renderTargetCI.sample_count = SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUTexture *render_target = SDL_CreateGPUTexture(render->device, &renderTargetCI);

    if (!render_target)
    {
        return false;
    }

    render->graphics = { pipeline_parameters, pipeline };
    render->vertex_buffer = { vertex_buffer, GPUBufferVertex, buffer_size, 0 };
    render->index_buffer = { index_buffer, GPUBufferIndex, buffer_size, 0 };
    render->render_target = render_target;
    render->sampler = sampler;
    render->transfer_buffer = { transfer_buffer, transferInfo.size };

    return true;
}

bool RenderContext::set_vertex_buffer(u32 vb)
{
    if (!buffers.in_bounds(vb))
    {
        return false;
    }

    if (buffers.get_ref(vb).usage != GPUBufferVertex)
    {
        return false;
    }

    active_vertex_buffer = vb;

    return true;
}

bool RenderContext::set_index_buffer(u32 ib)
{
    if (!buffers.in_bounds(ib))
    {
        return false;
    }

    if (buffers.get_ref(ib).usage != GPUBufferIndex)
    {
        return false;
    }

    active_index_buffer = ib;

    return true;
}

u32 RenderContext::allocate_gpu_buffer(GPUBufferUsage usage, u32 size)
{
    GPUBuffer buffer = {};

    SDL_GPUBufferCreateInfo ci = {};
    ci.usage = SDL_GPUBufferUsageFlags(usage);
    ci.size = size;

    buffer.buffer = SDL_CreateGPUBuffer(device, &ci);
    buffer.usage = usage;
    buffer.size = size;
    buffer.used = 0;

    if (!buffer.buffer)
    {
        return -1;
    }

    return buffers.add(buffer);;
}

TransferData add_to_transfer_buffer(RenderContext& context, DArray<MeshData>& data)
{
    if (data.size() == 0)
    {
        return TransferData();
    }

    int vcount = 0;
    int icount = 0;
    for (auto mesh : data)
    {
        vcount += mesh.vertices.size();
        icount += mesh.indices.size();
    }

    size_t total = vcount * sizeof(Vertex) + icount * sizeof(u16);
    if (total > context.transfer_buffer.size)
    {
        return TransferData();
    }

    u8* memory = (u8*) SDL_MapGPUTransferBuffer(context.device, context.transfer_buffer.buffer, true);
    if (!memory)
    {
        return TransferData();
    }

    size_t offset = 0;

    DArray<MeshDataSize> meshes = {};

    for (auto mesh : data)
    {
        int vertex_count = mesh.vertices.size();
        int index_count = mesh.indices.size();

        size_t vertex_byte = vertex_count * sizeof(Vertex);
        size_t index_byte = index_count * sizeof(u16);
        memcpy(memory + offset, mesh.vertices.data(), vertex_byte);
        offset += vertex_byte;
        memcpy(memory + offset, mesh.indices.data(), index_byte);
        offset += index_byte;

        meshes.add(MeshDataSize(vertex_count, index_count));
    }

    SDL_UnmapGPUTransferBuffer(context.device, context.transfer_buffer.buffer);

    return TransferData(meshes);
}

TransferData add_to_transfer_buffer_ref(RenderContext& context, DArray<MeshDataRef>& data)
{
    if (data.size() == 0)
    {
        return TransferData();
    }

    int vcount = 0;
    int icount = 0;
    for (auto mesh : data)
    {
        vcount += mesh.vertex_count;
        icount += mesh.index_count;
    }

    size_t total = vcount * sizeof(Vertex) + icount * sizeof(u16);
    if (total > context.transfer_buffer.size)
    {
        return TransferData();
    }

    u8* memory = (u8*) SDL_MapGPUTransferBuffer(context.device, context.transfer_buffer.buffer, true);
    if (!memory)
    {
        return TransferData();
    }

    size_t offset = 0;

    DArray<MeshDataSize> meshes = {};

    for (auto mesh : data)
    {
        size_t vertex_byte = mesh.vertex_count * sizeof(Vertex);
        size_t index_byte = mesh.index_count * sizeof(u16);
        memcpy(memory + offset, context.frame_vertex.data() + mesh.vertex_offset, vertex_byte);
        offset += vertex_byte;
        memcpy(memory + offset, context.frame_index.data() + mesh.index_offset, index_byte);
        offset += index_byte;

        meshes.add(MeshDataSize(mesh.vertex_count, mesh.index_count));
    }

    SDL_UnmapGPUTransferBuffer(context.device, context.transfer_buffer.buffer);

    return TransferData(meshes);
}

DArray<MeshReference> upload_mesh_data(RenderContext& context, TransferData& data)
{
    return upload_mesh_data_buffers(context, data, context.buffers.get_ref(context.active_vertex_buffer), context.buffers.get_ref(context.active_index_buffer));
}

DArray<MeshReference> upload_mesh_data_buffers(RenderContext& context, TransferData& data, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer)
{
    SDL_GPUCommandBuffer* command_buffer = context.frame.command_buffer;

    ASSERT(command_buffer);
    ASSERT(context.frame.copy_pass);

    DArray<MeshReference> refs = {};

    size_t transfer_offset = 0;

    for (auto mesh : data.meshes)
    {
        MeshReference reference = {};

        size_t vertex_byte = mesh.vertex_count * sizeof(Vertex);
        size_t index_byte = mesh.index_count * sizeof(u16);

        SDL_GPUTransferBufferLocation source;
        source.transfer_buffer = context.transfer_buffer.buffer;
        source.offset = transfer_offset;

        SDL_GPUBufferRegion destination;
        destination.buffer = vertex_buffer.buffer;
        destination.offset = vertex_buffer.used;
        destination.size = vertex_byte;
        SDL_UploadToGPUBuffer(context.frame.copy_pass, &source, &destination, false);

        reference.vertex_offset = vertex_buffer.used;
        vertex_buffer.used += vertex_byte;
        transfer_offset += vertex_byte;

        source.offset = transfer_offset;

        destination.buffer = index_buffer.buffer;
        destination.offset = index_buffer.used;
        destination.size = index_byte;
        SDL_UploadToGPUBuffer(context.frame.copy_pass, &source, &destination, false);

        reference.index_offset = index_buffer.used;
        index_buffer.used += index_byte;
        transfer_offset += index_byte;

        reference.vertex_count = mesh.vertex_count;
        reference.index_count = mesh.index_count;

        reference.vertex_buffer = context.active_vertex_buffer;
        reference.index_buffer = context.active_index_buffer;

        refs.add(reference);
    }

    return refs;
}

TextureHandle RenderContext::create_texture(u32 width, u32 height)
{
    // @todo
    // SDL_GPUTexture* sdl_texture = SDL_CreateGPUTexture();
    GPUTexture texture = GPUTexture(nullptr, width, height);
    return textures.add(texture);
}

void RenderContext::destroy_texture(TextureHandle handle)
{
    // if (is_texture_handle_valid())
    GPUTexture& texture = textures.get_ref(handle);
    SDL_ReleaseGPUTexture(device, texture.texture);
    texture = {};
}

GPUTexture RenderContext::get_texture(TextureHandle handle)
{
    return textures.get_ref(handle);
}

bool RenderContext::is_texture_handle_valid(TextureHandle handle)
{
    // @todo
    return textures.in_bounds(handle);
}

void draw_geometry(RenderContext& render, const Vertex vertices[], int vertex_count, const u16 indices[], int index_count)
{
    MeshDataRef ref = {};
    ref.vertex_offset = render.frame_vertex.size();
    ref.index_offset = render.frame_index.size();
    ref.vertex_count = vertex_count;
    ref.index_count = index_count;
    for (int i = 0; i < vertex_count; i++)
    {
        render.frame_vertex.add(vertices[i]);
    }
    for (int i = 0; i < index_count; i++)
    {
        render.frame_index.add(indices[i]);
    }

    render.frameGeometry.add(ref);
}

void draw_geometry_texture(RenderContext& render, GPUTexture texture, const Vertex vertices[], int vertex_count, const u16 indices[], int index_count)
{
    // @todo
}

void draw_mesh(RenderContext& render, MeshReference mesh)
{
    draw_mesh_buffers(render, mesh, render.buffers.get_ref(mesh.vertex_buffer), render.buffers.get_ref(mesh.index_buffer));
}

void draw_mesh_buffers(RenderContext& render, MeshReference mesh, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer)
{
    ASSERT(render.frame.render_pass);

    SDL_GPUBufferBinding vertex_binding = {};
    SDL_GPUBufferBinding index_binding = {};

    vertex_binding.buffer = vertex_buffer.buffer;
    vertex_binding.offset = mesh.vertex_offset;

    index_binding.buffer = index_buffer.buffer;
    index_binding.offset = mesh.index_offset;

    SDL_BindGPUVertexBuffers(render.frame.render_pass, 0, &vertex_binding, 1);
    SDL_BindGPUIndexBuffer(render.frame.render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(render.frame.render_pass, mesh.index_count, 1, 0, 0, 0);
}

melv::vec2 RenderContext::transformWorld(melv::vec2 p) const
{
    if (space == CoordinateSpace::World)
    {
        return camera->world_to_screen(p) + render_size / 2;
    }
    else
    {
        return p;
    }
}

melv::vec2 RenderContext::transformScreen(melv::vec2 p) const
{
    if (space == CoordinateSpace::World)
    {
        return camera->screen_to_world(p);
    }
    else
    {
        return p;
    }
}

melv::Rectangle RenderContext::transform_rectangle(melv::Rectangle r) const
{
    melv::vec2 t = transformWorld(r.get_position());
    melv::vec2 s = (space == CoordinateSpace::World) ? r.get_scale() * camera->zoom : r.get_scale();
    return melv::Rectangle(t,s);
}

void RenderContext::set_viewport(Viewport viewport)
{
    SDL_SetGPUViewport(frame.render_pass, &viewport);
}

TextureHandle render_text(RenderContext& render, String text, Font font, melv::Color color) {
    SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
    SDL_Surface* surface = TTF_RenderText_Solid(font.font, text.data, text.size, sdl_color);

    if (!surface) {
        return TextureHandle();
    }

    // @todo
    // create gpu texture
    // calculate text width and height
    SDL_GPUTexture* texture = nullptr;

	SDL_DestroySurface(surface);

    return TextureHandle();
}

Text create_text(RenderContext& render, String text, Font font, melv::Color color)
{
    TextureHandle texture = render_text(render, text, font, color);
    if (!render.is_texture_handle_valid(texture)) return Text();
    return Text(texture, text, color);
}

bool RenderContext::set_shaders(SDL_GPUShader* vertex, SDL_GPUShader* fragment)
{
    if (!this->graphics.pipeline)
    {
        return false;
    }

    SDL_GPUGraphicsPipeline *pipeline = create_gpu_graphics_pipeline(&graphics.parameters, this, vertex, fragment);

    if (!pipeline)
    {
        return false;
    }

    SDL_ReleaseGPUGraphicsPipeline(this->device, this->graphics.pipeline);

    this->graphics.pipeline = pipeline;
    return true;
}

// @todo
void render_texture(const RenderContext& render, melv::Rectangle area, GPUTexture texture, bool strech)
{
}
void render_texture_rotate(const RenderContext& render, melv::Rectangle area, GPUTexture texture, float angle, Flip flip, bool strech)
{
}
void render_textured_rectangle(const RenderContext& render, melv::Rectangle rect, GPUTexture texture, melv::Color color, bool strech, bool center)
{
}
void render_texture_with_tint(const RenderContext& render, melv::Rectangle area, GPUTexture texture, melv::ColorF tint, bool strech)
{
}

void draw_segment(RenderContext& context, melv::vec2 start, melv::vec2 end, float thick, melv::ColorF color)
{
    melv::vec2 dir = (end - start).normalized();
    melv::vec2 perp = melv::vec2(-dir.y, dir.x);

    melv::vec2 sleft = start + perp * thick;
    melv::vec2 sright = start - perp * thick;
    melv::vec2 eleft = end + perp * thick;
    melv::vec2 eright = end - perp * thick;

    Vertex vertices[4];
    u16 indices[6];
    vertices[0] = Vertex(context.transformWorld({ sleft.x, sleft.y }), {0, 0}, color);
    vertices[1] = Vertex(context.transformWorld({ sright.x, sright.y }), {0, 0}, color);
    vertices[2] = Vertex(context.transformWorld({ eleft.x, eleft.y }), {0, 0}, color);
    vertices[3] = Vertex(context.transformWorld({ eright.x, eright.y }), {0, 0}, color);

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 1;
    indices[5] = 3;

    draw_geometry(context, vertices, 4, indices, 6);
}

void draw_arrow(RenderContext& context, melv::vec2 start, melv::vec2 end, float thickness, melv::ColorF color)
{
    // 3 for the arrow head, 4 for the quadrilateral below
    Vertex vertices[7];

    melv::vec2 dir = end - start;
    float total_length = dir.magnitude();

    if (total_length < 1)
    {
        // subpixel arrow?
        return;
    }

    const float head_percentage = 0.2;  // 1 / 5 of the length is head
    const float head_width = thickness * 2;
    const float base_width = thickness;

    float head_size = total_length * head_percentage;
    dir = dir.normalized();
    melv::vec2 ortho = melv::vec2(-dir.y, dir.x);

    melv::vec2 head_start = end - dir * head_size;
    melv::vec2 arrow_left = head_start + ortho * head_width;
    melv::vec2 arrow_right = head_start - ortho * head_width;
    vertices[0] = Vertex(context.transformWorld({ end.x, end.y }), vec2(0, 0), color);
    vertices[1] = Vertex(context.transformWorld({ arrow_left.x, arrow_left.y }), vec2(0, 0), color);
    vertices[2] = Vertex(context.transformWorld({ arrow_right.x, arrow_right.y }), vec2(0, 0), color);

    melv::vec2 upper_base_left = head_start + ortho * base_width;
    melv::vec2 upper_base_right = head_start - ortho * base_width;
    melv::vec2 lower_base_left = upper_base_left - dir * total_length * (1.0 - head_percentage);
    melv::vec2 lower_base_right = upper_base_right - dir * total_length * (1.0 - head_percentage);
    vertices[3] = Vertex(context.transformWorld({ upper_base_left.x, upper_base_left.y }), vec2(0, 0), color);
    vertices[4] = Vertex(context.transformWorld({ upper_base_right.x, upper_base_right.y }), vec2(0, 0), color);
    vertices[5] = Vertex(context.transformWorld({ lower_base_left.x, lower_base_left.y }), vec2(0, 0), color);
    vertices[6] = Vertex(context.transformWorld({ lower_base_right.x, lower_base_right.y }), vec2(0, 0), color);

    const u16 indices[9] = {
        0, 1, 2,  // head
        3, 5, 4,
        4, 5, 6
    };

    draw_geometry(context, vertices, 7, indices, ARRAY_SIZE(indices));
}

void draw_arc(RenderContext& context, melv::vec2 center, float inner_radius, float outer_radius, float start_angle, float arc, melv::ColorF color)
{
    // resolution
    #define NVERTICES 64
    Vertex vertices[NVERTICES];

    // the angle between vertices and it's sin and cos
    const float angle = arc / float(NVERTICES / 2 - 1);
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);

    float xcomp = std::cosf(start_angle);
    float ycomp = std::sinf(start_angle);
    for (int i = 0; i < NVERTICES; i += 2)
    {
        float px0 = center.x + xcomp * inner_radius;
        float py0 = center.y + ycomp * inner_radius;
        vertices[i + 0] = Vertex(px0, py0, 0, 0, COLOR_ARG(color));

        float px1 = center.x + xcomp * outer_radius;
        float py1 = center.y + ycomp * outer_radius;
        vertices[i + 1] = Vertex(px1, py1, 0, 0, COLOR_ARG(color));

        vec2 v0 = context.transformWorld(vertices[i + 0].position());
        vec2 v1 = context.transformWorld(vertices[i + 1].position());
        vertices[i + 0].x = v0.x;
        vertices[i + 0].y = v0.y;
        vertices[i + 1].x = v1.x;
        vertices[i + 1].y = v1.y;

        // rotate the vector
        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;
        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    u16 indices[(NVERTICES / 2 - 1) * 6];
    for (int i = 0; i < NVERTICES - 2; i += 2)
    {
        indices[i * 3 + 0] = i + 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
        indices[i * 3 + 3] = i + 1;
        indices[i * 3 + 4] = i + 3;
        indices[i * 3 + 5] = i + 2;
    }

    draw_geometry(context, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

void draw_circle_empty(RenderContext& context, melv::vec2 position, float radius, float thick, melv::ColorF color)
{
	#define NSEGMENTS 32
	const float angle = CONSTANT_TAU / NSEGMENTS;
	const float c = std::cosf(angle);
	const float s = std::sinf(angle);

	float xcomp = 1.0f;
	float ycomp = 0.0f;
	for (int i = 0; i < NSEGMENTS; i++)
	{
		float new_xcomp = c * xcomp - s * ycomp;
		float new_ycomp = c * ycomp + s * xcomp;

		draw_segment(context, position + melv::vec2(xcomp * radius, ycomp * radius), position + melv::vec2(new_xcomp * radius, new_ycomp * radius), thick, color);

		xcomp = new_xcomp;
		ycomp = new_ycomp;
	}
}

void draw_circle(RenderContext& context, melv::vec2 position, float radius, melv::ColorF color)
{
    draw_circle_with_texture(context, position, radius, GPUTexture(), color);
}

void draw_circle_with_texture(RenderContext& context, melv::vec2 position, float radius, GPUTexture texture, melv::ColorF color)
{
    // change the number of vertices to use to configure how fine of an approximation we get
    #define NVERTICES 32
    Vertex vertices[NVERTICES + 1];

    Vertex center = Vertex(position, vec2(0.5, 0.5), color);

    // the angle between vertices and it's sin and cos
    const float angle = CONSTANT_TAU / float(NVERTICES);
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);

    float xcomp = 1.0;
    float ycomp = 0.0;
    for (int i = 1; i <= NVERTICES; i++)
    {
        float px = center.x + xcomp * radius;
        float py = center.y + ycomp * radius;
        vec2 t = context.transformWorld(vec2(px, py));
        float u = (xcomp + 1.0f) * 0.5f;
        float v = (ycomp + 1.0f) * 0.5f;
        vertices[i] = Vertex(vec2(t.x, t.y), vec2(u, v), color);

        // rotate the vector
        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;
        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    // transform the center later so that the perimeter points get calculated according to the original center before being transformed
    vec2 t = context.transformWorld(vec2(center.x, center.y));
    vertices[0] = Vertex(vec2(t.x, t.y), vec2(center.uvx, center.uvy), center.color());

    u16 indices[NVERTICES * 3];
    for (int i = 0; i < NVERTICES - 1; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    indices[(NVERTICES - 1) * 3 + 0] = 0;
    indices[(NVERTICES - 1) * 3 + 1] = NVERTICES;
    indices[(NVERTICES - 1) * 3 + 2] = 1;

    draw_geometry_texture(context, texture, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

void draw_circle_segment(RenderContext& context, melv::vec2 position, float radius, float start_angle, float angle, melv::ColorF color)
{
    draw_circle_segment_with_texture(context, position, radius, start_angle, angle, GPUTexture(), color);
}

void draw_circle_segment_with_texture(RenderContext& context, melv::vec2 position, float radius, float start_angle, float angle, GPUTexture texture, melv::ColorF color)
{
    // change the number of vertices to use to configure how fine of an approximation we get
    #define NVERTICES 32
    Vertex vertices[NVERTICES + 1];

    Vertex center = Vertex(position.x, position.y, 0.5, 0.5, COLOR_ARG(color));

    // the angle between vertices and it's sin and cos
    // if we have n vertices than we have n - 1 gaps between them so divide the angle by the number of gaps to fill
    const float step_angle = angle / float(NVERTICES - 1);
    const float c = std::cosf(step_angle);
    const float s = std::sinf(step_angle);

    float xcomp = std::cosf(start_angle);
    float ycomp = std::sinf(start_angle);
    for (int i = 1; i <= NVERTICES; i++)
    {
        float px = center.x + xcomp * radius;
        float py = center.y + ycomp * radius;
        vec2 t = context.transformWorld(vec2(px, py));
        float u = (xcomp + 1.0f) * 0.5f;
        float v = (ycomp + 1.0f) * 0.5f;
        vertices[i] = Vertex(t.x, t.y, u, v, color.r, color.g, color.b, color.a);

        // rotate the vector
        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;
        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    vec2 t = context.transformWorld(vec2(center.x, center.y));
    vertices[0] = Vertex(vec2(t.x, t.y), vec2(center.uvx, center.uvy), center.color());

    u16 indices[NVERTICES * 3];
    for (int i = 0; i < NVERTICES - 1; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    bool fullCircle = melv::normalize_angle_radians_f(angle) == 0;
    if (fullCircle)
    {
        indices[(NVERTICES - 1) * 3 + 0] = 0;
        indices[(NVERTICES - 1) * 3 + 1] = NVERTICES;
        indices[(NVERTICES - 1) * 3 + 2] = 1;
    }

    draw_geometry_texture(context, texture, vertices, ARRAY_SIZE(vertices), indices, (fullCircle ? NVERTICES : NVERTICES - 1) * 3);
    #undef NVERTICES
}

void draw_capsule(RenderContext& context, melv::vec2 center0, melv::vec2 center1, float radius, melv::ColorF color)
{
    // total number of vertices used for either half circle sides of the capsule shape
    #define NVERTICES 32
    Vertex vertices[NVERTICES + 1];

    melv::vec2 midpoint = (center0 + center1) / 2;

    vertices[0] = Vertex(midpoint, vec2(0, 0), color);

    // the angle between vertices and it's sin and cos
    const float angle = CONSTANT_TAU / float(NVERTICES);
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);

    melv::vec2 axis = (center1 - center0).normalized();

    // perpendicular vector
    float xcomp = -axis.y;
    float ycomp = axis.x;

    for (int i = 1; i <= NVERTICES / 2; i++)
    {
        float px = center0.x + xcomp * radius;
        float py = center0.y + ycomp * radius;
        vec2 t = context.transformWorld(vec2(px, py));
        vertices[i] = Vertex(vec2(t.x, t.y), vec2(0, 0), color);

        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;

        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    for (int i = NVERTICES / 2 + 1; i <= NVERTICES; i++)
    {
        float px = center1.x + xcomp * radius;
        float py = center1.y + ycomp * radius;
        vec2 t = context.transformWorld(vec2(px, py));
        vertices[i] = Vertex(vec2(t.x, t.y), vec2(0, 0), color);

        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;

        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    vec2 t = context.transformWorld(vec2(vertices[0].x, vertices[0].y));
    vertices[0].x = t.x;
    vertices[0].y = t.y;

    u16 indices[NVERTICES * 3];
    for (int i = 0; i < NVERTICES - 1; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    indices[(NVERTICES - 1) * 3 + 0] = 0;
    indices[(NVERTICES - 1) * 3 + 1] = NVERTICES;
    indices[(NVERTICES - 1) * 3 + 2] = 1;

    draw_geometry(context,  vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

void draw_quad(RenderContext& context, melv::RectPoints quad, melv::ColorF color)
{
    Vertex vertex [4];
    vertex[0]     = {
        context.transformWorld({ quad.p[0].x, quad.p[0].y }),
        { 0, 1 },
        { color.r, color.g, color.b, color.a },
    };
    vertex[1]    = {
        context.transformWorld({ quad.p[1].x, quad.p[1].y }),
        { 1, 1 },
        { color.r, color.g, color.b, color.a },
    };
    vertex[2]  = {
        context.transformWorld({ quad.p[2].x, quad.p[2].y }),
        { 0, 0 },
        { color.r, color.g, color.b, color.a },
    };
    vertex[3] = {
        context.transformWorld({ quad.p[3].x, quad.p[3].y }),
        { 1, 0 },
        { color.r, color.g, color.b, color.a },
    };
    u16 index [6] = {
        0, 3, 1,
        0, 2, 3,
    };

    draw_geometry(context, vertex, 4, index, 6);
}

void draw_quad_with_texture(RenderContext& context, melv::RectPoints quad, GPUTexture texture, melv::ColorF color)
{
    Vertex vertex [4];
    vertex[melv::QuadTopLeft]     = {
		context.transformWorld({ quad.p[melv::QuadTopLeft].x, quad.p[melv::QuadTopLeft].y }),
		{ 0, 1 },
		{ color.r, color.g, color.b, color.a },
	};
    vertex[melv::QuadTopRight]    = {
		context.transformWorld({ quad.p[melv::QuadTopRight].x, quad.p[melv::QuadTopRight].y }),
		{ 1, 1 },
		{ color.r, color.g, color.b, color.a },
	};
    vertex[melv::QuadBottomLeft]  = {
		context.transformWorld({ quad.p[melv::QuadBottomLeft].x, quad.p[melv::QuadBottomLeft].y }),
		{ 0, 0 },
		{ color.r, color.g, color.b, color.a },
	};
    vertex[melv::QuadBottomRight] = {
		context.transformWorld({ quad.p[melv::QuadBottomRight].x, quad.p[melv::QuadBottomRight].y }),
		{ 1, 0 },
		{ color.r, color.g, color.b, color.a },
	};
    u16 index [6] = {
        melv::QuadTopLeft, melv::QuadBottomRight, melv::QuadTopRight,
        melv::QuadTopLeft, melv::QuadBottomLeft, melv::QuadBottomRight,
    };

    draw_geometry_texture(context, texture, vertex, 4, index, 6);
}

void draw_path(RenderContext& context, melv::vec2 points[], int numPoints, float thick, melv::ColorF color)
{
    for (int i = 0; i < numPoints - 1; i++)
    {
        draw_segment(context, points[i], points[i + 1], thick, color);
    }
}

void draw_closed_path(RenderContext& context, melv::vec2 points[], int numPoints, float thick, melv::ColorF color)
{
    for (int i = 0; i < numPoints; i++)
    {
        draw_segment(context, points[i], points[(i + 1) % numPoints], thick, color);
    }
}

void draw_quadratic_bezier(RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, float thick, melv::ColorF color)
{
    melv::vec2 prev = p0;

    const int resolution = 32;

    for (int i = 0; i < resolution; i++)
    {
        float t = float(i) / float(resolution);
        float it = 1.0f - t;
        melv::vec2 p = (it * it * p0) + (2.0f * it * t * p1) + (t * t * p2);
        draw_segment(context, prev, p, thick, color);
        prev = p;
    }
}

void draw_cubic_bezier(RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, melv::vec2 p3, float thick, melv::ColorF color)
{
    melv::vec2 prev = p0;

    const int resolution = 32;

    for (int i = 0; i < resolution; i++)
    {
        float t = float(i) / float(resolution);
        float it = 1.0f - t;
        melv::vec2 p = (it * it * it * p0) + (3.0f * it * it * t * p1) + (3.0f * it * t * t * p2) + (t * t * t * p3);
        draw_segment(context, prev, p, thick, color);
        prev = p;
    }
}

bool unloadShader(RenderContext& context, Shader& shader)
{
    SDL_ReleaseGPUShader(context.device, shader.shader);
    return true;
}

bool loadShader(RenderContext& context, Shader& shader, const char* path)
{
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
    SDL_GPUShaderStage shaderStage = SDL_GPUShaderStage(shader.stage);

    BinaryData code = {};
    if (!load_file(path, code)) {
        log_error("Could not load shader %s", path);
        return false;
    }

    String extension = string_get_extension(String(path));
    if (string_compare(extension, String("dxil")))
    {
        format = SDL_GPU_SHADERFORMAT_DXIL;
    }
    else if (string_compare(extension, String("spv")))
    {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    }

    SDL_GPUShaderCreateInfo info = {};
    info.code_size = code.size;
    info.code = code.data;
    info.entrypoint = "main";
    info.format = format;
    info.stage = shaderStage;
    info.num_samplers = shader.numSamplers;
    info.num_storage_textures = shader.numStorageTextures;
    info.num_storage_buffers = shader.numStorageBuffers;
    info.num_uniform_buffers = shader.numUniformBuffers;

    SDL_GPUShader* shaderObj = SDL_CreateGPUShader(context.device, &info);
    if (!shaderObj) {
        log_error("%s", SDL_GetError());
        return false;
    }

    shader.shader = shaderObj;

    return true;
}

} // namespace
