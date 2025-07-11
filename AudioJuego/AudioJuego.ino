#include <Arduino.h>
#include <utility>
#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include "IAudioPlayer.h"
#include "AudioPlayer.h"
#include "BinauralAudioPlayer.h"
#include "QuickAudioPlayer.h"

#include "AudioManager.h"
#include "ControlsManager.h"
#include "GameManager.h"

#include "HRTFData.h"
#include "Vector3.h"

// WAV
const char* audioPowerOn = "power_on.wav";
const uint32_t INIT_DELAY = 6000;

const char* audioMenuSoundtrack = "menu_soundtrack_title.raw"; // done
const uint32_t loopStartMenuSoundtrack = 1424532; // Bytes a partir de donde empieza el loop

bool initDevice = true;
uint32_t timer = 0;
uint32_t lastUpdate = 0;

// Funciones
void SetDevInit();

void setup() {
    AudioMemory(700); // Reservar memoria de audio
    Serial.begin(9600);

    while (!Serial && millis() < 3000);  // Espera hasta que el puerto serial este listo
    Serial.println("Inicializando...");

    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("Can't start SD memory.");
        return;
    }

    ControlsManager::Setup();
    AudioManager::Setup();
    GameManager::Setup();

    SetDevInit();

    AudioManager::PrintConns();
}

void loop() {
    //Serial.printf("Free RAM1: %lu bytes\n", AudioMemoryUsageMax());

    uint32_t now = millis();
    uint32_t deltaTime = now - lastUpdate;
    lastUpdate = now;

    if (timer <= INIT_DELAY)
        timer += deltaTime;

    AudioManager::UpdateAll();
    ControlsManager::Update();
    GameManager::Update();


    if (!initDevice) return; // Solo ejecutar cuando se inicia la consola

    if (timer > INIT_DELAY) { // delay hasta iniciar el juego
        Serial.println("GameInit");
        initDevice = false;
        GameManager::Init(true);
    }
}

void SetDevInit() {
    AudioManager::ClearAll();
    /*ControlsManager::DisableControls();
    IAudioPlayer* menuSoundtrackPtr = new AudioPlayer(audioMenuSoundtrack, 0, loopStartMenuSoundtrack);
    float newVolume = 1;
    int8_t menuSoundtrack = AudioManager::Add(menuSoundtrackPtr, newVolume);
    Serial.printf("menuSoundTrack index: %d, volume: %.1f\n", menuSoundtrack, newVolume);
    AudioManager::PlayAll();*/

    Serial.println("DevInit");
    QuickAudioPlayer* powerOn = new QuickAudioPlayer(audioPowerOn);
    AudioManager::Add(powerOn, 1);

    initDevice = true;
    lastUpdate = millis();
}