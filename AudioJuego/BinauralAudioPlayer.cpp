#include "BinauralAudioPlayer.h"
#include <Arduino.h>

Vector3 BinauralAudioPlayer::targetPos = Vector3(0,0,0);
bool BinauralAudioPlayer::positionChanged = true;

EXTMEM int16_t BinauralAudioPlayer::buffer[HRTFData::CHUNK];
EXTMEM int16_t BinauralAudioPlayer::bufferLeft[HRTFData::CHUNK];
EXTMEM int16_t BinauralAudioPlayer::bufferRight[HRTFData::CHUNK];
EXTMEM int16_t BinauralAudioPlayer::tailBuffer[2][HRTFData::FIR_LENGTH-1] = {0};
EXTMEM int16_t BinauralAudioPlayer::hrtfQ15[2][HRTFData::FIR_LENGTH];

BinauralAudioPlayer::BinauralAudioPlayer(const char* fileName, int newLoop, uint32_t newLoopStart, uint32_t newLoopEnd) : AudioPlayer(fileName, newLoop, newLoopStart, newLoopEnd) {
    conn1 = new AudioConnection(queueL, 0, filterL, 0);
    conn2 = new AudioConnection(queueR, 0, filterR, 0);

    currentTetraIndex = 0;
    hrtfValid = false;

    memset(buffer, 0, HRTFData::CHUNK * sizeof(int16_t));
    memset(bufferLeft, 0, HRTFData::CHUNK * sizeof(int16_t));
    memset(bufferRight, 0, HRTFData::CHUNK * sizeof(int16_t));
    memset(tailBuffer[0], 0, (HRTFData::FIR_LENGTH-1) * sizeof(int16_t));
    memset(tailBuffer[1], 0, (HRTFData::FIR_LENGTH-1) * sizeof(int16_t));
    memset(hrtfQ15[0], 0, HRTFData::FIR_LENGTH * sizeof(int16_t));
    memset(hrtfQ15[1], 0, HRTFData::FIR_LENGTH * sizeof(int16_t));


    filterL.setHighpass(0, 150);
    filterR.setHighpass(0, 150);

    targetPos = Vector3(0,0,0);
}

BinauralAudioPlayer::~BinauralAudioPlayer() {
    delete conn1;
    delete conn2;
}

void BinauralAudioPlayer::Setup(const char* fileName, int newLoop) {
    AudioPlayer::Setup(fileName, newLoop);

    conn1 = new AudioConnection(queueL, 0, filterL, 0);
    conn2 = new AudioConnection(queueR, 0, filterR, 0);

    currentTetraIndex = 0;
    hrtfValid = false;
    
    memset(buffer, 0, HRTFData::CHUNK * sizeof(int16_t));
    memset(bufferLeft, 0, HRTFData::CHUNK * sizeof(int16_t));
    memset(bufferRight, 0, HRTFData::CHUNK * sizeof(int16_t));
    memset(tailBuffer[0], 0, (HRTFData::FIR_LENGTH-1) * sizeof(int16_t));
    memset(tailBuffer[1], 0, (HRTFData::FIR_LENGTH-1) * sizeof(int16_t));
    memset(hrtfQ15[0], 0, HRTFData::FIR_LENGTH * sizeof(int16_t));
    memset(hrtfQ15[1], 0, HRTFData::FIR_LENGTH * sizeof(int16_t));

    filterL.setHighpass(0, 150);
    filterR.setHighpass(0, 150);
}

