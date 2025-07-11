#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h> // Librería de audio de Teensy
#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <utility>
#include "AudioManager.h"
#include "IAudioPlayer.h"
#include "HRTFData.h"
#include <math.h>

class AudioPlayer : public IAudioPlayer {
private:
    static constexpr uint16_t BUFFER_SIZE = HRTFData::CHUNK;

    AudioPlayQueue queue;
    int16_t buffer[HRTFData::CHUNK];

protected:
    File audioFile;
    bool isPlaying;
    int loop;
    uint32_t loopStart;
    uint32_t loopEnd;

    virtual int FillBuffer();

public:
    AudioPlayer(const char* fileName, int newLoop = 0, uint32_t newLoopStart = 0, uint32_t newLoopEnd = 0);
    virtual ~AudioPlayer() override;

    virtual void Setup(const char* fileName, int newLoop = 0);
    virtual bool Update() override;
    void Play(bool looping = false);
    void Pause();
    void Resume();
    virtual std::pair<AudioStream*, AudioStream*> GetAudioSource() override;
    bool IsQuick() const { return false; }
};

#endif