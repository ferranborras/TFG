#include "ControlsManager.h"

int8_t ControlsManager::buttonsState = 0b10000000;
int8_t ControlsManager::buttonsPrevState = buttonsState;
uint16_t ControlsManager::joyX = 0;
uint16_t ControlsManager::joyY = 0;
int16_t ControlsManager::vibrationMillisLeft = 0;
int16_t ControlsManager::vibrationMillisRight = 0;
uint32_t ControlsManager::lastUpdate = 0;
int16_t ControlsManager::fastTimer = 0;

void ControlsManager::Setup() {
    pinMode(UP, INPUT_PULLUP);
    pinMode(RIGHT, INPUT_PULLUP);
    pinMode(DOWN, INPUT_PULLUP);
    pinMode(LEFT, INPUT_PULLUP);

    pinMode(MID_RIGHT, INPUT_PULLUP);
    pinMode(MID_LEFT, INPUT_PULLUP);

    pinMode(JOYSTICK, INPUT_PULLUP);
    pinMode(JOYSTICK_X, INPUT);
    pinMode(JOYSTICK_Y, INPUT);

    pinMode(VOLUME_UP, INPUT_PULLUP);
    pinMode(VOLUME_DOWN, INPUT_PULLUP);

    pinMode(VIBRATOR_RIGHT, OUTPUT);
    pinMode(VIBRATOR_LEFT, OUTPUT);

    digitalWrite(VIBRATOR_RIGHT, LOW);
    digitalWrite(VIBRATOR_LEFT, LOW);

    DisableControls();
};

void ControlsManager::Update() {
    uint32_t now = millis();
    uint32_t deltaTime = now - lastUpdate;
    lastUpdate = now;

    if (ControlsEnabled()) {
        UpdateButtonsState();
        UpdateAxis();
    }
    else {
        ClearAllBits();
    }

    UpdateVolume(deltaTime);
    UpdateVibrations(deltaTime);
}

bool ControlsManager::GetButtonDown(uint8_t button) {
    uint8_t index = GetButtonIndex(button);
    return GetButtonBit(index) && !GetPrevBit(index);
}

bool ControlsManager::GetButtonUp(uint8_t button) {
    uint8_t index = GetButtonIndex(button);
    return !GetButtonBit(index) && GetPrevBit(index);
}

bool ControlsManager::GetButtonHold(uint8_t button) {
    uint8_t index = GetButtonIndex(button);
    return GetButtonBit(index) && GetPrevBit(index);
}

bool ControlsManager::GetButtonBit(uint8_t index) {
    if (index > 7) return false;
    return buttonsState & (1 << index);
}

bool ControlsManager::GetPrevBit(uint8_t index) {
    if (index > 7) return false;
    return buttonsPrevState & (1 << index);
}

void ControlsManager::MarkButtonBit(uint8_t index) {
    if (index > 7) return;
    buttonsState |= (1 << index);
}

void ControlsManager::ClearButtonBit(uint8_t index) {
    if (index > 7) return;
    buttonsState &= ~(1 << index);
}

bool ControlsManager::ControlsEnabled() {
    return GetButtonBit(7);
}

void ControlsManager::SetButtonBit(uint8_t index, bool value) {
    if (value) MarkButtonBit(index);
    else ClearButtonBit(index);
}

uint8_t ControlsManager::GetButtonIndex(uint8_t button) {
    switch (button) {
        case UP: return 0;
        case RIGHT: return 1;
        case DOWN: return 2;
        case LEFT: return 3;

        case MID_RIGHT: return 4;
        case MID_LEFT: return 5;

        case JOYSTICK: return 6;
        default: return 8;
    }
}

void ControlsManager::EnableControls() {
    buttonsState |= (1 << 7);
}

void ControlsManager::DisableControls() {
    buttonsState &= ~(1 << 7);
}

void ControlsManager::ClearAllBits() {
    buttonsState &= 0b10000000;
    buttonsPrevState = buttonsState;
}

