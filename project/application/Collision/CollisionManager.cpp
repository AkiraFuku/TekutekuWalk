#include "CollisionManager.h"
#include "Collider.h"
#include "Physics.h"
#include "MathFunction.h" // SubtractやLengthSqなどの数学関数用
#include <cmath>
#include <algorithm>

std::unique_ptr<CollisionManager> CollisionManager::instance = nullptr;

CollisionManager::CollisionManager() {
    InitializeDefaultMatrix();
}

CollisionManager* CollisionManager::GetInstance() {
    if (instance == nullptr) {
        struct Helper : public CollisionManager {
            Helper() : CollisionManager() {}
        };
        instance = std::make_unique<Helper>();
    }
    return instance.get();
}

void CollisionManager::Finalize() {
    instance.reset();
}

void CollisionManager::InitializeDefaultMatrix() {
    responseMatrix_.clear();

    // ─── 重なる（Overlap: イベント検知・通過） ───
    // プレイヤー攻撃 × 敵
    SetCollisionResponse(CollisionCategory::PlayerAttack, CollisionCategory::Enemy, CollisionResponseType::Overlap);
    // プレイヤー攻撃 × 攻撃可能オブジェクト/弱点
    SetCollisionResponse(CollisionCategory::PlayerAttack, CollisionCategory::Attackable, CollisionResponseType::Overlap);
    // プレイヤー攻撃 × 攻撃不可エネミー
    SetCollisionResponse(CollisionCategory::PlayerAttack, CollisionCategory::InvincibleEnemy, CollisionResponseType::Overlap);

    // プレイヤー × 敵
    SetCollisionResponse(CollisionCategory::Player, CollisionCategory::Enemy, CollisionResponseType::Overlap);
    // プレイヤー × 収集アイテム
    SetCollisionResponse(CollisionCategory::Player, CollisionCategory::Collectible, CollisionResponseType::Overlap);
    // プレイヤー × 攻撃不可エネミー
    SetCollisionResponse(CollisionCategory::Player, CollisionCategory::InvincibleEnemy, CollisionResponseType::Overlap);
    // プレイヤー × 攻撃可能オブジェクト/弱点
    SetCollisionResponse(CollisionCategory::Player, CollisionCategory::Attackable, CollisionResponseType::Overlap);
    // プレイヤー × 敵の弾
    SetCollisionResponse(CollisionCategory::Player, CollisionCategory::EnemyProjectile, CollisionResponseType::Overlap);
    // プレイヤー × ゴール
    SetCollisionResponse(CollisionCategory::Player, CollisionCategory::Goal, CollisionResponseType::Overlap);

    // プレイヤー弾 × 敵
    SetCollisionResponse(CollisionCategory::Enemy, CollisionCategory::PlayerProjectile, CollisionResponseType::Overlap);
    // プレイヤー弾 × 攻撃可能オブジェクト/弱点
    SetCollisionResponse(CollisionCategory::PlayerProjectile, CollisionCategory::Attackable, CollisionResponseType::Overlap);

    // ─── 重ならない（Block: めり込みを押し出して位置補正） ───
    // プレイヤー × めり込めないオブジェクト (CollisionObject)
    SetCollisionResponse(CollisionCategory::Player, CollisionCategory::CollisionObject, CollisionResponseType::Block);
    // 敵 × めり込めないオブジェクト (CollisionObject)
    SetCollisionResponse(CollisionCategory::Enemy, CollisionCategory::CollisionObject, CollisionResponseType::Block);
}

CollisionResponseType CollisionManager::GetCollisionResponse(CollisionCategory a, CollisionCategory b) const {
    if (a == b) return CollisionResponseType::Ignore;
    int catA = static_cast<int>(a);
    int catB = static_cast<int>(b);
    auto key = std::make_pair((std::min)(catA, catB), (std::max)(catA, catB));
    auto it = responseMatrix_.find(key);
    if (it != responseMatrix_.end()) {
        return it->second;
    }
    return CollisionResponseType::Ignore;
}

void CollisionManager::SetCollisionResponse(CollisionCategory a, CollisionCategory b, CollisionResponseType response) {
    int catA = static_cast<int>(a);
    int catB = static_cast<int>(b);
    auto key = std::make_pair((std::min)(catA, catB), (std::max)(catA, catB));
    if (response == CollisionResponseType::Ignore) {
        responseMatrix_.erase(key);
    } else {
        responseMatrix_[key] = response;
    }
}

