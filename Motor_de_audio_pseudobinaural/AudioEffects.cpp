#include "AudioEffects.h"

void AudioEffects::Setup(AudioStream* audioSource) {
    conn1 = new AudioConnection(*audioSource, 0, mixerL,    0);
    conn2 = new AudioConnection(*audioSource, 0, mixerR,    0);
    conn3 = new AudioConnection(mixerL,       0, lowPassL,  0);
    conn4 = new AudioConnection(mixerR,       0, lowPassR,  0);
    conn5 = new AudioConnection(lowPassL,     0, delayL,    0);
    conn6 = new AudioConnection(lowPassR,     0, delayR,    0);

    // Podés setear valores iniciales también acá
    mixerL.gain(0, 1.0f);
    mixerR.gain(0, 1.0f);
    lowPassL.setLowpass(0, INIT_FREQ, 0.5f);
    lowPassR.setLowpass(0, INIT_FREQ, 0.5f);
    delayR.delay(0, 0);
    delayL.delay(0, 0);
}

std::pair<AudioStream*, AudioStream*> AudioEffects::GetAudioEffects() {
  return { &delayL, &delayR };
}

void AudioEffects::SetLocation(const std::pair<float, float>& newLocation) {
    float x = newLocation.first;
    float y = newLocation.second;

    float distance = sqrt(pow(x, 2) + pow(y, 2));

    float distToLeftEar = sqrt(pow((x + EAR_RADIUS), 2.0) + pow(y, 2.0));
    float distToRightEar = sqrt(pow((x - EAR_RADIUS), 2.0) + pow(y, 2.0));

    // Acoustic shadow
    float angleToLeftEar = asin(y / distToLeftEar);
    float angleToRightEar = asin(y / distToRightEar);

    float normalizedLeftAngle = abs(1 - abs(angleToLeftEar - 120.0) / 180.0);
    float normalizedRightAngle = abs(1 - abs(angleToRightEar - 60.0) / 180.0);

    // Mixer
    {
        // Panning
        float minDist = 1.0;
        float leftGain = 1.0 / pow(max(minDist, distToLeftEar), 2.0);
        float rightGain = 1.0 / pow(max(minDist, distToRightEar), 2.0);
        //float leftGain = 1.0 / (pow(distToLeftEar, 2.0) + 1.0);
        //float rightGain = 1.0 / (pow(distToRightEar, 2.0) + 1.0);

        // Normalize gain
        float maxGain = max(leftGain, rightGain);
        leftGain /= maxGain;
        rightGain /= maxGain;

        // Acoustic shadow
        float maxShadow = 0.2;
        float maxShadowL = maxShadow + (1.0 - maxShadow) * normalizedLeftAngle;
        float maxShadowR = maxShadow + (1.0 - maxShadow) * normalizedRightAngle;
        //maxShadowL = 1.0;
        //maxShadowR = 1.0;

        float gainByDist = 1.0 / pow(max(minDist, distance), 2.0);

        //gainByDist = 1.0;

        rightGain *= gainByDist;
        leftGain *= gainByDist;

        Serial.print("LeftGain: ");
        Serial.print(leftGain * maxShadowL);
        Serial.print(" | RightGain: ");
        Serial.println(rightGain * maxShadowR);

        mixerL.gain(0, rightGain * maxShadowR);
        mixerR.gain(0, leftGain * maxShadowL);
    }

    // Low pass
    {
        float freqCutoffL = INIT_FREQ * exp(-ATTENUATION_FACTOR * distToLeftEar);
        float freqCutoffR = INIT_FREQ * exp(-ATTENUATION_FACTOR * distToRightEar);

        float maxShadow = 0.2;
        float maxShadowL = maxShadow + (1.0 - maxShadow) * normalizedLeftAngle;
        float maxShadowR = maxShadow + (1.0 - maxShadow) * normalizedRightAngle;

        lowPassL.setLowpass(0, freqCutoffL * maxShadowL, 0.5);
        lowPassR.setLowpass(0, freqCutoffR * maxShadowR, 0.5);
    }

    // Delay
    {
        float currentDelayDif = (distToRightEar - distToLeftEar) / 2.0 * EAR_RADIUS;
        float correctionDelay = (currentDelayDif - prevDelayDif) * MAX_DELAY;

        if (correctionDelay != 0) {
            if (correctionDelay > 0) { delayL.delay(0, abs(correctionDelay)); }
            if (correctionDelay < 0) { delayR.delay(0, abs(correctionDelay)); }
            prevDelayDif = currentDelayDif;
        }
    }

    // Delay prev
    /*{
      // Ajustar delay
      float distance = sqrt(pow(x, 2.0) + pow(y, 2.0));
      float x1 = x / distance * 0.5;
      float rightDist = 0.5 - x1;
      float leftDist = 1.0 - rightDist;

      float currentDelayDif = rightDist - leftDist;
      float correctionDelay = (currentDelayDif - prevDelayDif) * MAX_DELAY;
        
      if (correctionDelay != 0) {
        if (correctionDelay > 0) { delayR.delay(0, abs(correctionDelay)); }
        if (correctionDelay < 0) { delayL.delay(0, abs(correctionDelay)); }
        prevDelayDif = currentDelayDif;
      }
    }*/
}