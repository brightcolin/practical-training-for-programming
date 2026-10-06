#ifndef PROGRESSMANAGER_H
#define PROGRESSMANAGER_H

#include <QString>

class ProgressManager
{
public:
    static bool prologueSeen();
    static void markPrologueSeen();
    static int highestUnlockedLevel();
    static int lastLevelIndex();
    static bool tutorialFlag(const QString &group, int index);
    static void markTutorialFlag(const QString &group, int index);
    static void saveLastLevelIndex(int levelIndex);
    static bool soundEnabled();
    static void setSoundEnabled(bool enabled);
    static bool ambienceEnabled();
    static void setAmbienceEnabled(bool enabled);
    static int soundVolume();
    static void setSoundVolume(int volume);
    static int musicVolume();
    static void setMusicVolume(int volume);
    static bool tutorialHintsEnabled();
    static void setTutorialHintsEnabled(bool enabled);
    static void resetTutorialHints();
    static void resetProgressKeepSettings();
    static void saveLevelResult(int levelIndex,
                                const QString &levelName,
                                bool cleared,
                                int remainingEnergy,
                                double elapsedSeconds,
                                int wavesCleared);
    static QString writableDataPath(const QString &fileName);
};

#endif
