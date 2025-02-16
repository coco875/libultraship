#include "ImGui_LLGL.h"
#include <SDL2/SDL_video.h>
#include <graphic/Fast3D/gfx_llgl.h>
#if defined(_WIN32)
#include <LLGL/Backend/Direct3D11/NativeHandle.h>
#endif
#include <LLGL/Backend/OpenGL/NativeHandle.h>

#if defined(_WIN32)
ID3D11Device* d3dDevice;
ID3D11DeviceContext* d3dDeviceContext;
#endif

GLXContext glDevice;

void ImGui_ImplSDL2_LLGL(SDL_Window* window, void* context) {
    
};

void ImGui_ImplLLGL_Init() {
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
            LLGL::OpenGL::RenderSystemNativeHandle nativeDeviceHandle;
            llgl_renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
            glDevice = nativeDeviceHandle.context;
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