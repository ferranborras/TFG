#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "HRTFData.h"
#include <Arduino.h>
#include <SD.h>

class AudioPlayer;
class BinauralAudioPlayer;

class AudioManager {
private:
    static constexpr uint16_t INITIAL_CAPACITY = 3;

    static EXTMEM AudioPlayer** monoSources;
    static EXTMEM uint16_t numMonoSources;
    static EXTMEM uint16_t monoCapacity;

    static EXTMEM BinauralAudioPlayer** stereoSources;
    static EXTMEM uint16_t numStereoSources;
    static EXTMEM uint16_t stereoCapacity;

    static void LoadHRTFData();
    static void LoadBinary(const char* path, void* dest, size_t expected_size);
    static bool ResizeMono(uint16_t newCapacity);
    static bool ResizeStereo(uint16_t newCapacity);
    //static void ConvertQuantizedHRTF();

public:
    static EXTMEM HRTFData hrtfData;

    AudioManager() = default;

    static void Setup();

    static bool Add(AudioPlayer* source);
    static bool Remove(AudioPlayer* source);
    static bool RemoveMonoAt(uint16_t i);
    static bool RemoveStereoAt(uint16_t i);

    static void PlayAll();
    static void PauseAll();
    static void StopAll();
    static void ResetAll();
    static void UpdateAll();
};

#endif