#include "GameManager.h"

uint8_t GameManager::state = OFF;
uint8_t GameManager::prevState = OFF;
uint32_t GameManager::lastUpdate = 0;
uint32_t GameManager::deltaTime = 1;
uint32_t GameManager::timer = 0;
int16_t GameManager::fastTimer = 0;

uint8_t GameManager::optionSelected = 0;
uint8_t GameManager::maxOptions = 0;

float GameManager::jetAz = 0;
float GameManager::jetEl = 0;
uint8_t GameManager::lives = MAX_LIVES;
uint8_t GameManager::points = 0;
int16_t GameManager::eventTimer = 0;
int8_t GameManager::missileDirection = -1;
int8_t GameManager::eventState = 0;
Vector3 GameManager::missileLocation = Vector3(0, -MISSILE_MAX_DISTANCE, 0);

int8_t GameManager::menuSoundtrack = -1;
int8_t GameManager::gameSoundtrack = -1;
int8_t GameManager::jetEngine = -1;
int8_t GameManager::beepAlarm = -1;
int8_t GameManager::missile = -1;
int8_t GameManager::voice = -1;
int8_t GameManager::intro = -1;
int8_t GameManager::outro = -1;

uint8_t GameManager::currentAudio = 0;

// Settings
int8_t GameManager::invertX = 1;
int8_t GameManager::invertY = 1;

uint8_t GameManager::generalVolume = GENERAL_VOLUME;
uint8_t GameManager::uiVolume = UI_VOLUME;
uint8_t GameManager::voicesVolume = VOICES_VOLUME;
uint8_t GameManager::ambientVolume = AMBIENT_VOLUME;
uint8_t GameManager::alarmVolume = ALARM_VOLUME;
uint8_t GameManager::musicVolume = MUSIC_VOLUME;

uint8_t GameManager::newGeneralVolume = GENERAL_VOLUME;
uint8_t GameManager::newUiVolume = UI_VOLUME;
uint8_t GameManager::newVoicesVolume = VOICES_VOLUME;
uint8_t GameManager::newAmbientVolume = AMBIENT_VOLUME;
uint8_t GameManager::newAlarmVolume = ALARM_VOLUME;
uint8_t GameManager::newMusicVolume = MUSIC_VOLUME;

void GameManager::Setup() {
    state = OFF;
    timer = 0;
    randomSeed(analogRead(A12)); // Usar una entrada flotante (analogica no conectada) como ruido para la semilla
}

void GameManager::Update() {
    if (state == OFF) return; // Si no se ha iniciado el juego

    UpdateActives();

    if (state != PAUSE) {
        uint32_t now = millis();
        deltaTime = now - lastUpdate;
        //Serial.printf("deltaTime (%d) = now (%d) - lastUpdate (%d)\n", deltaTime, now, lastUpdate);
        lastUpdate = now;
        timer += deltaTime;
    }

    ManageInit();
    ManageIntro();
    ManagePlay();
    ManageOutroWin();
    ManageOutroLost();
    ManagePause();
    ManageSettings();
}

void GameManager::Init(bool playIntro) {
    CM::DisableControls();
    if (playIntro || menuSoundtrack == -1) {
        AM::ClearAll();
        IAudioPlayer* menuSoundtrackPtr = new AudioPlayer(audioMenuSoundtrack, -1, loopStartMenuSoundtrack);
        float newVolume = MAX_VOLUME > 0 ? (float)(musicVolume) / (float)(MAX_VOLUME) : 0;
        menuSoundtrack = AM::Add(menuSoundtrackPtr, newVolume);
        Serial.printf("menuSoundTrack index: %d, volume: %.1f\n", menuSoundtrack, newVolume);
        AM::PlayAll();
    }
    
    timer = playIntro ? 0 : 8000; // Saltar presentacion
    fastTimer = 0;
    Serial.printf("Timer: %d\n", timer);
    optionSelected = 0; // Al iniciar el menu, dejar sin opcion seleccionada por defecto
    maxOptions = MENU_MAX_OPTIONS;

    prevState = state;
    state = INIT;
    Serial.println("Game::Init done");

    lastUpdate = millis();
}

