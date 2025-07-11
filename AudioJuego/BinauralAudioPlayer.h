#ifndef BINAURAL_AUDIO_PLAYER_H
#define BINAURAL_AUDIO_PLAYER_H

#include "Vector3.h"
#include "AudioPlayer.h"
#include "HRTFData.h"
#include <math.h>
#include <arm_math.h>  // CMSIS-DSP

class BinauralAudioPlayer : public AudioPlayer {
private:
    static constexpr uint8_t OVERLAP_AMOUNT = HRTFData::FIR_LENGTH-1;

    AudioPlayQueue queueL;
    AudioPlayQueue queueR;
    AudioFilterBiquad filterL;
    AudioFilterBiquad filterR;
    AudioConnection* conn1;
    AudioConnection* conn2;

    static EXTMEM int16_t buffer[HRTFData::CHUNK];
    static EXTMEM int16_t bufferLeft[HRTFData::CHUNK]; // Se utiliza para cargar info de audioFile y para resultado de convolucion
    static EXTMEM int16_t bufferRight[HRTFData::CHUNK]; // Se usa solo para resultado de convolucion
    static EXTMEM int16_t tailBuffer[2][HRTFData::FIR_LENGTH-1];

    //static EXTMEM float hrtf[2][HRTFData::FIR_LENGTH];
    static EXTMEM int16_t hrtfQ15[2][HRTFData::FIR_LENGTH];
    bool hrtfValid = false;

    uint16_t currentTetraIndex;
    static Vector3 targetPos; // (x, y, z)
    static bool positionChanged;
    int frame = 0;

    void FindContainingTetra(float (&gs)[4]);
    void CalculateHRTF(const float (&gs)[4]);
    void FASTRUN Convolve_Q15(const int16_t* input, const int16_t* hrtfChannel, int16_t* tailChannel, int16_t* output);

    int FillBuffer() override;

public:
    BinauralAudioPlayer(const char* fileName, int newLoop = 0, uint32_t newLoopStart = 0, uint32_t newLoopEnd = 0);
    ~BinauralAudioPlayer() override;

    void Setup(const char* fileName, int newLoop = 0) override;
    bool Update() override;
    void SetLocation(const Vector3& newLocation);
    std::pair<AudioStream*, AudioStream*> GetAudioSource() override;
    bool IsBinaural() const override { return true; }
};

#endif