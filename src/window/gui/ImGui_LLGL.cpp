#include <LLGL/LLGL.h>
#include <LLGL/Platform/NativeHandle.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#ifdef LLGL_BUILD_RENDERER_VULKAN
#include "imgui_impl_vulkan.h"
#endif
#ifdef __APPLE__
#include "imgui_impl_metal.h"
#endif
#ifdef WIN32
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx12.h"
#endif

#include <LLGL/Backend/OpenGL/NativeHandle.h>
#ifdef LLGL_BUILD_RENDERER_VULKAN
#include <LLGL/Backend/Vulkan/NativeHandle.h>
#endif
#ifdef __APPLE__
#include <LLGL/Backend/Metal/NativeHandle.h>
#endif
#ifdef WIN32
#include <LLGL/Backend/Direct3D11/NativeHandle.h>
#include <LLGL/Backend/Direct3D12/NativeHandle.h>
#endif

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>

#include "sdl_llgl.h"
#include "ImGui_LLGL.h"

#ifdef WIN32
ID3D11Device* d3d11Device = nullptr;
ID3D11DeviceContext* d3d11DeviceContext = nullptr;
ID3D12Device* d3d12Device = nullptr;
ID3D12CommandQueue* d3d12CommandQueue = nullptr;
ID3D12GraphicsCommandList* d3d12CommandList = nullptr;

static DXGI_FORMAT GetRTVFormat(LLGL::Format format) {
    switch (format) {
        case LLGL::Format::RGBA8UNorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

static DXGI_FORMAT GetDSVFormat(LLGL::Format format) {
    switch (format) {
        case LLGL::Format::D16UNorm:
            return DXGI_FORMAT_D16_UNORM;
        case LLGL::Format::D24UNormS8UInt:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case LLGL::Format::D32Float:
            return DXGI_FORMAT_D32_FLOAT;
        case LLGL::Format::D32FloatS8X24UInt:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

#define SAFE_RELEASE(OBJ)   \
    if ((OBJ) != nullptr) { \
        (OBJ)->Release();   \
        OBJ = nullptr;      \
    }

// Helper class to allocate D3D12 descriptors
class D3D12DescriptorHeapAllocator {
    ID3D12DescriptorHeap* d3dHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE d3dCPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE d3dGPUHandle;
    UINT d3dHandleSize = 0;
    std::vector<UINT> freeIndices;

  public:
    D3D12DescriptorHeapAllocator(ID3D12Device* d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors) {
        // Create D3D12 descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC d3dSRVDescriptorHeapDesc = {};
        {
            d3dSRVDescriptorHeapDesc.Type = type;
            d3dSRVDescriptorHeapDesc.NumDescriptors = numDescriptors;
            d3dSRVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            d3dSRVDescriptorHeapDesc.NodeMask = 0;
        }
        HRESULT result = d3dDevice->CreateDescriptorHeap(&d3dSRVDescriptorHeapDesc, IID_PPV_ARGS(&d3dHeap));
        LLGL_VERIFY(SUCCEEDED(result));

        d3dCPUHandle = d3dHeap->GetCPUDescriptorHandleForHeapStart();
        d3dGPUHandle = d3dHeap->GetGPUDescriptorHandleForHeapStart();
        d3dHandleSize = d3dDevice->GetDescriptorHandleIncrementSize(type);

        // Initialize free indices
        freeIndices.reserve(numDescriptors);
        for (UINT n = numDescriptors; n > 0; --n)
            freeIndices.push_back(n - 1);
    }

    ~D3D12DescriptorHeapAllocator() {
        SAFE_RELEASE(d3dHeap);
    }

    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE& outCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGPUHandle) {
        LLGL_VERIFY(!freeIndices.empty());

        UINT index = freeIndices.back();
        freeIndices.pop_back();

        outCPUHandle.ptr = d3dCPUHandle.ptr + (index * d3dHandleSize);
        outGPUHandle.ptr = d3dGPUHandle.ptr + (index * d3dHandleSize);
    }

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE inCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE inGPUHandle) {
        const UINT cpuIndex = static_cast<UINT>((inCPUHandle.ptr - d3dCPUHandle.ptr) / d3dHandleSize);
        const UINT gpuIndex = static_cast<UINT>((inGPUHandle.ptr - d3dGPUHandle.ptr) / d3dHandleSize);
        LLGL_VERIFY(cpuIndex == gpuIndex);
        freeIndices.push_back(cpuIndex);
    }

    ID3D12DescriptorHeap* GetNative() const {
        return d3dHeap;
    }
};

using D3D12DescriptorHeapAllocatorPtr = std::unique_ptr<D3D12DescriptorHeapAllocator>;

static D3D12DescriptorHeapAllocatorPtr g_heapAllocator;
#endif

#ifdef LLGL_BUILD_RENDERER_VULKAN
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
#endif

void InitImGui(SDLSurface& wnd, LLGL::RenderSystemPtr& renderer, LLGL::SwapChain* swapChain,
               LLGL::CommandBuffer* cmdBuffer) {
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
            ImGui_ImplSDL2_InitForD3D(wnd.wnd);
            // Setup renderer backend
            LLGL::Direct3D11::RenderSystemNativeHandle nativeDeviceHandleD11;
            renderer->GetNativeHandle(&nativeDeviceHandleD11, sizeof(nativeDeviceHandleD11));
            d3d11Device = nativeDeviceHandleD11.device;

            LLGL::Direct3D11::CommandBufferNativeHandle nativeContextHandleD11;
            cmdBuffer->GetNativeHandle(&nativeContextHandleD11, sizeof(nativeContextHandleD11));
            d3d11DeviceContext = nativeContextHandleD11.deviceContext;

            ImGui_ImplDX11_Init(d3d11Device, d3d11DeviceContext);
            break;
        case LLGL::RendererID::Direct3D12: {
            ImGui_ImplSDL2_InitForD3D(wnd.wnd);
            // Create SRV descriptor heap for ImGui's internal resources
            LLGL::Direct3D12::RenderSystemNativeHandle nativeDeviceHandleD12;
            renderer->GetNativeHandle(&nativeDeviceHandleD12, sizeof(nativeDeviceHandleD12));
            d3d12Device = nativeDeviceHandleD12.device;
            d3d12CommandQueue = nativeDeviceHandleD12.commandQueue;

            g_heapAllocator = D3D12DescriptorHeapAllocatorPtr(
                new D3D12DescriptorHeapAllocator{ d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64 });
            // Setup renderer backend
            LLGL::Direct3D12::CommandBufferNativeHandle nativeContextHandleD12;
            cmdBuffer->GetNativeHandle(&nativeContextHandleD12, sizeof(nativeContextHandleD12));
            d3d12CommandList = nativeContextHandleD12.commandList;

            // Initialize ImGui D3D12 backend
            ImGui_ImplDX12_InitInfo imGuiInfo = {};
            {
                imGuiInfo.Device = d3d12Device;
                imGuiInfo.CommandQueue = d3d12CommandQueue;
                imGuiInfo.NumFramesInFlight = 2;
                imGuiInfo.RTVFormat = GetRTVFormat(swapChain->GetColorFormat());
                imGuiInfo.DSVFormat = GetDSVFormat(swapChain->GetDepthStencilFormat());
                imGuiInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
                                                    D3D12_CPU_DESCRIPTOR_HANDLE* outCPUDescHandle,
                                                    D3D12_GPU_DESCRIPTOR_HANDLE* outGPUDescHandle) {
                    g_heapAllocator->Alloc(*outCPUDescHandle, *outGPUDescHandle);
                };
                imGuiInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info,
                                                   D3D12_CPU_DESCRIPTOR_HANDLE inCPUDescHandle,
                                                   D3D12_GPU_DESCRIPTOR_HANDLE inGPUDescHandle) {
                    g_heapAllocator->Free(inCPUDescHandle, inGPUDescHandle);
                };
            }
            ImGui_ImplDX12_Init(&imGuiInfo);
            break;
        }
#endif
#ifdef LLGL_BUILD_RENDERER_VULKAN
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
#endif
        default:
            io.BackendRendererName = "imgui_impl_null";
            break;
    }
}