void ControlsManager::UpdateButtonsState() {
    buttonsPrevState = buttonsState;

    SetButtonBit(GetButtonIndex(UP), digitalRead(UP) == LOW);
    SetButtonBit(GetButtonIndex(RIGHT), digitalRead(RIGHT) == LOW);
    SetButtonBit(GetButtonIndex(DOWN), digitalRead(DOWN) == LOW);
    SetButtonBit(GetButtonIndex(LEFT), digitalRead(LEFT) == LOW);

    SetButtonBit(GetButtonIndex(MID_RIGHT), digitalRead(MID_RIGHT) == LOW);
    SetButtonBit(GetButtonIndex(MID_LEFT), digitalRead(MID_LEFT) == LOW);

    SetButtonBit(GetButtonIndex(JOYSTICK), digitalRead(JOYSTICK) == LOW);
}

void ControlsManager::UpdateAxis() {
    joyX = analogRead(JOYSTICK_X); // 0 - 1023
    joyY = analogRead(JOYSTICK_Y);
}

float ControlsManager::GetX() {
    if (!ControlsEnabled()) return 0.0f;

    const int16_t newRange = 512; // Mitad de 1023
    int16_t delta = joyX - newRange; // Rango anterior: 0 a 1023 | Rango nuevo: -512 a 511

    // Si esta dentro del deadzone: 0
    if (abs(delta) < deadzone) return 0.0f;

    // Normalizar a rango [-1.0, 1.0]
    return (float)delta / (float)newRange;
}

float ControlsManager::GetY() {
    if (!ControlsEnabled()) return 0.0f;

    const int16_t newRange = 512;
    int16_t delta = joyY - newRange;

    if (abs(delta) < deadzone) return 0.0f;

    return (float)delta / (float)newRange;
}

int8_t ControlsManager::GetVolume() {
    if (digitalRead(VOLUME_DOWN) == HIGH) return -1;
    if (digitalRead(VOLUME_UP) == HIGH) return 1;
    return 0;
}

void ControlsManager::Vibrate(uint8_t pin, int16_t milliseconds) { // Si milliseconds < 0: tiempo indefinido
    if (pin == VIBRATOR_RIGHT) vibrationMillisRight = milliseconds;
    if (pin == VIBRATOR_LEFT) vibrationMillisLeft = milliseconds;
}

void ControlsManager::StopVibrations() {
    vibrationMillisRight = 0;
    vibrationMillisLeft = 0;
    digitalWrite(VIBRATOR_RIGHT, LOW);
    digitalWrite(VIBRATOR_LEFT, LOW);
}

void ControlsManager::UpdateVolume(uint32_t deltaTime) {
    if (fastTimer > 0) {
        fastTimer -= deltaTime;
        return;
    }

    int8_t volume = GetVolume();
    if (volume == 0)
        return;

    float amVolume = AudioManager::GetVolume();
    Serial.printf("amVolume: %.1f\n", amVolume);
    AudioManager::SetGlobalVolume(amVolume + (float)(volume)/10.0f);
    fastTimer = 200; // 0.2s entre cambios de volumen
}

void ControlsManager::UpdateVibrations(uint32_t deltaTime) {
    // RIGHT
    if (vibrationMillisRight > 0) {
        digitalWrite(VIBRATOR_RIGHT, HIGH);
        if ((uint32_t)vibrationMillisRight <= deltaTime)
            vibrationMillisRight = 0;
        else
            vibrationMillisRight -= deltaTime;
    }
    else if (vibrationMillisRight == -1)
        digitalWrite(VIBRATOR_RIGHT, HIGH);
    else {
        digitalWrite(VIBRATOR_RIGHT, LOW);
        vibrationMillisRight = 0;
    }

    // LEFT
    if (vibrationMillisLeft > 0) {
        digitalWrite(VIBRATOR_LEFT, HIGH);
        if ((uint32_t)vibrationMillisLeft <= deltaTime)
            vibrationMillisLeft = 0;
        else
            vibrationMillisLeft -= deltaTime;
    }
    else if (vibrationMillisLeft == -1)
        digitalWrite(VIBRATOR_LEFT, HIGH);
    else {
        digitalWrite(VIBRATOR_LEFT, LOW);
        vibrationMillisLeft = 0;
    }
}