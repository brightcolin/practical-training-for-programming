#include "soundmanager.h"

#include "progressmanager.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {
bool g_settingsLoaded = false;
bool g_soundEnabled = true;
bool g_musicEnabled = true;
int g_soundVolume = 82;
int g_musicVolume = 46;

void loadSettings()
{
    g_soundEnabled = ProgressManager::soundEnabled();
    g_musicEnabled = ProgressManager::ambienceEnabled();
    g_soundVolume = std::clamp(ProgressManager::soundVolume(), 0, 100);
    g_musicVolume = std::clamp(ProgressManager::musicVolume(), 0, 100);
    g_settingsLoaded = true;
}

void ensureSettingsLoaded()
{
    if (!g_settingsLoaded) {
        loadSettings();
    }
}

QString sfxPath(const QString &fileName)
{
    const QString relative = QStringLiteral("assets/sfx/") + fileName;
    const QStringList bases = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath()
    };

    for (const QString &base : bases) {
        QDir dir(base);
        for (int depth = 0; depth < 8; ++depth) {
            const QString direct = QDir::cleanPath(dir.filePath(relative));
            if (QFile::exists(direct)) {
                return direct;
            }
            const QString underSrc = QDir::cleanPath(dir.filePath(QStringLiteral("src/") + relative));
            if (QFile::exists(underSrc)) {
                return underSrc;
            }
            const QString underProjectSrc = QDir::cleanPath(dir.filePath(QStringLiteral("project/src/") + relative));
            if (QFile::exists(underProjectSrc)) {
                return underProjectSrc;
            }
            if (!dir.cdUp()) {
                break;
            }
        }
    }
    return QString();
}

QString fileForCue(SoundCue cue)
{
    switch (cue) {
    case SoundCue::UiClick: return QStringLiteral("ui-click.wav");
    case SoundCue::TerminalBoot: return QStringLiteral("terminal-boot.wav");
    case SoundCue::Dialogue: return QStringLiteral("dialogue-blip.wav");
    case SoundCue::MissionStart: return QStringLiteral("mission-start.wav");
    case SoundCue::Build: return QStringLiteral("build.wav");
    case SoundCue::Upgrade: return QStringLiteral("upgrade.wav");
    case SoundCue::Freeze: return QStringLiteral("freeze.wav");
    case SoundCue::BossAlert: return QStringLiteral("boss-alert.wav");
    case SoundCue::BossStinger: return QStringLiteral("boss-stinger.wav");
    case SoundCue::ErrorDeny: return QStringLiteral("error-deny.wav");
    case SoundCue::CoreDamage: return QStringLiteral("core-damage.wav");
    case SoundCue::CodexUnlock: return QStringLiteral("codex-unlock.wav");
    case SoundCue::Victory: return QStringLiteral("victory.wav");
    case SoundCue::Failure: return QStringLiteral("failure.wav");
    }
    return QString();
}

QString fileForMusic(MusicCue cue)
{
    switch (cue) {
    case MusicCue::Menu: return QStringLiteral("ambience-menu.wav");
    case MusicCue::Battle: return QStringLiteral("ambience-battle.wav");
    case MusicCue::Story: return QStringLiteral("story-bgm.wav");
    case MusicCue::Boss: return QStringLiteral("boss-bgm.wav");
    }
    return QString();
}

quint16 read16(const char *data)
{
    return static_cast<quint16>(static_cast<unsigned char>(data[0]))
           | (static_cast<quint16>(static_cast<unsigned char>(data[1])) << 8);
}

quint32 read32(const char *data)
{
    return static_cast<quint32>(static_cast<unsigned char>(data[0]))
           | (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8)
           | (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16)
           | (static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24);
}

#ifdef Q_OS_WIN
struct WaveData {
    QByteArray bytes;
    WAVEFORMATEX format = {};
    DWORD dataOffset = 0;
    DWORD dataSize = 0;
    int durationMs = 1200;
};

struct ActiveWave {
    std::shared_ptr<WaveData> wave;
    HWAVEOUT handle = nullptr;
    WAVEHDR header = {};
    bool music = false;
    double gain = 1.0;
};

QHash<QString, std::shared_ptr<WaveData>> g_waveCache;
std::vector<std::shared_ptr<ActiveWave>> g_activeWaves;
std::shared_ptr<ActiveWave> g_musicWave;
MusicCue g_currentMusic = MusicCue::Menu;
bool g_musicPlaying = false;

int volumePercent(bool music)
{
    ensureSettingsLoaded();
    return music ? g_musicVolume : g_soundVolume;
}