void GameManager::ManageInit() {
    //Serial.println("Game::ManageInit running");
    //Serial.printf("Timer: %d\n", timer);
    if (state != INIT || timer < 8000)
        return; // Duracion de la presentacion de inicio
    fastTimer -= fastTimer > 0 ? deltaTime : 0; // decrementar fastTimer

    CM::EnableControls();
    if (CM::GetY() != 0 && fastTimer <= 0) {
        if (optionSelected == 0) {
            Scroll(-1);
            QuickAudioPlayer* optionAudio = new QuickAudioPlayer(menuOptionsAudios[optionSelected-1]);
            float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
            AM::Add(optionAudio, newVolume);
        }
        else if (maxOptions > 1) {
            Scroll(CM::GetY());
            QuickAudioPlayer* optionAudio = new QuickAudioPlayer(menuOptionsAudios[optionSelected-1]);
            float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
            AM::Add(optionAudio, newVolume);
        }
        fastTimer = 2000;
    }

    if (CM::GetButtonUp(CM::DOWN))
        MenuApplyOption();
}

void GameManager::Intro() {
    CM::DisableControls();
    AM::ClearAll();
    IAudioPlayer* gameSoundtrackPtr = new AudioPlayer(audioGameSoundtrack, -1, loopStartGameSoundtrack);
    float newVolume = MAX_VOLUME > 0 ? (float)(musicVolume) / (float)(MAX_VOLUME) : 0;
    gameSoundtrack = AM::Add(gameSoundtrackPtr, newVolume);

    AudioPlayer* jetEnginePtr = new AudioPlayer(audioJetEngine, -1);
    newVolume = MAX_VOLUME > 0 ? (float)(ambientVolume) / (float)(MAX_VOLUME) : 0;
    jetEngine = AM::Add(jetEnginePtr, newVolume);
    AM::PlayAll();

    timer = 0;
    currentAudio = 0;

    prevState = state;
    state = INTRO;
}

void GameManager::ManageIntro() {
    if (state != INTRO || timer < 1000) return; // Delay audios
    //Serial.println("Manage Intro");

    //Serial.println(timer);
    if (currentAudio < INTRO_AUDIOS_SIZE && (currentAudio == 0 || (currentAudio > 0 && timer > introAudios[currentAudio-1].delay)) && intro == -1) {
        BinauralAudioPlayer* introPtr = new BinauralAudioPlayer(introAudios[currentAudio].filename, 0);
        float newVolume = MAX_VOLUME > 0 ? (float)(voicesVolume) / (float)(MAX_VOLUME) : 0;
        intro = AM::Add(introPtr, newVolume);
        if (intro != -1) {
            int8_t side = currentAudio % 2 == 0 ? 1 : -1;
            introPtr->SetLocation(Vector3(side * 2, 0, 0));
            introPtr->Play();
        }

        timer = 0;
        Serial.printf("CurrentAudio: %d\n", currentAudio);
        currentAudio++;
    }

    if (currentAudio >= INTRO_AUDIOS_SIZE && timer > 2000 && intro == -1)
        Play();
}

void GameManager::Play() {
    Serial.println("Play");
    CM::EnableControls();
    if (state != INTRO) { // Si no se ha reproducido la intro, (Ej: Reintentar la partida), iniciar loas audios necesarios
        AM::ClearAll();
        IAudioPlayer* gameSoundtrackPtr = new AudioPlayer(audioGameSoundtrack, -1, loopStartGameSoundtrack);
        float newVolume = MAX_VOLUME > 0 ? (float)(musicVolume) / (float)(MAX_VOLUME) : 0;
        gameSoundtrack = AM::Add(gameSoundtrackPtr, newVolume);

        IAudioPlayer* jetEnginePtr = new BinauralAudioPlayer(audioJetEngine, -1);
        newVolume = MAX_VOLUME > 0 ? (float)(ambientVolume) / (float)(MAX_VOLUME) : 0;
        jetEngine = AM::Add(jetEnginePtr, newVolume);
    }

    timer = 0;
    fastTimer = 0;
    eventTimer = 0;
    lives = MAX_LIVES;
    points = 0;
    jetAz = 0;
    jetEl = 0;
    missileDirection = -1;
    eventState = -1;
    missile = -1;
    voice = -1;
    beepAlarm = -1;
    missileLocation = Vector3(0, -MISSILE_MAX_DISTANCE, 0);

    prevState = state;
    state = PLAYING;
}

