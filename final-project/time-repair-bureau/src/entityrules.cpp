#include "entityrules.h"

#include <algorithm>

Tower TowerRule::create(const QPoint &cell, const TowerSpec &spec) const
{
    Tower tower;
    tower.kind = kind();
    tower.cell = cell;
    tower.range = spec.range;
    tower.damage = spec.damage;
    tower.cooldown = spec.cooldown;
    tower.maxHp = spec.hp;
    tower.hp = tower.maxHp;
    return tower;
}

int TowerRule::upgradeCost(const Tower &tower) const
{
    if (tower.level >= 3) {
        return 0;
    }
    return 55 + tower.level * 45 + static_cast<int>(tower.kind) * 8;
}

double EnemyRule::incomingDamageMultiplier() const
{
    return 1.0;
}

void EnemyRule::adjustSlow(double &, double &) const
{
}

double EnemyRule::wallAttackDamage() const
{
    return 16.0;
}

bool EnemyRule::shouldSplitOnDeath(const Enemy &) const
{
    return false;
}

namespace {
class ShooterTowerRule final : public TowerRule
{
public:
    TowerKind kind() const override { return TowerKind::Shooter; }
    QString name() const override { return QStringLiteral("指针炮台"); }
    QString shortName() const override { return QStringLiteral("1 指针炮台"); }
    QColor color() const override { return QColor(45, 130, 170); }
    void applyUpgrade(Tower &tower) const override
    {
        tower.damage *= 1.35;
        tower.cooldown *= 0.88;
        tower.range += 14.0;
    }
};

class SlowTowerRule final : public TowerRule
{
public:
    TowerKind kind() const override { return TowerKind::Slow; }
    QString name() const override { return QStringLiteral("凝滞棱镜"); }
    QString shortName() const override { return QStringLiteral("3 凝滞棱镜"); }
    QColor color() const override { return QColor(70, 160, 220); }
    void applyUpgrade(Tower &tower) const override
    {
        tower.cooldown *= 0.82;
        tower.range += 18.0;
    }
};

class SplashTowerRule final : public TowerRule
{
public:
    TowerKind kind() const override { return TowerKind::Splash; }
    QString name() const override { return QStringLiteral("悖论震荡器"); }
    QString shortName() const override { return QStringLiteral("4 悖论震荡"); }
    QColor color() const override { return QColor(148, 88, 210); }
    void applyUpgrade(Tower &tower) const override
    {
        tower.damage *= 1.25;
        tower.range += 12.0;
    }
};

class LaserTowerRule final : public TowerRule
{
public:
    TowerKind kind() const override { return TowerKind::Laser; }
    QString name() const override { return QStringLiteral("因果切割器"); }
    QString shortName() const override { return QStringLiteral("5 因果切割"); }
    QColor color() const override { return QColor(240, 196, 82); }
    void applyUpgrade(Tower &tower) const override
    {
        tower.damage *= 1.32;
        tower.cooldown *= 0.9;
    }
};

class ResourceTowerRule final : public TowerRule
{
public:
    TowerKind kind() const override { return TowerKind::Resource; }
    QString name() const override { return QStringLiteral("时能汲取仪"); }
    QString shortName() const override { return QStringLiteral("2 时能汲取"); }
    QColor color() const override { return QColor(64, 184, 132); }
    void applyUpgrade(Tower &tower) const override
    {
        tower.resourceTimer = std::min(tower.resourceTimer + 1.5, 2.8);
    }
};

class WallTowerRule final : public TowerRule
{
public:
    TowerKind kind() const override { return TowerKind::Wall; }
    QString name() const override { return QStringLiteral("锚点屏障"); }
    QString shortName() const override { return QStringLiteral("6 锚点屏障"); }
    QColor color() const override { return QColor(118, 128, 145); }
    void applyUpgrade(Tower &tower) const override
    {
        tower.maxHp *= 1.35;
        tower.hp = tower.maxHp;
    }
};

class NormalEnemyRule final : public EnemyRule
{
public:
    EnemyKind kind() const override { return EnemyKind::Normal; }
    QString name() const override { return QStringLiteral("裂隙残影"); }
    QColor color() const override { return QColor(130, 82, 255); }
    void configure(Enemy &enemy) const override
    {
        enemy.hp = 100.0;
        enemy.baseSpeed = 68.0;
    }
};

class FastEnemyRule final : public EnemyRule
{
public:
    EnemyKind kind() const override { return EnemyKind::Fast; }
    QString name() const override { return QStringLiteral("加速残影"); }
    QColor color() const override { return QColor(82, 218, 255); }
    void configure(Enemy &enemy) const override
    {
        enemy.hp = 65.0;
        enemy.baseSpeed = 112.0;
    }
};

class ArmoredEnemyRule final : public EnemyRule
{
public:
    EnemyKind kind() const override { return EnemyKind::Armored; }
    QString name() const override { return QStringLiteral("固化悖论"); }
    QColor color() const override { return QColor(190, 124, 92); }
    void configure(Enemy &enemy) const override
    {
        enemy.hp = 220.0;
        enemy.baseSpeed = 44.0;
    }
};

class ResistantEnemyRule final : public EnemyRule
{
public:
    EnemyKind kind() const override { return EnemyKind::Resistant; }
    QString name() const override { return QStringLiteral("抗滞异常体"); }
    QColor color() const override { return QColor(210, 218, 232); }
    void configure(Enemy &enemy) const override
    {
        enemy.hp = 150.0;
        enemy.baseSpeed = 62.0;
    }
    double incomingDamageMultiplier() const override
    {
        return 0.78;
    }
    void adjustSlow(double &seconds, double &factor) const override
    {
        factor = 0.72;
        seconds *= 0.65;
    }
};

class SplitterEnemyRule final : public EnemyRule
{
public:
    EnemyKind kind() const override { return EnemyKind::Splitter; }
    QString name() const override { return QStringLiteral("分叉时间体"); }
    QColor color() const override { return QColor(205, 104, 220); }
    void configure(Enemy &enemy) const override
    {
        enemy.hp = 135.0;
        enemy.baseSpeed = 58.0;
    }
    bool shouldSplitOnDeath(const Enemy &enemy) const override
    {
        return !enemy.splitCreated;
    }
};

class BossEnemyRule final : public EnemyRule
{
public:
    EnemyKind kind() const override { return EnemyKind::Boss; }
    QString name() const override { return QStringLiteral("纪元崩坏体"); }
    QColor color() const override { return QColor(255, 76, 132); }
    void configure(Enemy &enemy) const override
    {
        enemy.hp = 760.0;
        enemy.baseSpeed = 34.0;
        enemy.shield = 160.0;
    }
    double wallAttackDamage() const override
    {
        return 32.0;
    }
};

const ShooterTowerRule kShooterTowerRule;
const SlowTowerRule kSlowTowerRule;
const SplashTowerRule kSplashTowerRule;
const LaserTowerRule kLaserTowerRule;
const ResourceTowerRule kResourceTowerRule;
const WallTowerRule kWallTowerRule;

const NormalEnemyRule kNormalEnemyRule;
const FastEnemyRule kFastEnemyRule;
const ArmoredEnemyRule kArmoredEnemyRule;
const ResistantEnemyRule kResistantEnemyRule;
const SplitterEnemyRule kSplitterEnemyRule;
const BossEnemyRule kBossEnemyRule;
}

