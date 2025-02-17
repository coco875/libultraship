#include "ImGui_LLGL.h"
#include <SDL2/SDL_video.h>
#include <graphic/Fast3D/gfx_llgl.h>
#if defined(_WIN32)
#include <LLGL/Backend/Direct3D11/NativeHandle.h>
#endif
#include <LLGL/Backend/OpenGL/NativeHandle.h>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#if defined(_WIN32)
ID3D11Device* d3dDevice;
ID3D11DeviceContext* d3dDeviceContext;
#endif

#ifdef __APPLE__
void* glDevice;
#else
GLXContext glDevice;
#endif

void ImGui_ImplSDL2_LLGL(SDL_Window* window, void* context) {
    
};

void ImGui_ImplLLGL_Init(void* window) {
    switch (llgl_renderer->GetRendererID()) {
        #if defined(_WIN32)
        case LLGL::RendererID::Direct3D11:
            LLGL::Direct3D11::RenderSystemNativeHandle nativeDeviceHandle;
            llgl_renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
            d3dDevice = nativeDeviceHandle.device;

            LLGL::Direct3D11::CommandBufferNativeHandle nativeContextHandle;
            cmdBuffer->GetNativeHandle(&nativeContextHandle, sizeof(nativeContextHandle));
            d3dDeviceContext = nativeContextHandle.deviceContext;

            ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);
            break;
        case LLGL::RendererID::Direct3D12:
            LLGL::Direct3D12::RenderSystemNativeHandle nativeDeviceHandle;
            llgl_renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
            d3dDevice = nativeDeviceHandle.device;

            LLGL::Direct3D12::CommandBufferNativeHandle nativeContextHandle;
            cmdBuffer->GetNativeHandle(&nativeContextHandle, sizeof(nativeContextHandle));
            d3dCommandList = nativeContextHandle.commandList;

            ImGui_ImplDX12_Init(d3dDevice, d3dCommandList);
            break;
        #endif
        case LLGL::RendererID::OpenGL:
            #ifdef __APPLE__
            void* nativeDeviceHandle;
            llgl_renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
            glDevice = nativeDeviceHandle;

            #else
            LLGL::OpenGL::RenderSystemNativeHandle nativeDeviceHandle;
            llgl_renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
            glDevice = nativeDeviceHandle.context;
            #endif
            ImGui_ImplSDL2_InitForOpenGL((SDL_Window*)window, nullptr);
#ifdef __APPLE__
            ImGui_ImplOpenGL3_Init("#version 410 core");
#elif USE_OPENGLES
            ImGui_ImplOpenGL3_Init("#version 300 es");
#else
            ImGui_ImplOpenGL3_Init("#version 120");
#endif
            break;
        case LLGL::RendererID::Vulkan:
            // ImGui_ImplVulkan_Init();
            break;
        case LLGL::RendererID::Metal:
            // ImGui_ImplMetal_Init();
            break;
        default:
            // ImGui_ImplNull_Init();
            break;
    }
};

void ImGui_ImplLLGL_NewFrame() {
    switch (llgl_renderer->GetRendererID()) {
        #if defined(_WIN32)
        case LLGL::RendererID::Direct3D11:
            ImGui_ImplDX11_NewFrame();
            break;
        case LLGL::RendererID::Direct3D12:
            ImGui_ImplDX12_NewFrame();
            break;
        #endif
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_NewFrame();
            break;
        case LLGL::RendererID::Vulkan:
            // ImGui_ImplVulkan_NewFrame();
            break;
        case LLGL::RendererID::Metal:
            // ImGui_ImplMetal_NewFrame();
            break;
        default:
            // ImGui_ImplNull_NewFrame();
            break;
    }
}