void GameManager::ManagePlay() {
    if (state != PLAYING) return;

    if (timer > MAX_GAME_DURATION*1000 && eventState == -1 && missile == -1 && lives > 0) {
        OutroWin();
        return;
    }
    else if (lives <= 0 && missile == -1 && eventState == -1) {
        OutroLost();
        return;
    }

    // Control de la nave
    int8_t side = missileDirection < LEFT_AUDIOS_SIZE ? -1 : 1;
    missileLocation = Vector3(CM::GetX() * invertX, 0, CM::GetY() * invertY);

    // Control de eventos
    {
        eventTimer -= deltaTime;
        if (timer < 2000 || timer > MAX_GAME_DURATION*1000 - 2000) // Tiempo donde suceden eventos
            return;

        if (eventTimer <= 0 && eventState == -1 && voice == -1 && beepAlarm == -1 && missile == -1) { // Iniciar evento
            Serial.println("event ready");
            eventState = 0;
            missileDirection = random(0, LEFT_AUDIOS_SIZE + RIGHT_AUDIOS_SIZE);
            AM::RemoveAt(voice);
            AM::RemoveAt(beepAlarm);
        }
        if (eventState == 0) { // Activar aviso por voz de misil
            if (missileDirection < LEFT_AUDIOS_SIZE) {

                Serial.println("Voice missile left");
                BinauralAudioPlayer* voicePtr = new BinauralAudioPlayer(leftAudios[missileDirection].filename, 0);
                float newVolume = MAX_VOLUME > 0 ? (float)(voicesVolume) / (float)(MAX_VOLUME) : 0;
                voice = AM::Add(voicePtr, newVolume);
                if (voice != -1) {
                    int8_t side = random(0, 2) == 0 ? -1 : 1; // Aleatorizar el lado en el que suena la voz (confusion = reto)
                    voicePtr->SetLocation(Vector3(side*VOICE_DISTANCE, 0, 0));
                    voicePtr->Play();
                    eventState++;
                }
            }
            else if (missileDirection >= LEFT_AUDIOS_SIZE) {
                
                Serial.println("Voice missile right");
                BinauralAudioPlayer* voicePtr = new BinauralAudioPlayer(rightAudios[missileDirection-LEFT_AUDIOS_SIZE].filename, 0);
                float newVolume = MAX_VOLUME > 0 ? (float)(voicesVolume) / (float)(MAX_VOLUME) : 0;
                voice = AM::Add(voicePtr, newVolume);
                if (voice != -1) {
                    int8_t side = random(0, 2) == 0 ? -1 : 1; // Aleatorizar el lado en el que suena la voz (confusion = reto)
                    voicePtr->SetLocation(Vector3(side*VOICE_DISTANCE, 0, 0));
                    voicePtr->Play();
                    eventState++;
                }
            }
        }
        else if (eventState == 1 && voice == -1) { // Activar alarma
            BinauralAudioPlayer* alarmPtr = new BinauralAudioPlayer(audioBeepAlarm, 0);
            float newVolume = MAX_VOLUME > 0 ? (float)(alarmVolume) / (float)(MAX_VOLUME) : 0;
            beepAlarm = AM::Add(alarmPtr, newVolume);
            if (beepAlarm != -1) {
                int8_t side = missileDirection < LEFT_AUDIOS_SIZE ? -1 : 1;
                alarmPtr->SetLocation(missileLocation.normalized().multiply(ALARM_DISTANCE));
                alarmPtr->Play();
                eventState++;
            }
        }
        else if (eventState == 2) { // Actualizar la posicion de la alarma
            int8_t side = missileDirection < LEFT_AUDIOS_SIZE ? -1 : 1;
            if (-CM::GetX()*invertX*side < MISSILE_MIN_DISTANCE && beepAlarm == -1) { // Si colisiona el misil
                if (voice != -1) AM::RemoveAt(voice);
                if (beepAlarm != -1) AM::RemoveAt(beepAlarm);

                if (lives > 1)
                    CM::Vibrate(side < 0 ? CM::VIBRATOR_LEFT : CM::VIBRATOR_RIGHT, 200);
                else {
                    CM::Vibrate(CM::VIBRATOR_LEFT, 200);
                    CM::Vibrate(CM::VIBRATOR_RIGHT, 200);
                }

                BinauralAudioPlayer* explosionPtr = new BinauralAudioPlayer(audioExplosion, 0);
                float newVolume = MAX_VOLUME > 0 ? (float)(ambientVolume) / (float)(MAX_VOLUME) : 0;
                missile = AM::Add(explosionPtr, newVolume);
                if (missile != -1) {
                    explosionPtr->SetLocation(missileLocation.normalized().multiply(MISSILE_MIN_DISTANCE));
                    explosionPtr->Play();
                    lives--;
                    eventState = -1;
                    eventTimer = random(MIN_EVENT_TIMER, MAX_EVENT_TIMER+1);
                }
            }
            else if (beepAlarm == -1) {
                BinauralAudioPlayer* missilePtr = new BinauralAudioPlayer(audioMissile, 0);
                float newVolume = MAX_VOLUME > 0 ? (float)(ambientVolume) / (float)(MAX_VOLUME) : 0;
                missile = AM::Add(missilePtr, newVolume);
                if (missile != -1) {
                    missilePtr->SetLocation(missileLocation.normalized().multiply(MISSILE_MAX_DISTANCE));
                    missilePtr->Play();
                    eventState = -1;
                    eventTimer = random(MIN_EVENT_TIMER, MAX_EVENT_TIMER+1);
                }
            }
            else {
                static_cast<BinauralAudioPlayer*>(AM::GetAudio(beepAlarm))->SetLocation(missileLocation.normalized().multiply(ALARM_DISTANCE));
            }
        }
    }
}

