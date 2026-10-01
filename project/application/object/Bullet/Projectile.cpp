#include "Projectile.h"
#include "Enemy.h"
#include "RailMover.h"
#include "ModelManager.h"
#include "Collider.h"
#include "GameScene.h"
#include "DrawFunction.h"
#include "Physics.h"

Projectile::Projectile() {
    railMover_ = std::make_unique<RailMover>();
    object_ = std::make_unique<Object3d>();
   
}
CollisionCategory Projectile::GetCategory() const {
    switch (owner_) {
    case ProjectileOwner::Player:
        return CollisionCategory::PlayerProjectile;
    case ProjectileOwner::Enemy:
        return CollisionCategory::EnemyProjectile;
    default:
        return CollisionCategory::PlayerProjectile; // デフォルトはプレイヤー弾とする
    }
}
Projectile::~Projectile() = default;

void Projectile::Initialize(const RailPath* path, const ProjectileSpawnParam& param, ProjectileOwner owner) {
    owner_ = owner; // 持ち主を保存
    speed_ = param.speed * param.direction.x;

    // 2. 高度方向の速度（Y方向の入力に基づき設定）
    // param.direction.y は入力の上下 (-1.0 ~ 1.0)
    velocityY_ = param.speed * param.direction.y;


    // モデルの初期化（例としてSphereを使用）
    object_->Initialize();
    object_->SetModel("playerCursor.obj"); // 必要に応じて専用モデルへ
    object_->SetScale({ 0.5f, 0.5f, 0.5f });
    //モデルの向きを進行方向に合わせるための回転を設定
    Vector3 dir3D = { param.direction.x, 0.0f, param.direction.y };
    object_->SetRotate(dir3D);


    // レール設定
    railMover_->SetPath(path);
    railMover_->SetProgress(param.position.x);
    worldY_ = param.position.y;

    // 初期位置を設定
    Vector3 railPos = railMover_->GetCurrentPosition();
    object_->SetTranslate({ railPos.x, worldY_, railPos.z });

    collider_ = std::make_unique<Collider>();
    collider_->initialize(this, radius_);

}

void Projectile::Update() {
    if (isDead_) return;

    Vector3 prevPos = GetWorldPosition();

    // レール上の位置を更新
    railMover_->Advance(speed_);

    // 【追加】高度を更新
    worldY_ += velocityY_;

    // 座標の合成
    Vector3 railPos = railMover_->GetCurrentPosition();
    Vector3 finalPos = { railPos.x, worldY_, railPos.z }; // 更新された worldY_ を使う

    // 壁・地形との衝突判定（めり込む前に判定し、衝突した場合は潜った座標に更新せず消滅）
    if (CheckMapCollision(prevPos, finalPos)) {
        isDead_ = true;
        return;
    }

    object_->SetTranslate(finalPos);

    object_->Update();
    collider_->Update();

    if (--lifeTimer_ <= 0) {
        isDead_ = true;
    }
}
void Projectile::Draw() {
    if (isDead_) return;
    object_->Draw();
}

Vector3 Projectile::GetWorldPosition() const {
    return object_ ? object_->GetTranslate() : Vector3{ 0.0f, 0.0f, 0.0f };
}

void Projectile::OnCollision( GameObject* other) {
    other; // 使わない場合は警告回避のために記述
    isDead_ = true; // 敵に当たったら1回で確実に消滅（貫通しない）
}

bool Projectile::CheckMapCollision(const Vector3& prevPos, const Vector3& finalPos) {
    if (!scene_ || isDead_) return false;
    auto gs = dynamic_cast<GameScene*>(scene_);
    if (!gs) return false;

    const auto& triangles = gs->GetTriangle();
    if (triangles.empty()) return false;

    Vector3 moveVec = Subtract(finalPos, prevPos);
    float moveDist = Length(moveVec);
    if (moveDist <= 0.0001f) return false;

    // 移動線分レイ（進行方向に半径分少し伸ばすことで、めり込む前に衝突判定を取る）
    Vector3 moveDir = Normalize(moveVec);
    Ray ray;
    ray.origin = prevPos;
    ray.diff = Multiply(moveDist + radius_, moveDir);

    // 現在位置での球体データ（めり込み判定用）
    Sphere sphere = { finalPos, radius_ };

    for (const auto& tri : triangles) {
        // すり抜け足場(isOneway)は弾が貫通
        if (tri.isOneway) {
            continue;
        }

        // 1. 移動線分レイによる交差判定
        float dist = 0.0f;
        Vector3 hitPoint = {};
        RayTriangleCollisionResult result;
        if (CheckRayTriangle(ray, tri, &dist, &hitPoint, &result)) {
            if (result == RayTriangleCollisionResult::FrontFace || result == RayTriangleCollisionResult::BackFace) {
                // レイの有効範囲内でヒットしたら消滅
                if (dist >= 0.0f && dist <= 1.0f) {
                    return true;
                }
            }
        }

        // 2. 球体 vs 三角形の近接・交差判定（球の下面や側面が地面に接触した場合の検知）
        Vector3 closestPt = {};
        if (Physics::Intersect(sphere, tri, &closestPt)) {
            return true;
        }
    }
    return false;
}