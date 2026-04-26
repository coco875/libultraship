#ifdef __SWITCH__
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <switch.h>

namespace Ship {

// Small libnx-backed state cache for each logical port used by mapping classes.
struct NXControllerState {
    PadState State = {};
    HidVibrationDeviceHandle Handles[2][2] = {};
    HidSixAxisSensorHandle Sensors[4] = {};
    uint64_t LastExternalRumbleStyle = 0;
    bool Initialized = false;
};

class SwitchController {
  public:
    static SwitchController& GetInstance();
    bool ReadGyro(uint8_t portIndex, float& pitch, float& yaw, float& roll);
    void SendRumble(uint8_t portIndex, float lowFrequencyAmplitude, float highFrequencyAmplitude);
    uint64_t GetStyleSet(uint8_t portIndex);
    int32_t GetActiveStyleDebug(uint8_t portIndex);

  private:
    SwitchController() = default;
    bool EnsureInitialized(uint8_t portIndex);
    HidNpadIdType GetNpadId(uint8_t portIndex) const;
    bool ReadSixAxisState(uint8_t portIndex, HidSixAxisSensorState& state);

    std::array<NXControllerState, 4> mControllers;
};
} // namespace Ship
#endif