void GameManager::OutroWin() {
    Serial.println("Outro win");
    CM::DisableControls();

    timer = 0;
    fastTimer = 0;
    currentAudio = 0;
    optionSelected = 0;
    maxOptions = OUTRO_MAX_OPTIONS;

    prevState = state;
    state = OUTRO_WIN;
}

void GameManager::ManageOutroWin() {
    if (state != OUTRO_WIN) return;

    if (currentAudio < OUTRO_AUDIOS_SIZE && (currentAudio == 0 || (currentAudio > 0 && timer > outroAudiosWin[currentAudio-1].delay)) && outro == -1) {
        BinauralAudioPlayer* outroPtr = new BinauralAudioPlayer(outroAudiosWin[currentAudio].filename, 0);
        float newVolume = MAX_VOLUME > 0 ? (float)(voicesVolume) / (float)(MAX_VOLUME) : 0;
        outro = AM::Add(outroPtr, newVolume);
        if (outro != -1) {
            int8_t side = currentAudio % 2 == 0 ? 1 : -1;
            outroPtr->SetLocation(Vector3(side * 2, 0, 0));
            outroPtr->Play();
        }

        timer = 0;
        Serial.printf("CurrentAudio: %d\n", currentAudio);
        currentAudio++;
    }

    if (currentAudio >= OUTRO_AUDIOS_SIZE && timer > 3000 && outro == -1) { // 3s de delay desde el inicio del ultimo audio
        Init();
        /*if (jetEngine != -1) {
            AM::RemoveAt(jetEngine);
        }

        fastTimer -= fastTimer > 0 ? deltaTime : 0; // decrementar fastTimer

        CM::EnableControls();
        if (CM::GetY() != 0 && fastTimer <= 0) {
            if (optionSelected == 0) { // Si no hay opciones seleccionadas, seleccionar la primera con el movimiento del joystick
                Scroll(-1);
                QuickAudioPlayer* optionAudio = new QuickAudioPlayer(outroOptionsAudios[optionSelected-1]);
                float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
                AM::Add(optionAudio, newVolume);
            }
            else if (maxOptions > 1) { // Navegar entre opciones si hay mas de una opcion
                Scroll(CM::GetY());
                QuickAudioPlayer* optionAudio = new QuickAudioPlayer(outroOptionsAudios[optionSelected-1]);
                float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
                AM::Add(optionAudio, newVolume);
            }
            fastTimer = 2000;
        }

        if (CM::GetButtonUp(CM::DOWN))
            OutroApplyOption();*/
    }
}

void GameManager::OutroLost() {
    Serial.println("Outro lost");
    CM::DisableControls();
    AM::PrintConns();
    //AM::ClearQuicks();
    AM::PrintConns();

    timer = 0;
    outro = -1;
    fastTimer = 0;
    currentAudio = 0;
    optionSelected = 0;
    maxOptions = OUTRO_MAX_OPTIONS;

    prevState = state;
    state = OUTRO_LOST;
}

