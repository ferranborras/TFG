#include "BinauralAudioPlayer.h"
#include <Arduino.h>

float BinauralAudioPlayer::targetPos[3] = {0};
bool BinauralAudioPlayer::positionChanged = true;

EXTMEM int16_t BinauralAudioPlayer::buffer[HRTFData::CHUNK];
//EXTMEM float BinauralAudioPlayer::hrtf[2][HRTFData::FIR_LENGTH];
EXTMEM int16_t BinauralAudioPlayer::hrtfQ15[2][HRTFData::FIR_LENGTH];

//arm_rfft_fast_instance_f32 BinauralAudioPlayer::fftInstance;
//EXTMEM float BinauralAudioPlayer::fftInput[FFT_SIZE];
//EXTMEM float BinauralAudioPlayer::fftHrtfLeft[FFT_SIZE];
//EXTMEM float BinauralAudioPlayer::fftHrtfRight[FFT_SIZE];
//EXTMEM float BinauralAudioPlayer::fftOutput[FFT_SIZE];

void BinauralAudioPlayer::Setup(const char* fileName, int newLoop) {
    AudioPlayer::Setup(fileName, newLoop);

    currentTetraIndex = 0;
    hrtfValid = false;
    
    memset(buffer, 0, HRTFData::CHUNK * sizeof(int16_t));
    //memset(hrtf[0], 0, HRTFData::FIR_LENGTH * sizeof(float));
    //memset(hrtf[1], 0, HRTFData::FIR_LENGTH * sizeof(float));
    memset(hrtfQ15[0], 0, HRTFData::FIR_LENGTH * sizeof(int16_t));
    memset(hrtfQ15[1], 0, HRTFData::FIR_LENGTH * sizeof(int16_t));

    //arm_rfft_fast_init_f32(&fftInstance, FFT_SIZE);
    //memset(fftInput, 0, FFT_SIZE * sizeof(float));
    //memset(fftHrtfLeft, 0, FFT_SIZE * sizeof(float));
    //memset(fftHrtfRight, 0, FFT_SIZE * sizeof(float));
    //memset(fftOutput, 0, FFT_SIZE * sizeof(float));
}

void BinauralAudioPlayer::Update() {
    frame++;
    if (!audioFile || !isPlaying)
        return;

    //const int maxBuffersToFill = 2;
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
                Serial.println(frame);
                Convolve_Q15(buffer, hrtfQ15[0], bufferLeft);
                Convolve_Q15(buffer, hrtfQ15[1], bufferRight);
            }
            memcpy(queueL.getBuffer(), bufferLeft, bytesRead);
            memcpy(queueR.getBuffer(), bufferRight, bytesRead);
            queueL.playBuffer();
            queueR.playBuffer();
        }
    }

    
    if (!audioFile.available()) {
        if (loop == 0)
            Stop();
        else {
            if (loop > 0)
                loop--;
            Play();
        }
    }

}

