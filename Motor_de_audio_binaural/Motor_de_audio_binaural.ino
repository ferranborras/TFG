#include <Arduino.h>
#include <utility>
#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include "AudioManager.h"
#include "AudioPlayer.h"
#include "BinauralAudioPlayer.h"
#include "HRTFData.h"
#include "Vector3.h"
#include "RandomBinauralPosition.h"

// Audio objects
AudioOutputI2S          audioOutput;
AudioControlSGTL5000    audioShield;

EXTMEM BinauralAudioPlayer    loopedBinaural;
//EXTMEM AudioPlayer    loopedBinaural;
RandomBinauralPosition binauralPos(HRTFData::MIN_R, HRTFData::MAX_R);

float volume = 0.3;
const char* audio1 = "audio1.raw";
const char* audio2 = "BillieEilish_Ilomilo.raw";

float deltaTime = 0.016f;  // Aprox. 60 FPS (ajustar)

//Vector3 Orbit();

int loops = 0;

void setup() {
    AudioMemory(300); // Reservar memoria de audio

    Serial.begin(9600);
    //Serial.printf("PSRAM disponible: %d MB\n", extmem_size() / (1024 * 1024));
    while (!Serial && millis() < 3000);  // Espera hasta que el puerto serial esté listo
    Serial.println("Inicializando...");

    audioShield.enable();
    audioShield.volume(volume);

    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("Can't start SD memory.");
        return;
    }

    AudioManager::Setup();
    loopedBinaural.Setup(audio2, -1);

    AudioManager::Add(&loopedBinaural);
    std::pair<AudioStream*, AudioStream*> audio = loopedBinaural.GetAudioSource();
    new AudioConnection(*audio.first, 0, audioOutput, 0);
    new AudioConnection(*audio.second, 0, audioOutput, 1);

    loopedBinaural.Play();
}

void loop() {
    //Serial.print("\n\nLoops: "); Serial.println(loops++);
    int usage = AudioMemoryUsage();
    //if(usage > 120) {
        //Serial.println("ALERTA: Memoria de audio cerca del limite.");
    //}

    //Vector3 newPos = Vector3(-2.8f, -1.4f, -1.2f);
    Vector3 newPos = binauralPos.update(deltaTime);

    loopedBinaural.SetLocation(newPos);
        //newPos.print();
    AudioManager::UpdateAll();

    //Serial.print("AudioMemoryUsage: ");
    //Serial.print(usage);
    //Serial.print("/");
    //Serial.println(AudioMemoryUsageMax());
}

/*Vector3 Orbit() {
    float time = millis() / 1000.0;

    float angle = time * 2.0;
    if (angle >= 360) { angle -= 360; }

    float x = cos(angle) * distance;
    float y = sin(angle) * distance;

    return Vector3(x, y, distance);
}*/