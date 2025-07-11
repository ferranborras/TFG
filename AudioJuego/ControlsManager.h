#ifndef CONTROLS_MANAGER_H
#define CONTROLS_MANAGER_H

#include <Arduino.h>
#include "AudioManager.h"

class ControlsManager {
private:
    static uint32_t lastUpdate; // Millis del ultimo update
    static int16_t fastTimer;

    static int8_t buttonsState;     // Cada bit funciona como booleano de cada boton (bit 7: controles activados o desactivados)
    static int8_t buttonsPrevState; // Estado en el frame anterior

    static uint16_t joyX; // Joystick X axis
    static uint16_t joyY; // Joystick Y axis
    static constexpr uint8_t deadzone = 10; // margen muerto del joystick

    static int16_t vibrationMillisLeft;
    static int16_t vibrationMillisRight;

    static void UpdateButtonsState();
    static void UpdateAxis();
    static void UpdateVolume(uint32_t deltaTime);
    static void UpdateVibrations(uint32_t deltaTime);

    static uint8_t GetButtonIndex(uint8_t button);

    static bool GetButtonBit(uint8_t index);
    static bool GetPrevBit(uint8_t index);
    static void MarkButtonBit(uint8_t index);
    static void ClearButtonBit(uint8_t index);
    static void SetButtonBit(uint8_t index, bool value);
    static void ClearAllBits();

public:
    static constexpr uint8_t UP = 38;           // buttonsState bit 0
    static constexpr uint8_t RIGHT = 35;        // buttonsState bit 1
    static constexpr uint8_t DOWN = 36;         // buttonsState bit 2
    static constexpr uint8_t LEFT = 37;         // buttonsState bit 3

    static constexpr uint8_t MID_RIGHT = 29;    // buttonsState bit 4
    static constexpr uint8_t MID_LEFT = 28;     // buttonsState bit 5

    static constexpr uint8_t JOYSTICK = 27;     // buttonsState bit 6
    static constexpr uint8_t JOYSTICK_X = A15;
    static constexpr uint8_t JOYSTICK_Y = A16;

    static constexpr uint8_t VOLUME_UP = 32;
    static constexpr uint8_t VOLUME_DOWN = 31;

    static constexpr uint8_t VIBRATOR_RIGHT = 34;
    static constexpr uint8_t VIBRATOR_LEFT = 30;

    ControlsManager() = default;

    static void Setup();
    static void Update();

    static bool ControlsEnabled();
    static void EnableControls();
    static void DisableControls();

    static bool GetButtonDown(uint8_t button);
    static bool GetButtonUp(uint8_t button);
    static bool GetButtonHold(uint8_t button);

    static float GetX();
    static float GetY();

    static int8_t GetVolume();

    static void Vibrate(uint8_t pin, int16_t milliseconds);

    static void StopVibrations();
};

#endif