#include "config.hpp"
#include "draw.hpp"
#include "util/math_util.hpp"
#include "util/common.hpp"
#include "util/log.hpp"
#include "util/file_util.hpp"

#include "bundle/bundle_shaders.h"

#include <SDL3_image/SDL_image.h>

namespace melv
{

    static_assert(sizeof(Vertex) == 32);
    static_assert(sizeof(VertexInstance) == 16);
    static_assert(sizeof(InstanceData) == 32);

    void draw_mesh(RenderContext& render, MeshDraw& mesh);
    void draw_mesh_buffers(RenderContext& render, MeshDraw& mesh, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer);
    void draw_mesh_texture(RenderContext& render, MeshDraw& draw);
    void draw_mesh_texture_buffers(RenderContext& render, MeshDraw& draw, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer);
    void draw_generic(RenderContext& render, GraphicsPipeline& pipeline);

    void draw_shape();

    // to draw quads or point lights
    void draw_quads(RenderContext& render, DrawGroup& group, SDL_GPURenderPass* pass, int instance_buffer);
    void draw_quads_texture(RenderContext& render, DrawGroup& group);

    // failure if the size of the arrays don't match what's expected
    void generate_quad_mesh(DArray<VertexInstance>& out_vertex, DArray<u16>& out_index);
    void generate_circle_mesh(DArray<VertexInstance>& out_vertex, DArray<u16>& out_index);

    bool copy_frame_instance_data(RenderContext& render);
    void upload_frame_instance_data(RenderContext& render);
    bool copy_frame_light_data(RenderContext& render);
    void upload_frame_light_data(RenderContext& render);

    const int RenderTargetWidth = 1440;
    const int RenderTargetHeight = 810;

