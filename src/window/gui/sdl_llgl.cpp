#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>
#include <LLGL/Backend/OpenGL/NativeHandle.h>

#include "imgui_impl_sdl2.h"

#include <SDL2/SDL_syswm.h>
#include "sdl_llgl.h"

#if defined(LLGL_OS_LINUX) || defined(WIN32)
LLGL::OpenGL::RenderSystemNativeHandle handle;
#endif

SDLSurface::SDLSurface(const LLGL::Extent2D& size, const char* title, int rendererID,
                       LLGL::RenderSystemDescriptor& desc)
    : title_{ title }, size_{ size } {
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

    wnd = SDL_CreateWindow(title, 400, 200, (int)size.width, (int)size.height, flags);
    if (wnd == nullptr) {
        LLGL::Log::Errorf("%s\n", SDL_GetError());
        LLGL::Log::Errorf("Failed to create SDL2 window\n");
        exit(1);
    }

    switch (rendererID) {
        case LLGL::RendererID::OpenGL:
        case LLGL::RendererID::OpenGLES: {
            // Init LLGL
            desc = { "OpenGL" };

#ifdef LLGL_OS_LINUX
            SDL_GLContext ctx = SDL_GL_CreateContext(wnd);

            SDL_GL_MakeCurrent(wnd, ctx);

            handle = LLGL::OpenGL::RenderSystemNativeHandle{ (GLXContext)ctx };

            desc.nativeHandle = (void*)&handle;
            desc.nativeHandleSize = sizeof(LLGL::OpenGL::RenderSystemNativeHandle);
#elif defined(WIN32)
            SDL_GLContext ctx = SDL_GL_CreateContext(wnd);

            SDL_GL_MakeCurrent(wnd, ctx);

            handle = LLGL::OpenGL::RenderSystemNativeHandle{ (HGLRC)ctx };

            desc.nativeHandle = (void*)&handle;
            desc.nativeHandleSize = sizeof(LLGL::OpenGL::RenderSystemNativeHandle);
#endif
            break;
        }
        case LLGL::RendererID::Vulkan:
            desc = { "Vulkan" };
            break;
        case LLGL::RendererID::Metal:
            desc = { "Metal" };
            break;
        case LLGL::RendererID::Direct3D11:
            desc = { "Direct3D11" };
            break;
        case LLGL::RendererID::Direct3D12:
            desc = { "Direct3D12" };
            break;
        default:
            break;
    }

#ifdef __APPLE__
    SDL_GL_GetDrawableSize(wnd, (int*)&size.width, (int*)&size.height);
    size_ = size;
#endif
}

SDLSurface::~SDLSurface() {
}

bool SDLSurface::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(wnd, &wmInfo);
    auto* nativeHandlePtr = static_cast<LLGL::NativeHandle*>(nativeHandle);
#ifdef WIN32
    nativeHandlePtr->window = wmInfo.info.win.window;
#elif defined(__APPLE__)
    nativeHandlePtr->responder = wmInfo.info.cocoa.window;
#else
    nativeHandlePtr->display = wmInfo.info.x11.display;
    nativeHandlePtr->window = wmInfo.info.x11.window;
#endif
    return true;
}

LLGL::Extent2D SDLSurface::GetContentSize() const {
    return size_;
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
            SDL_GL_GetDrawableSize(wnd, (int*)&size_.width, (int*)&size_.height);
            swapChain->ResizeBuffers(size_);
        }
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
    return true;
}