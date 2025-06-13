#include "AudioPlayer.h"

// Audio objects
AudioOutputI2S          audioOutput;
AudioControlSGTL5000    audioShield;

AudioPlayer             loopedSong;

float volume = 0.5;

void setup() {
    AudioMemory(20); // Reservar memoria de audio

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
    loopedSong.Setup("audio1.raw", -1);
    AudioPlayQueue& audio = loopedSong.GetAudio();
    new AudioConnection(audio, 0, audioOutput, 0);
    new AudioConnection(audio, 0, audioOutput, 1);
    loopedSong.Play();
}

void loop() {
    loopedSong.Update();
    //Serial.println("Estoy ejecutandome");
}