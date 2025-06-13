#include "AudioManager.h"
#include "AudioPlayer.h"
#include "BinauralAudioPlayer.h"
#include <Arduino.h>

EXTMEM uint16_t AudioManager::numStereoSources = 0;
EXTMEM uint16_t AudioManager::numMonoSources = 0;
EXTMEM AudioPlayer** AudioManager::monoSources = nullptr;
EXTMEM BinauralAudioPlayer** AudioManager::stereoSources = nullptr;
EXTMEM uint16_t AudioManager::monoCapacity = 3;
EXTMEM uint16_t AudioManager::stereoCapacity = 3;

EXTMEM HRTFData AudioManager::hrtfData = {};

void AudioManager::Setup() {
    numMonoSources = 0;
    numStereoSources = 0;
    monoCapacity = 3;
    stereoCapacity = 3;
    monoSources = static_cast<AudioPlayer**>(extmem_malloc(monoCapacity * sizeof(AudioPlayer*)));
    stereoSources = static_cast<BinauralAudioPlayer**>(extmem_malloc(stereoCapacity * sizeof(BinauralAudioPlayer*)));

    if (!monoSources || !stereoSources) {
        Serial.println("Error: No se pudo asignar memoria para los arrays");
        while(1);
    }

    memset(monoSources, 0, monoCapacity * sizeof(AudioPlayer*));
    memset(stereoSources, 0, stereoCapacity * sizeof(AudioPlayer*));

    LoadHRTFData();

    /*for (int i = 0; i < 0; i++) {
        Serial.printf("Tetra %d:\n[%d, %d, %d, %d]\n\n", i, hrtfData.simplices[i][0],
                                                            hrtfData.simplices[i][1],
                                                            hrtfData.simplices[i][2],
                                                            hrtfData.simplices[i][3]);
    }*/

    /*int tetras[4] = {906, 907, 1769, 1770};
    int tetrasLength = 0;
    for (int i = 0; i < tetrasLength; i++) {
        Serial.printf("Tetra %d:\n[%d, %d, %d, %d]\n\n", tetras[i], hrtfData.simplices[tetras[i]][0],
                                                                    hrtfData.simplices[tetras[i]][1],
                                                                    hrtfData.simplices[tetras[i]][2],
                                                                    hrtfData.simplices[tetras[i]][3]);
    }*/
}

void AudioManager::LoadHRTFData() {
    LoadBinary("tetraCoords.bin", hrtfData.tetraCoords, sizeof(hrtfData.tetraCoords));
    LoadBinary("Tinv.bin", hrtfData.Tinv, sizeof(hrtfData.Tinv));
    LoadBinary("tri_simplices.bin", hrtfData.simplices, sizeof(hrtfData.simplices));
    LoadBinary("tri_neighbors.bin", hrtfData.neighbors, sizeof(hrtfData.neighbors));
    LoadBinary("FIRs.bin", hrtfData.FIRs, sizeof(hrtfData.FIRs));

    //Serial.println("Primeras muestras FIR (int16 raw):");
    /*for (int i = 0; i < 5; i++) {
        Serial.printf("Pos0 L:%.5f R:%.5f\n",
                        hrtfData.GetFIRSample(hrtfData.FIRs[0][0][i]),  // Primera posición, canal izquierdo
                        hrtfData.GetFIRSample(hrtfData.FIRs[0][1][i])); // Primera posición, canal derecho
    }*/
    //ConvertQuantizedHRTF();
}

void AudioManager::LoadBinary(const char* path, void* dest, size_t expected_size) {
    File file = SD.open(path);
    if(!file || file.size() != expected_size) {
        Serial.printf("Error en %s (tamaño: %d vs esperado: %d)\n",
                        path, file.size(), expected_size);
        file.close();
        return;
    }

    file.read((uint8_t*)dest, expected_size);
    file.close();
}

/*void AudioManager::ConvertQuantizedHRTF() {
    const float scale = 1.0f / 32767.0f;  // Factor de escala inverso
    for (uint16_t pos = 0; pos < HRTFData::MAX_POSITIONS; pos++) {
        for (uint8_t ch = 0; ch < 2; ch++) {
            for (uint8_t sample = 0; sample < HRTFData::FIR_LENGTH; sample++) {
                // Convertir de int16 a float normalizado
                hrtfData.FIRs[pos][ch][sample] *= scale;
            }
        }
    }
}*/

