#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h> // Librería de audio de Teensy
#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <utility>
#include "AudioEffects.h"

#define BUFFER_SIZE 256

class AudioPlayer {
private:
    AudioPlayQueue queue;
    AudioEffects effects;

    File audioFile;
    uint8_t buffer[BUFFER_SIZE];

    bool isPlaying;
    int loop;
    bool isStereo;

    int FillBuffer();

public:
    void Setup(const char* fileName, int newLoop = 0, bool isStereo = true);
    void Update();
    void Play();
    void Stop();
    void Pause();
    void Resume();
    AudioStream* GetAudioMono();
    std::pair<AudioStream*, AudioStream*> GetAudioStereo();
    void SetLocation(const std::pair<float, float>& newLocation);
};

#endif