void BinauralAudioPlayer::FindContainingTetra(float (&gs)[4]) {
    HRTFData& data = AudioManager::hrtfData;

    uint16_t i = 0;
    const uint16_t maxIterations = 20000;

    //Serial.println("Tinv:");
    /*for (int i = 0; i < 10; i++) {
        Serial.printf("Tetra %d: r1[%.2f, %.2f, %.2f], r2[%.2f, %.2f, %.2f], r3[%.2f, %.2f, %.2f]\n", i,
                                            data.Tinv[i][0][0], data.Tinv[i][0][1], data.Tinv[i][0][2],
                                            data.Tinv[i][1][0], data.Tinv[i][1][1], data.Tinv[i][1][2],
                                            data.Tinv[i][2][0], data.Tinv[i][2][1], data.Tinv[i][2][2]);
    }
    Serial.println("");*/

    while(i < maxIterations) {
        // Calcular g1, g2, g3
        float diff[3] = {
            targetPos[0] - data.tetraCoords[currentTetraIndex][3][0],
            targetPos[1] - data.tetraCoords[currentTetraIndex][3][1],
            targetPos[2] - data.tetraCoords[currentTetraIndex][3][2]
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

        //if (i < 100)
            //Serial.printf("i=%d Tetra %d: [g1=%.2f, g2=%.2f, g3=%.2f, g4=%.2f], Diff: [%.2f, %.2f, %.2f]\n", i, currentTetraIndex, gs[0], gs[1], gs[2], gs[3], diff[0], diff[1], diff[2]);

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
    //Serial.printf("\nTetraedro final: %d\n", currentTetraIndex);
}

void BinauralAudioPlayer::CalculateHRTF(const float (&gs)[4]) {
    HRTFData& data = AudioManager::hrtfData;
    uint16_t* vertexIndices = data.simplices[currentTetraIndex];
    //Serial.printf("Vertices: [%d, %d, %d, %d]\n\n", vertexIndices[0], vertexIndices[1], vertexIndices[2], vertexIndices[3]);

    for (int channel = 0; channel < 2; channel++) {
        for(int sample = 0; sample < HRTFData::FIR_LENGTH; sample++) {
            hrtfQ15[channel][sample] =
            (int16_t)((data.GetFIRSample(data.FIRs[vertexIndices[0]][channel][sample]) * gs[0] +
                        data.GetFIRSample(data.FIRs[vertexIndices[1]][channel][sample]) * gs[1] +
                        data.GetFIRSample(data.FIRs[vertexIndices[2]][channel][sample]) * gs[2] +
                        data.GetFIRSample(data.FIRs[vertexIndices[3]][channel][sample]) * gs[3]) * 32767.0f);

            // Debug: Muestra los primeros 5 coeficientes del canal izquierdo
            /*if (channel == 0 && sample < 10) {
                Serial.printf("HRTF[L][%d]=%.5f, hrtfA=%.5f, hrtfB=%.5f, hrtfC=%.5f, hrtfD=%.5f\n", sample, hrtf[channel][sample],
                                                                                    data.GetFIRSample(data.FIRs[vertexIndices[0]][channel][sample]),
                                                                                    data.GetFIRSample(data.FIRs[vertexIndices[1]][channel][sample]),
                                                                                    data.GetFIRSample(data.FIRs[vertexIndices[2]][channel][sample]),
                                                                                    data.GetFIRSample(data.FIRs[vertexIndices[3]][channel][sample]));
            }*/
        }
    }
    
}

void BinauralAudioPlayer::SetLocation(const Vector3& newLocation) {
    float prevX = targetPos[0];
    float prevY = targetPos[1];
    float prevZ = targetPos[2];

    const float MAX_DIST = 0.1f;

    targetPos[0] = newLocation.x;
    targetPos[1] = newLocation.y;
    targetPos[2] = newLocation.z;



    float dist = sqrt(targetPos[0]*targetPos[0] +
                      targetPos[1]*targetPos[1] +
                      targetPos[2]*targetPos[2]);

    if (dist > 0.0f) {
        targetPos[0] /= dist;
        targetPos[1] /= dist;
        targetPos[2] /= dist;

        dist = constrain(dist, HRTFData::MIN_R, HRTFData::MAX_R);

        targetPos[0] *= dist;
        targetPos[1] *= dist;
        targetPos[2] *= dist;
    }
    else {
        // Caso especial (frente al oyente)
        targetPos[0] = HRTFData::MIN_R;
        targetPos[1] = 0.0f;
        targetPos[2] = 0.0f;
    }

    float distToPrev = sqrt((prevX-targetPos[0]) * (prevX-targetPos[0]) +
                            (prevY-targetPos[1]) * (prevY-targetPos[1]) +
                            (prevZ-targetPos[2]) * (prevZ-targetPos[2]));

    if (distToPrev < MAX_DIST) {
        targetPos[0] = prevX;
        targetPos[1] = prevY;
        targetPos[2] = prevZ;
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
        bRead = audioFile.read(buffer, HRTFData::CHUNK);
        //bRead = audioFile.readBytes((char*)buffer + offset, (HRTFData::CHUNK/2)*sizeof(int16_t));
    }
    else
        Serial.println("Audio file is not avaliable.");
    
    return bRead;
}

//void BinauralAudioPlayer::ProcessBuffer() {
    // Convolucion
    //Convolve_FFT(buffer, hrtf[0], bufferLeft);
    //Convolve_FFT(buffer, hrtf[1], bufferRight);
//}

/*void BinauralAudioPlayer::Convolve(const int16_t* input, const float* hrtfChannel, int16_t* output) {

    const int inputLength = HRTFData::CHUNK + OVERLAP_AMOUNT;
    const int hrtfLength = HRTFData::FIR_LENGTH;
    const int outputLength = inputLength + hrtfLength - 1;

    for (int n = 0; n < outputLength; n++) {
        output[n] = 0; // Inicializar el acumulador
        for (int k = 0; k < hrtfLength; k++) {
            if (n - k >= 0 && n - k < inputLength) {
                output[n] += input[n - k] * hrtf[k]; // Sumatoria de la convolución
            }
        }
    }
}*/

/*void BinauralAudioPlayer::Convolve_FFT(const int16_t* input, const float* hrtfChannel, int16_t* output) {   

    for (uint16_t i = 0; i < HRTFData::CHUNK; i++) {
        fftInput[i] = (float)input[i];
        //fftInput[i] = (float)(32767 * sin(2 * PI * 1000 * i / 44100.0));
    }
    memset(fftInput + HRTFData::CHUNK, 0, (FFT_SIZE - HRTFData::CHUNK) * sizeof(float));  // Zero-padding
    memcpy(fftHrtf, hrtfChannel, HRTFData::FIR_LENGTH * sizeof(float));
    memset(fftHrtf + HRTFData::FIR_LENGTH, 0, (FFT_SIZE - HRTFData::FIR_LENGTH) * sizeof(float));
    
    arm_rfft_fast_f32(&fftInstance, fftInput, fftOutput, 0);  // FFT(input)
    arm_rfft_fast_f32(&fftInstance, fftHrtf, fftInput, 0);    // FFT(hrtf)

    arm_cmplx_mult_cmplx_f32(fftOutput, fftHrtf, fftOutput, FFT_SIZE / 2);
    arm_rfft_fast_f32(&fftInstance, fftOutput, fftInput, 1);  // IFFT

    for (int i = 0; i < HRTFData::CHUNK; i++) {
        output[i] = static_cast<int16_t>(fftInput[OVERLAP_AMOUNT + i]);
    }
}*/

void FASTRUN BinauralAudioPlayer::Convolve_Q15(const int16_t* input, const int16_t* hrtfChannel, int16_t* output) {
    int16_t outputQ15[HRTFData::CHUNK + HRTFData::FIR_LENGTH - 1];
    
    // Convolución directa (input y hrtfQ15 ya son int16_t)
    arm_conv_q15(
        (q15_t*)input,       // Cast a q15_t (equivalente a int16_t)
        HRTFData::CHUNK,
        (q15_t*)hrtfChannel,     // Cast a q15_t
        HRTFData::FIR_LENGTH,
        (q15_t*)outputQ15    // Resultado
    );

    //for (int i = 0; i < HRTFData::CHUNK; i++) {
        //output[i] = __SSAT(outputQ15[i], 16);  // Satura a 16 bits
    //}
    
    // Copia el resultado (CHUNK muestras, descartando el "tail")
    memcpy(output, outputQ15, HRTFData::CHUNK * sizeof(int16_t));
}

std::pair<AudioStream*, AudioStream*> BinauralAudioPlayer::GetAudioSource() {
    return {&queueL, &queueR};
}