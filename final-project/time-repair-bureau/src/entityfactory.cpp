#include "entityfactory.h"

#include "entityrules.h"

Tower EntityFactory::createTower(TowerKind kind, const QPoint &cell, const TowerSpec &spec)
{
    return EntityRules::tower(kind).create(cell, spec);
}

int EntityFactory::upgradeCost(const Tower &tower)
{
    return EntityRules::tower(tower.kind).upgradeCost(tower);
}

void EntityFactory::applyUpgrade(Tower &tower)
{
    ++tower.level;
    tower.maxHp *= 1.18;
    tower.hp = tower.maxHp;
    EntityRules::tower(tower.kind).applyUpgrade(tower);
}

std::unique_ptr<Enemy> EntityFactory::createEnemy(EnemyKind kind, int id)
{
    auto enemy = std::make_unique<Enemy>();
    enemy->id = id;
    enemy->kind = kind;
    EntityRules::enemy(kind).configure(*enemy);
    enemy->maxHp = enemy->hp;
    return enemy;
}
