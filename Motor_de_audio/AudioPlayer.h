#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h> // Librería de audio de Teensy
#include <Audio.h>
#include <SD.h>
#include <SPI.h>

#define BUFFER_SIZE 256

class AudioPlayer {
private:
    AudioPlayQueue queue;
    //AudioOutputI2S audioOutput;

    File audioFile;
    uint8_t buffer[BUFFER_SIZE];

    bool isPlaying;
    int loop;

    int FillBuffer();

public:
    void Setup(const char* fileName, int newLoop = 0);
    void Update();
    void Play();
    void Stop();
    void Pause();
    void Resume();
    AudioPlayQueue& GetAudio();
};

#endif