bool BinauralAudioPlayer::Update() {
    frame++;
    if (!audioFile)
        return true;
    if (!isPlaying)
        return false;

    if (queueL.available() && queueR.available() && audioFile.available()) {
        int bytesRead = FillBuffer();
        if (bytesRead > 0) {
            if (positionChanged) {
                float gs[4];
                FindContainingTetra(gs);
                CalculateHRTF(gs);
                positionChanged = false;
                hrtfValid = true;
            }

            if (hrtfValid) {
                //Serial.println(frame);
                Convolve_Q15(buffer, hrtfQ15[0], tailBuffer[0], bufferLeft);
                Convolve_Q15(buffer, hrtfQ15[1], tailBuffer[1], bufferRight);
            }
            memcpy(queueL.getBuffer(), bufferLeft, bytesRead);
            memcpy(queueR.getBuffer(), bufferRight, bytesRead);
            queueL.playBuffer();
            queueR.playBuffer();
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

void BinauralAudioPlayer::FindContainingTetra(float (&gs)[4]) {
    HRTFData& data = AudioManager::hrtfData;

    uint16_t i = 0;
    const uint16_t maxIterations = 20000;

    while(i < maxIterations) {
        // Calcular g1, g2, g3
        float diff[3] = {
            targetPos.x - data.tetraCoords[currentTetraIndex][3][0],
            targetPos.y - data.tetraCoords[currentTetraIndex][3][1],
            targetPos.z - data.tetraCoords[currentTetraIndex][3][2]
        };

        // Multiplicacion diff @ Tinv
        gs[0] = diff[0]*data.Tinv[currentTetraIndex][0][0] +
                diff[1]*data.Tinv[currentTetraIndex][1][0] +
                diff[2]*data.Tinv[currentTetraIndex][2][0];

        gs[1] = diff[0]*data.Tinv[currentTetraIndex][0][1] +
                diff[1]*data.Tinv[currentTetraIndex][1][1] +
                diff[2]*data.Tinv[currentTetraIndex][2][1];

        gs[2] = diff[0]*data.Tinv[currentTetraIndex][0][2] +
                diff[1]*data.Tinv[currentTetraIndex][1][2] +
                diff[2]*data.Tinv[currentTetraIndex][2][2];

        // Calcular g4
        gs[3] = 1.0f - gs[0] - gs[1] - gs[2];

        // Verificar si todos los pesos son positivos
        bool inside = true;
        for(uint8_t j = 0; j < 4; j++) {
            if(gs[j] < -1e-6) {
                inside = false;
                break;
            }
        }

        if(inside) {
            break;
        }

        // Encontrar el peso mas bajo
        uint8_t minIndex = 0;
        float minVal = gs[0];
        for(uint8_t j = 1; j < 4; j++) {
            if(gs[j] < minVal) {
                minVal = gs[j];
                minIndex = j;
            }
        }

        // Moverse al tetraedro vecino
        currentTetraIndex = data.neighbors[currentTetraIndex][minIndex];

        // Si no hay vecino, salir
        if(currentTetraIndex == -1) {
            break;
        }

        i++;
    }
}

void BinauralAudioPlayer::CalculateHRTF(const float (&gs)[4]) {
    HRTFData& data = AudioManager::hrtfData;
    uint16_t* vertexIndices = data.simplices[currentTetraIndex];

    for (int channel = 0; channel < 2; channel++) {
        for(int sample = 0; sample < HRTFData::FIR_LENGTH; sample++) {
            hrtfQ15[channel][sample] =
            (int16_t)(data.FIRs[vertexIndices[0]][channel][sample] * gs[0] +
                       data.FIRs[vertexIndices[1]][channel][sample] * gs[1] +
                        data.FIRs[vertexIndices[2]][channel][sample] * gs[2] +
                        data.FIRs[vertexIndices[3]][channel][sample] * gs[3]);
        }
    }

}

void BinauralAudioPlayer::SetLocation(const Vector3& newLocation) {
    float prevX = targetPos.x;
    float prevY = targetPos.y;
    float prevZ = targetPos.z;

    const float MAX_DIST = 0.02f;

    targetPos = newLocation;

    float dist = sqrt(targetPos.x*targetPos.x +
                      targetPos.y*targetPos.y +
                      targetPos.z*targetPos.z);

    if (dist > 0.0f) {
        targetPos.x /= dist;
        targetPos.y /= dist;
        targetPos.z /= dist;

        dist = constrain(dist, HRTFData::MIN_R, HRTFData::MAX_R);

        targetPos.x *= dist;
        targetPos.y *= dist;
        targetPos.z *= dist;
    }
    else {
        // Caso especial (frente al oyente)
        targetPos.x = 0.0f;
        targetPos.y = -HRTFData::MIN_R;
        targetPos.z = 0.0f;
    }

    float distToPrev = sqrt((prevX-targetPos.x) * (prevX-targetPos.x) +
                            (prevY-targetPos.y) * (prevY-targetPos.y) +
                            (prevZ-targetPos.z) * (prevZ-targetPos.z));

    if (distToPrev < MAX_DIST) {
        targetPos.x = prevX;
        targetPos.y = prevY;
        targetPos.z = prevZ;
        positionChanged = false;
        hrtfValid = true;
        return;
    }

    positionChanged = true;
    hrtfValid = false;
}

int BinauralAudioPlayer::FillBuffer() {
    if (!audioFile) {
        Serial.println("Error: FillBuffer(). No file found.");
        return 0;
    }

    int bRead = 0;

    /*for (uint16_t i = 0; i < HRTFData::CHUNK; i++) {
        buffer[i] = (int16_t)(32767 * sin(2 * PI * 1000 * i / 44100.0));
    }
    bRead = HRTFData::CHUNK;*/

    if (audioFile.available()) {
        uint32_t currentPos = audioFile.position();
        uint32_t bufferSize = HRTFData::CHUNK;

        if (loop != 0 && loopEnd > currentPos)
            bufferSize = min(bufferSize, loopEnd - currentPos);

        bRead = audioFile.read(buffer, bufferSize);
    }
    else
        Serial.println("Audio file is not avaliable.");

    return bRead;
}

void FASTRUN BinauralAudioPlayer::Convolve_Q15(const int16_t* input, const int16_t* hrtfChannel, int16_t* tailChannel, int16_t* output) {
    int16_t outputQ15[HRTFData::CHUNK + HRTFData::FIR_LENGTH - 1];

    // Convolución directa (input y hrtfQ15 ya son int16_t)
    arm_conv_q15(
        (q15_t*)input,       // Cast a q15_t (equivalente a int16_t)
        HRTFData::CHUNK,
        (q15_t*)hrtfChannel,     // Cast a q15_t
        HRTFData::FIR_LENGTH,
        (q15_t*)outputQ15    // Resultado
    );

    for (int i = 0; i < HRTFData::FIR_LENGTH-1; i++) {
        outputQ15[i] += tailChannel[i];
        output[i] = outputQ15[i] >> 1;  // Satura a 16 bits
    }
    for (int i = HRTFData::FIR_LENGTH-1; i < HRTFData::CHUNK; i++) {
        output[i] = outputQ15[i] >> 1;
        //output[i] = __SSAT(outputQ15[i], 16);  // Satura a 16 bits
    }
    memcpy(tailChannel, outputQ15 + HRTFData::CHUNK, (HRTFData::FIR_LENGTH-1) * sizeof(int16_t));
    
    // Copia el resultado (CHUNK muestras, descartando el "tail")
    //memcpy(output, outputQ15, HRTFData::CHUNK * sizeof(int16_t));
}

std::pair<AudioStream*, AudioStream*> BinauralAudioPlayer::GetAudioSource() {
    return {&filterL, &filterR};
}