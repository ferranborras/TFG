#include "QuickAudioPlayer.h"

QuickAudioPlayer::QuickAudioPlayer(const char* filename) {
    wavIndex = AM::GetFreeWavIndex();
    if (wavIndex == -1) {
        isPlaying = true; // forzar eliminar en update
        return;
    }

    playWav = &AM::wavPool[wavIndex];
    AM::MarkWavIndex(wavIndex);
    playWav->play(filename);
    isPlaying = false;
}

QuickAudioPlayer::~QuickAudioPlayer() {
    Serial.println("Quick eliminado");
    if (playWav) playWav->stop();
    AM::FreeWavIndex(wavIndex);
    //delete playWav;
}

bool QuickAudioPlayer::Update() {
    if (playWav->isPlaying())
        isPlaying = true; // Asegurar que ha tenido tiempo de empezar antes de eliminar

    if (!isPlaying) // Evitar eliminar si aun no ha empezado
        return false;

    return !playWav->isPlaying(); // Eliminar si ha terminado
}

std::pair<AudioStream*, AudioStream*> QuickAudioPlayer::GetAudioSource() {
    return {playWav, playWav};
}