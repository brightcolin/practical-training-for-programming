#ifndef GAMETYPES_H
#define GAMETYPES_H

#include <QPoint>
#include <QPointF>
#include <QString>

enum class TileKind {
    Stable,
    Path,
    Discount,
    Anchor,
    Accelerate,
    Portal
};

enum class TowerKind {
    Shooter,
    Slow,
    Splash,
    Laser,
    Resource,
    Wall
};

enum class EnemyKind {
    Normal,
    Fast,
    Armored,
    Resistant,
    Splitter,
    Boss
};

enum class ProjectileKind {
    Bolt,
    Slow,
    Splash
};

struct WaveEntry {
    EnemyKind kind = EnemyKind::Normal;
    int count = 0;
    double interval = 1.0;
};

struct TowerSpec {
    int cost = 60;
    double range = 150.0;
    double damage = 28.0;
    double cooldown = 0.65;
    double hp = 100.0;
};

struct Enemy {
    int id = 0;
    EnemyKind kind = EnemyKind::Normal;
    double hp = 100.0;
    double maxHp = 100.0;
    double baseSpeed = 70.0;
    double progress = 0.0;
    double attackCooldown = 0.0;
    double portalCooldown = 0.0;
    double slowTime = 0.0;
    double slowFactor = 1.0;
    double burnTime = 0.0;
    double burnDps = 0.0;
    double shield = 0.0;
    double hitFlash = 0.0;
    double abilityTimer = 0.0;
    int phase = 0;
    bool reachedCore = false;
    bool splitCreated = false;
    bool dead() const { return hp <= 0.0; }
};

struct Tower {
    TowerKind kind = TowerKind::Shooter;
    QPoint cell;
    int level = 1;
    double range = 150.0;
    double damage = 28.0;
    double cooldown = 0.65;
    double cooldownLeft = 0.0;
    double hp = 100.0;
    double maxHp = 100.0;
    double resourceTimer = 0.0;
};

struct Projectile {
    ProjectileKind kind = ProjectileKind::Bolt;
    QPointF pos;
    int targetId = -1;
    double speed = 430.0;
    double damage = 28.0;
    bool expired = false;
};

struct GameSummary {
    QString levelName;
    int levelIndex = 0;
    bool reverseMode = false;
    bool victory = false;
    int energy = 0;
    int towersBuilt = 0;
    int enemiesDefeated = 0;
    int wavesCleared = 0;
    int totalWaves = 0;
    double elapsedSeconds = 0.0;
};

#endif
