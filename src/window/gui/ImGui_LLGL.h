#ifndef IMGUI_LLGL_H
#define IMGUI_LLGL_H

#include <SDL2/SDL_video.h>
#include "window/gui/Gui.h"

void ImGui_ImplLLGL_Init(Ship::GuiWindowInitData& mImpl);
void ImGui_ImplLLGL_NewFrame();

#endif