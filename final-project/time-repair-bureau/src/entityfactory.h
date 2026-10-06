#ifndef ENTITYFACTORY_H
#define ENTITYFACTORY_H

#include "gametypes.h"

#include <memory>

class EntityFactory
{
public:
    static Tower createTower(TowerKind kind, const QPoint &cell, const TowerSpec &spec);
    static int upgradeCost(const Tower &tower);
    static void applyUpgrade(Tower &tower);
    static std::unique_ptr<Enemy> createEnemy(EnemyKind kind, int id);
};

#endif

