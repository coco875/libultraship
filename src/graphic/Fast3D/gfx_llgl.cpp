
#include "gfx_cc.h"
#include "gfx_rendering_api.h"
#include "window/gui/Gui.h"
#include "gfx_pc.h"
#include <prism/processor.h>
#include <LLGL/LLGL.h>

LLGL::RenderSystemPtr llgl_renderer;
LLGL::SwapChain* llgl_swapChain;
LLGL::CommandBuffer* llgl_cmdBuffer;

const char* gfx_llgl_get_name(void) {
    // renderer->GetName();
    return "LLGL";
}

int gfx_llgl_get_max_texture_size(void) {
    return 0;
}

struct GfxClipParameters gfx_llgl_get_clip_parameters(void) {
    return { false, false };
}

void gfx_llgl_unload_shader(struct ShaderProgram* old_prg) {
}

void gfx_llgl_load_shader(struct ShaderProgram* new_prg) {
}

struct ShaderProgram* gfx_llgl_create_and_load_new_shader(uint64_t shader_id0, uint32_t shader_id1) {
    return nullptr;
}

struct ShaderProgram* gfx_llgl_lookup_shader(uint64_t shader_id0, uint32_t shader_id1) {
    return nullptr;
}

void gfx_llgl_shader_get_info(struct ShaderProgram* prg, uint8_t* num_inputs, bool used_textures[2]) {
}

uint32_t gfx_llgl_new_texture(void) {
    return 0;
}

void gfx_llgl_select_texture(int tile, uint32_t texture_id) {
}

void gfx_llgl_upload_texture(const uint8_t* rgba32_buf, uint32_t width, uint32_t height) {
}

void gfx_llgl_set_sampler_parameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) {
}

void gfx_llgl_set_depth_test_and_mask(bool depth_test, bool z_upd) {
}

void gfx_llgl_set_zmode_decal(bool zmode_decal) {
}

void gfx_llgl_set_viewport(int x, int y, int width, int height) {
}

void gfx_llgl_set_scissor(int x, int y, int width, int height) {
}

void gfx_llgl_set_use_alpha(bool use_alpha) {
}

void gfx_llgl_draw_triangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
}

void gfx_llgl_init(void) {
    LLGL::Report report;
    llgl_renderer = LLGL::RenderSystem::Load("OpenGL", &report);

    if (!llgl_renderer) {
        auto a = report.GetText();
        SPDLOG_ERROR("Failed to load \"%s\" module. Falling back to \"Null\" device.\n", "OpenGL");
        SPDLOG_ERROR("Reason for failure: %s", report.HasErrors() ? report.GetText() : "Unknown\n");
        llgl_renderer = LLGL::RenderSystem::Load("Null");
        if (!llgl_renderer)
        {
            SPDLOG_ERROR("Failed to load \"Null\" module. Exiting.\n");
            exit(1);
        }
    }
}

void gfx_llgl_on_resize(void) {
}

void gfx_llgl_start_frame(void) {
    llgl_cmdBuffer->Begin();
    llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
}

void gfx_llgl_end_frame(void) {
    // llgl_cmdBuffer->EndRenderPass();
    // llgl_cmdBuffer->End();
    // llgl_swapChain->Present();
}

void gfx_llgl_finish_render(void) {
    llgl_cmdBuffer->EndRenderPass();
    llgl_cmdBuffer->End();
    llgl_swapChain->Present();
}

int gfx_llgl_create_framebuffer(void) {
    return 0;
}

void gfx_llgl_update_framebuffer_parameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
    bool opengl_invert_y, bool render_target, bool has_depth_buffer, bool can_extract_depth) {
}

void gfx_llgl_start_draw_to_framebuffer(int fb_id, float noise_scale) {
}

void gfx_llgl_copy_framebuffer(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
    int dstY0, int dstX1, int dstY1) {
}

void gfx_llgl_clear_framebuffer(bool color, bool depth) {
    llgl_cmdBuffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue{ 0.0f, 0.0f, 0.0f, 1.0f });
}

void gfx_llgl_read_framebuffer_to_cpu(int fb_id, uint32_t width, uint32_t height, uint16_t* rgba16_buf) {
}

void gfx_llgl_resolve_msaa_color_buffer(int fb_id_target, int fb_id_source) {
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> gfx_llgl_get_pixel_depth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    return {};
}

void* gfx_llgl_get_framebuffer_texture_id(int fb_id) {
    return nullptr;
}

void gfx_llgl_select_texture_fb(int fb_id) {
}

void gfx_llgl_delete_texture(uint32_t texture_id) {
}

void gfx_llgl_set_texture_filter(FilteringMode mode) {
}

FilteringMode gfx_llgl_get_texture_filter(void) {
    return FILTER_NONE;
}

void gfx_llgl_enable_srgb_mode(void) {
}

struct GfxRenderingAPI gfx_llgl_api = { gfx_llgl_get_name,
    gfx_llgl_get_max_texture_size,
    gfx_llgl_get_clip_parameters,
    gfx_llgl_unload_shader,
    gfx_llgl_load_shader,
    gfx_llgl_create_and_load_new_shader,
    gfx_llgl_lookup_shader,
    gfx_llgl_shader_get_info,
    gfx_llgl_new_texture,
    gfx_llgl_select_texture,
    gfx_llgl_upload_texture,
    gfx_llgl_set_sampler_parameters,
    gfx_llgl_set_depth_test_and_mask,
    gfx_llgl_set_zmode_decal,
    gfx_llgl_set_viewport,
    gfx_llgl_set_scissor,
    gfx_llgl_set_use_alpha,
    gfx_llgl_draw_triangles,
    gfx_llgl_init,
    gfx_llgl_on_resize,
    gfx_llgl_start_frame,
    gfx_llgl_end_frame,
    gfx_llgl_finish_render,
    gfx_llgl_create_framebuffer,
    gfx_llgl_update_framebuffer_parameters,
    gfx_llgl_start_draw_to_framebuffer,
    gfx_llgl_copy_framebuffer,
    gfx_llgl_clear_framebuffer,
    gfx_llgl_read_framebuffer_to_cpu,
    gfx_llgl_resolve_msaa_color_buffer,
    gfx_llgl_get_pixel_depth,
    gfx_llgl_get_framebuffer_texture_id,
    gfx_llgl_select_texture_fb,
    gfx_llgl_delete_texture,
    gfx_llgl_set_texture_filter,
    gfx_llgl_get_texture_filter,
    gfx_llgl_enable_srgb_mode };