void CollisionManager::CheckAllCollisions(const std::vector<Collider*>& colliders) {
    size_t count = colliders.size();
    if (count < 2) return;

    for (size_t i = 0; i < count; ++i) {
        Collider* colA = colliders[i];
        if (!colA) continue;

        for (size_t j = i + 1; j < count; ++j) {
            Collider* colB = colliders[j];
            if (!colB) continue;

            // 1. カテゴリチェックで不要なペアを即座にスキップ（最優先で高速化）
            if (!ShouldCheckCollision(colA->GetCategory(), colB->GetCategory())) {
                continue;
            }

            // 2. 距離によるブロードフェーズ除外
            Vector3 posA = colA->GetWorldPosition();
            Vector3 posB = colB->GetWorldPosition();
            Vector3 diff = Subtract(posA, posB);
            float distSq = Dot(diff, diff);

            if (distSq > kBroadPhaseMaxDistanceSq) {
                continue;
            }

            // 3. 優先度の取得と並び替え
            int priorityA = GetCategoryPriority(colA->GetCategory());
            int priorityB = GetCategoryPriority(colB->GetCategory());

            Collider* first = colA;
            Collider* second = colB;

            if (priorityB > priorityA) {
                first = colB;
                second = colA;
            }

            // 4. 精密判定とイベント呼び出し
            CheckCollision(first, second);
        }
    }
}

int CollisionManager::GetCategoryPriority(CollisionCategory category)
{
    switch (category) {
    case CollisionCategory::Collectible:      return 120; // 収集アイテム最優先
    case CollisionCategory::PlayerAttack:     return 110; // プレイヤー攻撃最優先（エネミーより先に対処し相打ち防止）
    case CollisionCategory::CollisionObject:  return 100; // めり込めないオブジェクト最優先
    case CollisionCategory::Attackable:       return 100; // 攻撃可能オブジェクト最優先
    case CollisionCategory::InvincibleEnemy:  return 100; // 攻撃（無敵）不可エネミー最優先
    case CollisionCategory::Enemy:            return 100; // エネミー最優先
    case CollisionCategory::Player:           return 80;  // プレイヤー
    case CollisionCategory::PlayerProjectile: return 50;  // プレイヤーの弾
    case CollisionCategory::EnemyProjectile:  return 50;  // 敵の弾
    case CollisionCategory::Goal:             return 10;  // ゴール
    default:                                  return 0;
    }
}

void CollisionManager::CheckCollision(Collider* a, Collider* b) {
    if ((!a->IsCollide()) || (!b->IsCollide()))
    {
        return; // どちらかが衝突不可なら判定しない
    }

    CollisionResponseType response = GetCollisionResponse(a->GetCategory(), b->GetCategory());
    if (response == CollisionResponseType::Ignore) {
        return;
    }

    auto spheresA = a->GetWorldSpheres();
    auto spheresB = b->GetWorldSpheres();

    bool isHit = false;
    Vector3 totalPushA = { 0.0f, 0.0f, 0.0f };
    Vector3 totalPushB = { 0.0f, 0.0f, 0.0f };

    for (const auto& sa : spheresA) {
        for (const auto& sb : spheresB) {
            // 統一された Physics::Intersect (2乗距離・平方根なし) を使用
            if (Physics::Intersect(sa, sb)) {
                isHit = true;

                // 重ならない設定（Block）の場合はめり込み押し出しベクトルを計算
                if (response == CollisionResponseType::Block) {
                    Vector3 diff = Subtract(sa.center, sb.center);
                    float dist = Length(diff);
                    float rSum = sa.radius + sb.radius;
                    float penetration = rSum - dist;

                    if (penetration > 0.0f) {
                        Vector3 pushDir;
                        if (dist > 0.0001f) {
                            pushDir = Multiply(1.0f / dist, diff);
                        } else {
                            pushDir = { 0.0f, 1.0f, 0.0f }; // 中心が完全一致した場合は真上に押し戻す
                        }

                        // 静的オブジェクト（めり込めない障害物など）は動かさず、動的オブジェクトのみを押し出す
                        bool aIsStatic = (a->GetCategory() == CollisionCategory::CollisionObject);
                        bool bIsStatic = (b->GetCategory() == CollisionCategory::CollisionObject);

                        if (aIsStatic && !bIsStatic) {
                            totalPushB = Add(totalPushB, Multiply(-penetration, pushDir));
                        } else if (!aIsStatic && bIsStatic) {
                            totalPushA = Add(totalPushA, Multiply(penetration, pushDir));
                        } else {
                            totalPushA = Add(totalPushA, Multiply(penetration * 0.5f, pushDir));
                            totalPushB = Add(totalPushB, Multiply(-penetration * 0.5f, pushDir));
                        }
                    }
                }
            }
        }
    }

    if (isHit) {
        // 重ならない設定（Block）の場合、オーナーの位置を補正してスフィアをめり込ませない
        if (response == CollisionResponseType::Block) {
            if (a->GetOwner() && Dot(totalPushA, totalPushA) > 1e-6f) {
                a->GetOwner()->AddPosition(totalPushA);
                a->Update();
            }
            if (b->GetOwner() && Dot(totalPushB, totalPushB) > 1e-6f) {
                b->GetOwner()->AddPosition(totalPushB);
                b->Update();
            }
        }

        // お互いに「相手」を渡して通知する
        a->OnCollision(b);
        b->OnCollision(a);
    }
}

bool CollisionManager::ShouldCheckCollision(CollisionCategory catA, CollisionCategory catB) const {
    return GetCollisionResponse(catA, catB) != CollisionResponseType::Ignore;
}