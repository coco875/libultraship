#include "gfx_bgfx.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <map>
#include <unordered_map>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#include "gfx_cc.h"
#include "gfx_rendering_api.h"
#include "bgfx/imgui_impl_bgfx.h"
#include "window/gui/Gui.h"
#include "window/Window.h"
#include "gfx_pc.h"
#include "Fast3dWindow.h"
#include <public/bridge/consolevariablebridge.h>
#include <bgfx/bgfx.h>
#include <SDL_syswm.h>
#include "Context.h"
#include <brtshaderc.h>
#include "imgui_impl_sdl2.h"
#include "bx/platform.h"

#include <vector>
#include <algorithm>

struct TextureDataBGFX {
    uint16_t width;
    uint16_t height;
    bool linear_filtering;
    bgfx::TextureHandle ptr = BGFX_INVALID_HANDLE;
};

bgfx::ViewId kClearView = 0;

struct BGFXContext {
    const bgfx::ViewId kClearView = 0;
    std::vector<TextureDataBGFX> textures;

    int current_tile;
    uint32_t current_texture_ids[SHADER_MAX_TEXTURES];

} context;

const bgfx::Caps* caps = bgfx::getCaps();

static void gfx_bgfx_init(void) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo((SDL_Window*) wnd->GetWindowPtr(), &wmi)) {
        printf(
                "SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
                SDL_GetError());
        return;
    }

    bgfx::PlatformData pd{};

#if BX_PLATFORM_WINDOWS
    pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
    pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_LINUX
    if (wmi.subsystem == SDL_SYSWM_WAYLAND) {
        pd.nwh = (void*)(uintptr_t)wmi.info.wl.surface;
        pd.ndt = wmi.info.wl.display;
        pd.type = bgfx::NativeWindowHandleType::Wayland;
    }
    else {
        pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
        pd.ndt = wmi.info.x11.display;
        pd.type = bgfx::NativeWindowHandleType::Default;
    }
#elif BX_PLATFORM_EMSCRIPTEN
    pd.nwh = (void*)"#canvas";
#endif
    pd.context = NULL;
    pd.backBuffer = NULL;
    pd.backBufferDS = NULL;

    bgfx::Init bgfx_init;
    bgfx_init.type = bgfx::RendererType::OpenGL; // auto choose renderer
    bgfx_init.resolution.width = wnd->GetWidth();
    bgfx_init.resolution.height = wnd->GetHeight();
    bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx_init.platformData = pd;
    bgfx::init(bgfx_init);

    bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR);
    bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
    bgfx::touch(kClearView);
}

static int gfx_bgfx_get_max_texture_size() {
    return caps->limits.maxTextureSize;
}

static const char* gfx_bgfx_get_name() {
    return "BGFX";
}

static struct GfxClipParameters gfx_bgfx_get_clip_parameters(void) {
    return { false, false };
}

static void gfx_bgfx_vertex_array_set_attribs(struct ShaderProgram* prg) {

}

static void gfx_bgfx_set_uniforms(struct ShaderProgram* prg) {

}

static void gfx_bgfx_unload_shader(struct ShaderProgram* old_prg) {

}

static void gfx_bgfx_load_shader(struct ShaderProgram* new_prg) {

}

static struct ShaderProgram* gfx_bgfx_create_and_load_new_shader(uint64_t shader_id0, uint32_t shader_id1) {
    // compile vertex shader, with default arguments.
    const bgfx::Memory* memVsh =  shaderc::compileShader(shaderc::ST_VERTEX, "vs_cubes.sc");
    bgfx::ShaderHandle vsh = bgfx::createShader(memVsh);

    // compile fragment shader, with specific arguments for defines, varying def file, shader profile.
    const bgfx::Memory* memFsh =  shaderc::compileShader(shaderc::ST_FRAGMENT, "fs_cubes.sc", "myDefines", "varying.def.sc", "ps_5_0");
    bgfx::ShaderHandle fsh = bgfx::createShader(memFsh);

    // build program using shaders
    auto mProgram = bgfx::createProgram(vsh, fsh, true);
    return nullptr;
}

static struct ShaderProgram* gfx_bgfx_lookup_shader(uint64_t shader_id0, uint32_t shader_id1) {
    return nullptr;
}

static void gfx_bgfx_shader_get_info(struct ShaderProgram* prg, uint8_t* num_inputs, bool used_textures[2]) {

}

static uint32_t gfx_bgfx_new_texture(void) {
    context.textures.resize(context.textures.size() + 1);
    return (uint32_t)(context.textures.size() - 1);
}

static void gfx_bgfx_delete_texture(uint32_t texID) {

}

static void gfx_bgfx_select_texture(int tile, uint32_t texture_id) {

}

static void gfx_bgfx_upload_texture(const uint8_t* rgba32_buf, uint32_t width, uint32_t height) {
    // TextureDataBGFX* texture_data = &context.textures[context.current_texture_ids[context.current_tile]];

    // bimg::ImageContainer* image = bimg::imageAlloc(
    //     entry::getAllocator(),
    //     bimg::TextureFormat::RGBA32U,
    //     width,
    //     height,
    //     1,
    //     1,
    //     false,
    //     false,
    //     rgba32_buf
    // );

    // context.
}

