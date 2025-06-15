#ifndef GFX_LLGL_H
#define GFX_LLGL_H

#include "gfx_rendering_api.h"
#include "graphic/Fast3D/interpreter.h"
#include <LLGL/LLGL.h>
#include "LLGL/Utils/VertexFormat.h"
#include <functional>
#include <cstddef>

extern LLGL::RenderSystemPtr llgl_renderer;
extern LLGL::SwapChain* llgl_swapChain;
extern LLGL::CommandBuffer* llgl_cmdBuffer;
namespace Fast {

// Hash function for std::tuple<bool, int, int>
struct tuple_bool_int_int_hash {
    std::size_t operator()(const std::tuple<bool, int, int>& t) const {
        std::size_t h1 = std::hash<bool>{}(std::get<0>(t));
        std::size_t h2 = std::hash<int>{}(std::get<1>(t));
        std::size_t h3 = std::hash<int>{}(std::get<2>(t));
        // Combine hashes
        return h1 ^ (h2 << 1) ^ (h3 << 4);
    }
};

struct ShaderProgramLLGL {
    int numInputs;
    int frameCountBinding;
    int noiseScaleBinding;
    std::optional<int> bindingTexture[2];
    std::optional<int> bindingTextureSampl[2];
    std::optional<int> bindingMask[2];
    std::optional<int> bindingMaskSampl[2];
    std::optional<int> bindingBlend[2];
    std::optional<int> bindingBlendSampl[2];
    LLGL::VertexFormat vertexFormat;
    LLGL::PipelineState* pipeline[2][2]; // [depth disabled][zmode decal]
    LLGL::Buffer* vertexBuffer;
    size_t vertexBufferSize = 0;
};
class GfxRenderingAPILLGL {
  public:
    GfxRenderingAPILLGL(GfxWindowBackend* backend);
    ~GfxRenderingAPILLGL() = default;
    const char* GetName();
    int GetMaxTextureSize();
    GfxClipParameters GetClipParameters();
    void UnloadShader(ShaderProgram* oldPrg);
    void LoadShader(ShaderProgram* newPrg);
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint32_t shaderId1);
    ShaderProgram* LookupShader(uint64_t shaderId0, uint32_t shaderId1);
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]);
    uint32_t NewTexture();
    void SelectTexture(int tile, uint32_t textureId);
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height);
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt);
    void SetDepthTestAndMask(bool depth_test, bool z_upd);
    void SetZmodeDecal(bool decal);
    void SetViewport(int x, int y, int width, int height);
    void SetScissor(int x, int y, int width, int height);
    void SetUseAlpha(bool useAlpha);
    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris);
    void Init();
    void OnResize();
    void StartFrame();
    void EndFrame();
    void FinishRender();
    int CreateFramebuffer();
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth);
    void StartDrawToFramebuffer(int fbId, float noiseScale);
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0,
                         int dstX1, int dstY1);
    void ClearFramebuffer(bool color, bool depth);
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf);
    void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc);
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates);
    void* GetFramebufferTextureId(int fbId);
    void SelectTextureFb(int fbId);
    void DeleteTexture(uint32_t texId);
    void SetTextureFilter(FilteringMode mode);
    FilteringMode GetTextureFilter();
    void SetSrgbMode();
    ImTextureID GetTextureById(int id);

  private:
    int current_tile;
    uint32_t current_texture_ids[6] = { 0, 0, 0, 0, 0, 0 };
    std::unordered_map<std::tuple<bool, int, int>, LLGL::Sampler*, tuple_bool_int_int_hash> samplers = {};
    std::vector<std::pair<LLGL::Texture*, LLGL::Sampler*>> textures;
    std::vector<std::pair<LLGL::RenderTarget*, int>> framebuffers;
    bool srgb_mode = false;
    Fast::FilteringMode current_filter_mode = Fast::FILTER_NONE;
    std::string llgl_build_fs_shader(const CCFeatures& cc_features, LLGL::PipelineLayoutDescriptor& layoutDesc);
    std::string llgl_build_vs_shader(const CCFeatures& cc_features, LLGL::PipelineLayoutDescriptor& layoutDesc,
                                     LLGL::VertexFormat& vertexFormat);
    std::map<std::pair<uint64_t, uint32_t>, struct ShaderProgramLLGL> mShaderProgramPool;
    bool disable_depth = true;
    bool disable_write_depth = true;
    struct ShaderProgramLLGL* mCurrentShaderProgram = nullptr;
    std::vector<LLGL::Buffer*> garbage_collection_buffers;
    int frame_count = 0;
    float noise_scale = 0.0f;
    LLGL::Buffer* frameCountBuffer;
    LLGL::Buffer* noiseScaleBuffer;
    int current_framebuffer_id = 0;
    GfxWindowBackend* mWindowBackend = nullptr;
};
} // namespace Fast

#endif