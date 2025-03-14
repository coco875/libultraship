#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"
#ifdef __APPLE__
#include "imgui_impl_metal.h"
#endif
#ifdef WIN32
#include "imgui_impl_dx11.h"
#endif

#include <LLGL/Backend/OpenGL/NativeHandle.h>
#include <LLGL/Backend/Vulkan/NativeHandle.h>
#ifdef __APPLE__
#include <LLGL/Backend/Metal/NativeHandle.h>
#endif

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>

#include "sdl_llgl.h"
#include "ImGui_LLGL.h"

#ifdef WIN32
ID3D11Device* d3dDevice = nullptr;
ID3D11DeviceContext* d3dDeviceContext = nullptr;
#endif

static VkFormat GetVulkanColorFormat(LLGL::Format format) {
    return (format == LLGL::Format::BGRA8UNorm ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM);
}

static VkFormat GetVulkanDepthStencilFormat(LLGL::Format format) {
    return (format == LLGL::Format::D32Float ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_D24_UNORM_S8_UINT);
}

static VkRenderPass CreateVulkanRenderPass(VkDevice vulkanDevice, LLGL::SwapChain* swapChain) {
    VkAttachmentDescription vulkanAttachmentDescs[2] = {};
    {
        vulkanAttachmentDescs[0].flags = 0;
        vulkanAttachmentDescs[0].format = GetVulkanColorFormat(swapChain->GetColorFormat());
        vulkanAttachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;
        vulkanAttachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        vulkanAttachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    {
        vulkanAttachmentDescs[1].flags = 0;
        vulkanAttachmentDescs[1].format = GetVulkanDepthStencilFormat(swapChain->GetDepthStencilFormat());
        vulkanAttachmentDescs[1].samples = VK_SAMPLE_COUNT_1_BIT;
        vulkanAttachmentDescs[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        vulkanAttachmentDescs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        vulkanAttachmentDescs[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
        vulkanAttachmentDescs[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    VkAttachmentReference vulkanAttachmentRefs[2] = {};
    {
        vulkanAttachmentRefs[0].attachment = 0;
        vulkanAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    {
        vulkanAttachmentRefs[1].attachment = 1;
        vulkanAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    }

    VkSubpassDescription vulkanSubpassDescs[1] = {};
    {
        vulkanSubpassDescs[0].flags = 0;
        vulkanSubpassDescs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vulkanSubpassDescs[0].inputAttachmentCount = 0;
        vulkanSubpassDescs[0].pInputAttachments = nullptr;
        vulkanSubpassDescs[0].colorAttachmentCount = 1;
        vulkanSubpassDescs[0].pColorAttachments = &vulkanAttachmentRefs[0];
        vulkanSubpassDescs[0].pResolveAttachments = nullptr;
        vulkanSubpassDescs[0].pDepthStencilAttachment = &vulkanAttachmentRefs[1];
        vulkanSubpassDescs[0].preserveAttachmentCount = 0;
        vulkanSubpassDescs[0].pPreserveAttachments = nullptr;
    }

    VkRenderPassCreateInfo vulkanRenderPassInfo = {};
    {
        vulkanRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        vulkanRenderPassInfo.pNext = nullptr;
        vulkanRenderPassInfo.flags = 0;
        vulkanRenderPassInfo.attachmentCount = sizeof(vulkanAttachmentDescs) / sizeof(vulkanAttachmentDescs[0]);
        vulkanRenderPassInfo.pAttachments = vulkanAttachmentDescs;
        vulkanRenderPassInfo.subpassCount = sizeof(vulkanSubpassDescs) / sizeof(vulkanSubpassDescs[0]);
        vulkanRenderPassInfo.pSubpasses = vulkanSubpassDescs;
        vulkanRenderPassInfo.dependencyCount = 0;
        vulkanRenderPassInfo.pDependencies = nullptr;
    }
    VkRenderPass vulkanRenderPass = VK_NULL_HANDLE;
    VkResult result = vkCreateRenderPass(vulkanDevice, &vulkanRenderPassInfo, nullptr, &vulkanRenderPass);
    assert(result == VK_SUCCESS);
    return vulkanRenderPass;
}

void InitImGui(SDLSurface& wnd, LLGL::RenderSystemPtr& renderer, LLGL::SwapChain* swapChain) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // | ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    void* ctx = nullptr;

    switch (auto a = renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplSDL2_InitForOpenGL(wnd.wnd, ctx);
            // Setup renderer backend
#ifdef __APPLE__
            ImGui_ImplOpenGL3_Init("#version 410 core");
#else
            ImGui_ImplOpenGL3_Init("#version 120");
#endif
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplSDL2_InitForOpenGL(wnd.wnd, ctx);
            ImGui_ImplOpenGL3_Init("#version 300 es");
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            ImGui_ImplSDL2_InitForMetal(wnd.wnd);
            Imgui_Metal_llgl_Init(renderer);
            break;
#endif
#ifdef WIN32
        case LLGL::RendererID::Direct3D11:
            // Setup renderer backend
            LLGL::Direct3D11::RenderSystemNativeHandle nativeDeviceHandle;
            renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
            d3dDevice = nativeDeviceHandle.device;

            LLGL::Direct3D11::CommandBufferNativeHandle nativeContextHandle;
            cmdBuffer->GetNativeHandle(&nativeContextHandle, sizeof(nativeContextHandle));
            d3dDeviceContext = nativeContextHandle.deviceContext;

            ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);
            break;
#endif
        case LLGL::RendererID::Vulkan: {
            ImGui_ImplSDL2_InitForVulkan(wnd.wnd);
            LLGL::Vulkan::RenderSystemNativeHandle instance;
            renderer->GetNativeHandle(&instance, sizeof(LLGL::Vulkan::RenderSystemNativeHandle));
            VkDevice vulkanDevice = instance.device;

            // Create Vulkan render pass
            VkRenderPass vulkanRenderPass = CreateVulkanRenderPass(vulkanDevice, swapChain);

            ImGui_ImplVulkan_InitInfo initInfo = {};
            {
                initInfo.Instance = instance.instance;
                initInfo.PhysicalDevice = instance.physicalDevice;
                initInfo.Device = instance.device;
                initInfo.QueueFamily = instance.queueFamily;
                initInfo.Queue = instance.queue;
                initInfo.DescriptorPool = VK_NULL_HANDLE;
                initInfo.DescriptorPoolSize = 64;
                initInfo.RenderPass = vulkanRenderPass;
                initInfo.MinImageCount = 2;
                initInfo.ImageCount = 2;
                initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            }
            ImGui_ImplVulkan_Init(&initInfo);
            break;
        }
        default:
            io.BackendRendererName = "imgui_impl_null";
            break;
    }
}

void NewFrameImGui(LLGL::CommandBuffer* cmdBuffer, LLGL::RenderSystemPtr& renderer) {
    ImGui_ImplSDL2_NewFrame();
    switch (renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_NewFrame();
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_NewFrame();
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            Imgui_Metal_llgl_NewFrame(cmdBuffer);
            break;
#endif
#ifdef WIN32
        case LLGL::RendererID::Direct3D11:
            ImGui_ImplDX11_NewFrame();
            break;
#endif
        case LLGL::RendererID::Vulkan:
            ImGui_ImplVulkan_NewFrame();
            break;
        default:
            break;
    }
    ImGui::NewFrame();
}

void RenderImGui(LLGL::RenderSystemPtr& renderer, LLGL::CommandBuffer* llgl_cmdBuffer) {
    ImGui::Render();
    switch (renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal: {
            Imgui_Metal_llgl_RenderDrawData(ImGui::GetDrawData());
            break;
        }
#endif
#ifdef WIN32
        case LLGL::RendererID::Direct3D11:
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            break;
#endif
        case LLGL::RendererID::Vulkan: {
            LLGL::Vulkan::CommandBufferNativeHandle cmdBuffer;
            llgl_cmdBuffer->GetNativeHandle(&cmdBuffer, sizeof(LLGL::Vulkan::CommandBufferNativeHandle));
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer.commandBuffer);
            break;
        }
        default:
            break;
    }
    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        if (renderer->GetRendererID() == LLGL::RendererID::OpenGL ||
            renderer->GetRendererID() == LLGL::RendererID::OpenGLES) {
            SDL_Window* backupCurrentWindow = SDL_GL_GetCurrentWindow();
            SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
        } else {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}

void ShutdownImGui(LLGL::RenderSystemPtr& renderer) {
    // Shutdown ImGui
    switch (renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_Shutdown();
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_Shutdown();
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal:
            Imgui_Metal_llgl_Shutdown();
            break;
#endif
#ifdef WIN32
        case LLGL::RendererID::Direct3D11:
            ImGui_ImplDX11_Shutdown();
            break;
#endif
        case LLGL::RendererID::Vulkan:
            ImGui_ImplVulkan_Shutdown();
            break;
        default:
            break;
    }
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}