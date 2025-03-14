#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>
#include <LLGL/Backend/OpenGL/NativeHandle.h>

#include "imgui_impl_sdl2.h"

#include "sdl_llgl.h"

SDLSurface::SDLSurface(const LLGL::Extent2D& size, const char* title, int rendererID,
                       LLGL::RenderSystemDescriptor& desc)
    : title_{ title }, size{ size } {
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    switch (rendererID) {
        case LLGL::RendererID::OpenGL:
            flags |= SDL_WINDOW_OPENGL;
            break;
        case LLGL::RendererID::OpenGLES:
            flags |= SDL_WINDOW_OPENGL;
            break;
        case LLGL::RendererID::Metal:
            flags |= SDL_WINDOW_METAL;
            break;
        case LLGL::RendererID::Vulkan:
            flags |= SDL_WINDOW_VULKAN;
            break;
        default:
            break;
    }

    wnd = SDL_CreateWindow(title, 400, 200, (int) size.width, (int) size.height, flags);
    if (wnd == nullptr) {
        LLGL::Log::Errorf("Failed to create SDL2 window\n");
        exit(1);
    }

    switch (rendererID) {
        case LLGL::RendererID::OpenGL:
        case LLGL::RendererID::OpenGLES: {
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            SDL_GL_GetDrawableSize(wnd, (int*) &size.width, (int*) &size.height);

            SDL_GLContext ctx = SDL_GL_CreateContext(wnd);

            SDL_GL_MakeCurrent(wnd, ctx);

            // Init LLGL
            desc = { "OpenGL" };
#ifndef __APPLE__
            auto handle = LLGL::OpenGL::RenderSystemNativeHandle{ (GLXContext) ctx };
#else
            auto handle = LLGL::OpenGL::RenderSystemNativeHandle{ (void*) ctx };
#endif
            desc.nativeHandle = (void*) &handle;
            desc.nativeHandleSize = sizeof(LLGL::OpenGL::RenderSystemNativeHandle);
            break;
        }
        case LLGL::RendererID::Vulkan:
            desc = { "Vulkan" };
            break;
        case LLGL::RendererID::Metal:
            desc = { "Metal" };
            break;
        default:
            break;
    }

#ifdef __APPLE__
    if (wnd) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(wnd, &wmInfo);
        float scale = Imgui_Metal_llgl_GetContentScale(wmInfo.info.cocoa.window);
        this->size = LLGL::Extent2D(size.width * scale, size.height * scale);
    }
#endif
}

SDLSurface::~SDLSurface() {
}

bool SDLSurface::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(wnd, &wmInfo);
    auto* nativeHandlePtr = static_cast<LLGL::NativeHandle*>(nativeHandle);
#ifdef __APPLE__
    nativeHandlePtr->responder = wmInfo.info.cocoa.window;
#else
    nativeHandlePtr->display = wmInfo.info.x11.display;
    nativeHandlePtr->window = wmInfo.info.x11.window;
#endif
    return true;
}

LLGL::Extent2D SDLSurface::GetContentSize() const {
    return size;
}

bool SDLSurface::AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) {
    return false;
}

LLGL::Display* SDLSurface::FindResidentDisplay() const {
    return nullptr;
}

bool SDLSurface::ProcessEvents(LLGL::SwapChain* swapChain) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (event.type == SDL_WINDOWEVENT &&
            (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
            uint32_t width = event.window.data1;
            uint32_t height = event.window.data2;
#ifdef __APPLE__
            if (wnd) {
                SDL_SysWMinfo wmInfo;
                SDL_VERSION(&wmInfo.version);
                SDL_GetWindowWMInfo(wnd, &wmInfo);
                float scale = Imgui_Metal_llgl_GetContentScale(wmInfo.info.cocoa.window);
                width *= scale;
                height *= scale;
            }
#endif
            size = { width, height };
            swapChain->ResizeBuffers(size);
        }
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
    return true;
}