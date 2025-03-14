/*
 * LLGL Example ImGui
 * Created on 02/22/2025 by L.Hermanns
 * Published under the BSD-3 Clause License
 * ----------------------------------------
 * Metal Backend
 */

#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>
#include <LLGL/Backend/Metal/NativeHandle.h>

#include "imgui.h"
#include "imgui_impl_metal.h"

#import <Metal/Metal.h>

extern LLGL::RenderSystemPtr llgl_renderer;


// ImGuiContext*       imGuiContext    = nullptr;
id<MTLDevice> mtlDevice = nil;
id<MTLCommandBuffer> mtlCommandBuffer = nil;
id<MTLRenderCommandEncoder> mtlRenderCmdEncoder = nil;
MTLRenderPassDescriptor* mtlRenderPassDesc = nullptr;

void Imgui_Metal_llgl_Init(LLGL::RenderSystemPtr& renderer) {
    // Setup renderer backend
    LLGL::Metal::RenderSystemNativeHandle nativeDeviceHandle;
    renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
    mtlDevice = nativeDeviceHandle.device;

    ImGui_ImplMetal_Init(mtlDevice);
}

void Imgui_Metal_llgl_Shutdown() {
    // ImGui::SetCurrentContext(imGuiContext);

    ImGui_ImplMetal_Shutdown();
    // Release Metal handles
    if (mtlDevice != nil)
        [mtlDevice release];
}

void Imgui_Metal_llgl_NewFrame(LLGL::CommandBuffer* cmdBuffer) {

    LLGL::Metal::CommandBufferNativeHandle nativeContextHandle;
    cmdBuffer->GetNativeHandle(&nativeContextHandle, sizeof(nativeContextHandle));

    mtlCommandBuffer = nativeContextHandle.commandBuffer;
    LLGL_VERIFY(mtlCommandBuffer != nil);

    mtlRenderCmdEncoder = (id<MTLRenderCommandEncoder>)nativeContextHandle.commandEncoder;
    LLGL_VERIFY(mtlRenderCmdEncoder != nil);

    mtlRenderPassDesc = nativeContextHandle.renderPassDesc;
    LLGL_VERIFY(mtlRenderPassDesc != nullptr);

    ImGui_ImplMetal_NewFrame(mtlRenderPassDesc);
}

void Imgui_Metal_llgl_RenderDrawData(ImDrawData* data) {
    // Encode render commands
    ImGui_ImplMetal_RenderDrawData(data, mtlCommandBuffer, mtlRenderCmdEncoder);

    [mtlRenderCmdEncoder release];
    [mtlRenderPassDesc release];
    [mtlCommandBuffer release];
}

extern "C" float Imgui_Metal_llgl_GetContentScale(NSWindow *wnd_) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12 
    return [wnd_ backingScaleFactor]; 
#else 
    return 1.0; 
#endif 
}