    void render_present(RenderContext& context, SDL_Window* window) {
        if (!copy_frame_instance_data(context))
        {
            return;
        }

        if (context.doLighting)
        {
            if (!copy_frame_light_data(context))
            {
                return;
            }
        }

        if (!context.get_command_buffer())
        {
            return;
        }

        if (!context.start_copy_pass())
        {
            context.cancel_command_buffer();
            return;
        }

        upload_frame_instance_data(context);

        if (context.doLighting)
        {
            upload_frame_light_data(context);
        }

        context.end_copy_pass();

        if (!context.start_render_pass())
        {
            context.cancel_command_buffer();
            return;
        }

        GraphicsPipeline& pipelineDefault = context.graphics.get(context.graphics_default);
        SDL_BindGPUGraphicsPipeline(context.frame.render_pass, pipelineDefault.pipeline);
        for (MeshDraw& draw : context.frameMeshDraw)
        {
            draw_mesh(context, draw);
        }

        GraphicsPipeline& pipelineTexture = context.graphics.get(context.graphics_texture);
        SDL_BindGPUGraphicsPipeline(context.frame.render_pass, pipelineTexture.pipeline);
        for (MeshDraw& draw : context.frameMeshDrawTex)
        {
            draw_mesh_texture(context, draw);
        }

        GraphicsPipeline& pipelineInstanceTexture = context.graphics.get(context.graphics_instance_texture);
        SDL_BindGPUGraphicsPipeline(context.frame.render_pass, pipelineInstanceTexture.pipeline);
        for (DrawGroup& group : pipelineInstanceTexture.groups)
        {
            draw_quads_texture(context, group);
        }

        for (GraphicsPipeline& pipeline : context.graphics)
        {
            SDL_BindGPUGraphicsPipeline(context.frame.render_pass, pipeline.pipeline);
            // draw_generic(context, pipeline);
        }

        context.end_render_pass();

        SDL_GPUTexture* swapchain = nullptr;
        u32 swapchain_width = 0;
        u32 swapchain_height = 0;
        SDL_WaitAndAcquireGPUSwapchainTexture(context.frame.command_buffer, window, &swapchain, &swapchain_width, &swapchain_height);

        if (!swapchain)
        {
            context.cancel_command_buffer();
            return;
        }

        if (context.doLighting)
        {
            if (!context.start_light_pass())
            {
                context.cancel_command_buffer();
                return;
            }

            DrawGroup lightGroup = {
                TEXTURE_INVALID,
                0,
                context.lights.size(),
                context.lights.size(),
                nullptr,
                MatrixDontUse
            };
            GraphicsPipeline& pipelineLight = context.graphics.get(context.graphics_light);
            SDL_BindGPUGraphicsPipeline(context.frame.light_pass, pipelineLight.pipeline);
            draw_quads(context, lightGroup, context.frame.light_pass, context.light_buffer);

            context.end_light_pass();

            context.copy_to_swapchain(context.light_target, swapchain, swapchain_width, swapchain_height);

            if (false)
            {
                if (!context.start_composition_pass(swapchain))
                {
                    context.cancel_command_buffer();
                    return;
                }

                GraphicsPipeline& pipelineComposition = context.graphics.get(context.graphics_composition);
                SDL_BindGPUGraphicsPipeline(context.frame.light_pass, pipelineComposition.pipeline);
                // @todo

                context.end_composition_pass();
            }
        }
        else
        {
            // copy render target to swapchain
            context.copy_to_swapchain(context.render_target, swapchain, swapchain_width, swapchain_height);
        }

        context.submit_command_buffer();

        for (GPUBuffer& buffer : context.buffers)
        {
            if (buffer.per_frame)
            {
                buffer.used = 0;
            }
        }

        context.frameMeshDraw.discard_data();
        context.instanceData.mark_empty();

        for (GraphicsPipeline& pipeline : context.graphics)
        {
            for (DrawGroup& group : pipeline.groups)
            {
                group.used = 0;
            }
        }
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

    SDL_GPUFence* RenderContext::submit_command_buffer_and_get_fence()
    {
        ASSERT(frame.command_buffer);
        return SDL_SubmitGPUCommandBufferAndAcquireFence(frame.command_buffer);
    }

    void RenderContext::wait_on_fence(SDL_GPUFence* fence)
    {
        SDL_WaitForGPUFences(device, true, &fence, 1);
    }

    void RenderContext::release_fence(SDL_GPUFence* fence)
    {
        SDL_ReleaseGPUFence(device, fence);
    }

    void RenderContext::cancel_command_buffer()
    {
        if (frame.command_buffer)
        {
            SDL_CancelGPUCommandBuffer(frame.command_buffer);
            frame.command_buffer = nullptr;
        }
    }

    DrawGroup& RenderContext::get_draw_group(DrawGroupId id) const
    {
        return graphics.get(id.graphics).groups.get_ref(id.draw);
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

        SDL_GPUDepthStencilTargetInfo depth_stencil_info = {};
        depth_stencil_info.texture = depth_target;               /**< The texture that will be used as the depth stencil target by the render pass. */
        depth_stencil_info.clear_depth = 1;                     /**< The value to clear the depth component to at the beginning of the render pass. Ignored if SDL_GPU_LOADOP_CLEAR is not used. */
        depth_stencil_info.load_op = SDL_GPU_LOADOP_CLEAR;                 /**< What is done with the depth contents at the beginning of the render pass. */
        depth_stencil_info.store_op = SDL_GPU_STOREOP_STORE;               /**< What is done with the depth results of the render pass. */
        depth_stencil_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;         /**< What is done with the stencil contents at the beginning of the render pass. */
        depth_stencil_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;       /**< What is done with the stencil results of the render pass. */
        depth_stencil_info.cycle = true;                            /**< true cycles the texture if the texture is bound and any load ops are not LOAD */
        depth_stencil_info.clear_stencil = 0;                   /**< The value to clear the stencil component to at the beginning of the render pass. Ignored if SDL_GPU_LOADOP_CLEAR is not used. */
        // depth_stencil_info.mip_level;                       /**< The mip level to use as the depth stencil target. */
        // depth_stencil_info.layer;                           /**< The layer index to use as the depth stencil target. */

        render_pass = SDL_BeginGPURenderPass(frame.command_buffer, color_targets, 1, &depth_stencil_info);

        set_mvp(nullptr, MatrixDontUse);

        frame.render_pass = render_pass;
        return render_pass ? true : false;
    }

    void RenderContext::end_render_pass() {
        ASSERT(frame.render_pass);
        SDL_EndGPURenderPass(frame.render_pass);
        frame.render_pass = nullptr;
    }

    bool RenderContext::start_light_pass() {
        SDL_GPURenderPass* light_pass = nullptr;

        SDL_GPUColorTargetInfo color_targets[1] = {};
        color_targets[0].texture = light_target;
        color_targets[0].mip_level = 0;
        color_targets[0].layer_or_depth_plane = 0;
        color_targets[0].clear_color = SDL_FColor { COLOR_ARG(light_clear_color) };
        color_targets[0].load_op = SDL_GPU_LOADOP_CLEAR;
        color_targets[0].store_op = SDL_GPU_STOREOP_STORE;
        color_targets[0].resolve_texture = nullptr;
        color_targets[0].resolve_mip_level = 0;
        color_targets[0].resolve_layer = 0;
        color_targets[0].cycle = true;
        color_targets[0].cycle_resolve_texture = false;

        light_pass = SDL_BeginGPURenderPass(frame.command_buffer, color_targets, 1, nullptr);

        set_mvp(nullptr, MatrixDontUse);

        frame.light_pass = light_pass;
        return light_pass ? true : false;
    }

    void RenderContext::end_light_pass() {
        ASSERT(frame.light_pass);
        SDL_EndGPURenderPass(frame.light_pass);
        frame.light_pass = nullptr;
    }

    bool RenderContext::start_composition_pass(SDL_GPUTexture *swapchain) {
        SDL_GPURenderPass* composition_pass = nullptr;

        // @todo

        SDL_GPUColorTargetInfo color_targets[1] = {};
        color_targets[0].texture = swapchain;
        color_targets[0].mip_level = 0;
        color_targets[0].layer_or_depth_plane = 0;
        color_targets[0].clear_color = SDL_FColor { COLOR_ARG(light_clear_color) };
        color_targets[0].load_op = SDL_GPU_LOADOP_CLEAR;
        color_targets[0].store_op = SDL_GPU_STOREOP_STORE;
        color_targets[0].resolve_texture = nullptr;
        color_targets[0].resolve_mip_level = 0;
        color_targets[0].resolve_layer = 0;
        color_targets[0].cycle = true;
        color_targets[0].cycle_resolve_texture = false;

        composition_pass = SDL_BeginGPURenderPass(frame.command_buffer, color_targets, 1, nullptr);

        set_mvp(nullptr, MatrixDontUse);

        frame.composition_pass = composition_pass;
        return composition_pass ? true : false;
    }

    void RenderContext::end_composition_pass() {
        ASSERT(frame.composition_pass);
        SDL_EndGPURenderPass(frame.composition_pass);
        frame.composition_pass = nullptr;
    }

    void RenderContext::copy_to_swapchain(SDL_GPUTexture *texture, SDL_GPUTexture* swapchain, u32 swapchain_width, u32 swapchain_height)
    {
        SDL_GPUBlitRegion rt_region = {};
        rt_region.texture = texture;
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
        SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, enableGpuDebug, nullptr);
        if (!device)
        {
            log_error("Couldn't create gpu device: %s", SDL_GetError());
            return false;
        }

    #if GRAPHICS_DEBUG
        int driver_count = SDL_GetNumGPUDrivers();
        log_info("Available SDL GPU driver count: %d", driver_count);
        for (int i = 0;  i < driver_count; i++)
        {
            log_info("%s", SDL_GetGPUDriver(i));
        }

        log_info("Created a device using: %s", SDL_GetGPUDeviceDriver(device));
    #endif // GRAPHICS_DEBUG

        if (!SDL_ClaimWindowForGPUDevice(device, window))
        {
            log_error("Couldn't claim gpu device");
            return false;
        }

        int render_size_x, render_size_y;
        SDL_GetWindowSize(window, &render_size_x, &render_size_y);

        render->device = device;
        render->render_size = melv::vec2(render_size_x, render_size_y);

        melv::mat4x4 orthographic = melv::orthographic_projection_matrix(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
        melv::mat4x4 camera = melv::camera_matrix(melv::vec2(0, 0), melv::vec2(1,1));
        mat4mul(&render->mvp, &orthographic, &camera);

        return true;
    }

    GraphicsPipelineParameters get_default_graphics_pipeline_parameters()
    {
        GraphicsPipelineParameters parameters = {};
        parameters.format = RenderFormat;
        parameters.depth_format = SDL_GPU_TEXTUREFORMAT_INVALID;
        parameters.input = InputVertex;
        return parameters;
    }

    SDL_GPUGraphicsPipeline* create_gpu_graphics_pipeline(GraphicsPipelineParameters* parameters, RenderContext* render, SDL_GPUShader* vertex, SDL_GPUShader* fragment)
    {
        SDL_GPUVertexInputState vertex_input = {};

        SDL_GPUVertexBufferDescription vertex_buffer_description[VBufferDescriptionCountMax] = {};
        SDL_GPUVertexAttribute vertex_attributes[InputAttributeCountMax] = {};

        if (parameters->input == InputVertex)
        {
            vertex_buffer_description[0].slot = 0;                        /**< The binding slot of the vertex buffer. */
            vertex_buffer_description[0].pitch = sizeof(Vertex);                       /**< The size of a single element + the offset between elements. */
            vertex_buffer_description[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;  /**< Whether attribute addressing is a function of the vertex index or instance index. */

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

           vertex_input = {
                vertex_buffer_description,  /**< A pointer to an array of vertex buffer descriptions. */
                VBufferDescriptionCountVertex,                          /**< The number of vertex buffer descriptions in the above array. */
                vertex_attributes,                   /**< A pointer to an array of vertex attribute descriptions. */
                InputAttributeCountVertex                          /**< The number of vertex attribute descriptions in the above array. */
            };
        }
        else if (parameters->input == InputInstance)
        {
            vertex_buffer_description[0].slot = 0;
            vertex_buffer_description[0].pitch = sizeof(VertexInstance);
            vertex_buffer_description[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

            vertex_buffer_description[1].slot = 1;
            vertex_buffer_description[1].pitch = sizeof(InstanceData);
            vertex_buffer_description[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;

            // vertex position
            vertex_attributes[0].location = 0;
            vertex_attributes[0].buffer_slot = 0;
            vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            vertex_attributes[0].offset = 0;

            // uv
            vertex_attributes[1].location = 1;
            vertex_attributes[1].buffer_slot = 0;
            vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            vertex_attributes[1].offset = sizeof(float) * 2;

            // instance position
            vertex_attributes[2].location = 2;
            vertex_attributes[2].buffer_slot = 1;
            vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            vertex_attributes[2].offset = 0;

            // rotation
            vertex_attributes[3].location = 3;
            vertex_attributes[3].buffer_slot = 1;
            vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
            vertex_attributes[3].offset = OFFSETOF(InstanceData, rotation);

            // scale
            vertex_attributes[4].location = 4;
            vertex_attributes[4].buffer_slot = 1;
            vertex_attributes[4].format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM;
            vertex_attributes[4].offset = OFFSETOF(InstanceData, scale);

            // color
            vertex_attributes[5].location = 5;
            vertex_attributes[5].buffer_slot = 1;
            vertex_attributes[5].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
            vertex_attributes[5].offset = OFFSETOF(InstanceData, color);

            // source offset
            vertex_attributes[6].location = 6;
            vertex_attributes[6].buffer_slot = 1;
            vertex_attributes[6].format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM;
            vertex_attributes[6].offset = OFFSETOF(InstanceData, sourceOffset);

            // source scale
            vertex_attributes[7].location = 7;
            vertex_attributes[7].buffer_slot = 1;
            vertex_attributes[7].format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM;
            vertex_attributes[7].offset = OFFSETOF(InstanceData, sourceScale);

            vertex_input = {
                vertex_buffer_description,  /**< A pointer to an array of vertex buffer descriptions. */
                VBufferDescriptionCountInstance,                          /**< The number of vertex buffer descriptions in the above array. */
                vertex_attributes,                   /**< A pointer to an array of vertex attribute descriptions. */
                InputAttributeCountInstance                          /**< The number of vertex attribute descriptions in the above array. */
            };
        }
        else if (parameters->input == InputLight)
        {
            vertex_buffer_description[0].slot = 0;
            vertex_buffer_description[0].pitch = sizeof(VertexInstance);
            vertex_buffer_description[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

            vertex_buffer_description[1].slot = 1;
            vertex_buffer_description[1].pitch = sizeof(PointLight);
            vertex_buffer_description[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;

            // vertex position
            vertex_attributes[0].location = 0;
            vertex_attributes[0].buffer_slot = 0;
            vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            vertex_attributes[0].offset = 0;

            // @todo we don't actually need them for lights
            // uv
            vertex_attributes[1].location = 1;
            vertex_attributes[1].buffer_slot = 0;
            vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            vertex_attributes[1].offset = sizeof(float) * 2;

            // light position
            vertex_attributes[2].location = 2;
            vertex_attributes[2].buffer_slot = 1;
            vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            vertex_attributes[2].offset = 0;

            // radius
            vertex_attributes[3].location = 3;
            vertex_attributes[3].buffer_slot = 1;
            vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
            vertex_attributes[3].offset = OFFSETOF(PointLight, radius);

            // brightness
            vertex_attributes[4].location = 4;
            vertex_attributes[4].buffer_slot = 1;
            vertex_attributes[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
            vertex_attributes[4].offset = OFFSETOF(PointLight, brightness);

            // color
            vertex_attributes[5].location = 5;
            vertex_attributes[5].buffer_slot = 1;
            vertex_attributes[5].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
            vertex_attributes[5].offset = OFFSETOF(PointLight, color);

            vertex_input = {
                vertex_buffer_description,  /**< A pointer to an array of vertex buffer descriptions. */
                VBufferDescriptionCountLight,                          /**< The number of vertex buffer descriptions in the above array. */
                vertex_attributes,                   /**< A pointer to an array of vertex attribute descriptions. */
                InputAttributeCountLight                          /**< The number of vertex attribute descriptions in the above array. */
            };
        }
        else
        {
            panic("Invalid graphics pipeline vertex input format");
        }

        SDL_GPURasterizerState rasterizer = {};
        rasterizer.fill_mode = SDL_GPU_FILLMODE_FILL;         /**< Whether polygons will be filled in or drawn as lines. */
        rasterizer.cull_mode = SDL_GPU_CULLMODE_BACK;         /**< The facing direction in which triangles will be culled. */
        rasterizer.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;       /**< The vertex winding that will cause a triangle to be determined as front-facing. */
        // rasterizer.depth_bias_constant_factor;  /**< A scalar factor controlling the depth value added to each fragment. */
        // rasterizer.depth_bias_clamp;            /**< The maximum depth bias of a fragment. */
        // rasterizer.depth_bias_slope_factor;     /**< A scalar factor applied to a fragment's slope in depth calculations. */
        // rasterizer.enable_depth_bias;            /**< true to bias fragment depth values. */
        rasterizer.enable_depth_clip = true;            /**< true to enable depth clip, false to enable depth clamp. */

        // @todo add this to parameters
        SDL_GPUMultisampleState multisample = {};
        multisample.sample_count = SDL_GPU_SAMPLECOUNT_1;  /**< The number of samples to be used in rasterization. */
        multisample.sample_mask = 0;               /**< Reserved for future use. Must be set to 0. */
        multisample.enable_mask = false;                 /**< Reserved for future use. Must be set to false. */
        multisample.enable_alpha_to_coverage = false;    /**< true enables the alpha-to-coverage feature. */

        SDL_GPUDepthStencilState stencil = {};
        // equal so that we do blending if their depths are equal
        stencil.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        stencil.enable_depth_test = true;                     /**< true enables the depth test. */
        stencil.enable_depth_write = true;                    /**< true enables depth writes. Depth writes are always disabled when enable_depth_test is false. */
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
        target_info.depth_stencil_format = parameters->depth_format;
        target_info.has_depth_stencil_target = (parameters->depth_format != SDL_GPU_TEXTUREFORMAT_INVALID);

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

    bool init_gpu_renderer(RenderContext* render, RenderInitConfig* conf, SDL_Window* window)
    {
        DefaultShaders shaders = {};
        if (!create_default_shaders(render->device, &shaders))
        {
            log_error("Couldn't create default shaders");
            return false;
        }

        GraphicsPipelineParameters pipeline_parameters = get_default_graphics_pipeline_parameters();
        GraphicsPipelineParameters pipeline_parameters_instance = get_default_graphics_pipeline_parameters();
        GraphicsPipelineParameters pipeline_parameters_light = get_default_graphics_pipeline_parameters();

        pipeline_parameters.depth_format = DepthFormat;
        pipeline_parameters_instance.depth_format = DepthFormat;

        pipeline_parameters_instance.input = InputInstance;
        pipeline_parameters_light.input = InputLight;

        SDL_GPUGraphicsPipeline* pipeline = create_gpu_graphics_pipeline(&pipeline_parameters, render, shaders.vertex, shaders.fragment);
        if (!pipeline) {
            log_error("Failed to create graphics pipeline: %s", SDL_GetError());
            return false;
        }

        SDL_GPUGraphicsPipeline* pipeline_texture = create_gpu_graphics_pipeline(&pipeline_parameters, render, shaders.vertex, shaders.fragmentTexture);
        if (!pipeline_texture)
        {
            log_error("Failed to create graphics pipeline: %s", SDL_GetError());
            return false;
        }

        SDL_GPUGraphicsPipeline* pipeline_instance_texture = create_gpu_graphics_pipeline(&pipeline_parameters_instance, render, shaders.vertexInstance, shaders.fragmentTexture);
        if (!pipeline_instance_texture)
        {
            log_error("Failed to create graphics pipeline: %s", SDL_GetError());
            return false;
        }

        SDL_GPUGraphicsPipeline* pipeline_light = nullptr;
        if (conf->doLights)
        {
            pipeline_light = create_gpu_graphics_pipeline(&pipeline_parameters_light, render, shaders.vertexLight, shaders.fragmentLight);
            if (!pipeline_light)
            {
                log_error("Failed to create graphics pipeline: %s", SDL_GetError());
                return false;
            }
        }

        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = InitTransferBufferSize;
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(render->device, &transferInfo);
        SDL_GPUTransferBuffer* group_transfer_buffer = SDL_CreateGPUTransferBuffer(render->device, &transferInfo);
        SDL_GPUTransferBuffer* light_transfer_buffer = SDL_CreateGPUTransferBuffer(render->device, &transferInfo);

        if (!(transfer_buffer && group_transfer_buffer && light_transfer_buffer))
        {
            log_error("Couldn't create transfer buffer");
            return false;
        }

        SDL_GPUBufferCreateInfo vertexBufferCI = { SDL_GPU_BUFFERUSAGE_VERTEX, InitVertexBufferSize };
        SDL_GPUBufferCreateInfo indexBufferCI = { SDL_GPU_BUFFERUSAGE_INDEX, InitIndexBufferSize };
        SDL_GPUBufferCreateInfo instanceBufferCI = { SDL_GPU_BUFFERUSAGE_VERTEX, InitInstanceBufferSize };
        SDL_GPUBufferCreateInfo lightBufferCI = { SDL_GPU_BUFFERUSAGE_VERTEX, InitInstanceBufferSize };

        SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(render->device, &vertexBufferCI);
        SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(render->device, &indexBufferCI);
        SDL_GPUBuffer* instance_buffer = SDL_CreateGPUBuffer(render->device, &instanceBufferCI);
        SDL_GPUBuffer *light_buffer = nullptr;
        if (conf->doLights)
        {
            light_buffer = SDL_CreateGPUBuffer(render->device, &lightBufferCI);
        }

        if (!(vertex_buffer && index_buffer && instance_buffer))
        {
            log_error("Couldn't create vertex buffer");
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

        SDL_GPUTexture *render_target = nullptr;
        {
            // to suppress a d3d12 warning
            // it will still be there if the user overwrites this value though
            SDL_PropertiesID renderTargetProperties = SDL_CreateProperties();
            SDL_SetFloatProperty(renderTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_R_FLOAT", ClearColorDefault.r);
            SDL_SetFloatProperty(renderTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_G_FLOAT", ClearColorDefault.g);
            SDL_SetFloatProperty(renderTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_B_FLOAT", ClearColorDefault.b);
            SDL_SetFloatProperty(renderTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_A_FLOAT", ClearColorDefault.a);

            SDL_GPUTextureCreateInfo renderTargetCI = {};
            renderTargetCI.type = SDL_GPU_TEXTURETYPE_2D;
            renderTargetCI.format = RenderFormat;
            renderTargetCI.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
            renderTargetCI.width = RenderTargetWidth;
            renderTargetCI.height = RenderTargetHeight;
            renderTargetCI.layer_count_or_depth = 1;
            renderTargetCI.num_levels = 1;
            renderTargetCI.sample_count = SDL_GPU_SAMPLECOUNT_1;
            render_target = SDL_CreateGPUTexture(render->device, &renderTargetCI);

            SDL_DestroyProperties(renderTargetProperties);
        }

        if (!render_target)
        {
            log_error("Couldn't create render target: %s", SDL_GetError());
            return false;
        }

        SDL_GPUTexture *depth_target = nullptr;
        {
            // to suppress a d3d12 warning
            SDL_PropertiesID depthTargetProperties = SDL_CreateProperties();
            SDL_SetFloatProperty(depthTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT", 1);

            SDL_GPUTextureCreateInfo depthTargetCI = {};
            depthTargetCI.type = SDL_GPU_TEXTURETYPE_2D;
            depthTargetCI.format = DepthFormat; // D16
            depthTargetCI.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
            depthTargetCI.width = RenderTargetWidth;
            depthTargetCI.height = RenderTargetHeight;
            depthTargetCI.layer_count_or_depth = 1;
            depthTargetCI.num_levels = 1;
            depthTargetCI.sample_count = SDL_GPU_SAMPLECOUNT_1;
            depth_target = SDL_CreateGPUTexture(render->device, &depthTargetCI);

            SDL_DestroyProperties(depthTargetProperties);
        }

        if (!depth_target)
        {
            log_error("Couldn't create depth target: %s", SDL_GetError());
            return false;
        }

        SDL_GPUTexture* light_target = nullptr;
        if (conf->doLights)
        {
            // to suppress a d3d12 warning
            // it will still be there if the user overwrites this value though
            SDL_PropertiesID lightTargetProperties = SDL_CreateProperties();
            SDL_SetFloatProperty(lightTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_R_FLOAT", LightClearColorDefault.r);
            SDL_SetFloatProperty(lightTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_G_FLOAT", LightClearColorDefault.g);
            SDL_SetFloatProperty(lightTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_B_FLOAT", LightClearColorDefault.b);
            SDL_SetFloatProperty(lightTargetProperties, "SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_A_FLOAT", LightClearColorDefault.a);

            SDL_GPUTextureCreateInfo lightTargetCI = {};
            lightTargetCI.type = SDL_GPU_TEXTURETYPE_2D;
            lightTargetCI.format = RenderFormat;
            lightTargetCI.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
            lightTargetCI.width = RenderTargetWidth;
            lightTargetCI.height = RenderTargetHeight;
            lightTargetCI.layer_count_or_depth = 1;
            lightTargetCI.num_levels = 1;
            lightTargetCI.sample_count = SDL_GPU_SAMPLECOUNT_1;
            light_target = SDL_CreateGPUTexture(render->device, &lightTargetCI);
            if (!light_target)
            {
                log_error("Couldn't create light target: %s", SDL_GetError());
                return false;
            }

            SDL_DestroyProperties(lightTargetProperties);
        }

        render->clear_color = ClearColorDefault;
        render->light_clear_color = LightClearColorDefault;

        render->graphics_default = render->graphics.add(GraphicsPipeline(pipeline_parameters, pipeline, true, true));
        render->graphics_texture = render->graphics.add(GraphicsPipeline(pipeline_parameters, pipeline_texture, true, true));
        render->graphics_instance_texture = render->graphics.add(GraphicsPipeline(pipeline_parameters_instance, pipeline_instance_texture, true, true));
        render->graphics_light = render->graphics.add(GraphicsPipeline(pipeline_parameters_light, pipeline_light, true, true));
        render->vertex_buffer = render->buffers.add({ vertex_buffer, GPUBufferVertex, InitVertexBufferSize, 0 });
        render->index_buffer = render->buffers.add({ index_buffer, GPUBufferIndex, InitIndexBufferSize, 0 });
        render->instance_buffer = render->buffers.add({ instance_buffer, GPUBufferVertex, InitInstanceBufferSize, 0 });
        render->light_buffer = render->buffers.add({ light_buffer, GPUBufferVertex, InitInstanceBufferSize, 0 });
        render->render_target = render_target;
        render->light_target = light_target;
        render->depth_target = depth_target;
        render->sampler = sampler;
        render->transfer_buffer = { transfer_buffer, transferInfo.size };
        render->group_transfer_buffer = { group_transfer_buffer, transferInfo.size };
        render->light_transfer_buffer = { light_transfer_buffer, transferInfo.size };
        render->doLighting = conf->doLights;

        if (!render->upload_common_mesh_data())
        {
            log_error("Couldn't upload mesh data: %s", SDL_GetError());
            return false;
        }

        destroy_default_shaders(render->device, &shaders);

        return true;
    }

    bool create_default_shaders(SDL_GPUDevice* device, DefaultShaders* shaders)
    {
        SDL_GPUShaderCreateInfo vertexInfo = {};
        SDL_GPUShaderCreateInfo vertexInstanceInfo = {};
        SDL_GPUShaderCreateInfo vertexLightInfo = {};
        SDL_GPUShaderCreateInfo fragmentInfo = {};
        SDL_GPUShaderCreateInfo fragmentTextureInfo = {};
        SDL_GPUShaderCreateInfo fragmentLightInfo = {};

        vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        vertexInstanceInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        vertexLightInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
        fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        fragmentTextureInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        fragmentLightInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

        vertexInfo.entrypoint = "main";
        vertexInstanceInfo.entrypoint = "main";
        vertexLightInfo.entrypoint = "main";
        fragmentInfo.entrypoint = "main";
        fragmentTextureInfo.entrypoint = "main";
        fragmentLightInfo.entrypoint = "main";

        // mvp
        vertexInfo.num_uniform_buffers = 1;
        vertexInstanceInfo.num_uniform_buffers = 1;
        vertexLightInfo.num_uniform_buffers = 1;

        fragmentTextureInfo.num_storage_textures = 1;
        fragmentTextureInfo.num_samplers = 1;

        // @todo other shader formats

        SDL_GPUShaderFormat shaderFormat = SDL_GetGPUShaderFormats(device);
        if (shaderFormat & SDL_GPU_SHADERFORMAT_DXIL)
        {
            vertexInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
            vertexInstanceInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
            vertexLightInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
            fragmentInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
            fragmentTextureInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
            fragmentLightInfo.format = SDL_GPU_SHADERFORMAT_DXIL;

            vertexInfo.code_size = vertex_dxil_len;
            vertexInfo.code = vertex_dxil;
            vertexInstanceInfo.code_size = vertex_instance_dxil_len;
            vertexInstanceInfo.code = vertex_instance_dxil;
            vertexLightInfo.code_size = vertex_light_dxil_len;
            vertexLightInfo.code = vertex_light_dxil;
            fragmentInfo.code_size = fragment_dxil_len;
            fragmentInfo.code = fragment_dxil;
            fragmentTextureInfo.code_size = fragment_texture_dxil_len;
            fragmentTextureInfo.code = fragment_texture_dxil;
            fragmentLightInfo.code_size = fragment_light_dxil_len;
            fragmentLightInfo.code = fragment_light_dxil;
        }
        else if (shaderFormat & SDL_GPU_SHADERFORMAT_SPIRV)
        {
            vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            vertexInstanceInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            vertexLightInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            fragmentTextureInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            fragmentLightInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;

            vertexInfo.code_size = vertex_spv_len;
            vertexInfo.code = vertex_spv;
            vertexInstanceInfo.code_size = vertex_instance_spv_len;
            vertexInstanceInfo.code = vertex_instance_spv;
            vertexLightInfo.code_size = vertex_light_spv_len;
            vertexLightInfo.code = vertex_light_spv;
            fragmentInfo.code_size = fragment_spv_len;
            fragmentInfo.code = fragment_spv;
            fragmentTextureInfo.code_size = fragment_texture_spv_len;
            fragmentTextureInfo.code = fragment_texture_spv;
            fragmentLightInfo.code_size = fragment_light_spv_len;
            fragmentLightInfo.code = fragment_light_spv;
        }
        else if (shaderFormat & SDL_GPU_SHADERFORMAT_MSL)
        {
            vertexInfo.format = SDL_GPU_SHADERFORMAT_MSL;
            vertexInstanceInfo.format = SDL_GPU_SHADERFORMAT_MSL;
            vertexLightInfo.format = SDL_GPU_SHADERFORMAT_MSL;
            fragmentInfo.format = SDL_GPU_SHADERFORMAT_MSL;
            fragmentTextureInfo.format = SDL_GPU_SHADERFORMAT_MSL;
            fragmentLightInfo.format = SDL_GPU_SHADERFORMAT_MSL;

            vertexInfo.code_size = vertex_msl_len;
            vertexInfo.code = vertex_msl;
            vertexInstanceInfo.code_size = vertex_instance_msl_len;
            vertexInstanceInfo.code = vertex_instance_msl;
            vertexLightInfo.code_size = vertex_light_msl_len;
            vertexLightInfo.code = vertex_light_msl;
            fragmentInfo.code_size = fragment_msl_len;
            fragmentInfo.code = fragment_msl;
            fragmentTextureInfo.code_size = fragment_texture_msl_len;
            fragmentTextureInfo.code = fragment_texture_msl;
            fragmentLightInfo.code_size = fragment_light_msl_len;
            fragmentLightInfo.code = fragment_light_msl;
        }
        else
        {
            log_info("No supported shader formats");
            return false;
        }

        SDL_GPUShader *vertex = SDL_CreateGPUShader(device, &vertexInfo);
        SDL_GPUShader *vertex_instance = SDL_CreateGPUShader(device, &vertexInstanceInfo);
        SDL_GPUShader *vertex_light = SDL_CreateGPUShader(device, &vertexLightInfo);
        SDL_GPUShader *fragment = SDL_CreateGPUShader(device, &fragmentInfo);
        SDL_GPUShader *fragment_texture = SDL_CreateGPUShader(device, &fragmentTextureInfo);
        SDL_GPUShader *fragment_light = SDL_CreateGPUShader(device, &fragmentLightInfo);

        if (!(vertex && vertex_instance && fragment && fragment_texture && vertex_light && fragment_light))
        {
            log_error("%s", SDL_GetError());
            return false;
        }

        shaders->vertex = vertex;
        shaders->vertexInstance = vertex_instance;
        shaders->fragment = fragment;
        shaders->fragmentTexture = fragment_texture;
        shaders->vertexLight = vertex_light;
        shaders->fragmentLight = fragment_light;

        return true;
    }

    void destroy_default_shaders(SDL_GPUDevice* device, DefaultShaders* shaders)
    {
        SDL_ReleaseGPUShader(device, shaders->vertex);
        SDL_ReleaseGPUShader(device, shaders->vertexInstance);
        SDL_ReleaseGPUShader(device, shaders->fragment);
        SDL_ReleaseGPUShader(device, shaders->fragmentTexture);
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

    void RenderContext::set_mvp(mat4x4* mat, DrawMatrixUsage usage)
    {
        switch (usage)
        {
            case MatrixDontUse:
            {
                float half_width = RenderTargetWidth/2;
                float half_height = RenderTargetHeight/2;
                melv::mat4x4 orthographic = melv::orthographic_projection_matrix(-half_width, half_width,
                                                                                 -half_height, half_height,
                                                                                  0, 1);

                vec2 cpos = camera ? camera->position : vec2(0,0);
                vec2 cscale = camera ? vec2(camera->zoom, camera->zoom) : vec2(1,1);

                melv::mat4x4 cameraMatrix = melv::camera_matrix(cpos, cscale);
                mat4mul(&mvp, &orthographic, &cameraMatrix);

                SDL_PushGPUVertexUniformData(frame.command_buffer, 0, &mvp, sizeof(melv::mat4x4));
                break;
            }
            case MatrixIsModel:
            {
                float half_width = RenderTargetWidth/2;
                float half_height = RenderTargetHeight/2;
                melv::mat4x4 orthographic = melv::orthographic_projection_matrix(-half_width, half_width,
                                                                                 -half_height, half_height,
                                                                                  0, 1);

                vec2 cpos = camera ? camera->position : vec2(0,0);
                vec2 cscale = camera ? vec2(camera->zoom, camera->zoom) : vec2(1,1);

                melv::mat4x4 cameraMatrix = melv::camera_matrix(cpos, cscale);
                mat4mul(&mvp, &orthographic, &cameraMatrix);

                mat4mul(&mvp, mat, &mvp);

                SDL_PushGPUVertexUniformData(frame.command_buffer, 0, &mvp, sizeof(melv::mat4x4));
                break;
            }
            case MatrixIsMVP:
            {
                SDL_PushGPUVertexUniformData(frame.command_buffer, 0, mat, sizeof(melv::mat4x4));
                break;
            }
        }
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

        return buffers.add(buffer);
    }

    bool RenderContext::upload_common_mesh_data()
    {
        DArray<VertexInstance> quad = {};
        DArray<u16> quad_indices = {};
        DArray<VertexInstance> circle = {};
        DArray<u16> circle_indices = {};

        MeshReference mesh[ShapeCount] = {};

        generate_quad_mesh(quad, quad_indices);
        generate_circle_mesh(circle, circle_indices);

        mesh[ShapeQuad].vertex_count = quad.size();
        mesh[ShapeQuad].index_count = quad_indices.size();
        mesh[ShapeCircle].vertex_count = circle.size();
        mesh[ShapeCircle].index_count = circle_indices.size();

        size_t offset = 0;
        size_t vertex_offset = 0;
        size_t index_offset = 0;
        u8 *memory = (u8*) SDL_MapGPUTransferBuffer(device, transfer_buffer.buffer, false);

        // vertex
        mesh[ShapeQuad].vertex_offset = vertex_offset;
        auto size = sizeof(VertexInstance) * quad.size();
        memcpy(memory + offset, quad.data(), size);
        offset += size;
        vertex_offset += size;

        mesh[ShapeCircle].vertex_offset = vertex_offset;
        size = sizeof(VertexInstance) * circle.size();
        memcpy(memory + offset, circle.data(), size);
        offset += size;
        vertex_offset += size;

        // index
        mesh[ShapeQuad].index_offset = index_offset;
        size = sizeof(u16) * quad_indices.size();
        memcpy(memory + offset, quad_indices.data(), size);
        offset += size;
        index_offset += size;

        mesh[ShapeCircle].index_offset = index_offset;
        size = sizeof(u16) * circle_indices.size();
        memcpy(memory + offset, circle_indices.data(), size);
        offset += size;
        index_offset += size;

        SDL_UnmapGPUTransferBuffer(device, transfer_buffer.buffer);

        if (!get_command_buffer())
        {
            log_error("Couldn't get command buffer to upload common mesh data: %s", SDL_GetError());
            return false;
        }

        if (!start_copy_pass())
        {
            log_error("Couldn't start a copy pass to upload common mesh data: %s", SDL_GetError());
            return false;
        }

        {
            SDL_GPUTransferBufferLocation source = {};
            source.transfer_buffer = transfer_buffer.buffer;
            source.offset = 0;

            SDL_GPUBufferRegion destination = {};
            destination.buffer = buffers[vertex_buffer].buffer;
            destination.offset = 0;
            destination.size = vertex_offset;
            SDL_UploadToGPUBuffer(frame.copy_pass, &source, &destination, false);
            buffers[vertex_buffer].used += vertex_offset;
        }

        {
            SDL_GPUTransferBufferLocation source = {};
            source.transfer_buffer = transfer_buffer.buffer;
            source.offset = vertex_offset;

            SDL_GPUBufferRegion destination = {};
            destination.buffer = buffers[index_buffer].buffer;
            destination.offset = 0;
            destination.size = index_offset;
            SDL_UploadToGPUBuffer(frame.copy_pass, &source, &destination, false);
            buffers[index_buffer].used += index_offset;
        }

        end_copy_pass();
        submit_command_buffer();

        quad.reset();
        quad_indices.reset();
        circle.reset();
        circle_indices.reset();

        for (int i = 0; i < ShapeCount; i++)
        {
            mesh_common[i] = mesh[i];
        }

        return true;
    }

    bool RenderContext::load_gpu_texture(const char* path, Texture& texture)
    {
        if (!(frame.command_buffer && frame.copy_pass))
        {
            return false;
        }

        // reference: SDL_image/src/IMG_gpu.c/LoadGPUTexture
        SDL_Surface *surface = IMG_Load(path);
        if (!surface)
        {
            return false;
        }

        int width = surface->w;
        int height = surface->h;

        const TextureFormat StandardTextureFormat = RenderFormat;
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.format = StandardTextureFormat;
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = texture.mip_levels;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width = width;
        textureInfo.height = height;
        SDL_GPUTexture *ptr = SDL_CreateGPUTexture(device, &textureInfo);
        if (!ptr)
        {
            SDL_DestroySurface(surface);
            return false;
        }

        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.size = width * height * 4; // @Hardcode depends on RenderFormat
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
        if (!transfer)
        {
            SDL_DestroySurface(surface);
            SDL_ReleaseGPUTexture(device, ptr);
            return false;
        }

        u8* dst = (u8*) SDL_MapGPUTransferBuffer(device, transfer, false);
        if (!dst)
        {
            SDL_DestroySurface(surface);
            SDL_ReleaseGPUTexture(device, ptr);
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            return false;
        }

        const u8* src = (u8*) surface->pixels;
        const int row_bytes = width * 4;
        if (row_bytes == surface->pitch)
        {
            std::memcpy(dst, src, row_bytes * height);
        }
        else
        {
            for (int y = 0; y < height; y++)
            {
                std::memcpy(dst + y * row_bytes, src + y * surface->pitch, row_bytes);
            }
        }
        SDL_UnmapGPUTransferBuffer(device, transfer);

        SDL_GPUTextureTransferInfo texture_transfer_info = {};
        SDL_GPUTextureRegion texture_region = {};
        texture_transfer_info.transfer_buffer = transfer;
        texture_region.texture = ptr;
        texture_region.w = width;
        texture_region.h = height;
        texture_region.d = 1;
        SDL_UploadToGPUTexture(frame.copy_pass, &texture_transfer_info, &texture_region, false);

        SDL_DestroySurface(surface);
        SDL_ReleaseGPUTransferBuffer(device, transfer);

        GPUTexture tex = {};
        tex.texture = ptr;
        tex.width = width;
        tex.height = height;
        tex.format = textureInfo.format;
        tex.sampler = 0; // @todo @Hardcode

        texture.index = textures.add(tex);
        return true;
    }

    size_t RenderContext::calculate_resource_video_memory_usage() const
    {
        size_t sum = 0;

        for (auto& buffer : buffers)
        {
            sum += buffer.size;
        }

        size_t render_texture_size = SDL_CalculateGPUTextureFormatSize(RenderFormat, RenderTargetWidth, RenderTargetHeight, 1);
        size_t light_texture_size = 0; // SDL_CalculateGPUTextureFormatSize(); // @todo
        size_t depth_texture_size = SDL_CalculateGPUTextureFormatSize(DepthFormat, RenderTargetWidth, RenderTargetHeight, 1);
        sum += render_texture_size;
        sum += light_texture_size;
        sum += depth_texture_size;

        for (auto& texture : textures)
        {
            sum += SDL_CalculateGPUTextureFormatSize(texture.format, texture.width, texture.height, 1);
        }

        return sum;
    }

    bool RenderContext::resize_transfer_buffer(TransferBuffer& buffer, u32 nsize)
    {
        if (buffer.size < nsize)
        {
            SDL_GPUTransferBufferCreateInfo transferInfo = {};
            transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferInfo.size = nsize;
            SDL_GPUTransferBuffer* tbuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

            if (!tbuffer)
            {
                return false;
            }

            SDL_ReleaseGPUTransferBuffer(device, buffer.buffer);
            buffer.buffer = tbuffer;
            buffer.size = transferInfo.size;
        }

        return true;
    }

    bool RenderContext::resize_gpu_buffer(GPUBuffer& buffer, u32 nsize)
    {
        if (buffer.size < nsize)
        {
    #if GRAPHICS_DEBUG
            log_info("Resize GPU buffer");
    #endif
            SDL_GPUBufferCreateInfo ci = { buffer.usage, nsize };
            SDL_GPUBuffer *gbuffer = SDL_CreateGPUBuffer(device, &ci);
            if (!gbuffer)
            {
                return false;
            }

            SDL_ReleaseGPUBuffer(device, buffer.buffer);
            buffer.buffer = gbuffer;
            buffer.size = ci.size;
            buffer.used = 0;
        }

        return true;
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

        u8* memory = (u8*) SDL_MapGPUTransferBuffer(context.device, context.transfer_buffer.buffer, false);
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

    bool copy_frame_instance_data(RenderContext& render)
    {
        InstanceData* memory = (InstanceData*) SDL_MapGPUTransferBuffer(render.device, render.group_transfer_buffer.buffer, false);
        ASSERT(memory);
        for (GraphicsPipeline& pipeline : render.graphics)
        {
            if (pipeline.frame_data)
            {
                for (DrawGroup& group : pipeline.groups)
                {
                    for (int i = group.offset; i < group.offset + group.used; i++)
                    {
                        memory[i] = render.instanceData[i];
                    }
                    for (int i = group.offset + group.used; i < group.offset + group.capacity; i++)
                    {
                        memory[i] = {};
                    }
                }
            }
        }
        SDL_UnmapGPUTransferBuffer(render.device, render.group_transfer_buffer.buffer);

        return true;
    }

    void upload_frame_instance_data(RenderContext& render)
    {
        if (render.instanceData.size() > 0)
        {
            SDL_GPUTransferBufferLocation source = {};
            SDL_GPUBufferRegion destination = {};

            source.transfer_buffer = render.group_transfer_buffer.buffer;
            source.offset = 0;

            destination.buffer = render.buffers[render.instance_buffer].buffer;
            destination.offset = 0;
            destination.size = render.instanceData.size() * sizeof(InstanceData);

            SDL_UploadToGPUBuffer(render.frame.copy_pass, &source, &destination, false);
        }
    }

    bool copy_frame_light_data(RenderContext& render)
    {
        PointLight* memory = (PointLight*) SDL_MapGPUTransferBuffer(render.device, render.light_transfer_buffer.buffer, false);
        ASSERT(memory);
        GraphicsPipeline& pipeline = render.graphics[render.graphics_light];

        u32 index = 0;
        for (auto& light : render.lights)
        {
            memory[index] = light;
            index += 1;
        }

        SDL_UnmapGPUTransferBuffer(render.device, render.light_transfer_buffer.buffer);

        return true;
    }

    void upload_frame_light_data(RenderContext& render)
    {
        if (render.lights.size() > 0)
        {
            SDL_GPUTransferBufferLocation source = {};
            SDL_GPUBufferRegion destination = {};

            source.transfer_buffer = render.light_transfer_buffer.buffer;
            source.offset = 0;

            destination.buffer = render.buffers[render.light_buffer].buffer;
            destination.offset = 0;
            destination.size = render.lights.size() * sizeof(PointLight);

            SDL_UploadToGPUBuffer(render.frame.copy_pass, &source, &destination, false);
        }
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

    bool RenderContext::make_texture_upload(SDL_Surface* surface, TextureUpload* upload)
    {
        TextureFormat format = SDL_GetGPUTextureFormatFromPixelFormat(surface->format);
        if (!SDL_GPUTextureSupportsFormat(device, format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_SAMPLER))
        {
            log_error("Texture format isn't supported: %s", SDL_GetPixelFormatName(surface->format));
            return false;
        }
        Texture texture = create_texture(format, surface->w, surface->h);
        TextureUpload data = {};
        data.target = texture;
        data.src = surface;

        *upload = data;
        return true;
    }

    Texture RenderContext::create_texture(TextureFormat format, u32 width, u32 height)
    {
        return create_texture_verbose(format, TextureUsageSampler, width, height, 1, SampleCount1);
    }

    Texture RenderContext::create_texture_verbose(TextureFormat format, TextureUsage usage, u32 width, u32 height, int mip_levels, SampleCount sampleCount)
    {
        SDL_GPUTextureCreateInfo ci = {};
        ci.type = SDL_GPU_TEXTURETYPE_2D;  // @hardcode
        ci.format = SDL_GPUTextureFormat(format);
        ci.usage = SDL_GPUTextureUsageFlags(usage);
        ci.width = width;
        ci.height = height;
        ci.layer_count_or_depth = 1;  // @hardcode
        ci.num_levels = mip_levels;
        ci.sample_count = SDL_GPUSampleCount(sampleCount);
        SDL_GPUTexture* sdl_texture = SDL_CreateGPUTexture(device, &ci);
        if (!sdl_texture)
        {
            return TEXTURE_INVALID;
        }
        GPUTexture texture = GPUTexture(sdl_texture, format, width, height);
        return textures.add(texture);
    }

    void RenderContext::destroy_texture(Texture handle)
    {
        if (handle.is_valid())
        {
            GPUTexture& texture = textures.get(handle.index);
            SDL_ReleaseGPUTexture(device, texture.texture);
            textures.remove(handle.index);
        }
    }

    GPUTexture& RenderContext::get_texture(Texture handle)
    {
        return textures.get(handle.index);
    }

    DrawGroupId RenderContext::make_draw_group(GraphicsPipelineId id, Texture texture, int size)
    {
        GraphicsPipeline& pipeline = graphics.get(id);
        DrawGroup group = {};
        group.texture = texture;
        group.offset = next_offset;
        group.capacity = size;
        group.used = 0;

        next_offset += group.capacity;

        u32 newsize = group.offset + size;

        size_t memory_req = newsize * sizeof(InstanceData);

        if (!resize_transfer_buffer(group_transfer_buffer, memory_req))
        {
            return DRAW_GROUPID_INVALID;
        }

        if (!resize_gpu_buffer(buffers[instance_buffer], memory_req))
        {
            return DRAW_GROUPID_INVALID;
        }

        this->instanceData.ensure_size(newsize);
        this->instanceData.mark_full();

        u32 groupIndex = pipeline.groups.add(group);

        return DrawGroupId(id, groupIndex);
    }

    GraphicsPipelineId RenderContext::make_graphics_pipeline(GraphicsPipelineParameters& params, SDL_GPUShader* vertex, SDL_GPUShader* fragment)
    {
        SDL_GPUGraphicsPipeline* pipeline = create_gpu_graphics_pipeline(&params, this, vertex, fragment);
        if (!pipeline)
        {
            return GRAPHICS_PIPELINE_INVALID;
        }

        GraphicsPipeline graphics_pipeline = GraphicsPipeline(params, pipeline, 0, 0, 0);

        return graphics.add(graphics_pipeline);
    }

    bool add_point_light(RenderContext& render, PointLight& light)
    {
        size_t memory_req = (render.lights.size() + 1) * sizeof(PointLight);
        if (!render.resize_transfer_buffer(render.light_transfer_buffer, memory_req))
        {
            return false;
        }

        if (!render.resize_gpu_buffer(render.buffers[render.light_buffer], memory_req))
        {
            return false;
        }

        render.lights.add(light);
        return true;
    }

    bool add_point_lights(RenderContext& render, PointLight *lights, int num_lights)
    {
        size_t memory_req = (render.lights.size() + num_lights) * sizeof(PointLight);
        if (!render.resize_transfer_buffer(render.light_transfer_buffer, memory_req))
        {
            return false;
        }

        if (!render.resize_gpu_buffer(render.buffers[render.light_buffer], memory_req))
        {
            return false;
        }

        render.lights.ensure_size(render.lights.size() + num_lights);
        for (int i = 0; i < num_lights; i++)
        {
            render.lights.add(lights[i]);
        }

        return true;
    }

    void queue_draw_mesh(RenderContext& render, MeshDraw& draw)
    {
        if (!draw.texture.is_valid())
        {
            render.frameMeshDraw.add(draw);
        }
        else
        {
            render.frameMeshDrawTex.add(draw);
        }
    }

    bool queue_draw_group(RenderContext& render, InstanceData& data, DrawGroupId groupId)
    {
        DrawGroup& group = render.get_draw_group(groupId);
        queue_draw(render, group, data);

        return true;
    }

    bool queue_draw(RenderContext& render, DrawGroup& group, InstanceData& data)
    {
        if (group.capacity < group.used + 1)
        {
            return false;
        }

        render.instanceData[group.offset + group.used] = data;
        group.used += 1;

        render.instanceData.mark_full();

        return true;
    }

    bool draw(RenderContext& render, Draw& d, DrawGroupId groupId)
    {
        InstanceData data = {};
        data.x = d.position.x;
        data.y = d.position.y;
        data.z = d.position.z;
        data.rotation = d.rotation;
        data.scale = pack_scale(d.scale);
        data.color = colorToHex(d.color);
        data.sourceOffset = pack_unorm16x2(d.sourceOffset);
        data.sourceScale = pack_unorm16x2(d.sourceScale);
        return queue_draw_group(render, data, groupId);
    }

    void draw_mesh(RenderContext& render, MeshDraw& draw)
    {
        render.set_mvp(draw.matrix, draw.matrix_usage);
        draw_mesh_buffers(render, draw, render.buffers.get_ref(draw.mesh.vertex_buffer), render.buffers.get_ref(draw.mesh.index_buffer));
    }

    void draw_mesh_buffers(RenderContext& render, MeshDraw& draw, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer)
    {
        ASSERT(render.frame.render_pass);

        SDL_GPUBufferBinding vertex_binding = {};
        SDL_GPUBufferBinding index_binding = {};

        vertex_binding.buffer = vertex_buffer.buffer;
        vertex_binding.offset = draw.mesh.vertex_offset;

        index_binding.buffer = index_buffer.buffer;
        index_binding.offset = draw.mesh.index_offset;

        SDL_BindGPUVertexBuffers(render.frame.render_pass, 0, &vertex_binding, 1);
        SDL_BindGPUIndexBuffer(render.frame.render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        SDL_DrawGPUIndexedPrimitives(render.frame.render_pass, draw.mesh.index_count, 1, 0, 0, 0);
    }

    void draw_mesh_texture(RenderContext& render, MeshDraw& draw)
    {
        render.set_mvp(draw.matrix, draw.matrix_usage);
        draw_mesh_texture_buffers(render, draw, render.buffers.get_ref(draw.mesh.vertex_buffer), render.buffers.get_ref(draw.mesh.index_buffer));
    }

    void draw_mesh_texture_buffers(RenderContext& render, MeshDraw& draw, GPUBuffer& vertex_buffer, GPUBuffer& index_buffer)
    {
        ASSERT(render.frame.render_pass);

        GPUTexture& texture = render.textures.get(draw.texture.index);

        SDL_GPUTextureSamplerBinding sampler_binding = {};
        sampler_binding.texture = texture.texture;
        sampler_binding.sampler = render.sampler;

        SDL_BindGPUFragmentStorageTextures(render.frame.render_pass, 0, &texture.texture, 1);
        SDL_BindGPUFragmentSamplers(render.frame.render_pass, 0, &sampler_binding, 1);

        SDL_GPUBufferBinding vertex_binding = {};
        SDL_GPUBufferBinding index_binding = {};

        vertex_binding.buffer = vertex_buffer.buffer;
        vertex_binding.offset = draw.mesh.vertex_offset;

        index_binding.buffer = index_buffer.buffer;
        index_binding.offset = draw.mesh.index_offset;

        SDL_BindGPUVertexBuffers(render.frame.render_pass, 0, &vertex_binding, 1);
        SDL_BindGPUIndexBuffer(render.frame.render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        SDL_DrawGPUIndexedPrimitives(render.frame.render_pass, draw.mesh.index_count, 1, 0, 0, 0);
    }

    void draw_quads(RenderContext& render, DrawGroup& group, SDL_GPURenderPass* pass, int instance_buffer)
    {
        ASSERT(pass);

        render.set_mvp(group.matrix, group.matrix_usage);

        SDL_GPUBufferBinding vertex_bindings[2] = {};
        SDL_GPUBufferBinding index_binding = {};

        vertex_bindings[0].buffer = render.buffers[render.vertex_buffer].buffer;
        vertex_bindings[0].offset = 0;

        vertex_bindings[1].buffer = render.buffers[instance_buffer].buffer;
        vertex_bindings[1].offset  = 0;

        index_binding.buffer = render.buffers[render.index_buffer].buffer;
        index_binding.offset = 0;

        SDL_BindGPUVertexBuffers(pass, 0, vertex_bindings, 2);
        SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(pass, 6, group.used, 0, 0, 0);
    }

    void draw_quads_texture(RenderContext& render, DrawGroup& group)
    {
        ASSERT(render.frame.render_pass);

        render.set_mvp(group.matrix, group.matrix_usage);

        GPUTexture& texture = render.textures.get(group.texture.index);

        SDL_GPUTextureSamplerBinding sampler_binding = {};
        sampler_binding.texture = texture.texture;
        sampler_binding.sampler = render.sampler;

        SDL_BindGPUFragmentStorageTextures(render.frame.render_pass, 0, &texture.texture, 1);
        SDL_BindGPUFragmentSamplers(render.frame.render_pass, 0, &sampler_binding, 1);

        SDL_GPUBufferBinding vertex_bindings[2] = {};
        SDL_GPUBufferBinding index_binding = {};

        // @Hardcode
        // quad is at the start of the buffer

        vertex_bindings[0].buffer = render.buffers[render.vertex_buffer].buffer;
        vertex_bindings[0].offset = 0;

        vertex_bindings[1].buffer = render.buffers[render.instance_buffer].buffer;
        vertex_bindings[1].offset = group.offset;

        index_binding.buffer = render.buffers[render.index_buffer].buffer;
        index_binding.offset = 0;

        SDL_BindGPUVertexBuffers(render.frame.render_pass, 0, vertex_bindings, 2);
        SDL_BindGPUIndexBuffer(render.frame.render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(render.frame.render_pass, 6, group.used, 0, 0, 0);
    }

    void draw_generic(RenderContext& render, GraphicsPipeline& pipeline)
    {
        ASSERT(render.frame.render_pass);

        for (DrawGroup& group : pipeline.groups)
        {
            render.set_mvp(group.matrix, group.matrix_usage);

            GPUTexture texture = render.textures.get(group.texture.index);

            SDL_GPUTextureSamplerBinding sampler_binding = {};
            sampler_binding.texture = texture.texture;
            sampler_binding.sampler = render.sampler;

            SDL_BindGPUFragmentStorageTextures(render.frame.render_pass, 0, &texture.texture, 1);
            SDL_BindGPUFragmentSamplers(render.frame.render_pass, 0, &sampler_binding, 1);

            SDL_GPUBufferBinding vertex_bindings[2] = {};
            SDL_GPUBufferBinding index_binding = {};

            // @Hardcode
            // quad is at the start of the buffer

            if (pipeline.use_predefined_buffers)
            {
                vertex_bindings[0].buffer = render.buffers[render.vertex_buffer].buffer;
                vertex_bindings[0].offset = 0;

                vertex_bindings[1].buffer = render.buffers[render.instance_buffer].buffer;
                vertex_bindings[1].offset = group.offset;

                index_binding.buffer = render.buffers[render.index_buffer].buffer;
                index_binding.offset = 0;
            }
            else
            {
                vertex_bindings[0].buffer = render.buffers.get_ref(pipeline.vertex_buffer).buffer;
                vertex_bindings[0].offset = 0;

                vertex_bindings[1].buffer = render.buffers.get_ref(pipeline.instance_buffer).buffer;
                vertex_bindings[1].offset = group.offset;

                index_binding.buffer = render.buffers.get_ref(pipeline.index_buffer).buffer;
                index_binding.offset = 0;
            }

            SDL_BindGPUVertexBuffers(render.frame.render_pass, 0, vertex_bindings, 2);
            SDL_BindGPUIndexBuffer(render.frame.render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_DrawGPUIndexedPrimitives(render.frame.render_pass, 6, group.used, 0, 0, 0);
        }
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

    Texture render_text(RenderContext& render, String text, Font font, melv::Color color) {
        SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
        SDL_Surface* surface = TTF_RenderText_Solid(font.font, text.data, text.size, sdl_color);

        if (!surface) {
            return Texture();
        }

        // @todo
        // create gpu texture
        // calculate text width and height
        SDL_GPUTexture* texture = nullptr;

    	SDL_DestroySurface(surface);

        return TEXTURE_INVALID;
    }

    Text create_text(RenderContext& render, String text, Font font, melv::Color color)
    {
        Texture texture = render_text(render, text, font, color);
        if (!texture.is_valid()) return Text();
        return Text(texture, text, color);
    }

    bool RenderContext::set_shaders(GraphicsPipeline* gp, SDL_GPUShader* vertex, SDL_GPUShader* fragment)
    {
        if (!gp->pipeline)
        {
            return false;
        }

        SDL_GPUGraphicsPipeline *pipeline = create_gpu_graphics_pipeline(&gp->parameters, this, vertex, fragment);

        if (!pipeline)
        {
            return false;
        }

        SDL_ReleaseGPUGraphicsPipeline(this->device, gp->pipeline);

        gp->pipeline = pipeline;
        return true;
    }

    void generate_quad_mesh(DArray<VertexInstance>& out_vertex, DArray<u16>& out_index)
    {
        out_vertex = DArray<VertexInstance> (4, true);
        out_index = DArray<u16> (6, true);

        out_vertex[0] = VertexInstance(-0.5, -0.5, 0, 1);
        out_vertex[1] = VertexInstance(0.5, -0.5, 1, 1);
        out_vertex[2] = VertexInstance(-0.5, 0.5, 0, 0);
        out_vertex[3] = VertexInstance(0.5, 0.5, 1, 0);

        out_index[0] = 0;
        out_index[1] = 1;
        out_index[2] = 3;
        out_index[3] = 0;
        out_index[4] = 3;
        out_index[5] = 2;
    }

    void generate_circle_mesh(DArray<VertexInstance>& out_vertex, DArray<u16>& out_index)
    {
        constexpr int NVERTICES = 32;

        out_vertex = DArray<VertexInstance> (NVERTICES + 1);
        out_index = DArray<u16> (NVERTICES * 3);

        VertexInstance center = VertexInstance (0, 0, 0.5, 0.5);
        out_vertex[0] = center;

        // the angle between vertices and it's sin and cos
        const float angle = CONSTANT_TAU / float(NVERTICES);
        const float c = std::cosf(angle);
        const float s = std::sinf(angle);

        float xcomp = 1.0;
        float ycomp = 0.0;
        for (int i = 1; i <= NVERTICES; i++)
        {
            float px = center.x + xcomp;
            float py = center.y + ycomp;
            float u = (xcomp + 1.0f) * 0.5f;
            float v = (ycomp + 1.0f) * 0.5f;
            out_vertex[i] = VertexInstance(px, py, u, v);

            // rotate the vector
            float n_xcomp = xcomp * c - ycomp * s;
            float n_ycomp = xcomp * s + ycomp * c;
            xcomp = n_xcomp;
            ycomp = n_ycomp;
        }

        for (int i = 0; i < NVERTICES - 1; i++)
        {
            out_index[i * 3 + 0] = 0;
            out_index[i * 3 + 1] = i + 1;
            out_index[i * 3 + 2] = i + 2;
        }

        out_index[(NVERTICES - 1) * 3 + 0] = 0;
        out_index[(NVERTICES - 1) * 3 + 1] = NVERTICES;
        out_index[(NVERTICES - 1) * 3 + 2] = 1;
    }

    void unloadShader(RenderContext& context, Shader& shader)
    {
        SDL_ReleaseGPUShader(context.device, shader.shader);
    }

    ShaderLoadResult loadShader(RenderContext& context, Shader& shader, const char* path)
    {
        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
        SDL_GPUShaderStage shaderStage = SDL_GPUShaderStage(shader.stage);

        BinaryData code = {};
        if (!load_file(path, code)) {
            log_error("Could not load shader %s", path);
            return SHADER_LOAD_FAIL;
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
        else if (string_compare(extension, String("msl")))
        {
            format = SDL_GPU_SHADERFORMAT_MSL;
        }

        SDL_GPUShaderFormat expectedFormat = SDL_GetGPUShaderFormats(context.device);
        if (!(expectedFormat & format))
        {
            return SHADER_LOAD_INCOMPATIBLE_FORMAT;
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
            return SHADER_LOAD_FAIL;
        }

        shader.shader = shaderObj;

        return SHADER_LOAD_SUCCESS;
    }

    u16 pack_unorm16(float x, float range)
    {
        x = melv::clamp(0, range, x);
        x /= range;
        u16 rx = u16(x * float(0xffff) + 0.5f);
        return rx;
    }

    float unpack_unorm16(u16 x, float range)
    {
        float rx = (float(x) / float(0xffff)) * range;
        return rx;
    }

    u32 pack_unorm16x2(vec2 v)
    {
        v.x = melv::clamp(0, 1, v.x);
        v.y = melv::clamp(0, 1, v.y);
        u32 rx = u32(v.x * float(0xFFFF) + 0.5f) & 0xffff;
        u32 ry = u32(v.y * float(0xFFFF) + 0.5f) << 16;

        return rx | ry;
    }

    vec2 unpack_unorm16x2(u32 v)
    {
        float x = float(v & 0xffff) / float(0xffff);
        float y = float(v >> 16) / float(0xffff);

        return vec2(x, y);
    }

    u32 pack_scale(vec2 v)
    {
        u32 x = pack_unorm16(v.x, MaxInstanceScale);
        u32 y = pack_unorm16(v.y, MaxInstanceScale);
        return x | (y << 16);
    }

    vec2 unpack_scale(u32 v)
    {
        float x = unpack_unorm16(v & 0xffff, MaxInstanceScale);
        float y = unpack_unorm16(v >> 16, MaxInstanceScale);
        return vec2(x, y);
    }

} // namespace
