#ifndef QUICK_AUDIO_PLAYER_H
#define QUICK_AUDIO_PLAYER_H

#include <Audio.h>
#include <SD.h>
#include "IAudioPlayer.h"
#include "AudioManager.h"
using AM = AudioManager;

class QuickAudioPlayer : public IAudioPlayer {
private:
    AudioPlaySdWav* playWav;
    bool isPlaying;
    int8_t wavIndex;
public:
    QuickAudioPlayer(const char* filename);
    ~QuickAudioPlayer() override;

    bool Update() override;
    std::pair<AudioStream*, AudioStream*> GetAudioSource() override;
    bool IsQuick() const override { return true; }
};

#endif