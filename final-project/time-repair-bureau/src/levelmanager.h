#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include "gametypes.h"

#include <QPoint>
#include <QString>

#include <array>
#include <vector>

struct LevelData {
    QString name;
    QString briefing;
    QString victoryText;
    int initialEnergy = 240;
    std::vector<TileKind> tiles;
    std::vector<QPoint> pathCells;
    QPoint portalFrom = QPoint(5, 1);
    QPoint portalTo = QPoint(8, 3);
    std::vector<std::vector<WaveEntry>> waves;
};

class LevelManager
{
public:
    static LevelData loadLevel(int levelIndex, const QString &customLevelPath, int rows, int cols);
    static std::array<TowerSpec, 6> loadTowerSpecs();
    static int builtInLevelCount();
    static QString levelTitle(int levelIndex);
    static QString levelDifficulty(int levelIndex);
    static QString levelBriefing(int levelIndex);
    static QString levelSelectSummary(int levelIndex);
    static QString levelNextHook(int levelIndex);
    static QString levelVictoryText(int levelIndex);

private:
    static QString configPath(const QString &fileName);
    static LevelData builtInLevel(int levelIndex, int rows, int cols);
    static LevelData customLevel(const QString &path, int rows, int cols);
    static std::vector<std::vector<WaveEntry>> wavesForLevel(int levelIndex);
};

#endif