const TowerRule &EntityRules::tower(TowerKind kind)
{
    switch (kind) {
    case TowerKind::Shooter: return kShooterTowerRule;
    case TowerKind::Slow: return kSlowTowerRule;
    case TowerKind::Splash: return kSplashTowerRule;
    case TowerKind::Laser: return kLaserTowerRule;
    case TowerKind::Resource: return kResourceTowerRule;
    case TowerKind::Wall: return kWallTowerRule;
    }
    return kShooterTowerRule;
}

const EnemyRule &EntityRules::enemy(EnemyKind kind)
{
    switch (kind) {
    case EnemyKind::Normal: return kNormalEnemyRule;
    case EnemyKind::Fast: return kFastEnemyRule;
    case EnemyKind::Armored: return kArmoredEnemyRule;
    case EnemyKind::Resistant: return kResistantEnemyRule;
    case EnemyKind::Splitter: return kSplitterEnemyRule;
    case EnemyKind::Boss: return kBossEnemyRule;
    }
    return kNormalEnemyRule;
}

const std::array<TowerKind, 6> &EntityRules::towerBuildOrder()
{
    static const std::array<TowerKind, 6> order = {
        TowerKind::Shooter,
        TowerKind::Resource,
        TowerKind::Slow,
        TowerKind::Splash,
        TowerKind::Laser,
        TowerKind::Wall
    };
    return order;
}

std::vector<SplitEnemySpec> EntityRules::splitChildren(const Enemy &enemy)
{
    if (!EntityRules::enemy(enemy.kind).shouldSplitOnDeath(enemy)) {
        return {};
    }

    return {
        SplitEnemySpec{EnemyKind::Fast, 45.0, 96.0, 0.0},
        SplitEnemySpec{EnemyKind::Fast, 45.0, 96.0, -18.0}
    };
}
