#ifndef I_AUDIO_PLAYER_H
#define I_AUDIO_PLAYER_H

#include <Audio.h>

class IAudioPlayer {
public:
    virtual ~IAudioPlayer() {}
    virtual bool Update() = 0; // Retorna true si ha terminado

    virtual std::pair<AudioStream*, AudioStream*> GetAudioSource() = 0;
    virtual bool IsBinaural() const { return false; }
    virtual bool IsQuick() const { return false; }
};

#endif