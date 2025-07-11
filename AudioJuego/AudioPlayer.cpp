#include "AudioPlayer.h"

AudioPlayer::AudioPlayer(const char* fileName, int newLoop, uint32_t newLoopStart, uint32_t newLoopEnd) {
    if (audioFile)
        audioFile.close();

    audioFile = SD.open(fileName);
    if (!audioFile) {
        Serial.println("Error opening file.");
        return;
    }

    loop = newLoop;
    loopStart = newLoopStart;
    //if (loopStart >= audioFile.size())
        //loopStart = 0;
    loopEnd = newLoopEnd;
    if (loopEnd <= loopStart)
        loopEnd = audioFile.size();
    isPlaying = false;
}

AudioPlayer::~AudioPlayer() {
    if (audioFile && audioFile.available())
        audioFile.close();

    Serial.println("AudioPlayer destruido");
}

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

bool AudioPlayer::Update() {
    if (!audioFile) {
        Serial.println("audioFile does not exist");
        return true;
    }
    
    if (!isPlaying)
        return false;

    if (queue.available() && audioFile.available()) {
        int bytesRead = FillBuffer();
        if (bytesRead > 0) {
            memcpy(queue.getBuffer(), buffer, bytesRead);
            queue.playBuffer();
        }
    }

    if (!audioFile.available() || (loop != 0 && audioFile.position() >= loopEnd)) {
        if (loop == 0)
            return true;
        else {
            if (loop > 0)
                loop--;
            Play(true);
        }
    }
    return false;
}

void AudioPlayer::Play(bool looping) {
    if (!audioFile) {
        Serial.println("Error: Play(). No file found.");
        return;
    }
    if (looping)
        audioFile.seek(loopStart);
    else
        audioFile.seek(0);
    isPlaying = true;
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
        uint32_t currentPos = audioFile.position();
        uint32_t bufferSize = BUFFER_SIZE;

        if (loop != 0 && loopEnd > currentPos)
            bufferSize = min(bufferSize, loopEnd - currentPos);

        bytesRead = audioFile.read(buffer, bufferSize);
    }
    else {
        Serial.println("Audio file is not avaliable.");
    }

    return bytesRead;
}

std::pair<AudioStream*, AudioStream*> AudioPlayer::GetAudioSource() {
    return {&queue, &queue};
}