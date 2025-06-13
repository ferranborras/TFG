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

    Serial.println("audioFile opened succesfully");
}

void AudioPlayer::Update() {
    //Serial.println("Soy un archivo externo");
    if (!audioFile) {
        //Serial.println("Error: Update(). No file found.");
        return;
    }

    if (isPlaying && audioFile.available()) {
        while (queue.available()) {
            int bytesRead = FillBuffer();
            if (bytesRead > 0) {
                //Serial.println("Playing buffer");
                memcpy(queue.getBuffer(), buffer, bytesRead);
                queue.playBuffer();
            }
            else {
                break;
            }
        }
    }
    else if (!audioFile.available()) {
        if (loop != 0) {
            if (loop > 0)
                loop--;
            Serial.print("Loop: ");
            Serial.println(loop);

            Play();
        }
        else {
            Stop();
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
        //Serial.println("Buffer refilled.");
    }
    else {
        Serial.println("Audio file is not avaliable.");
    }

    return bytesRead;
}

AudioPlayQueue& AudioPlayer::GetAudio() {
    return queue;
}