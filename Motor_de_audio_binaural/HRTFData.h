#ifndef HRTF_DATA_H
#define HRTF_DATA_H

#include <utility>
#include <SPI.h>

struct HRTFData {
public:
    static constexpr uint16_t CHUNK = 256;
    static constexpr uint16_t MAX_TETRA = 7275;
    static constexpr uint16_t MAX_POSITIONS = 1570;
    static constexpr uint8_t FIR_LENGTH = 128;
    static constexpr float MAX_R = 3.414f;
    static constexpr float MIN_R = 0.075f;

    float tetraCoords[MAX_TETRA][4][3];
    int16_t FIRs[MAX_POSITIONS][2][FIR_LENGTH];
    float Tinv[MAX_TETRA][3][3];
    uint16_t simplices[MAX_TETRA][4];
    int16_t neighbors[MAX_TETRA][4];

    static float GetFIRSample(int16_t sample_quantized) {
        return static_cast<float>(sample_quantized) / 32767.0f;
    }
};

#endif