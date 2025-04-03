#ifndef SDL_LLGL_H
#define SDL_LLGL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>

class SDLSurface final : public LLGL::Surface {
  public:
    // Constructor and destructor
    SDLSurface(const LLGL::Extent2D& size, const char* title, int rendererID, LLGL::RenderSystemDescriptor& desc);
    ~SDLSurface();

    // Interface implementation
    bool GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) override;
    LLGL::Extent2D GetContentSize() const override;
    bool AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) override;
    LLGL::Display* FindResidentDisplay() const override;
    bool ProcessEvents(LLGL::SwapChain* swapChain);

    SDL_Window* wnd = nullptr;
    LLGL::Extent2D size_;

  private:
    std::string title_;
};

#endif