void GameManager::ManageOutroLost() {
    if (state != OUTRO_LOST) return;
    Serial.println("ManageOutroLost");
    if (currentAudio < OUTRO_AUDIOS_SIZE && (currentAudio == 0 || (currentAudio > 0 && timer > outroAudiosLost[currentAudio-1].delay)) && outro == -1) {
        Serial.println("Muere en el primer if");
        BinauralAudioPlayer* outroPtr = new BinauralAudioPlayer(outroAudiosLost[currentAudio].filename, 0);
        float newVolume = MAX_VOLUME > 0 ? (float)(voicesVolume) / (float)(MAX_VOLUME) : 0;
        outro = AM::Add(outroPtr, newVolume);
        if (outro != -1) {
            int8_t side = currentAudio % 2 == 0 ? 1 : -1;
            outroPtr->SetLocation(Vector3(side * 2, 0, 0));
            outroPtr->Play();

            timer = 0;
            Serial.printf("CurrentAudio: %d\n", currentAudio);
            currentAudio++;
        }
    }

    if (currentAudio >= OUTRO_AUDIOS_SIZE && timer > 5000 && outro == -1) { // 3s de delay desde el inicio del ultimo audio
        Init();
        /*Serial.println("Muere en el segundo if");
        if (jetEngine != -1) {
            AM::RemoveAt(jetEngine);
            jetEngine = -1;
        }

        fastTimer -= fastTimer > 0 ? deltaTime : 0; // decrementar fastTimer

        CM::EnableControls();
        if (CM::GetY() != 0 && fastTimer <= 0) {
            if (optionSelected == 0) { // Si no hay opciones seleccionadas, seleccionar la primera con el movimiento del joystick
                AM::PrintConns();
                Scroll(-1);
                AM::PrintConns();
                Serial.printf("Option selected: %d", optionSelected);
                QuickAudioPlayer* optionAudio = new QuickAudioPlayer("ui_settings.wav");
                float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
                AM::Add(optionAudio, newVolume);
            }
            else if (maxOptions > 1) { // Navegar entre opciones si hay mas de una opcion
                AM::PrintConns();
                Scroll(CM::GetY());
                AM::PrintConns();
                Serial.printf("Option selected: %d", optionSelected);
                QuickAudioPlayer* optionAudio = new QuickAudioPlayer("ui_settings.wav");
                float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
                AM::Add(optionAudio, newVolume);
            }
            fastTimer = 2000;
        }

        if (CM::GetButtonUp(CM::DOWN))
            OutroApplyOption();*/
    }
    Serial.println("ManageOutroLost 2");
}

void GameManager::Pause() {
    AM::PauseAll();
    CM::EnableControls();

    optionSelected = 0;
    maxOptions = PAUSE_MAX_OPTIONS;

    prevState = state;
    state = PAUSE;
}
void GameManager::Resume() {
    AM::ResumeAll();
    prevState = state;
    state = PLAYING;
}

void GameManager::ManagePause() {
    if (state != PAUSE) return;

    if (CM::GetY() != 0) {
        if (optionSelected == 0) {
            Scroll(-1);
            QuickAudioPlayer* optionAudio = new QuickAudioPlayer(pauseOptionsAudios[optionSelected-1]);
            float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
            AM::Add(optionAudio, newVolume);
        }
        else if (maxOptions > 1) {
            Scroll(CM::GetY());
            QuickAudioPlayer* optionAudio = new QuickAudioPlayer(pauseOptionsAudios[optionSelected-1]);
            float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
            AM::Add(optionAudio, newVolume);
        }
    }

    if (CM::GetButtonUp(CM::DOWN))
        PauseApplyOption();
}

void GameManager::Settings() {
    //AM::PauseAll();
    if (menuSoundtrack != -1) {
        float newVolume = MAX_VOLUME > 0 ? (float)(musicVolume) / (float)(MAX_VOLUME) : 0;
        AM::SetVolume(menuSoundtrack, newVolume/2.0f);
    }
    CM::EnableControls();

    fastTimer = 0;

    optionSelected = 0;
    maxOptions = SETTINGS_MAX_OPTIONS;

    newGeneralVolume = generalVolume;
    newUiVolume = uiVolume;
    newVoicesVolume = voicesVolume;
    newAmbientVolume = ambientVolume;
    newAlarmVolume = alarmVolume;
    newMusicVolume = musicVolume;

    prevState = state;
    state = SETTINGS;
}