double musicGain(MusicCue cue)
{
    // The source tracks come from different masters. These gains align their
    // measured RMS loudness to the quieter story track, so one music setting
    // has a consistent perceived level across menus, combat and boss scenes.
    switch (cue) {
    case MusicCue::Menu: return 0.61;
    case MusicCue::Battle: return 0.65;
    case MusicCue::Story: return 1.0;
    case MusicCue::Boss: return 0.53;
    }
    return 1.0;
}

DWORD volumeWord(bool music, double gain = 1.0)
{
    const double effectivePercent = std::clamp(volumePercent(music) * gain, 0.0, 100.0);
    const DWORD value = static_cast<DWORD>(std::round(effectivePercent * 65535.0 / 100.0));
    return (value & 0xffffu) | ((value & 0xffffu) << 16);
}

std::shared_ptr<WaveData> loadWave(const QString &path)
{
    if (g_waveCache.contains(path)) {
        return g_waveCache.value(path);
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }

    auto wave = std::make_shared<WaveData>();
    wave->bytes = file.readAll();
    const char *raw = wave->bytes.constData();
    const int size = wave->bytes.size();
    if (size < 44 || std::memcmp(raw, "RIFF", 4) != 0 || std::memcmp(raw + 8, "WAVE", 4) != 0) {
        return nullptr;
    }

    bool foundFmt = false;
    bool foundData = false;
    int pos = 12;
    while (pos + 8 <= size) {
        const char *id = raw + pos;
        const quint32 chunkSize = read32(raw + pos + 4);
        const int payload = pos + 8;
        if (payload + static_cast<int>(chunkSize) > size) {
            break;
        }

        if (std::memcmp(id, "fmt ", 4) == 0 && chunkSize >= 16) {
            wave->format.wFormatTag = read16(raw + payload);
            wave->format.nChannels = read16(raw + payload + 2);
            wave->format.nSamplesPerSec = read32(raw + payload + 4);
            wave->format.nAvgBytesPerSec = read32(raw + payload + 8);
            wave->format.nBlockAlign = read16(raw + payload + 12);
            wave->format.wBitsPerSample = read16(raw + payload + 14);
            wave->format.cbSize = 0;
            foundFmt = true;
        } else if (std::memcmp(id, "data", 4) == 0 && chunkSize > 0) {
            wave->dataOffset = static_cast<DWORD>(payload);
            wave->dataSize = chunkSize;
            foundData = true;
        }

        pos = payload + static_cast<int>(chunkSize) + (chunkSize % 2);
    }

    if (!foundFmt || !foundData || wave->format.nAvgBytesPerSec == 0) {
        return nullptr;
    }
    wave->durationMs = std::max(180, static_cast<int>(wave->dataSize * 1000.0 / wave->format.nAvgBytesPerSec) + 80);
    g_waveCache.insert(path, wave);
    return wave;
}

void closeWave(const std::shared_ptr<ActiveWave> &active)
{
    if (!active || !active->handle) {
        return;
    }
    waveOutReset(active->handle);
    waveOutUnprepareHeader(active->handle, &active->header, sizeof(WAVEHDR));
    waveOutClose(active->handle);
    active->handle = nullptr;
}

