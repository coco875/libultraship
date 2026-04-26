#include "libultraship/bridge/controllerbridge.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/Context.h"
#ifdef __SWITCH__
#include "ship/port/switch/SwitchController.h"
#endif

extern "C" {

void ControllerBlockGameInput(uint16_t inputBlockId) {
    Ship::Context::GetInstance()->GetControlDeck()->BlockGameInput(static_cast<int32_t>(inputBlockId));
}

void ControllerUnblockGameInput(uint16_t inputBlockId) {
    Ship::Context::GetInstance()->GetControlDeck()->UnblockGameInput(static_cast<int32_t>(inputBlockId));
}

int32_t ControllerReadSwitchGyroDebug(uint8_t portIndex, float* pitch, float* yaw, float* roll) {
    if (pitch == nullptr || yaw == nullptr || roll == nullptr) {
        return 0;
    }

#ifdef __SWITCH__
    return Ship::SwitchController::GetInstance().ReadGyro(portIndex, *pitch, *yaw, *roll) ? 1 : 0;
#else
    *pitch = 0.0f;
    *yaw = 0.0f;
    *roll = 0.0f;
    return 0;
#endif
}

uint64_t ControllerGetSwitchStyleSetDebug(uint8_t portIndex) {
#ifdef __SWITCH__
    return Ship::SwitchController::GetInstance().GetStyleSet(portIndex);
#else
    (void)portIndex;
    return 0;
#endif
}

int32_t ControllerGetSwitchStyleDebug(uint8_t portIndex) {
#ifdef __SWITCH__
    return Ship::SwitchController::GetInstance().GetActiveStyleDebug(portIndex);
#else
    (void)portIndex;
    return 0;
#endif
}
}
