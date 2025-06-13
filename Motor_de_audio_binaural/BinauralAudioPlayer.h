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

    static EXTMEM int16_t buffer[HRTFData::CHUNK];
    int16_t bufferLeft[HRTFData::CHUNK]; // Se utiliza para cargar info de audioFile y para resultado de convolucion
    int16_t bufferRight[HRTFData::CHUNK]; // Se usa solo para resultado de convolucion

    //static EXTMEM float hrtf[2][HRTFData::FIR_LENGTH];
    static EXTMEM int16_t hrtfQ15[2][HRTFData::FIR_LENGTH];
    bool hrtfValid = false;
    
    // Auxiliar
    //static constexpr uint16_t FFT_SIZE = 512; // Potencia de 2 >= (CHUNK + FIR_LENGTH - 1 = 383)
    //static arm_rfft_fast_instance_f32 fftInstance;
    //static EXTMEM float fftInput[FFT_SIZE];
    //static EXTMEM float fftHrtfLeft[FFT_SIZE];
    //static EXTMEM float fftHrtfRight[FFT_SIZE];
    //static EXTMEM float fftOutput[FFT_SIZE];
    
    uint16_t currentTetraIndex;
    static float targetPos[3]; // [x, y, z]
    static bool positionChanged;
    int frame = 0;

    void FindContainingTetra(float (&gs)[4]);
    void CalculateHRTF(const float (&gs)[4]);
    //void ProcessBuffer();
    //void Convolve(const int16_t* input, const float* hrtf, float* output);
    /*void Convolve(const int16_t (&input)[HRTFData::CHUNK + OVERLAP_AMOUNT],
                const float (&hrtf)[HRTFData::FIR_LENGTH],
                float (&output)[HRTFData::CHUNK + 2 * (HRTFData::FIR_LENGTH - 1)]);*/
    //void Convolve_FFT(const int16_t* input, const float* hrtfChannel, int16_t* output);
    void FASTRUN Convolve_Q15(const int16_t* input, const int16_t* hrtfChannel, int16_t* output);

    int FillBuffer() override;

public:
    void Setup(const char* fileName, int newLoop = 0) override;
    void Update() override;
    void SetLocation(const Vector3& newLocation);
    std::pair<AudioStream*, AudioStream*> GetAudioSource() override;
    bool IsBinaural() const override { return true; }
};

#endif