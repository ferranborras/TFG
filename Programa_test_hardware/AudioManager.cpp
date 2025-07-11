#include "AudioManager.h"

EXTMEM AudioOutputI2S       AudioManager::audioOutput;
EXTMEM AudioControlSGTL5000 AudioManager::audioShield;
float AudioManager::globalVolume = 0.3;
EXTMEM AudioMixer4          AudioManager::mixerMono;
EXTMEM AudioMixer4          AudioManager::mixerL;
EXTMEM AudioMixer4          AudioManager::mixerR;
EXTMEM AudioConnection*     AudioManager::connMonoToLeft = nullptr;
EXTMEM AudioConnection*     AudioManager::connMonoToRight = nullptr;
EXTMEM AudioConnection*     AudioManager::connLeftToOutput = nullptr;
EXTMEM AudioConnection*     AudioManager::connRightToOutput = nullptr;
EXTMEM AudioConnection*     AudioManager::monoConns[4] = {};
EXTMEM AudioConnection*     AudioManager::stereoConns[2][3] = {};

EXTMEM IAudioPlayer*        AudioManager::monoSources[4] = {};
EXTMEM IAudioPlayer* AudioManager::stereoSources[3] = {};
int8_t AudioManager::mixersIndex = 8;

EXTMEM HRTFData AudioManager::hrtfData = {};

void AudioManager::Setup() {
    globalVolume = 0.3;
    audioShield.enable();
    audioShield.volume(globalVolume);

    connMonoToLeft = new AudioConnection(mixerMono, 0, mixerL, 3);
    connMonoToRight = new AudioConnection(mixerMono, 0, mixerR, 3);
    connLeftToOutput = new AudioConnection(mixerL, 0, audioOutput, 0);
    connRightToOutput = new AudioConnection(mixerR, 0, audioOutput, 1);

    mixersIndex = 8;

    if (!monoSources || !stereoSources) {
        Serial.println("Error: No se pudo asignar memoria para los arrays");
        while(1);
    }

    memset(monoSources, 0, MONO_CAPACITY * sizeof(IAudioPlayer*));
    memset(stereoSources, 0, STEREO_CAPACITY * sizeof(IAudioPlayer*));

    LoadHRTFData();

    ClearAll();
}

