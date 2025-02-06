#pragma once
#include "window/Window.h"
#include "graphic/Fast3D/gfx_window_manager_api.h"
#include "graphic/Fast3D/gfx_rendering_api.h"
#include "public/bridge/gfxbridge.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

union Gfx;

namespace Fast {
class Fast3dWindow : public Ship::Window {
  public:
    Fast3dWindow();
    Fast3dWindow(std::vector<std::shared_ptr<Ship::GuiWindow>> guiWindows);
    ~Fast3dWindow();

    void Init() override;
    void Close() override;
    void StartFrame() override;
    void EndFrame() override;
    bool IsFrameReady() override;
    void HandleEvents() override;
    void SetCursorVisibility(bool visible) override;
    uint32_t GetWidth() override;
    uint32_t GetHeight() override;
    int32_t GetPosX() override;
    int32_t GetPosY() override;
    void SetMousePos(Ship::Coords pos) override;
    Ship::Coords GetMousePos() override;
    Ship::Coords GetMouseDelta() override;
    Ship::CoordsF GetMouseWheel() override;
    bool GetMouseState(Ship::MouseBtn btn) override;
    void SetMouseCapture(bool capture) override;
    bool IsMouseCaptured() override;
    uint32_t GetCurrentRefreshRate() override;
    bool SupportsWindowedFullscreen() override;
    bool CanDisableVerticalSync() override;
    void SetResolutionMultiplier(float multiplier) override;
    void SetMsaaLevel(uint32_t value) override;
    void SetFullscreen(bool isFullscreen) override;
    bool IsFullscreen() override;
    bool IsRunning() override;
    const char* GetKeyName(int32_t scancode) override;

    void InitWindowManager();
    void SetTargetFps(int32_t fps);
    void SetMaximumFrameLatency(int32_t latency);
    void GetPixelDepthPrepare(float x, float y);
    uint16_t GetPixelDepth(float x, float y);
    void SetTextureFilter(FilteringMode filteringMode);
    void SetRendererUCode(UcodeHandlers ucode);
    void EnableSRGBMode();
    void* GetWindowPtr();
    bool DrawAndRunGraphicsCommands(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtxReplacements);

  protected:
    static bool KeyDown(int32_t scancode);
    static bool KeyUp(int32_t scancode);
    static void AllKeysUp();
    static bool MouseButtonDown(int button);
    static bool MouseButtonUp(int button);
    static void OnFullscreenChanged(bool isNowFullscreen);

  private:
    GfxRenderingAPI* mRenderingApi;
    GfxWindowManagerAPI* mWindowManagerApi;
};
} // namespace Fast
