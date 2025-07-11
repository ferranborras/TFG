#include <Arduino.h>
#include <utility>
#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <string>
#include <Wire.h>

#include "IAudioPlayer.h"
#include "AudioPlayer.h"
#include "BinauralAudioPlayer.h"
#include "QuickAudioPlayer.h"

#include "AudioManager.h"
using AM = AudioManager;
#include "ControlsManager.h"
using CM = ControlsManager;

#include "HRTFData.h"
#include "Vector3.h"

// WAV
const char* audio1 = "menu_soundtrack_title.raw";
const uint32_t loopStartMenuSoundtrack = 1424532; // Bytes a partir de donde empieza el loop
const char* audio2 = "game_missile.raw";

uint16_t timer = 0;
uint16_t lastUpdate = 0;

int8_t bgMusic = -1;
int8_t binauralSound = -1;

void UpdateActives();

void setup() {
    AudioMemory(300); // Reservar memoria de audio

    Serial.begin(9600);

    while (!Serial && millis() < 3000);  // Espera hasta que el puerto serial este listo
    Serial.println("Inicializando...");

    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("Can't start SD memory.");
        return;
    }

    CM::Setup();
    CM::EnableControls();
    AM::Setup();

    IAudioPlayer* bgMusicPtr = new AudioPlayer(audio1, -1, loopStartMenuSoundtrack);
    bgMusic = AM::Add(bgMusicPtr, 1.0f);

    //BinauralAudioPlayer* binauralSoundPtr = new BinauralAudioPlayer(audio1, -1);
    //binauralSound = AM::Add(binauralSoundPtr, 1);
    AM::PlayAll();
    Serial.printf("bgMusic: %d, binauralSound: %d\n", bgMusic, binauralSound);
}

void loop() {
    UpdateActives();
    //Serial.printf("bgMusic: %d, binauralSound: %d\n", bgMusic, binauralSound);

    uint16_t now = millis();
    timer = now - lastUpdate;
    lastUpdate = now;

    float x = CM::GetX();
    float y = CM::GetY();
    //Serial.printf("X: %.2f | Y: %.2f", x, y);

    Vector3 binauralLoc = Vector3(-x, y, 0).normalized().multiply(HRTFData::MAX_R);
    if (binauralLoc.magnitude() < HRTFData::MIN_R) {
        float x2y2 = binauralLoc.x * binauralLoc.x + binauralLoc.y * binauralLoc.y;
        float minR2 = HRTFData::MIN_R * HRTFData::MIN_R;

        if (x2y2 < minR2)
            binauralLoc.z = sqrt(minR2 - x2y2);
    }
    if (binauralSound != -1) {
        IAudioPlayer* player = AM::GetAudio(binauralSound);
        BinauralAudioPlayer* binaural = static_cast<BinauralAudioPlayer*>(player); // o dynamic_cast si usas RTTI
        binaural->SetLocation(binauralLoc);
    }

    std::string buttons = "";
    if (CM::GetButtonHold(CM::UP)) buttons += " UP";
    if (CM::GetButtonHold(CM::RIGHT)) {
        buttons += " RIGHT";
        AM::PauseAll();
    }
    if (CM::GetButtonHold(CM::DOWN)){ 
        buttons += " DOWN";
        AM::ResumeAll();    
    }
    if (CM::GetButtonHold(CM::LEFT)) buttons += " LEFT";

    if (CM::GetButtonHold(CM::MID_RIGHT)) buttons += " MID_RIGHT";
    if (CM::GetButtonHold(CM::MID_LEFT)) buttons += " MID_LEFT";

    if (CM::GetButtonHold(CM::JOYSTICK)) buttons += " JOYSTICK";

    if (CM::GetButtonHold(CM::VOLUME_UP)) buttons += " VOL_UP";
    if (CM::GetButtonHold(CM::VOLUME_DOWN)) buttons += " VOL_DOWN";

    if (buttons != "") {
        CM::Vibrate(CM::VIBRATOR_RIGHT, 100);
        //Serial.print(" | Buttons:");
    }
    //Serial.println(buttons.c_str());

    AM::UpdateAll();
    CM::Update();
}

void UpdateActives() {
    if (!AM::Exists(bgMusic)) bgMusic = -1;
    if (!AM::Exists(binauralSound)) binauralSound = -1;
}