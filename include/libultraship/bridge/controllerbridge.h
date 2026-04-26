#pragma once

#include <stdint.h>
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prevents controller gamepad input from being processed by the game.
 *
 * Multiple callers can independently block input; each must supply a unique @p inputBlockId
 * and must call ControllerUnblockGameInput() with the same ID to release its block.
 *
 * @param inputBlockId Caller-chosen identifier that distinguishes this blocker.
 */
API_EXPORT void ControllerBlockGameInput(uint16_t inputBlockId);

/**
 * @brief Releases a previously established game input block.
 *
 * Input is only fully unblocked once all outstanding blockers have called this function.
 *
 * @param inputBlockId The same ID passed to ControllerBlockGameInput().
 */
API_EXPORT void ControllerUnblockGameInput(uint16_t inputBlockId);

/**
 * @brief Reads current Switch gyro values for a controller port.
 * @return 1 when gyro data was read successfully, 0 otherwise.
 */
API_EXPORT int32_t ControllerReadSwitchGyroDebug(uint8_t portIndex, float* pitch, float* yaw, float* roll);

/**
 * @brief Returns the raw Switch style bitmask for a controller port.
 */
API_EXPORT uint64_t ControllerGetSwitchStyleSetDebug(uint8_t portIndex);

/**
 * @brief Returns a normalized active style code for a controller port.
 * 0=None, 1=Handheld, 2=FullKey, 3=JoyDual, 4=JoyLeft, 5=JoyRight.
 */
API_EXPORT int32_t ControllerGetSwitchStyleDebug(uint8_t portIndex);

#ifdef __cplusplus
};
#endif
