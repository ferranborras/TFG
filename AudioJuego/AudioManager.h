#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <algorithm>
#include "IAudioPlayer.h"
#include "AudioPlayer.h"
#include "HRTFData.h"

class IAudioPlayer;
class AudioPlayer;
class BinauralAudioPlayer;
class QuickAudioPlayer;

class AudioManager {
private:
    static EXTMEM AudioOutputI2S        audioOutput;
    static EXTMEM AudioControlSGTL5000  audioShield;
    static float globalVolume;
    static EXTMEM AudioMixer4           mixerMono, mixerL, mixerR;
    static EXTMEM AudioConnection*      connMonoToLeft;
    static EXTMEM AudioConnection*      connMonoToRight;
    static EXTMEM AudioConnection*      connLeftToOutput;
    static EXTMEM AudioConnection*      connRightToOutput;
    static EXTMEM AudioConnection*      monoConns[4];
    static EXTMEM AudioConnection*      stereoConns[2][3];

    static EXTMEM IAudioPlayer* monoSources[4];
    static constexpr uint8_t MONO_CAPACITY = 4;

    static EXTMEM IAudioPlayer* stereoSources[3];
    static constexpr uint8_t STEREO_CAPACITY = 3;
    static int8_t mixersIndex;  // Cada bit corresponde con una entrada de mixer mono como boleanos 0: libre | 1: ocupado
                                    // Con 3 AudioMixer4 entradas hay 12 entradas en total, pero 2 mixers (stereo) van sincronizados
                                    // En total son necesarias 8 entradas correspondientes a los bits de int8_t
                                    // La ultima entrada de stereo siempre ocupada por mixerMono (0000 1000) = 8
                                    //                                                           mono^   ^stereo

    static void LoadHRTFData();
    static void LoadBinary(const char* path, void* dest, size_t expected_size);
    static int GetFreeMonoIndex();
    static int GetFreeStereoIndex();
    static void MarkMonoIndex(uint8_t index);
    static void FreeMonoIndex(uint8_t index);
    static void MarkStereoIndex(uint8_t index);
    static void FreeStereoIndex(uint8_t index);
    static bool GetIndexBit(uint8_t index);

public:
    static EXTMEM HRTFData hrtfData;

    static EXTMEM AudioPlaySdWav wavPool[MONO_CAPACITY]; // Para no crear y eliminar objetos playWav constantemente (no tienen destructor dinamico)
    static uint8_t wavPoolIndex; // Bits como booleanos (sobran 4 bits pero aun asi es mas eficiente que un array de booleanos)

    AudioManager() = default;

    static void Setup();

    static int8_t Add(IAudioPlayer* source, float volume = -1);
    static IAudioPlayer* GetAudio(int8_t index);
    static bool Exists(uint8_t index);

    static bool RemoveAt(uint8_t i);
    static bool RemoveMonoAt(uint8_t i);
    static bool RemoveStereoAt(uint8_t i);

    static void ResumeAll();
    static void PauseAll();
    static void ClearAll();
    static void PlayAll();
    static void UpdateAll();
    static void ClearQuicks();

    static int GetFreeWavIndex();
    static void MarkWavIndex(uint8_t index);
    static void FreeWavIndex(uint8_t index);

    static float GetVolume();
    static void SetVolume(int8_t index, float newVolume);
    static void SetGlobalVolume(float newVolume);

    static uint8_t GetMonoNum();
    static uint8_t GetStereoNum();
    static bool IsEmpty();

    static void PrintConns();
};

#endif