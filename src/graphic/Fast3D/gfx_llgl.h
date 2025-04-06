#ifndef GFX_LLGL_H
#define GFX_LLGL_H

#include "gfx_rendering_api.h"
#include <LLGL/LLGL.h>

extern LLGL::RenderSystemPtr llgl_renderer;
extern LLGL::SwapChain* llgl_swapChain;
extern LLGL::CommandBuffer* llgl_cmdBuffer;

extern struct GfxRenderingAPI gfx_llgl_api;

#endif