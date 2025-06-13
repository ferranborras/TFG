#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h> // Librería de audio de Teensy
#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <utility>
#include "AudioManager.h"
#include <math.h>

class AudioPlayer {
private:
    static constexpr uint16_t BUFFER_SIZE = 256;

    AudioPlayQueue queue;
    int16_t buffer[BUFFER_SIZE];

protected:
    File audioFile;
    bool isPlaying;
    int loop;

    virtual int FillBuffer();

public:
    virtual void Setup(const char* fileName, int newLoop = 0);
    virtual void Update();
    void Play();
    void Stop();
    void Pause();
    void Resume();
    virtual std::pair<AudioStream*, AudioStream*> GetAudioSource();
    virtual bool IsBinaural() const { return false; }
};

#endif