static void gfx_bgfx_set_sampler_parameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {

}

static void gfx_bgfx_set_depth_test_and_mask(bool depth_test, bool z_upd) {

}

static void gfx_bgfx_set_zmode_decal(bool zmode_decal) {

}

static void gfx_bgfx_set_viewport(int x, int y, int width, int height) {

}

static void gfx_bgfx_set_scissor(int x, int y, int width, int height) {

}

static void gfx_bgfx_set_use_alpha(bool use_alpha) {

}

static void gfx_bgfx_draw_triangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {

}

static void gfx_bgfx_on_resize(void) {
    bgfx::reset(Ship::Context::GetInstance()->GetWindow()->GetWidth(), Ship::Context::GetInstance()->GetWindow()->GetHeight(), BGFX_RESET_VSYNC);
}

static void gfx_bgfx_start_frame(void) {
    // This dummy draw call is here to make sure that view 0 is cleared
    // if no other draw calls are submitted to view 0.
//    bgfx::dbgTextClear();
//
//    const bgfx::Stats* stats = bgfx::getStats();
//    bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters."
//        , stats->width
//        , stats->height
//        , stats->textWidth
//        , stats->textHeight
//        );
//
//    bgfx::setDebug(BGFX_DEBUG_STATS);
}

static void gfx_bgfx_end_frame(void) {
    bgfx::frame();
}

static void gfx_bgfx_finish_render(void) {
}

static int gfx_bgfx_create_framebuffer() {
    return 0;
}

static void gfx_bgfx_update_framebuffer_parameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
    bool opengl_invert_y, bool render_target, bool has_depth_buffer,
    bool can_extract_depth) {

}

void gfx_bgfx_start_draw_to_framebuffer(int fb_id, float noise_scale) {

}

void gfx_bgfx_clear_framebuffer(bool color, bool depth) {
    uint16_t flags = 0;
    if (color) {
        flags |= BGFX_CLEAR_COLOR;
    }
    if (depth) {
        flags |= BGFX_CLEAR_DEPTH;
    }
    bgfx::setViewClear(kClearView, flags);
}

void gfx_bgfx_resolve_msaa_color_buffer(int fb_id_target, int fb_id_source) {

}

void* gfx_bgfx_get_framebuffer_texture_id(int fb_id) {
    return nullptr;
}

void gfx_bgfx_select_texture_fb(int fb_id) {

}

void gfx_bgfx_copy_framebuffer(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
    int dstY0, int dstX1, int dstY1) {

}

void gfx_bgfx_read_framebuffer_to_cpu(int fb_id, uint32_t width, uint32_t height, uint16_t* rgba16_buf) {
   
}

static std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> gfx_bgfx_get_pixel_depth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> res;
    return res;
}

void gfx_bgfx_set_texture_filter(FilteringMode mode) {

}

FilteringMode gfx_bgfx_get_texture_filter(void) {
    return FilteringMode::FILTER_NONE;
}

void gfx_bgfx_enable_srgb_mode(void) {
    
}

struct GfxRenderingAPI gfx_bgfx_api = { 
    gfx_bgfx_get_name,
    gfx_bgfx_get_max_texture_size,
    gfx_bgfx_get_clip_parameters,
    gfx_bgfx_unload_shader,
    gfx_bgfx_load_shader,
    gfx_bgfx_create_and_load_new_shader,
    gfx_bgfx_lookup_shader,
    gfx_bgfx_shader_get_info,
    gfx_bgfx_new_texture,
    gfx_bgfx_select_texture,
    gfx_bgfx_upload_texture,
    gfx_bgfx_set_sampler_parameters,
    gfx_bgfx_set_depth_test_and_mask,
    gfx_bgfx_set_zmode_decal,
    gfx_bgfx_set_viewport,
    gfx_bgfx_set_scissor,
    gfx_bgfx_set_use_alpha,
    gfx_bgfx_draw_triangles,
    gfx_bgfx_init,
    gfx_bgfx_on_resize,
    gfx_bgfx_start_frame,
    gfx_bgfx_end_frame,
    gfx_bgfx_finish_render,
    gfx_bgfx_create_framebuffer,
    gfx_bgfx_update_framebuffer_parameters,
    gfx_bgfx_start_draw_to_framebuffer,
    gfx_bgfx_copy_framebuffer,
    gfx_bgfx_clear_framebuffer,
    gfx_bgfx_read_framebuffer_to_cpu,
    gfx_bgfx_resolve_msaa_color_buffer,
    gfx_bgfx_get_pixel_depth,
    gfx_bgfx_get_framebuffer_texture_id,
    gfx_bgfx_select_texture_fb,
    gfx_bgfx_delete_texture,
    gfx_bgfx_set_texture_filter,
    gfx_bgfx_get_texture_filter,
    gfx_bgfx_enable_srgb_mode
};