std::shared_ptr<ActiveWave> playWaveFile(const QString &path, bool loop, bool music, double gain = 1.0)
{
    const std::shared_ptr<WaveData> wave = loadWave(path);
    if (!wave) {
        return nullptr;
    }

    auto active = std::make_shared<ActiveWave>();
    active->wave = wave;
    active->music = music;
    active->gain = gain;
    if (waveOutOpen(&active->handle, WAVE_MAPPER, &wave->format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        return nullptr;
    }

    waveOutSetVolume(active->handle, volumeWord(music, active->gain));
    active->header.lpData = wave->bytes.data() + wave->dataOffset;
    active->header.dwBufferLength = wave->dataSize;
    if (loop) {
        active->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        active->header.dwLoops = 0xffffffffu;
    }

    if (waveOutPrepareHeader(active->handle, &active->header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR
        || waveOutWrite(active->handle, &active->header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        closeWave(active);
        return nullptr;
    }

    if (!loop) {
        g_activeWaves.push_back(active);
        QTimer::singleShot(wave->durationMs, [active]() {
            closeWave(active);
            g_activeWaves.erase(std::remove_if(g_activeWaves.begin(), g_activeWaves.end(), [active](const std::shared_ptr<ActiveWave> &item) {
                                    return item == active;
                                }),
                                g_activeWaves.end());
        });
    }
    return active;
}
#endif
}

void SoundManager::reloadSettings()
{
    loadSettings();
    refreshVolumes();
}

void SoundManager::setSoundEnabled(bool enabled)
{
    ensureSettingsLoaded();
    g_soundEnabled = enabled;
    ProgressManager::setSoundEnabled(enabled);
    refreshVolumes();
}

void SoundManager::setMusicEnabled(bool enabled)
{
    ensureSettingsLoaded();
    g_musicEnabled = enabled;
    ProgressManager::setAmbienceEnabled(enabled);
    refreshVolumes();
}

void SoundManager::setSoundVolume(int volume)
{
    ensureSettingsLoaded();
    g_soundVolume = std::clamp(volume, 0, 100);
    ProgressManager::setSoundVolume(g_soundVolume);
    refreshVolumes();
}

void SoundManager::setMusicVolume(int volume)
{
    ensureSettingsLoaded();
    g_musicVolume = std::clamp(volume, 0, 100);
    ProgressManager::setMusicVolume(g_musicVolume);
    refreshVolumes();
}

void SoundManager::play(SoundCue cue)
{
    ensureSettingsLoaded();
    if (!g_soundEnabled) {
        return;
    }
#ifdef Q_OS_WIN
    // Keep effects created after a page switch aligned with live preferences.
    refreshVolumes();
    playWaveFile(sfxPath(fileForCue(cue)), false, false);
#endif
}

void SoundManager::startMusic(MusicCue cue)
{
    ensureSettingsLoaded();
    if (!g_musicEnabled) {
#ifdef Q_OS_WIN
        g_currentMusic = cue;
#endif
        stopMusic();
        return;
    }
#ifdef Q_OS_WIN
    if (g_musicPlaying && cue == g_currentMusic && g_musicWave && g_musicWave->handle) {
        refreshVolumes();
        return;
    }
    stopMusic();
    g_musicWave = playWaveFile(sfxPath(fileForMusic(cue)), true, true, musicGain(cue));
    if (g_musicWave && g_musicWave->handle) {
        g_currentMusic = cue;
        g_musicPlaying = true;
    }
#else
    Q_UNUSED(cue);
#endif
}

void SoundManager::resumeMusic()
{
#ifdef Q_OS_WIN
    startMusic(g_currentMusic);
#endif
}

void SoundManager::stopMusic()
{
#ifdef Q_OS_WIN
    closeWave(g_musicWave);
    g_musicWave.reset();
#endif
    g_musicPlaying = false;
}

void SoundManager::refreshVolumes()
{
    ensureSettingsLoaded();
#ifdef Q_OS_WIN
    if (g_musicWave && g_musicWave->handle) {
        const DWORD musicVolume = g_musicEnabled
                                      ? volumeWord(true, g_musicWave->gain)
                                      : 0;
        waveOutSetVolume(g_musicWave->handle, musicVolume);
    }
    for (const auto &active : g_activeWaves) {
        if (active && active->handle) {
            const DWORD soundVolume = g_soundEnabled
                                          ? volumeWord(false, active->gain)
                                          : 0;
            waveOutSetVolume(active->handle, soundVolume);
        }
    }
#endif
}

QString SoundManager::statusText()
{
    const std::array<SoundCue, 14> cues = {
        SoundCue::UiClick,
        SoundCue::TerminalBoot,
        SoundCue::Dialogue,
        SoundCue::MissionStart,
        SoundCue::Build,
        SoundCue::Upgrade,
        SoundCue::Freeze,
        SoundCue::BossAlert,
        SoundCue::BossStinger,
        SoundCue::ErrorDeny,
        SoundCue::CoreDamage,
        SoundCue::CodexUnlock,
        SoundCue::Victory,
        SoundCue::Failure
    };

    int found = 0;
    QString firstPath;
    for (SoundCue cue : cues) {
        const QString path = sfxPath(fileForCue(cue));
        if (!path.isEmpty()) {
            ++found;
            if (firstPath.isEmpty()) {
                firstPath = path;
            }
        }
    }

    int musicFound = 0;
    for (MusicCue cue : {MusicCue::Menu, MusicCue::Battle, MusicCue::Story, MusicCue::Boss}) {
        if (!sfxPath(fileForMusic(cue)).isEmpty()) {
            ++musicFound;
        }
    }

    if (found == static_cast<int>(cues.size()) && musicFound == 4) {
        return QStringLiteral("短音效：14/14；背景音乐：4/4\n背景音乐使用独立循环通道，短音效会叠加播放。\n位置：%1").arg(firstPath);
    }
    return QStringLiteral("短音效：%1/14；背景音乐：%2/4\n缺失文件时对应反馈会静音，请确认 assets/sfx 随项目一起存在。")
        .arg(found)
        .arg(musicFound);
}
