#include "progressmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <algorithm>
#include <cmath>

namespace {
bool readJsonObject(const QString &path, QJsonObject &root)
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    root = document.object();
    return true;
}

bool writeJsonObject(const QString &path, const QJsonObject &root)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

QJsonObject readProgressRoot()
{
    QJsonObject root;
    if (readJsonObject(ProgressManager::writableDataPath(QStringLiteral("progress.json")), root)) {
        return root;
    }
    if (readJsonObject(ProgressManager::writableDataPath(QStringLiteral("progress.backup.json")), root)) {
        return root;
    }
    return QJsonObject();
}

void writeProgressRoot(const QJsonObject &root)
{
    if (writeJsonObject(ProgressManager::writableDataPath(QStringLiteral("progress.json")), root)) {
        writeJsonObject(ProgressManager::writableDataPath(QStringLiteral("progress.backup.json")), root);
    }
}
}

bool ProgressManager::prologueSeen()
{
    return readProgressRoot().value(QStringLiteral("prologueSeen")).toBool(false);
}

void ProgressManager::markPrologueSeen()
{
    QJsonObject root = readProgressRoot();
    root.insert(QStringLiteral("prologueSeen"), true);
    root.insert(QStringLiteral("highestUnlockedLevel"), std::max(0, root.value(QStringLiteral("highestUnlockedLevel")).toInt(0)));
    writeProgressRoot(root);
}

int ProgressManager::highestUnlockedLevel()
{
    return std::max(0, readProgressRoot().value(QStringLiteral("highestUnlockedLevel")).toInt(0));
}

int ProgressManager::lastLevelIndex()
{
    const QJsonObject root = readProgressRoot();
    return std::max(0, root.value(QStringLiteral("lastLevelIndex")).toInt(root.value(QStringLiteral("highestUnlockedLevel")).toInt(0)));
}

bool ProgressManager::tutorialFlag(const QString &group, int index)
{
    const QJsonArray values = readProgressRoot().value(group).toArray();
    return values.contains(index);
}

void ProgressManager::markTutorialFlag(const QString &group, int index)
{
    QJsonObject root = readProgressRoot();
    QJsonArray values = root.value(group).toArray();
    if (!values.contains(index)) {
        values.append(index);
    }
    root.insert(group, values);
    writeProgressRoot(root);
}

void ProgressManager::saveLastLevelIndex(int levelIndex)
{
    QJsonObject root = readProgressRoot();
    root.insert(QStringLiteral("lastLevelIndex"), std::max(0, levelIndex));
    root.insert(QStringLiteral("highestUnlockedLevel"),
                std::max(root.value(QStringLiteral("highestUnlockedLevel")).toInt(0), 0));
    writeProgressRoot(root);
}

bool ProgressManager::soundEnabled()
{
    return readProgressRoot().value(QStringLiteral("soundEnabled")).toBool(true);
}

void ProgressManager::setSoundEnabled(bool enabled)
{
    QJsonObject root = readProgressRoot();
    root.insert(QStringLiteral("soundEnabled"), enabled);
    writeProgressRoot(root);
}

bool ProgressManager::ambienceEnabled()
{
    return readProgressRoot().value(QStringLiteral("ambienceEnabled")).toBool(true);
}

void ProgressManager::setAmbienceEnabled(bool enabled)
{
    QJsonObject root = readProgressRoot();
    root.insert(QStringLiteral("ambienceEnabled"), enabled);
    writeProgressRoot(root);
}

int ProgressManager::soundVolume()
{
    return std::clamp(readProgressRoot().value(QStringLiteral("soundVolume")).toInt(82), 0, 100);
}

void ProgressManager::setSoundVolume(int volume)
{
    QJsonObject root = readProgressRoot();
    root.insert(QStringLiteral("soundVolume"), std::clamp(volume, 0, 100));
    writeProgressRoot(root);
}

int ProgressManager::musicVolume()
{
    return std::clamp(readProgressRoot().value(QStringLiteral("musicVolume")).toInt(46), 0, 100);
}

void ProgressManager::setMusicVolume(int volume)
{
    QJsonObject root = readProgressRoot();
    root.insert(QStringLiteral("musicVolume"), std::clamp(volume, 0, 100));
    writeProgressRoot(root);
}

bool ProgressManager::tutorialHintsEnabled()
{
    return readProgressRoot().value(QStringLiteral("tutorialHintsEnabled")).toBool(true);
}

void ProgressManager::setTutorialHintsEnabled(bool enabled)
{
    QJsonObject root = readProgressRoot();
    root.insert(QStringLiteral("tutorialHintsEnabled"), enabled);
    writeProgressRoot(root);
}

void ProgressManager::resetTutorialHints()
{
    QJsonObject root = readProgressRoot();
    root.remove(QStringLiteral("towerIntroductions"));
    root.remove(QStringLiteral("enemyIntroductions"));
    root.insert(QStringLiteral("tutorialHintsEnabled"), true);
    writeProgressRoot(root);
}

void ProgressManager::resetProgressKeepSettings()
{
    const QJsonObject root = readProgressRoot();
    QJsonObject fresh;
    fresh.insert(QStringLiteral("soundEnabled"), root.value(QStringLiteral("soundEnabled")).toBool(true));
    fresh.insert(QStringLiteral("ambienceEnabled"), root.value(QStringLiteral("ambienceEnabled")).toBool(true));
    fresh.insert(QStringLiteral("soundVolume"), std::clamp(root.value(QStringLiteral("soundVolume")).toInt(82), 0, 100));
    fresh.insert(QStringLiteral("musicVolume"), std::clamp(root.value(QStringLiteral("musicVolume")).toInt(46), 0, 100));
    fresh.insert(QStringLiteral("tutorialHintsEnabled"), root.value(QStringLiteral("tutorialHintsEnabled")).toBool(true));
    writeProgressRoot(fresh);
}

void ProgressManager::saveLevelResult(int levelIndex,
                                      const QString &levelName,
                                      bool cleared,
                                      int remainingEnergy,
                                      double elapsedSeconds,
                                      int wavesCleared)
{
    QJsonObject root = readProgressRoot();

    QJsonObject level;
    level.insert(QStringLiteral("name"), levelName);
    level.insert(QStringLiteral("cleared"), cleared);
    level.insert(QStringLiteral("remainingEnergy"), remainingEnergy);
    level.insert(QStringLiteral("elapsedSeconds"), std::round(elapsedSeconds * 10.0) / 10.0);
    level.insert(QStringLiteral("wavesCleared"), wavesCleared);
    root.insert(QStringLiteral("level_%1").arg(levelIndex + 1), level);
    root.insert(QStringLiteral("lastLevelIndex"), std::max(0, levelIndex));
    if (cleared) {
        root.insert(QStringLiteral("highestUnlockedLevel"),
                    std::max(root.value(QStringLiteral("highestUnlockedLevel")).toInt(0), levelIndex + 1));
    } else {
        root.insert(QStringLiteral("highestUnlockedLevel"),
                    std::max(root.value(QStringLiteral("highestUnlockedLevel")).toInt(0), levelIndex));
    }

    writeProgressRoot(root);
}

QString ProgressManager::writableDataPath(const QString &fileName)
{
    QString dirPath = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        dirPath = QDir::currentPath() + QStringLiteral("/data");
        dir = QDir(dirPath);
        dir.mkpath(QStringLiteral("."));
    }
    return dir.filePath(fileName);
}
