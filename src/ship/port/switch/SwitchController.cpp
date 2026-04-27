#ifdef __SWITCH__
#include "SwitchController.h"
#include <algorithm>

namespace Ship {

static constexpr uint64_t CONTROLLER_MASK = 1UL;

SwitchController& SwitchController::GetInstance() {
    static SwitchController instance;
    return instance;
}

HidNpadIdType SwitchController::GetNpadId(uint8_t portIndex) const {
    const uint8_t clampedIndex = std::min<uint8_t>(portIndex, 3);
    return static_cast<HidNpadIdType>(HidNpadIdType_No1 + clampedIndex);
}

bool SwitchController::EnsureInitialized(uint8_t portIndex) {
    if (portIndex >= mControllers.size()) {
        return false;
    }

    auto& controller = mControllers[portIndex];
    if (controller.Initialized) {
        return true;
    }

    const auto npadId = GetNpadId(portIndex);
    const uint64_t padMask = (CONTROLLER_MASK << npadId) | (CONTROLLER_MASK << HidNpadIdType_Handheld);

    padInitializeWithMask(&controller.State, padMask);
    padUpdate(&controller.State);

    hidInitializeVibrationDevices(controller.Handles[0], 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidInitializeVibrationDevices(controller.Handles[1], 2, npadId, HidNpadStyleTag_NpadJoyDual);

    hidGetSixAxisSensorHandles(&controller.Sensors[0], 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidGetSixAxisSensorHandles(&controller.Sensors[1], 1, npadId, HidNpadStyleTag_NpadFullKey);
    hidGetSixAxisSensorHandles(&controller.Sensors[2], 2, npadId, HidNpadStyleTag_NpadJoyDual);

    for (auto& sensor : controller.Sensors) {
        hidStartSixAxisSensor(sensor);
    }

    controller.Initialized = true;
    return true;
}

bool SwitchController::ReadSixAxisState(uint8_t portIndex, HidSixAxisSensorState& state) {
    if (!EnsureInitialized(portIndex)) {
        return false;
    }

    auto& controller = mControllers[portIndex];
    padUpdate(&controller.State);
    const uint64_t styleSet = padGetStyleSet(&controller.State);

    if (styleSet & HidNpadStyleTag_NpadFullKey) {
        hidGetSixAxisSensorStates(controller.Sensors[1], &state, 1);
        return true;
    }

    if (styleSet & HidNpadStyleTag_NpadJoyDual) {
        const uint64_t attributes = padGetAttributes(&controller.State);
        if (attributes & HidNpadAttribute_IsLeftConnected) {
            hidGetSixAxisSensorStates(controller.Sensors[2], &state, 1);
            return true;
        }
        if (attributes & HidNpadAttribute_IsRightConnected) {
            hidGetSixAxisSensorStates(controller.Sensors[3], &state, 1);
            return true;
        }
    }

    if (styleSet & HidNpadStyleTag_NpadHandheld) {
        hidGetSixAxisSensorStates(controller.Sensors[0], &state, 1);
        return true;
    }

    return false;
}

bool SwitchController::ReadGyro(uint8_t portIndex, float& pitch, float& yaw, float& roll) {
    HidSixAxisSensorState sixAxisState = {};
    if (!ReadSixAxisState(portIndex, sixAxisState)) {
        pitch = 0.0f;
        yaw = 0.0f;
        roll = 0.0f;
        return false;
    }

    pitch = sixAxisState.angular_velocity.x * 8.0f;
    yaw = sixAxisState.angular_velocity.y * 8.0f;
    roll = sixAxisState.angular_velocity.z * 8.0f;
    return true;
}

void SwitchController::SendRumble(uint8_t portIndex, float lowFrequencyAmplitude, float highFrequencyAmplitude) {
    if (!EnsureInitialized(portIndex)) {
        return;
    }

    auto& controller = mControllers[portIndex];
    padUpdate(&controller.State);
    const uint64_t styleSet = padGetStyleSet(&controller.State);

    uint64_t externalStyle = 0;
    if (styleSet & HidNpadStyleTag_NpadFullKey) {
        externalStyle = HidNpadStyleTag_NpadFullKey;
    } else if (styleSet & HidNpadStyleTag_NpadJoyDual) {
        externalStyle = HidNpadStyleTag_NpadJoyDual;
    }

    if (externalStyle != 0 && controller.LastExternalRumbleStyle != externalStyle) {
        hidInitializeVibrationDevices(controller.Handles[1], 2, GetNpadId(portIndex),
                                      static_cast<HidNpadStyleTag>(externalStyle));
        controller.LastExternalRumbleStyle = externalStyle;
    }

    HidVibrationValue vibrationValues[2] = {};
    for (auto& value : vibrationValues) {
        value.amp_low = std::clamp(lowFrequencyAmplitude, 0.0f, 1.0f);
        value.amp_high = std::clamp(highFrequencyAmplitude, 0.0f, 1.0f);
        value.freq_low = 160.0f;
        value.freq_high = 320.0f;
    }

    if (padIsHandheld(&controller.State)) {
        hidSendVibrationValues(controller.Handles[0], vibrationValues, 2);
    }

    if (externalStyle != 0) {
        hidSendVibrationValues(controller.Handles[1], vibrationValues, 2);
    }
}

} // namespace Ship
#endif