#include "AudioPlayer.h"

void AudioPlayer::Setup(const char* fileName, int newLoop, bool stereo) {
    if (audioFile)
        audioFile.close();

    audioFile = SD.open(fileName);
    if (!audioFile) {
        Serial.println("Error opening file.");
        return;
    }

    loop = newLoop;
    isPlaying = false;
    isStereo = stereo;

    if (isStereo) {
        effects.Setup(&queue);
    }

    Serial.println("audioFile opened succesfully");
}

void AudioPlayer::Update() {
    if (!audioFile) {
        return;
    }

    if (isPlaying && audioFile.available()) {
        int filled = 0;
        const int maxBuffersToFill = 2;
        while (queue.available() && filled < maxBuffersToFill) {
            int bytesRead = FillBuffer();
            if (bytesRead > 0) {
                //Serial.println("Playing buffer");
                memcpy(queue.getBuffer(), buffer, bytesRead);
                queue.playBuffer();
                filled++;
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

std::pair<AudioStream*, AudioStream*> AudioPlayer::GetAudioStereo() {
  return effects.GetAudioEffects();
}

AudioStream* AudioPlayer::GetAudioMono() {
    return &queue;
}

void AudioPlayer::SetLocation(const std::pair<float, float>& newLocation) {
    if (!isStereo)
        return;

    effects.SetLocation(newLocation);
}