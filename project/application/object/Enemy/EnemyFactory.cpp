#include "EnemyFactory.h"
#include "TestEnemy.h"
#include "BoundEnemy.h"
#include "ChaseEnemy.h"
#include "HoverEnemy.h"
#include "ShieldEnemy.h"

std::unique_ptr<Enemy> EnemyFactory::Create(Enemy::EnemyType type) {
    switch (type) {
    case Enemy::EnemyType::Normal:
        return std::make_unique<TestEnemy>();
    case Enemy::EnemyType::Bound:
        return std::make_unique<BoundEnemy>();
    case Enemy::EnemyType::Chase:
        return std::make_unique<ChaseEnemy>();
    case Enemy::EnemyType::Hover:
        return std::make_unique<HoverEnemy>();
    case Enemy::EnemyType::Shield:
        return std::make_unique<ShieldEnemy>();
    default:
        return std::make_unique<TestEnemy>();
    }
}
