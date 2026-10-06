#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QString>

enum class SoundCue {
    UiClick,
    TerminalBoot,
    Dialogue,
    MissionStart,
    Build,
    Upgrade,
    Freeze,
    BossAlert,
    BossStinger,
    ErrorDeny,
    CoreDamage,
    CodexUnlock,
    Victory,
    Failure
};

enum class MusicCue {
    Menu,
    Battle,
    Story,
    Boss
};

class SoundManager
{
public:
    static void reloadSettings();
    static void setSoundEnabled(bool enabled);
    static void setMusicEnabled(bool enabled);
    static void setSoundVolume(int volume);
    static void setMusicVolume(int volume);
    static void play(SoundCue cue);
    static void startMusic(MusicCue cue);
    static void resumeMusic();
    static void stopMusic();
    static void refreshVolumes();
    static QString statusText();
};

#endif
