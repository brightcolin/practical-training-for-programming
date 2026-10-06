#ifndef ENTITYRULES_H
#define ENTITYRULES_H

#include "gametypes.h"

#include <QColor>
#include <QString>

#include <array>
#include <vector>

class TowerRule
{
public:
    virtual ~TowerRule() = default;

    virtual TowerKind kind() const = 0;
    virtual QString name() const = 0;
    virtual QString shortName() const = 0;
    virtual QColor color() const = 0;

    virtual Tower create(const QPoint &cell, const TowerSpec &spec) const;
    virtual int upgradeCost(const Tower &tower) const;
    virtual void applyUpgrade(Tower &tower) const = 0;
};

class EnemyRule
{
public:
    virtual ~EnemyRule() = default;

    virtual EnemyKind kind() const = 0;
    virtual QString name() const = 0;
    virtual QColor color() const = 0;
    virtual void configure(Enemy &enemy) const = 0;

    virtual double incomingDamageMultiplier() const;
    virtual void adjustSlow(double &seconds, double &factor) const;
    virtual double wallAttackDamage() const;
    virtual bool shouldSplitOnDeath(const Enemy &enemy) const;
};

struct SplitEnemySpec {
    EnemyKind kind = EnemyKind::Fast;
    double hp = 45.0;
    double baseSpeed = 96.0;
    double progressOffset = 0.0;
};

class EntityRules
{
public:
    static const TowerRule &tower(TowerKind kind);
    static const EnemyRule &enemy(EnemyKind kind);
    static const std::array<TowerKind, 6> &towerBuildOrder();
    static std::vector<SplitEnemySpec> splitChildren(const Enemy &enemy);
};

#endif