void GameManager::ManageSettings() {
    //Serial.printf("state: %d\n", state);
    if (state != SETTINGS) return;
    //Serial.println("Manage Settings");

    fastTimer -= fastTimer > 0 ? deltaTime : 0; // decrementar fastTimer
    if (CM::GetY() != 0 && fastTimer <= 0) {
        if (optionSelected == 0) {
            Scroll(-1);
            QuickAudioPlayer* optionAudio = new QuickAudioPlayer(settingsOptionsAudios[optionSelected-1]);
            float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
            AM::Add(optionAudio, newVolume);
        }
        else if (maxOptions > 1) {
            Scroll(CM::GetY());
            QuickAudioPlayer* optionAudio = new QuickAudioPlayer(settingsOptionsAudios[optionSelected-1]);
            float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
            AM::Add(optionAudio, newVolume);
        }
        fastTimer = 2000;
        Serial.printf("Settings Option selected: %d\n", optionSelected);
    }

    float x = CM::GetX();
    if (x != 0 && fastTimer <= 0) { // actualizar input cada minimo margen de tiempo
        uint8_t volume = 0;
        switch(optionSelected) {
            case 1: volume = SetVolume(x, newGeneralVolume);
                Serial.printf("Settings option: General volume | %.1f\n", newGeneralVolume);
                break;
            case 2: volume = SetVolume(x, newUiVolume);
                Serial.printf("Settings option: UI volume | %.1f\n", newUiVolume);
                break;
            case 3: volume = SetVolume(x, newVoicesVolume);
                Serial.printf("Settings option: Voices volume | %.1f\n", newVoicesVolume);
                break;
            case 4: volume = SetVolume(x, newAmbientVolume);
                Serial.printf("Settings option: Ambient volume | %.1f\n", newAmbientVolume);
                break;
            case 5: volume = SetVolume(x, newAlarmVolume);
                Serial.printf("Settings option: Alarm volume | %.1f\n", newAlarmVolume);
                break;
            case 6: volume = SetVolume(x, newMusicVolume);
                Serial.printf("Settings option: Music volume | %.1f\n", newMusicVolume);
                break;
            default:
                break;
        }
        QuickAudioPlayer* volumeSample = new QuickAudioPlayer(audioVolumeSample);
        float newVolume = MAX_VOLUME > 0 ? (float)(volume) / (float)(MAX_VOLUME) : 0;
        AM::Add(volumeSample, newVolume);
        fastTimer = 2000;
    }

    if (CM::GetButtonUp(CM::DOWN))
        SettingsApplyOption();
}

void GameManager::Scroll(float direction) {
    if (direction == 0) return;

    if (direction < 0)
        optionSelected = optionSelected < maxOptions ? optionSelected+1 : 1; // (Incrementar opcion) Si se pasa seleccionar la inicial
    else
        optionSelected = optionSelected > 1 ? optionSelected-1 : maxOptions; // (Decrementar opcion) Si se pasa seleccionar la final

    Serial.printf("Scroll optionSelected: %d\n", optionSelected);
    CM::Vibrate(CM::VIBRATOR_LEFT, 200);
    if (optionSelected == 0 || maxOptions > 1) { // Si solo hay una opcion, reproducir el sonido cuando se selecciona por primera vez
        AM::ClearQuicks();

        QuickAudioPlayer* scrollSound = new QuickAudioPlayer(audioScroll);
        float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
        AM::Add(scrollSound, newVolume);
    }
}

uint8_t GameManager::SetVolume(float direction, uint8_t &option) {
    if (direction == 0) return 0;

    if (direction > 0)
        option += option < MAX_VOLUME ? 1 : 0; // Incrementar opcion
    else
        option -= option > 0 ? 1 : 0; // Decrementar opcion

    return option;
}

void GameManager::MenuApplyOption() {
    switch(optionSelected) {
        case 1: Intro();
            break;
        case 2: Settings();
            break;
        default: return; // opcion sin seleccionar
    }

    CM::Vibrate(CM::VIBRATOR_RIGHT, 200);
    QuickAudioPlayer* selectSound = new QuickAudioPlayer(audioSelect);
    float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
    AM::Add(selectSound, newVolume);
}

void GameManager::OutroApplyOption() {
    switch(optionSelected) {
        case 1: Play();
            break;
        case 2: Settings();
            break;
        case 3: Init();
            break;
        default: return; // opcion sin seleccionar
    }

    CM::Vibrate(CM::VIBRATOR_RIGHT, 200);
    QuickAudioPlayer* selectSound = new QuickAudioPlayer(audioSelect);
    float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
    AM::Add(selectSound, newVolume);
}

