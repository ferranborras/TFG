#include "AudioPlayer.h"

void AudioPlayer::Setup(const char* fileName, int newLoop) {
    if (audioFile)
        audioFile.close();

    audioFile = SD.open(fileName);
    if (!audioFile) {
        Serial.println("Error opening file.");
        return;
    }

    loop = newLoop;
    isPlaying = false;
}

void AudioPlayer::Update() {
    if (!audioFile || !isPlaying)
        return;

    //int filled = 0;
    //const int maxBuffersToFill = 2;
    if (queue.available() && audioFile.available()) {
        int bytesRead = FillBuffer();
        if (bytesRead > 0) {
            //Aprovechar este espacio para obtener los bytes con filtro aplicado si se trata de un AudioPlayer binaural (llamada a HRTFSource.Update(queue))
            memcpy(queue.getBuffer(), buffer, bytesRead);
            queue.playBuffer();
            //filled++;
        }
    }
    
    if (!audioFile.available()) {
        if (loop == 0)
            Stop();
        else {
            if (loop > 0)
                loop--;
            Play();
        }
    }
}

void AudioPlayer::Play() {
    if (!audioFile) {
        Serial.println("Error: Play(). No file found.");
        return;
    }
    audioFile.seek(0);
    isPlaying = true;
}

void AudioPlayer::Stop() {
    if (!audioFile) {
        Serial.println("Error: Stop(). No file found.");
        return;
    }
    Serial.println("Audio playing ended.");

    isPlaying = false;

    AudioManager::Remove(this);

    audioFile.close();
}

void AudioPlayer::Pause() {
    if (!audioFile) {
        Serial.println("Error: Pause(). No file found.");
        return;
    }

    isPlaying = false;
    Serial.println("Audio paused.");
}

void AudioPlayer::Resume() {
    if (!audioFile) {
        Serial.println("Error: Resume(). No file found.");
        return;
    }

    isPlaying = true;
    Serial.println("Audio resumed.");
}

int AudioPlayer::FillBuffer() {
    if (!audioFile) {
        Serial.println("Error: FillBuffer(). No file found.");
        return 0;
    }

    int bytesRead = 0;
    
    if (audioFile.available()) {
        bytesRead = audioFile.read(buffer, BUFFER_SIZE);
    }
    else {
        Serial.println("Audio file is not avaliable.");
    }

    return bytesRead;
}

std::pair<AudioStream*, AudioStream*> AudioPlayer::GetAudioSource() {
    return {&queue, &queue};
}