#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <algorithm>
#include <math.h>
#include "Vector3.h"

#include "IAudioPlayer.h"
#include "AudioPlayer.h"
#include "BinauralAudioPlayer.h"
#include "QuickAudioPlayer.h"

#include "AudioManager.h"
using AM = AudioManager;
#include "ControlsManager.h"
using CM = ControlsManager;

#define OFF 0
#define INIT 1
#define INTRO 2
#define PLAYING 3
#define OUTRO_WIN 4
#define OUTRO_LOST 5
#define SETTINGS 6
#define PAUSE 7

class GameManager {
private:

    // RAW
    static constexpr uint16_t ALARM_DURATION = 5000;
    static constexpr const char* audioBeepAlarm = "game_alarm.raw"; // done
    static constexpr const char* audioJetEngine = "game_jet_engine.raw"; // done
    static constexpr const char* audioMissile = "game_missile.raw"; // done
    static constexpr const char* audioExplosion = "game_explosion.raw"; // done
    static constexpr const char* audioMenuSoundtrack = "menu_soundtrack_title.raw"; // done
    static constexpr uint32_t loopStartMenuSoundtrack = 1424532; // Bytes a partir de donde empieza el loop
    static constexpr const char* audioGameSoundtrack = "game_soundtrack.raw"; // done
    static constexpr uint32_t loopStartGameSoundtrack = 3496622; // Bytes a partir de donde empieza el loop

    static uint8_t state;
    static uint8_t prevState;
    static uint32_t lastUpdate;
    static uint32_t deltaTime;
    static uint32_t timer;
    static int16_t fastTimer;

    static uint8_t optionSelected;
    static uint8_t maxOptions;

    static constexpr uint8_t MAX_LIVES = 3;
    static constexpr uint8_t MAX_GAME_DURATION = 60; // segundos
    static uint8_t lives;
    static uint8_t points;
    static float jetAz;
    static float jetEl;
    static constexpr uint8_t MAX_ROTATION_ANGLE = 90;
    static constexpr uint16_t ROTATION_VEL = 500;
    static constexpr uint8_t JET_VEL = 30;
    static constexpr float JET_DISTANCE = 2.0f;
    static constexpr int16_t MIN_EVENT_TIMER = 3000;
    static constexpr int16_t MAX_EVENT_TIMER = 6000;
    static int16_t eventTimer;
    static int8_t missileDirection;
    static int8_t eventState;
    static constexpr float VOICE_DISTANCE = 1.0f;
    static Vector3 missileLocation;
    static constexpr float MISSILE_MAX_DISTANCE = 2.0f;// 10.0f;
    static constexpr float MISSILE_MIN_DISTANCE = 0.5f;
    static constexpr uint8_t MISSILE_VEL = 40;
    static constexpr float ALARM_DISTANCE = 1.5f;

    static int8_t menuSoundtrack;
    static int8_t gameSoundtrack;
    static int8_t jetEngine;
    static int8_t beepAlarm;
    static int8_t missile;
    static int8_t voice;
    static int8_t intro;
    static int8_t outro;

    // Settings
    static constexpr uint8_t MAX_VOLUME = 10;
    static constexpr uint8_t GENERAL_VOLUME = MAX_VOLUME/2;
    static constexpr uint8_t UI_VOLUME = MAX_VOLUME/2;
    static constexpr uint8_t VOICES_VOLUME = MAX_VOLUME/2;
    static constexpr uint8_t AMBIENT_VOLUME = MAX_VOLUME/2;
    static constexpr uint8_t ALARM_VOLUME = MAX_VOLUME/2;
    static constexpr uint8_t MUSIC_VOLUME = MAX_VOLUME/2;

    static uint8_t generalVolume;
    static uint8_t uiVolume;
    static uint8_t voicesVolume;
    static uint8_t ambientVolume;
    static uint8_t alarmVolume;
    static uint8_t musicVolume;

    static uint8_t newGeneralVolume;
    static uint8_t newUiVolume;
    static uint8_t newVoicesVolume;
    static uint8_t newAmbientVolume;
    static uint8_t newAlarmVolume;
    static uint8_t newMusicVolume;

    struct AudioDelayed {
        const char* filename;
        uint16_t delay;
    };
    static uint8_t currentAudio;
    static constexpr uint8_t INTRO_AUDIOS_SIZE = 6;
    static constexpr AudioDelayed introAudios[] = {
        {"intro_female1.raw", 10000}, // done
        {"intro_male1.raw", 7000}, // done
        {"intro_female2.raw", 7000}, // done
        {"intro_male2.raw", 5000}, // done
        {"intro_female3.raw", 6000}, // done
        {"intro_male3.raw", 2000}, // done
    };

