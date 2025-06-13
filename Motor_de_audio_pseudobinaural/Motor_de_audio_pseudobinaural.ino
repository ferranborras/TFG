#include "AudioPlayer.h"
#include <utility>

// Audio objects
AudioOutputI2S          audioOutput;
AudioControlSGTL5000    audioShield;

AudioPlayer             loopedSong;

float volume = 0.6;
const char* audio1 = "audio1.raw";
const char* audio2 = "BillieEilish_Ilomilo.raw";

// Provisional
float distance = 2.0;

std::pair<float, float> Orbit();

void setup() {
    AudioMemory(100); // Reservar memoria de audio

    Serial.begin(9600);
    while (!Serial && millis() < 3000);  // Espera hasta que el puerto serial esté listo
    Serial.println("Inicializando...");

    audioShield.enable();
    audioShield.volume(volume);

    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("Can't start SD memory.");
        return;
    }

    // LoopedSong
    loopedSong.Setup(audio2, -1);
    //AudioStream* audio = loopedSong.GetAudioMono();
    std::pair<AudioStream*, AudioStream*> audio = loopedSong.GetAudioStereo();
    new AudioConnection(*audio.first, 0, audioOutput, 0);
    new AudioConnection(*audio.second, 0, audioOutput, 1);
    loopedSong.Play();
}

void loop() {
    std::pair<float, float> location = Orbit();

    //Serial.print("AudioMemoryUsageMax: ");
    //Serial.println(AudioMemoryUsageMax());

    loopedSong.SetLocation(location);
    loopedSong.Update();
}

std::pair<float, float> Orbit() {
    float time = millis() / 1000.0;

    float angle = time * 2.0;
    if (angle >= 360) { angle -= 360; }

    float x = cos(angle) * distance;
    float y = sin(angle) * distance;

    return {x, y};
}