void AudioManager::UpdateAll() {
    for (uint16_t i = 0; i < numStereoSources; i++) {
        if (stereoSources[i] != nullptr) {
            stereoSources[i]->Update();
        }
    }

    for (uint16_t i = 0; i < numMonoSources; i++) {
        if (monoSources[i] != nullptr)
            monoSources[i]->Update();
    }
}

bool AudioManager::Add(AudioPlayer* source) {
    if (source->IsBinaural()) {
        if (numStereoSources >= stereoCapacity) {
            if (!ResizeStereo(stereoCapacity + 3)) {
                Serial.println("ResizeStereo failed.");
                return false;
            }
        }
        stereoSources[numStereoSources++] = static_cast<BinauralAudioPlayer*>(source);
        return true;
    }

    if (numMonoSources >= monoCapacity) {
        if (!ResizeMono(monoCapacity + 3))
            return false;
    }
    monoSources[numMonoSources++] = source;
    return true;
}

bool AudioManager::Remove(AudioPlayer* source) {
    if (source->IsBinaural()) {
        for (uint16_t i = 0; i < numStereoSources; ++i) {
            if (stereoSources[i] == source) {
                for (uint16_t j = i; j < numStereoSources - 1; ++j) {
                    stereoSources[j] = stereoSources[j + 1];
                }
                numStereoSources--;
                return true;
            }
        }
    }
    else {
        for (uint16_t i = 0; i < numMonoSources; ++i) {
            if (monoSources[i] == source) {
                for (uint16_t j = i; j < numMonoSources - 1; ++j) {
                    monoSources[j] = monoSources[j + 1];
                }
                numMonoSources--;
                return true;
            }
        }
    }
    return false;
}

bool AudioManager::RemoveMonoAt(uint16_t i) {
    if (i >= numMonoSources)
        return false;

    for (uint16_t j = i; j < numMonoSources - 1; ++j) {
        monoSources[j] = monoSources[j + 1];
    }
    numMonoSources--;
    return true;
}

bool AudioManager::RemoveStereoAt(uint16_t i) {
    if (i >= numStereoSources)
        return false;

    for (uint16_t j = i; j < numStereoSources - 1; ++j) {
        stereoSources[j] = stereoSources[j + 1];
    }
    numStereoSources--;
    return true;
}

bool AudioManager::ResizeMono(uint16_t newCapacity) {
    AudioPlayer** newArray = static_cast<AudioPlayer**>(extmem_malloc(newCapacity * sizeof(AudioPlayer*)));
    if (!newArray) return false;

    for (uint16_t i = 0; i < numMonoSources; ++i)
        newArray[i] = monoSources[i];

    delete[] monoSources;
    monoSources = newArray;
    monoCapacity = newCapacity;
    return true;
}

bool AudioManager::ResizeStereo(uint16_t newCapacity) {
    BinauralAudioPlayer** newArray = static_cast<BinauralAudioPlayer**>(extmem_malloc(newCapacity * sizeof(BinauralAudioPlayer*)));;
    if (!newArray) return false;

    for (uint16_t i = 0; i < numStereoSources; ++i)
        newArray[i] = stereoSources[i];

    delete[] stereoSources;
    stereoSources = newArray;
    stereoCapacity = newCapacity;
    return true;
}

void AudioManager::PlayAll() {
    for (uint16_t i = 0; i < numStereoSources; i++) {
        if (stereoSources[i] != nullptr)
            stereoSources[i]->Resume();
    }

    for (uint16_t i = 0; i < numMonoSources; i++) {
        if (monoSources[i] != nullptr)
            monoSources[i]->Resume();
    }
}

void AudioManager::PauseAll() {
    for (uint16_t i = 0; i < numStereoSources; i++) {
        if (stereoSources[i] != nullptr)
            stereoSources[i]->Pause();
    }

    for (uint16_t i = 0; i < numMonoSources; i++) {
        if (monoSources[i] != nullptr)
            monoSources[i]->Pause();
    }
}

void AudioManager::ResetAll() {
    for (uint16_t i = 0; i < numStereoSources; i++) {
        if (stereoSources[i] != nullptr)
            stereoSources[i]->Play();
    }

    for (uint16_t i = 0; i < numMonoSources; i++) {
        if (monoSources[i] != nullptr)
            monoSources[i]->Play();
    }
}

void AudioManager::StopAll() {
    for (uint16_t i = 0; i < numStereoSources; i++) {
        if (stereoSources[i] != nullptr)
            stereoSources[i]->Stop();
    }

    for (uint16_t i = 0; i < numMonoSources; i++) {
        if (monoSources[i] != nullptr)
            monoSources[i]->Stop();
    }
}