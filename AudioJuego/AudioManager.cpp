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
EXTMEM AudioConnection*     AudioManager::monoConns[4] = {nullptr};
EXTMEM AudioConnection*     AudioManager::stereoConns[2][3] = {nullptr};

EXTMEM IAudioPlayer*        AudioManager::monoSources[4] = {nullptr};
EXTMEM IAudioPlayer* AudioManager::stereoSources[3] = {nullptr};
int8_t AudioManager::mixersIndex = 8;

EXTMEM HRTFData AudioManager::hrtfData = {};
EXTMEM AudioPlaySdWav AudioManager::wavPool[MONO_CAPACITY];
uint8_t AudioManager::wavPoolIndex = 0;

void AudioManager::Setup() {
    globalVolume = 0.3;
    audioShield.enable();
    audioShield.volume(globalVolume);

    connMonoToLeft = new AudioConnection(mixerMono, 0, mixerL, 3);
    connMonoToRight = new AudioConnection(mixerMono, 0, mixerR, 3);
    connLeftToOutput = new AudioConnection(mixerL, 0, audioOutput, 0);
    connRightToOutput = new AudioConnection(mixerR, 0, audioOutput, 1);

    mixersIndex = 8;
    wavPoolIndex = 0;

    if (!monoSources || !stereoSources) {
        Serial.println("Error: No se pudo asignar memoria para los arrays");
        while(1);
    }

    memset(monoSources, 0, sizeof(monoSources));
    memset(stereoSources, 0, sizeof(stereoSources));
    memset(monoConns, 0, sizeof(monoConns));
    memset(stereoConns, 0, sizeof(stereoConns));


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
        if (stereoSources[i] != nullptr && stereoConns[0][i] != nullptr && stereoConns[1][i] != nullptr) {
            if (stereoSources[i]->Update())
                RemoveStereoAt(i);
        }
    }

    for (uint8_t i = 0; i < MONO_CAPACITY; i++) {
        if (monoSources[i] != nullptr && monoConns[i] != nullptr) {
            if (monoSources[i]->Update())
                RemoveMonoAt(i);
        }
    }
}

void AudioManager::PrintConns() {
    Serial.println("stereoConns:");
    for (uint8_t i = 0; i < STEREO_CAPACITY; i++) {
        Serial.printf("[0][%d] != null: %d\n", i, (stereoConns[0][i] != nullptr));
        Serial.printf("[1][%d] != null: %d\n", i, (stereoConns[1][i] != nullptr));
        Serial.println("");
    }

    Serial.println("\nmonoConns:");
    for (uint8_t i = 0; i < MONO_CAPACITY; i++) {
        Serial.printf("[%d] != null: %d\n", i, (monoConns[i] != nullptr));
    }
    Serial.println("");
}

int8_t AudioManager::Add(IAudioPlayer* source, float volume) {
    if (source->IsQuick() && volume <= 0)
        return -1;

    if (source->IsBinaural()) {
        int index = GetFreeStereoIndex();
        if (index == -1) {
            delete source;
            return index;
        }

        //Serial.printf("Stereo index: %d\n", index);
        std::pair<AudioStream*, AudioStream*> audioS = source->GetAudioSource();
        stereoConns[0][index] = new AudioConnection(*audioS.first, 0, mixerL, index);
        stereoConns[1][index] = new AudioConnection(*audioS.second, 0, mixerR, index);
        MarkStereoIndex(index);

        stereoSources[index] = source;

        if (volume >= 0) SetVolume(index, volume);
        return index;
    }

    int index = GetFreeMonoIndex();
    if (index == -1) {
        delete source;
        return index;
    }

    //Serial.printf("Mono index: %d\n", index);
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

    Serial.printf("RemoveMonoAt(%d) | monoConns[%d] != nullptr: %d, monoSources[%d] != nullptr: %d\n", i, i, (monoConns[i] != nullptr), i, (monoSources[i] != nullptr));

    // Eliminar sus conexiones
    if (monoConns[i] != nullptr)
        delete monoConns[i];
    monoConns[i] = nullptr;
    Serial.println("monoConns deleted");

    // Eliminar el objeto
    if (monoSources[i] != nullptr)
        delete monoSources[i];
    monoSources[i] = nullptr;
    Serial.println("monoSources deleted");

    // Liberar el index
    FreeMonoIndex(i);
    Serial.println("Mono index free");

    //Serial.printf("Eliminado monoSources[%d]\n\n", i);
    return true;
}

bool AudioManager::RemoveStereoAt(uint8_t i) {
    if (i >= STEREO_CAPACITY) return false;

    //Serial.printf("RemoveStereoAt(%d) | stereoConns[%d] != nullptr: %d, stereoSources[%d] != nullptr: %d\n", i, i, (stereoConns[0][i] != nullptr), i, (stereoConns[1][i] != nullptr));

    // Eliminar sus conexiones
    if (stereoConns[0][i] != nullptr)
        delete stereoConns[0][i];
    if (stereoConns[1][i] != nullptr)
        delete stereoConns[1][i];
    stereoConns[0][i] = nullptr;
    stereoConns[1][i] = nullptr;
    //Serial.println("stereoConns deleted");

    // Eliminar el objeto
    if (stereoSources[i] != nullptr)
        delete stereoSources[i];
    stereoSources[i] = nullptr;
    //Serial.println("stereoSources deleted");

    // Liberar el index
    FreeStereoIndex(i);
    //Serial.println("Stereo index free");

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
        //Serial.printf("monoSources[%d] != nullptr: %d ", i, monoSources[i] != nullptr);
        if (monoSources[i] != nullptr) {
            Serial.printf("monoSources[%d]->IsQuick(): %d, monoConns[%d] != nullptr: %d\n", i, monoSources[i]->IsQuick(), i, monoConns[i] != nullptr);
            if (monoSources[i]->IsQuick())
                RemoveMonoAt(i);
        }
        //Serial.println("");
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

int AudioManager::GetFreeWavIndex() {
    for (int i = 0; i < 4; i++) {
        if (!(wavPoolIndex & (1 << i)))
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

void AudioManager::MarkWavIndex(uint8_t index) {
    if (index > 3) return;
    mixersIndex |= (1 << index);
}

void AudioManager::FreeWavIndex(uint8_t index) {
    if (index > 3) // No se puede desactivar la primera entrada (siempre ocupada por la salida de mixerMono)
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
        mixerMono.gain(index-4, volume);
        return;
    }

    mixerL.gain(index, volume);
    mixerR.gain(index, volume);
}

void AudioManager::SetGlobalVolume(float newVolume) {
    globalVolume = std::clamp(newVolume, 0.0f, 1.0f);
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
    if (index == 3 || index < 0 || index > 7) return false;
    return GetIndexBit(index);
}