void AudioManager::LoadHRTFData() {
    LoadBinary("tetraCoords.bin", hrtfData.tetraCoords, sizeof(hrtfData.tetraCoords));
    LoadBinary("Tinv.bin", hrtfData.Tinv, sizeof(hrtfData.Tinv));
    LoadBinary("tri_simplices.bin", hrtfData.simplices, sizeof(hrtfData.simplices));
    LoadBinary("tri_neighbors.bin", hrtfData.neighbors, sizeof(hrtfData.neighbors));
    LoadBinary("FIRs.bin", hrtfData.FIRs, sizeof(hrtfData.FIRs));
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

void AudioManager::UpdateAll() {
    for (uint8_t i = 0; i < STEREO_CAPACITY; i++) {
        if (stereoSources[i] != nullptr) {
            if (stereoSources[i]->Update())
                RemoveStereoAt(i);
        }
    }

    for (uint8_t i = 0; i < MONO_CAPACITY; i++) {
        if (monoSources[i] != nullptr) {
            if (monoSources[i]->Update())
                RemoveMonoAt(i);
        }
    }
}

int8_t AudioManager::Add(IAudioPlayer* source, float volume) {
    if (source->IsQuick() && volume <= 0)
        return -1;

    if (source->IsBinaural()) {
        int index = GetFreeStereoIndex();
        if (index == -1)
            return index;

        std::pair<AudioStream*, AudioStream*> audioS = source->GetAudioSource();
        stereoConns[0][index] = new AudioConnection(*audioS.first, 0, mixerL, index);
        stereoConns[1][index] = new AudioConnection(*audioS.second, 0, mixerR, index);
        MarkStereoIndex(index);

        stereoSources[index] = source;

        if (volume >= 0) SetVolume(index, volume);
        return index;
    }

    int index = GetFreeMonoIndex();
    if (index == -1)
        return index;

    std::pair<AudioStream*, AudioStream*> audioS = source->GetAudioSource();
    monoConns[index] = new AudioConnection(*audioS.first, 0, mixerMono, index);
    MarkMonoIndex(index);

    monoSources[index] = source;
    if (volume >= 0) SetVolume(index+4, volume);
    return index+4;
}

IAudioPlayer* AudioManager::GetAudio(int8_t index) {
    if (index == 3) return nullptr;

    if (index < 4)
        return stereoSources[index];
    return monoSources[index-4];
}

bool AudioManager::RemoveAt(uint8_t i) {
    if (i < 0 || i > 7 || i == 3) return false;

    if (i < 4)
        return RemoveStereoAt(i);
    return RemoveMonoAt(i-4);
}

bool AudioManager::RemoveMonoAt(uint8_t i) {
    if (i >= MONO_CAPACITY) return false;

    if (monoSources[i] == nullptr) {
        Serial.printf("No se puede eliminar: monoSources[%d] == nullptr\n", i);
        return false;
    }

    // Eliminar sus conexiones
    delete monoConns[i];
    monoConns[i] = nullptr;

    // Eliminar el objeto
    delete monoSources[i];
    monoSources[i] = nullptr;

    // Liberar el index
    FreeMonoIndex(i);

    Serial.printf("Eliminado monoSources[%d]\n", i);
    return true;
}

bool AudioManager::RemoveStereoAt(uint8_t i) {
    if (i >= STEREO_CAPACITY) return false;

    if (stereoSources[i] == nullptr) return false;

    // Eliminar sus conexiones
    delete stereoConns[0][i];
    delete stereoConns[1][i];
    stereoConns[0][i] = nullptr;
    stereoConns[1][i] = nullptr;

    // Eliminar el objeto
    delete stereoSources[i];
    stereoSources[i] = nullptr;

    // Liberar el index
    FreeStereoIndex(i);

    return true;
}

void AudioManager::ResumeAll() {
    for (uint8_t i = 0; i < STEREO_CAPACITY; i++) {
        if (stereoSources[i] != nullptr)
            static_cast<AudioPlayer*>(stereoSources[i])->Resume();
    }

    for (uint8_t i = 0; i < MONO_CAPACITY; i++) {
        if (monoSources[i] != nullptr) {
            if (!monoSources[i]->IsQuick())
                static_cast<AudioPlayer*>(monoSources[i])->Resume();
        }
    }
}

void AudioManager::PauseAll() {
    for (uint8_t i = 0; i < STEREO_CAPACITY; i++) {
        if (stereoSources[i] != nullptr)
            static_cast<AudioPlayer*>(stereoSources[i])->Pause();
    }

    for (uint8_t i = 0; i < MONO_CAPACITY; i++) {
        if (monoSources[i] != nullptr) {
            if (monoSources[i]->IsQuick())
                RemoveMonoAt(i);
            else
                static_cast<AudioPlayer*>(monoSources[i])->Pause();
        }
    }
}

void AudioManager::PlayAll() {
    for (uint8_t i = 0; i < STEREO_CAPACITY; i++) {
        if (stereoSources[i] != nullptr)
            static_cast<AudioPlayer*>(stereoSources[i])->Play();
    }

    for (uint8_t i = 0; i < MONO_CAPACITY; i++) {
        if (monoSources[i] != nullptr) {
            if (!monoSources[i]->IsQuick())
                static_cast<AudioPlayer*>(monoSources[i])->Play();
        }
    }
}

void AudioManager::ClearAll() {
    for (uint8_t i = 0; i < 8; i++)
        RemoveAt(i);
}

void AudioManager::ClearQuicks() {
    for (uint8_t i = 0; i < MONO_CAPACITY; i++) {
        if (monoSources[i] != nullptr) {
            if (monoSources[i]->IsQuick())
                RemoveMonoAt(i);
        }
    }
}

int AudioManager::GetFreeMonoIndex() {
    for (int i = 0; i < 4; i++) {
        if (!(mixersIndex & (1 << (i+4))))
            return i;
    }
    return -1;
}

int AudioManager::GetFreeStereoIndex() {
    for (int i = 0; i < 3; i++) {
        if (!(mixersIndex & (1 << i)))
            return i;
    }
    return -1;
}

bool AudioManager::GetIndexBit(uint8_t index) {
    if (index > 7) return false;
    return mixersIndex & (1 << index);
}

void AudioManager::MarkMonoIndex(uint8_t index) {
    if (index > 3) return;
    mixersIndex |= (1 << (index+4));
}

void AudioManager::FreeMonoIndex(uint8_t index) {
    if (index > 3) return;
    mixersIndex &= ~(1 << (index+4));
}

void AudioManager::MarkStereoIndex(uint8_t index) {
    if (index > 3) return;
    mixersIndex |= (1 << index);
}

void AudioManager::FreeStereoIndex(uint8_t index) {
    if (index > 2) // No se puede desactivar la primera entrada (siempre ocupada por la salida de mixerMono)
        return;
    mixersIndex &= ~(1 << index);
}

bool AudioManager::IsEmpty() {
    return mixersIndex == 0b00001000;
}

float AudioManager::GetVolume() {
    return globalVolume;
}

void AudioManager::SetVolume(int8_t index, float newVolume) {
    float volume = std::clamp(newVolume, 0.0f, 1.0f);
    if (index > 3) {
        Serial.printf("MixerMono index: %d, volume: %.1f, newVolume: %.1f\n", index, volume, newVolume);
        mixerMono.gain(index-4, volume);
        return;
    }

    mixerL.gain(index, volume);
    mixerR.gain(index, volume);
}

void AudioManager::SetGlobalVolume(float newVolume) {
    globalVolume = std::clamp(newVolume, 0.0f, 1.0f);
    Serial.printf("SetGlobalVolume() | newVolume: %.1f, globalVolume: %.1f\n", newVolume, globalVolume);
    audioShield.volume(globalVolume);
}

uint8_t AudioManager::GetMonoNum() {
    uint8_t num = 0;
    for (uint8_t i = 4; i < 8; i++) // bits 4-7 corresponden a mixerMono
        num += GetIndexBit(i) ? 1 : 0; // Si el bit es 1 incrementamos cantidad

    return num;
}

uint8_t AudioManager::GetStereoNum() {
    uint8_t num = 0;
    for (uint8_t i = 0; i < 3; i++) // bit 3: conexion con mixerMono no cuenta
        num += GetIndexBit(i) ? 1 : 0; // Si el bit es 1 incrementamos cantidad

    return num;
}

bool AudioManager::Exists(uint8_t index) {
    if (index == 3) return false;
    return GetIndexBit(index);
}
