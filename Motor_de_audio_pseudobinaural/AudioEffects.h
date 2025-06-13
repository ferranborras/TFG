#ifndef AUDIO_EFFECTS_H
#define AUDIO_EFFECTS_H

#include <Audio.h>
#include <math.h>
#include <utility>

#include <Arduino.h> // Librería de audio de Teensy
#include <SD.h>
#include <SPI.h>

#define EAR_RADIUS 1.0
#define MAX_DELAY 0.6
#define ATTENUATION_FACTOR 0.1
#define INIT_FREQ 8000.0

class AudioEffects {
private:
    AudioMixer4             mixerL, mixerR;
    AudioFilterBiquad       lowPassL, lowPassR;
    AudioEffectDelay        delayL, delayR;

    AudioConnection* conn1;
    AudioConnection* conn2;
    AudioConnection* conn3;
    AudioConnection* conn4;
    AudioConnection* conn5;
    AudioConnection* conn6;

    float prevDelayDif = 0.0;

public:
    void Setup(AudioStream* audioSource);
    void SetLocation(const std::pair<float, float>& newLocation);
    std::pair<AudioStream*, AudioStream*> GetAudioEffects();
};

#endif