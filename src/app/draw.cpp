#include "draw.hpp"
#include "util/math_util.hpp"
#include "util/common.hpp"
#include "util/log.hpp"
#include "util/file_util.hpp"

namespace melv
{

bool start_frame(RenderContext& context, SDL_Window* window) {
    context.frame.command_buffer = nullptr;

    if (!context.get_command_buffer())
    {
        return false;
    }

    context.frame.swapchain = {};

    SDL_GPUTexture* swapchain = nullptr;
    u32 swapchain_width = 0;
    u32 swapchain_height = 0;
    SDL_WaitAndAcquireGPUSwapchainTexture(context.frame.command_buffer, window, &swapchain, &swapchain_width, &swapchain_height);

    if (!swapchain)
    {
        return false;
    }

    context.frame.swapchain = { swapchain, swapchain_width, swapchain_height };
    return true;
}

void end_frame(RenderContext& context) {
    if (context.frame.command_buffer)
    {
        SDL_SubmitGPUCommandBuffer(context.frame.command_buffer);
    }
}


bool RenderContext::get_command_buffer()
{
    frame.command_buffer = nullptr;

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    frame.command_buffer = command_buffer;
    return command_buffer ? true : false;
}

bool RenderContext::start_render_pass() {
    SDL_GPURenderPass* render_pass = nullptr;

    // no point if we didn't get the swapchain texture
    if (frame.swapchain.texture)
    {
        SDL_GPUColorTargetInfo color_targets[1] = {};
        color_targets[0].texture = frame.swapchain.texture;
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

        SDL_BindGPUGraphicsPipeline(render_pass, graphics);

        melv::mat4x4 orthographic = melv::orthographic_projection_matrix(-1.0, 1.0, -1.0, 1.0, 0.0, 1.0);
        melv::mat4x4 camera = melv::camera_matrix(melv::vec2(0, 0), melv::vec2(1,1));
        mat4mul(&mvp, &orthographic, &camera);
        SDL_PushGPUVertexUniformData(frame.command_buffer, 0, &mvp, sizeof(melv::mat4x4));
    }

    frame.render_pass = render_pass;
    return render_pass ? true : false;
}

void RenderContext::end_render_pass() {
    ASSERT(frame.render_pass);
    SDL_EndGPURenderPass(frame.render_pass);
    frame.render_pass = nullptr;
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

bool initialize_render_context(RenderContext* render, SDL_Window* window)
{
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    if (!device)
    {
        return false;
    }

    SDL_ClaimWindowForGPUDevice(device, window);

    int render_size_x, render_size_y;
    SDL_GetWindowSize(window, &render_size_x, &render_size_y);

    render->device = device;
    render->render_size = melv::vec2(render_size_x, render_size_y);

    melv::mat4x4 orthographic = melv::orthographic_projection_matrix(-1.0, 1.0, -1.0, 1.0, 0.0, 1.0);
    melv::mat4x4 camera = melv::camera_matrix(melv::vec2(0, 0), melv::vec2(1,1));
    mat4mul(&render->mvp, &orthographic, &camera);

    return true;
}

bool init_gpu_renderer(RenderContext* render, SDL_Window* window, SDL_GPUShader* vertex, SDL_GPUShader* fragment)
{
    if (render->gpu_inited())
    {
        return true;
    }

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

    color_target_description[0].format = SDL_GetGPUSwapchainTextureFormat(render->device, window);               /**< The pixel format of the texture to be used as a color target. */
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

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(render->device, &pipelineInfo);
    if (!pipeline) {
        log_error("Failed to create graphics pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_GPUBuffer* vertex_buffer = nullptr;
    SDL_GPUBuffer* index_buffer = nullptr;

    SDL_GPUBufferCreateInfo vertex_info = {};
    vertex_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_info.size = 1024;  // @todo
    SDL_GPUBufferCreateInfo index_info = {};
    index_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_info.size = 1024;  // @todo
    vertex_buffer = SDL_CreateGPUBuffer(render->device, &vertex_info);
    index_buffer = SDL_CreateGPUBuffer(render->device, &index_info);

    if (!(vertex_buffer && index_buffer)) {
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = 1024;  // @todo
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(render->device, &transferInfo);

    SDL_GPUTextureCreateInfo renderTargetCI = {};
    renderTargetCI.type = SDL_GPU_TEXTURETYPE_2D;
    // @todo this needs to change if swapchain changes
    // or create it in a known format
    renderTargetCI.format = SDL_GetGPUSwapchainTextureFormat(render->device, window);
    renderTargetCI.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    renderTargetCI.width = render->render_size.x;
    renderTargetCI.height = render->render_size.y;
    renderTargetCI.layer_count_or_depth = 1;
    renderTargetCI.num_levels = 1;
    renderTargetCI.sample_count = SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUTexture *render_target = SDL_CreateGPUTexture(render->device, &renderTargetCI);

    if (!render_target)
    {
        return false;
    }

    render->graphics = pipeline;
    render->render_target = render_target;
    render->vertex_buffer = { vertex_buffer, vertex_info.size, 0 };
    render->index_buffer = { index_buffer, index_info.size, 0 };
    render->transfer_buffer = { transfer_buffer, transferInfo.size };

    return true;
}

MeshReference add_mesh(RenderContext& context, MeshData data)
{
    MeshReference reference = {};

    u8* memory = (u8*) SDL_MapGPUTransferBuffer(context.device, context.transfer_buffer.buffer, true);
    size_t vertex_byte = data.vertices.size() * sizeof(Vertex);
    size_t index_byte = data.indices.size() * sizeof(u16);
    memcpy(memory, data.vertices.data(), vertex_byte);
    memcpy(memory + vertex_byte, data.indices.data(), index_byte);
    SDL_UnmapGPUTransferBuffer(context.device, context.transfer_buffer.buffer);

    SDL_GPUCommandBuffer* command_buffer = context.frame.command_buffer;
    ASSERT(command_buffer);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass)
    {
        return reference;
    }

    SDL_GPUTransferBufferLocation source;
    source.transfer_buffer = context.transfer_buffer.buffer;
    source.offset = 0;

    SDL_GPUBufferRegion destination;
    destination.buffer = context.vertex_buffer.buffer;
    destination.offset = context.vertex_buffer.used;
    destination.size = vertex_byte;
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);

    reference.vertex_offset = context.vertex_buffer.used;

    context.vertex_buffer.used += vertex_byte;

    source.offset = vertex_byte;

    destination.buffer = context.index_buffer.buffer;
    destination.offset = context.index_buffer.used;
    destination.size = index_byte;
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);

    reference.index_offset = context.index_buffer.used;

    context.index_buffer.used += index_byte;

    SDL_EndGPUCopyPass(copy_pass);

    reference.vertex_count = data.vertices.size();
    reference.index_count = data.indices.size();

    return reference;
}

void draw_mesh(RenderContext& render, MeshReference mesh)
{
    ASSERT(render.frame.render_pass);

    SDL_GPUBufferBinding vertex_binding = {};
    SDL_GPUBufferBinding index_binding = {};

    vertex_binding.buffer = render.vertex_buffer.buffer;
    vertex_binding.offset = mesh.vertex_offset;

    index_binding.buffer = render.index_buffer.buffer;
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

SDL_FPoint RenderContext::transform_sdl_point(SDL_FPoint p) const
{
    melv::vec2 t = transformWorld(melv::vec2(p.x, p.y));
    return { t.x, t.y };
}

SDL_Vertex RenderContext::transform_sdl_vertex(SDL_Vertex v) const
{
    SDL_FPoint t = transform_sdl_point(v.position);
    v.position = t;
    return v;
}

SDL_FRect RenderContext::transform_sdl_rectangle(SDL_FRect r) const
{
    melv::Rectangle t = transform_rectangle(melv::Rectangle(r.x + r.w / 2, r.y + r.h / 2, r.w, r.h));
    return SDL_FRect { t.x - t.w / 2, t.y - t.h / 2, t.w, t.h };
}

void draw_texture(const RenderContext& context, melv::Rectangle area, SDL_Texture* texture)
{
    SDL_FRect dst = { area.x, area.y, area.w, area.h };
    dst = context.transform_sdl_rectangle(dst);
    SDL_RenderTexture(context.renderer, texture, NULL, &dst);
}

SDL_Texture* render_text(SDL_Renderer* renderer, String text, Font font, melv::Color color) {
    SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
    SDL_Surface* surface = TTF_RenderText_Solid(font.font, text.data, text.size, sdl_color);

    if (!surface) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_DestroySurface(surface);

    return texture;
}

Text create_text(SDL_Renderer* renderer, String text, Font font, melv::Color color)
{
    SDL_Texture* texture = render_text(renderer, text, font, color);
    if (!texture) return Text();
    return Text(texture, text, color);
}

void render_text_size(SDL_Renderer* renderer, Text text, melv::vec2 where, melv::vec2 absolute_scale)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

    if (!absolute_scale.x)
    {
        absolute_scale = melv::vec2(tex_w, tex_h);
    }

    SDL_FRect src = { 0,0,tex_w,tex_h };
    SDL_FRect dst = {where.x - absolute_scale.x/2, where.y - absolute_scale.y/2, absolute_scale.x, absolute_scale.y};

    SDL_RenderTexture(renderer, text.texture, &src, &dst);
}

void render_text_scale(SDL_Renderer* renderer, Text text, melv::vec2 where, melv::vec2 scale_factor)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

    if (!scale_factor.x)
    {
        scale_factor = melv::vec2(1,1);
    }

    auto scale = melv::vec2(tex_w * scale_factor.x, tex_h * scale_factor.y);

    SDL_FRect src = { 0,0,tex_w,tex_h };
    SDL_FRect dst = {where.x - scale.x/2, where.y - scale.y/2, scale.x, scale.y};

    SDL_RenderTexture(renderer, text.texture, &src, &dst);
}

void RenderContext::set_viewport(Viewport viewport)
{
    SDL_SetGPUViewport(frame.render_pass, &viewport);
}

void draw_triangle(const RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, ColorF color)
{
    SDL_Vertex vertices[3];
    vertices[0].position = {p0.x, p0.y};
    vertices[1].position = {p1.x, p1.y};
    vertices[2].position = {p2.x, p2.y};
    for (int i = 0; i < 3; i++) vertices[i].color    = {COLOR_ARG(color)};

    int indices[3] = {0, 1, 2};

    SDL_RenderGeometry(context.renderer, nullptr, vertices, 3, indices, 3);
}

void draw_triangle_texture(const RenderContext& context, SDL_Texture* texture, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, ColorF color)
{
    SDL_Vertex vertices[3];
    vertices[0].position = {p0.x, p0.y};
    vertices[1].position = {p1.x, p1.y};
    vertices[2].position = {p2.x, p2.y};
    for (int i = 0; i < 3; i++) vertices[i].color    = {COLOR_ARG(color)};

    int indices[3] = {0, 1, 2};

    SDL_RenderGeometry(context.renderer, texture, vertices, 3, indices, 3);
}

void draw_rectangle(const RenderContext& context, melv::Rectangle area, melv::ColorF color)
{
    SDL_SetRenderDrawColorFloat(context.renderer, COLOR_ARG(color));
    SDL_FRect dst = { area.x - area.w / 2, area.y - area.h / 2, area.w, area.h };
    dst = context.transform_sdl_rectangle(dst);
    SDL_RenderFillRect(context.renderer, &dst);
}

void draw_rectangle_rotated(const RenderContext& context, melv::Rectangle area, float rotation, melv::ColorF color)
{
	auto quad = get_rotated_points(area, rotation);
	draw_quad(context, quad, color);
}

void draw_segment(const RenderContext& context, melv::vec2 start, melv::vec2 end, float thick, melv::ColorF color)
{
    melv::vec2 dir = (end - start).normalized();
    melv::vec2 perp = melv::vec2(-dir.y, dir.x);

    melv::vec2 sleft = start + perp * thick;
    melv::vec2 sright = start - perp * thick;
    melv::vec2 eleft = end + perp * thick;
    melv::vec2 eright = end - perp * thick;

    SDL_Vertex vertices[4];
    int indices[6];
    vertices[0].position = context.transform_sdl_point({ sleft.x, sleft.y });
    vertices[0].color = { COLOR_ARG(color) };
    vertices[1].position = context.transform_sdl_point({ sright.x, sright.y });
    vertices[1].color = { COLOR_ARG(color) };
    vertices[2].position = context.transform_sdl_point({ eleft.x, eleft.y });
    vertices[2].color = { COLOR_ARG(color) };
    vertices[3].position = context.transform_sdl_point({ eright.x, eright.y });
    vertices[3].color = { COLOR_ARG(color) };

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 1;
    indices[5] = 3;

    SDL_RenderGeometry(context.renderer, nullptr, vertices, 4, indices, 6);
}

void draw_arrow(const RenderContext& context, melv::vec2 start, melv::vec2 end, float thick, float head_ratio, melv::ColorF color)
{
    melv::vec2 dir = end - start;
    float length = dir.magnitude();
    dir /= length;
    melv::vec2 perp = melv::vec2(-dir.y, dir.x);
    melv::vec2 head_start = start + dir * (1.0f - head_ratio) * length;
    float wide = thick * 1.5;

    draw_segment(context, start, head_start, thick, color);

    SDL_Vertex vertices[3];
    int indices[3];

    vertices[0] = {
        context.transform_sdl_point ( SDL_FPoint { head_start.x + perp.x * wide, head_start.y + perp.y * wide } ),
        SDL_FColor { COLOR_ARG(color) },
    };
    vertices[1] = {
        context.transform_sdl_point ( SDL_FPoint { head_start.x - perp.x * wide, head_start.y - perp.y * wide } ),
        SDL_FColor { COLOR_ARG(color) },
    };
    vertices[2] = {
        context.transform_sdl_point ( SDL_FPoint { end.x, end.y } ),
        SDL_FColor { COLOR_ARG(color) },
    };

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    SDL_RenderGeometry(context.renderer, nullptr, vertices, 3, indices, 3);
}

void draw_arrow(const RenderContext& context, melv::vec2 start, melv::vec2 end, float thickness, melv::ColorF color)
{
    // 3 for the arrow head, 4 for the quadrilateral below
    SDL_Vertex vertices[7];

    for (int i = 0; i < 7; i++) vertices[i].color = SDL_FColor {color.r, color.g, color.b, color.a};

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
    vertices[0].position = context.transform_sdl_point(SDL_FPoint { end.x, end.y });
    vertices[1].position = context.transform_sdl_point(SDL_FPoint { arrow_left.x, arrow_left.y });
    vertices[2].position = context.transform_sdl_point(SDL_FPoint { arrow_right.x, arrow_right.y });

    melv::vec2 upper_base_left = head_start + ortho * base_width;
    melv::vec2 upper_base_right = head_start - ortho * base_width;
    melv::vec2 lower_base_left = upper_base_left - dir * total_length * (1.0 - head_percentage);
    melv::vec2 lower_base_right = upper_base_right - dir * total_length * (1.0 - head_percentage);
    vertices[3].position = context.transform_sdl_point(SDL_FPoint { upper_base_left.x, upper_base_left.y });
    vertices[4].position = context.transform_sdl_point(SDL_FPoint { upper_base_right.x, upper_base_right.y });
    vertices[5].position = context.transform_sdl_point(SDL_FPoint { lower_base_left.x, lower_base_left.y });
    vertices[6].position = context.transform_sdl_point(SDL_FPoint { lower_base_right.x, lower_base_right.y });

    const int indices[9] = {
        0, 1, 2,  // head
        3, 5, 4,
        4, 5, 6
    };

    SDL_RenderGeometry(context.renderer, nullptr, vertices, 7, indices, ARRAY_SIZE(indices));
}

void draw_arc(const RenderContext& context, melv::vec2 center, float inner_radius, float outer_radius, float start_angle, float arc, melv::ColorF color)
{
    // resolution
    #define NVERTICES 64
    SDL_Vertex vertices[NVERTICES];

    // the angle between vertices and it's sin and cos
    const float angle = arc / float(NVERTICES / 2 - 1);
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);

