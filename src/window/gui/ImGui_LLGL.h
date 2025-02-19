#ifndef IMGUI_LLGL_H
#define IMGUI_LLGL_H

#include <SDL2/SDL_video.h>
#include "Gui.h"

void ImGui_ImplSDL2_LLGL(SDL_Window* window, void* context);
void ImGui_ImplLLGL_Init(Ship::GuiWindowInitData window);
void ImGui_ImplLLGL_NewFrame();

#endif