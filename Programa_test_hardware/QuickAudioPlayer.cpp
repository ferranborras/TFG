#include "QuickAudioPlayer.h"

QuickAudioPlayer::QuickAudioPlayer(const char* filename) {
    playWav = new AudioPlaySdWav();
    playWav->play(filename);
}

QuickAudioPlayer::~QuickAudioPlayer() {
    delete playWav;
}

bool QuickAudioPlayer::Update() {
    return !playWav->isPlaying();
}