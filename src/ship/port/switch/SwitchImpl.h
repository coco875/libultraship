#pragma once

#include <cstdint>
#include <string>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include "SwitchPerformanceProfiles.h"

namespace Ship {
enum SwitchProfiles { MAXIMUM, HIGH, BOOST, STOCK, POWERSAVINGM1, POWERSAVINGM2, POWERSAVINGM3 };

enum SwitchPhase { PreInitPhase, PostInitPhase };

class Switch {
  public:
    static void Init(SwitchPhase phase);
    static void Exit();
    static void ImGuiSetupFont(ImFontAtlas* fonts);
    static void ImGuiProcessEvent(bool wantsTextInput);
    static void CreateKeyboard();
    static void ShowKeyboard();
    static bool IsRunning();
    static void GetDisplaySize(int* width, int* height);
    static void ApplyOverclock();
    static void ShowErrorApplet(const char* format, ...);
    static void ThrowMissingOTR(std::string otrPath);
    static char* GetControllerUUID(int controller);
};
}; // namespace Ship