void GameManager::PauseApplyOption() {
    switch(optionSelected) {
        case 1: Resume();
            break;
        case 2: Play();
            break;
        case 3: Settings();
            break;
        case 4: Init();
            break;
        default: return; // opcion sin seleccionar
    }

    CM::Vibrate(CM::VIBRATOR_RIGHT, 200);
    QuickAudioPlayer* selectSound = new QuickAudioPlayer(audioSelect);
    float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
    AM::Add(selectSound, newVolume);
}

void GameManager::SettingsApplyOption() {
    switch(optionSelected) {
        case 7: RestoreSettings();
            break;
        case 8: CancelSettings();
                ApplySettings();
                if (prevState == INIT) Init(false);
                else Pause();
            break;
        case 9: ApplySettings();
            if (prevState == INIT) Init(false);
            else Pause();
            break;
        default: return; // opcion sin seleccionar o no clickable
    }

    CM::Vibrate(CM::VIBRATOR_RIGHT, 200);
    QuickAudioPlayer* selectSound = new QuickAudioPlayer(audioSelect);
    float newVolume = MAX_VOLUME > 0 ? (float)(uiVolume) / (float)(MAX_VOLUME) : 0;
    AM::Add(selectSound, newVolume);
}

void GameManager::RestoreSettings() {
    newGeneralVolume = GENERAL_VOLUME;
    newUiVolume = UI_VOLUME;
    newVoicesVolume = VOICES_VOLUME;
    newAmbientVolume = AMBIENT_VOLUME;
    newAlarmVolume = ALARM_VOLUME;
    newMusicVolume = MUSIC_VOLUME;
}

void GameManager::CancelSettings() {
    newGeneralVolume = generalVolume;
    newUiVolume = uiVolume;
    newVoicesVolume = voicesVolume;
    newAmbientVolume = ambientVolume;
    newAlarmVolume = alarmVolume;
    newMusicVolume = musicVolume;
}

void GameManager::ApplySettings() {
    generalVolume = newGeneralVolume;
    uiVolume = MAX_VOLUME > 0 ? (uint8_t)((float)(generalVolume * newUiVolume / (float)MAX_VOLUME)) : 0;
    voicesVolume = MAX_VOLUME > 0 ? (uint8_t)((float)(generalVolume * newVoicesVolume / (float)MAX_VOLUME)) : 0;
    ambientVolume = MAX_VOLUME > 0 ? (uint8_t)((float)(generalVolume * newAmbientVolume / (float)MAX_VOLUME)) : 0;
    alarmVolume = MAX_VOLUME > 0 ? (uint8_t)((float)(generalVolume * newAlarmVolume / (float)MAX_VOLUME)) : 0;
    musicVolume = MAX_VOLUME > 0 ? (uint8_t)((float)(generalVolume * newMusicVolume / (float)MAX_VOLUME)) : 0;

    if (menuSoundtrack != -1) AM::SetVolume(menuSoundtrack, (float)musicVolume);
    if (gameSoundtrack != -1) AM::SetVolume(gameSoundtrack, (float)musicVolume);
    if (jetEngine != -1) AM::SetVolume(jetEngine, (float)ambientVolume);
    if (beepAlarm != -1) AM::SetVolume(beepAlarm, (float)alarmVolume);
    if (missile != -1) AM::SetVolume(missile, (float)ambientVolume);
    if (voice != -1) AM::SetVolume(voice, (float)voicesVolume);
    if (intro != -1) AM::SetVolume(intro, (float)ambientVolume);
    if (outro != -1) AM::SetVolume(outro, (float)ambientVolume);
}

void GameManager::UpdateActives() {
    if (!AM::Exists(menuSoundtrack)) menuSoundtrack = -1;
    if (!AM::Exists(gameSoundtrack)) gameSoundtrack = -1;
    if (!AM::Exists(jetEngine)) jetEngine = -1;
    if (!AM::Exists(beepAlarm)) beepAlarm = -1;
    if (!AM::Exists(missile)) missile = -1;
    if (!AM::Exists(voice)) voice = -1;
    if (!AM::Exists(intro)) intro = -1;
    if (!AM::Exists(outro)) outro = -1;
}