    float xcomp = std::cosf(start_angle);
    float ycomp = std::sinf(start_angle);
    for (int i = 0; i < NVERTICES; i += 2)
    {
        vertices[i + 0].position.x = center.x + xcomp * inner_radius;
        vertices[i + 0].position.y = center.y + ycomp * inner_radius;
        vertices[i + 0].color = SDL_FColor { color.r, color.g, color.b, color.a };

        vertices[i + 1].position.x = center.x + xcomp * outer_radius;
        vertices[i + 1].position.y = center.y + ycomp * outer_radius;
        vertices[i + 1].color = SDL_FColor { color.r, color.g, color.b, color.a };

        vertices[i + 0].position = context.transform_sdl_point(vertices[i + 0].position);
        vertices[i + 1].position = context.transform_sdl_point(vertices[i + 1].position);

        // rotate the vector
        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;
        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    int indices[(NVERTICES / 2 - 1) * 6];
    for (int i = 0; i < NVERTICES - 2; i += 2)
    {
        indices[i * 3 + 0] = i + 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
        indices[i * 3 + 3] = i + 1;
        indices[i * 3 + 4] = i + 3;
        indices[i * 3 + 5] = i + 2;
    }

    SDL_RenderGeometry(context.renderer, NULL, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

void draw_circle_empty(const RenderContext& context, melv::vec2 position, float radius, float thick, melv::ColorF color)
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

void draw_circle(const RenderContext& context, melv::vec2 position, float radius, melv::ColorF color)
{
    draw_circle_with_texture(context, position, radius, nullptr, color);
}

void draw_circle_with_texture(const RenderContext& context, melv::vec2 position, float radius, SDL_Texture* texture, melv::ColorF color)
{
    // change the number of vertices to use to configure how fine of an approximation we get
    #define NVERTICES 32
    SDL_Vertex vertices[NVERTICES + 1];

    SDL_Vertex center;
    center.position = SDL_FPoint { position.x, position.y };
    center.color = SDL_FColor { COLOR_ARG(color) };
    center.tex_coord = SDL_FPoint { 0.5, 0.5 };

    // the angle between vertices and it's sin and cos
    const float angle = CONSTANT_TAU / float(NVERTICES);
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);

    float xcomp = 1.0;
    float ycomp = 0.0;
    for (int i = 1; i <= NVERTICES; i++)
    {
        vertices[i].position.x = center.position.x + xcomp * radius;
        vertices[i].position.y = center.position.y + ycomp * radius;
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };
        vertices[i].tex_coord.x = (xcomp + 1.0f) * 0.5f;
        vertices[i].tex_coord.y = (ycomp + 1.0f) * 0.5f;

        vertices[i].position = context.transform_sdl_point(vertices[i].position);

        // rotate the vector
        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;
        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    // transform the center later so that the perimeter points get calculated according to the original center before being transformed
    vertices[0].position = context.transform_sdl_point(center.position);
    vertices[0].color = center.color;
    vertices[0].tex_coord = center.tex_coord;

    int indices[NVERTICES * 3];
    for (int i = 0; i < NVERTICES - 1; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    indices[(NVERTICES - 1) * 3 + 0] = 0;
    indices[(NVERTICES - 1) * 3 + 1] = NVERTICES;
    indices[(NVERTICES - 1) * 3 + 2] = 1;

    SDL_RenderGeometry(context.renderer, texture, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

void draw_circle_segment(const RenderContext& context, melv::vec2 position, float radius, float start_angle, float angle, melv::ColorF color)
{
    draw_circle_segment_with_texture(context, position, radius, start_angle, angle, nullptr, color);
}

void draw_circle_segment_with_texture(const RenderContext& context, melv::vec2 position, float radius, float start_angle, float angle, SDL_Texture* texture, melv::ColorF color)
{
    // change the number of vertices to use to configure how fine of an approximation we get
    #define NVERTICES 32
    SDL_Vertex vertices[NVERTICES + 1];

    SDL_Vertex center;
    center.position = SDL_FPoint { position.x, position.y};
    center.color = SDL_FColor { COLOR_ARG(color) };
    center.tex_coord = SDL_FPoint { 0.5, 0.5 };

    // the angle between vertices and it's sin and cos
    // if we have n vertices than we have n - 1 gaps between them so divide the angle by the number of gaps to fill
    const float step_angle = angle / float(NVERTICES - 1);
    const float c = std::cosf(step_angle);
    const float s = std::sinf(step_angle);

    float xcomp = std::cosf(start_angle);
    float ycomp = std::sinf(start_angle);
    for (int i = 1; i <= NVERTICES; i++)
    {
        vertices[i].position.x = center.position.x + xcomp * radius;
        vertices[i].position.y = center.position.y + ycomp * radius;
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };
        vertices[i].tex_coord.x = (xcomp + 1.0f) * 0.5f;
        vertices[i].tex_coord.y = (ycomp + 1.0f) * 0.5f;

        vertices[i].position = context.transform_sdl_point(vertices[i].position);

        // rotate the vector
        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;
        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    vertices[0].position = context.transform_sdl_point(center.position);
    vertices[0].color = center.color;
    vertices[0].tex_coord = center.tex_coord;

    int indices[NVERTICES * 3];
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

    SDL_RenderGeometry(context.renderer, texture, vertices, ARRAY_SIZE(vertices), indices, (fullCircle ? NVERTICES : NVERTICES - 1) * 3);
    #undef NVERTICES
}

void draw_capsule(const RenderContext& context, melv::vec2 center0, melv::vec2 center1, float radius, melv::ColorF color)
{
    // total number of vertices used for either half circle sides of the capsule shape
    #define NVERTICES 32
    SDL_Vertex vertices[NVERTICES + 1];

    melv::vec2 midpoint = (center0 + center1) / 2;

    vertices[0].position = { midpoint.x, midpoint.y };
    vertices[0].color = SDL_FColor { COLOR_ARG(color) };

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
        vertices[i].position.x = center0.x + xcomp * radius;
        vertices[i].position.y = center0.y + ycomp * radius;
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };

        vertices[i].position = context.transform_sdl_point(vertices[i].position);

        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;

        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    for (int i = NVERTICES / 2 + 1; i <= NVERTICES; i++)
    {
        vertices[i].position.x = center1.x + xcomp * radius;
        vertices[i].position.y = center1.y + ycomp * radius;
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };

        vertices[i].position = context.transform_sdl_point(vertices[i].position);

        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;

        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    vertices[0].position = context.transform_sdl_point(vertices[0].position);

    int indices[NVERTICES * 3];
    for (int i = 0; i < NVERTICES - 1; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    indices[(NVERTICES - 1) * 3 + 0] = 0;
    indices[(NVERTICES - 1) * 3 + 1] = NVERTICES;
    indices[(NVERTICES - 1) * 3 + 2] = 1;

    SDL_RenderGeometry(context.renderer, NULL, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

// it needs to be a convex polygon and the points need to be in order they are supposed to be rendered in
void draw_polygon(RenderContext& context, melv::vec2 points[], int numPoints, melv::ColorF color)
{
    context.vertex_scratch.ensure_size(numPoints + 1);
    context.index_scratch.ensure_size(numPoints * 3);

    melv::vec2 average = {};
    for (int i = 0; i < numPoints; i++)
    {
        SDL_Vertex vertex = {};
        vertex.position = context.transform_sdl_point({points[i].x, points[i].y});
        vertex.color = SDL_FColor { COLOR_ARG(color) };
        context.vertex_scratch.add(vertex);
        average += points[i];
    }

    average /= numPoints;

    SDL_Vertex vertex = {};
    vertex.position = { average.x, average.y };
    vertex.color = SDL_FColor { COLOR_ARG(color) };
    int averageIndex = context.vertex_scratch.add(vertex);

    for (int i = 0; i < numPoints; i++)
    {
        context.index_scratch.add(averageIndex);
        context.index_scratch.add(i);
        context.index_scratch.add((i + 1) % numPoints);
    }

    SDL_RenderGeometry(context.renderer, nullptr, context.vertex_scratch.data(), context.vertex_scratch.size(), context.index_scratch.data(), context.index_scratch.size());
}

void draw_quad(const RenderContext& context, melv::RectPoints quad, melv::ColorF color)
{
    SDL_Vertex vertex [4];
    vertex[0]     = {
        context.transform_sdl_point(SDL_FPoint { quad.p[0].x, quad.p[0].y }),
        SDL_FColor { color.r, color.g, color.b, color.a },
        SDL_FPoint { 0, 1 }
    };
    vertex[1]    = {
        context.transform_sdl_point(SDL_FPoint { quad.p[1].x, quad.p[1].y }),
        SDL_FColor { color.r, color.g, color.b, color.a },
        SDL_FPoint { 1, 1 }
    };
    vertex[2]  = {
        context.transform_sdl_point(SDL_FPoint { quad.p[2].x, quad.p[2].y }),
        SDL_FColor { color.r, color.g, color.b, color.a },
        SDL_FPoint { 0, 0 }
    };
    vertex[3] = {
        context.transform_sdl_point(SDL_FPoint { quad.p[3].x, quad.p[3].y }),
        SDL_FColor{ color.r, color.g, color.b, color.a },
        SDL_FPoint { 1, 0 }
    };
    int index [6] = {
        0, 3, 1,
        0, 2, 3,
    };

    SDL_RenderGeometry(context.renderer, nullptr, vertex, 4, index, 6);
}

void draw_quad_with_texture(const RenderContext& context, melv::RectPoints quad, SDL_Texture* texture, melv::ColorF color)
{
    SDL_Vertex vertex [4];
    vertex[melv::QuadTopLeft]     = {
		context.transform_sdl_point(SDL_FPoint { quad.p[melv::QuadTopLeft].x, quad.p[melv::QuadTopLeft].y }),
		SDL_FColor { color.r, color.g, color.b, color.a },
		SDL_FPoint { 0, 1 }
	};
    vertex[melv::QuadTopRight]    = {
		context.transform_sdl_point(SDL_FPoint { quad.p[melv::QuadTopRight].x, quad.p[melv::QuadTopRight].y }),
		SDL_FColor { color.r, color.g, color.b, color.a },
		SDL_FPoint { 1, 1 }
	};
    vertex[melv::QuadBottomLeft]  = {
		context.transform_sdl_point(SDL_FPoint { quad.p[melv::QuadBottomLeft].x, quad.p[melv::QuadBottomLeft].y }),
		SDL_FColor { color.r, color.g, color.b, color.a },
		SDL_FPoint { 0, 0 }
	};
    vertex[melv::QuadBottomRight] = {
		context.transform_sdl_point(SDL_FPoint { quad.p[melv::QuadBottomRight].x, quad.p[melv::QuadBottomRight].y }),
		SDL_FColor{ color.r, color.g, color.b, color.a },
		SDL_FPoint { 1, 0 }
	};
    int index [6] = {
        melv::QuadTopLeft, melv::QuadBottomRight, melv::QuadTopRight,
        melv::QuadTopLeft, melv::QuadBottomLeft, melv::QuadBottomRight,
    };

    SDL_RenderGeometry(context.renderer, texture, vertex, 4, index, 6);
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

void render_texture(const RenderContext& render, melv::Rectangle area, Texture* texture, bool stretch)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(texture, &tex_w, &tex_h);
    SDL_FRect src = { 0, 0, tex_w, tex_h };
    float width = stretch ? area.w : tex_w;
    float height = stretch ? area.h : tex_h;
    SDL_FRect dst = { area.x - width / 2, area.y - height / 2, width, height };
    dst = render.transform_sdl_rectangle(dst);
    SDL_RenderTexture(render.renderer, texture, &src, &dst);
}

void render_texture_rotate(const RenderContext& render, melv::Rectangle area, Texture* texture, float angle, Flip flip, bool strech)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(texture, &tex_w, &tex_h);
    SDL_FRect src = { 0, 0, tex_w, tex_h };
    float width = strech ? area.w : tex_w;
    float height = strech ? area.h : tex_h;
    SDL_FRect dst = { area.x - width / 2, area.y - height / 2, width, height };
    dst = render.transform_sdl_rectangle(dst);

    SDL_RenderTextureRotated(render.renderer, texture, &src, &dst, angle * melv::RADIAN_TO_DEGREE_F, nullptr, SDL_FlipMode(flip));
}

void render_textured_rectangle(const RenderContext& render, melv::Rectangle rect, SDL_Texture* texture, melv::Color color, bool strech, bool center) {
    SDL_SetRenderDrawColor(render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
        SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
        SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(render.renderer, &area);

    float tex_w, tex_h;
    SDL_GetTextureSize(texture, &tex_w, &tex_h);
    SDL_FRect src = { 0, 0, tex_w, tex_h};
    float width = strech ? area.w : tex_w;
    float height = strech ? area.h : tex_h;
    SDL_FRect dst = { area.x, area.y, width, height };
    dst = render.transform_sdl_rectangle(dst);
    SDL_RenderTexture(render.renderer, texture, &src, &dst);
}

void render_texture_with_tint(const RenderContext& render, melv::Rectangle area, Texture* texture, melv::ColorF tint, bool strech)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(texture, &tex_w, &tex_h);
    SDL_FRect src = { 0, 0, tex_w, tex_h };
    float width = strech ? area.w : tex_w;
    float height = strech ? area.h : tex_h;
    SDL_FRect dst = { area.x - width / 2, area.y - height / 2, width, height };
    dst = render.transform_sdl_rectangle(dst);

    SDL_SetTextureColorModFloat(texture, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaModFloat(texture, tint.a);
    SDL_RenderTexture(render.renderer, texture, &src, &dst);
}

void draw_quadratic_bezier(const RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, float thick, melv::ColorF color)
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

void draw_cubic_bezier(const RenderContext& context, melv::vec2 p0, melv::vec2 p1, melv::vec2 p2, melv::vec2 p3, float thick, melv::ColorF color)
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