    static constexpr uint8_t LEFT_AUDIOS_SIZE = 10;
    static constexpr AudioDelayed leftAudios[] = {
        {"left_female1.raw", 2000}, // done
        {"left_male1.raw", 2000}, // done
        {"left_female2.raw", 2000}, // done
        {"left_male2.raw", 3000}, // done
        {"left_female3.raw", 2000}, // done
        {"left_male3.raw", 2000}, // done
        {"left_female4.raw", 2000}, // done
        {"left_male4.raw", 2000}, // done
        {"left_female5.raw", 2000}, // done
        {"left_male5.raw", 2000}, // done
    };
    static constexpr uint8_t RIGHT_AUDIOS_SIZE = 10;
    static constexpr AudioDelayed rightAudios[] = {
        {"right_female1.raw", 2000}, // done
        {"right_male1.raw", 2000}, // done
        {"right_female2.raw", 2000}, // done
        {"right_male2.raw", 3000}, // done
        {"right_female3.raw", 2000}, // done
        {"right_male3.raw", 2000}, // done
        {"right_female4.raw", 2000}, // done
        {"right_male4.raw", 2000}, // done
        {"right_female5.raw", 2000}, // done
        {"right_male5.raw", 2000}, // done
    };

    static constexpr uint8_t OUTRO_AUDIOS_SIZE = 2;
    static constexpr AudioDelayed outroAudiosWin[] = {
        {"outro_female_win.raw", 6000}, // done
        {"outro_male_win.raw", 3000}, // done
    };
    static constexpr AudioDelayed outroAudiosLost[] = {
        {"outro_female_lost.raw", 3000}, // done
        {"outro_male_lost.raw", 2000}, // done
    };

    // WAV
    // Options audios
    static constexpr const char* audioScroll = "ui_scroll.wav"; // done
    static constexpr const char* audioSelect = "ui_select.wav"; // done

    static constexpr uint8_t MENU_MAX_OPTIONS = 2;
    static constexpr const char* menuOptionsAudios[] = {
        "ui_new_game.wav", // done
        "ui_settings.wav" // done
    };
    static constexpr uint8_t PAUSE_MAX_OPTIONS = 4;
    static constexpr const char* pauseOptionsAudios[] = {
        "ui_resume.wav", // done
        "ui_restart.wav", // done
        "ui_settings.wav", // done
        "ui_exit.wav" // done
    };
    static constexpr uint8_t OUTRO_MAX_OPTIONS = 3;
    static constexpr const char* outroOptionsAudios[] = {
        "ui_restart.wav", // done
        "ui_settings.wav", // done
        "ui_exit.wav" // done
    };
    static constexpr uint8_t SETTINGS_MAX_OPTIONS = 9;
    static int8_t invertX;
    static int8_t invertY;
    static constexpr const char* audioVolumeSample = "ui_volume.wav"; // done
    static constexpr const char* settingsOptionsAudios[] = {
        "ui_general_volume.wav", // done
        "ui_interface_volume.wav", // done
        "ui_voices_volume.wav", // done
        "ui_ambient_volume.wav", // done
        "ui_alarm_volume.wav", // done
        "ui_music_volume.wav", // done
        "ui_restore_default.wav", // done
        "ui_cancel.wav", // done
        "ui_accept.wav" // done
    };

    static void UpdateActives();

    static void ManageInit();
    static void ManageIntro();
    static void ManagePlay();
    static void ManageOutroWin();
    static void ManageOutroLost();
    static void ManagePause();
    static void ManageSettings();

    static void MenuApplyOption();
    static void OutroApplyOption();
    static void PauseApplyOption();
    static void SettingsApplyOption();
    static void RestoreSettings();
    static void ApplySettings();
    static void CancelSettings();

    static void Scroll(float direction);
    static uint8_t SetVolume(float direction, uint8_t &option);

    static void StartGameIntro();
    static void StartGameOutro();
public:
    GameManager() = default;

    static void Setup();
    static void Update();

    static void Init(bool playIntro = true);
    static void Intro();
    static void Play();
    static void OutroWin();
    static void OutroLost();
    static void Pause();
    static void Resume();
    static void Settings();
};

#endif