void NewFrameImGui(LLGL::RenderSystemPtr& renderer, LLGL::CommandBuffer* cmdBuffer) {
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
        case LLGL::RendererID::Direct3D12:
            ImGui_ImplDX12_NewFrame();
            break;
#endif
#ifdef LLGL_BUILD_RENDERER_VULKAN
        case LLGL::RendererID::Vulkan:
            ImGui_ImplVulkan_NewFrame();
            break;
#endif
        default:
            break;
    }
}

void RenderImGui(ImDrawData* data, LLGL::RenderSystemPtr& renderer, LLGL::CommandBuffer* llgl_cmdBuffer) {
    switch (renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            ImGui_ImplOpenGL3_RenderDrawData(data);
            break;
        case LLGL::RendererID::OpenGLES:
            ImGui_ImplOpenGL3_RenderDrawData(data);
            break;
#ifdef __APPLE__
        case LLGL::RendererID::Metal: {
            Imgui_Metal_llgl_RenderDrawData(data);
            break;
        }
#endif
#ifdef WIN32
        case LLGL::RendererID::Direct3D11:
            ImGui_ImplDX11_RenderDrawData(data);
            break;
        case LLGL::RendererID::Direct3D12: {
            ID3D12DescriptorHeap* d3dHeap = g_heapAllocator->GetNative();
            d3d12CommandList->SetDescriptorHeaps(1, &d3dHeap);

            ImGui_ImplDX12_RenderDrawData(data, d3d12CommandList);
            break;
        }
#endif
#ifdef LLGL_BUILD_RENDERER_VULKAN
        case LLGL::RendererID::Vulkan: {
            LLGL::Vulkan::CommandBufferNativeHandle cmdBuffer;
            llgl_cmdBuffer->GetNativeHandle(&cmdBuffer, sizeof(LLGL::Vulkan::CommandBufferNativeHandle));
            ImGui_ImplVulkan_RenderDrawData(data, cmdBuffer.commandBuffer);
            break;
        }
#endif
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
        case LLGL::RendererID::Direct3D12:
            ImGui_ImplDX12_Shutdown();
            break;
#endif
#ifdef LLGL_BUILD_RENDERER_VULKAN
        case LLGL::RendererID::Vulkan:
            ImGui_ImplVulkan_Shutdown();
            break;
#endif
        default:
            break;
    }
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}