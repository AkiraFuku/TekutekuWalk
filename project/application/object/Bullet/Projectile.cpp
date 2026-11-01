#include "Projectile.h"
#include "Enemy.h"
#include "RailMover.h"
#include "RailPath.h"
#include "ModelManager.h"
#include "Collider.h"
#include "Scene.h"
#include "DrawFunction.h"
#include "Physics.h"
#include "JsonManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void Projectile::LoadGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("GameObjectsConfig", filePath)) {
        return;
    }
    const auto& root = jsonMgr->Get("GameObjectsConfig");
    if (!root.contains("projectile")) return;

    const auto& j = root["projectile"];
    s_config.modelPath = j.value("modelPath", s_config.modelPath);
    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() >= 3) {
        s_config.scale.x = j["scale"][0].get<float>();
        s_config.scale.y = j["scale"][1].get<float>();
        s_config.scale.z = j["scale"][2].get<float>();
    }
    s_config.radius = j.value("radius", s_config.radius);
    s_config.lifeTimer = j.value("lifeTimer", s_config.lifeTimer);
    s_config.gravity = j.value("gravity", s_config.gravity);
    s_config.drag = j.value("drag", s_config.drag);
}

void Projectile::SaveGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    nlohmann::json root;
    if (jsonMgr->Load("GameObjectsConfig", filePath)) {
        root = jsonMgr->Get("GameObjectsConfig");
    }

    root["projectile"] = {
        {"modelPath", s_config.modelPath},
        {"scale", {s_config.scale.x, s_config.scale.y, s_config.scale.z}},
        {"radius", s_config.radius},
        {"lifeTimer", s_config.lifeTimer},
        {"gravity", s_config.gravity},
        {"drag", s_config.drag}
    };

    jsonMgr->Save(filePath, root);
}

void Projectile::ImGuiDrawDebugInfo() {
#ifdef USE_IMGUI
    ImGui::Begin("Debug/Projectile");
    ImGui::Text("--- Projectile Global Config ---");

    if (ImGui::Button("💾 Save Projectile Config")) {
        SaveGlobalConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("🔄 Reload")) {
        LoadGlobalConfig();
    }

    ImGui::SliderFloat3("Scale", &s_config.scale.x, 0.05f, 3.0f);
    ImGui::SliderFloat("Radius", &s_config.radius, 0.05f, 3.0f);
    ImGui::SliderInt("Life Timer (frames)", &s_config.lifeTimer, 5, 180);
    ImGui::SliderFloat("Gravity (Bullet Drop)", &s_config.gravity, -0.05f, 0.0f, "%.4f");
    ImGui::SliderFloat("Air Drag", &s_config.drag, 0.90f, 1.0f, "%.3f");

    ImGui::End();
#endif
}

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
    static bool s_loaded = false;
    if (!s_loaded) {
        LoadGlobalConfig();
        s_loaded = true;
    }

    owner_ = owner; // 持ち主を保存
    speed_ = param.speed * param.direction.x;

    // 2. 高度方向の速度（Y方向の入力に基づき設定）
    velocityY_ = param.speed * param.direction.y;

    radius_ = s_config.radius;
    lifeTimer_ = s_config.lifeTimer;

    // モデルの初期化
    object_->Initialize();
    object_->SetModel(s_config.modelPath);
    object_->SetScale(s_config.scale);

    // レール設定
    railMover_->SetPath(path);
    railMover_->SetProgress(param.position.x);
    worldY_ = param.position.y;

    // 初期向きをレール進行方向と仰角に合わせて設定
    Vector3 railDir = railMover_->GetCurrentDirection();
    if (param.direction.x < 0.0f) {
        railDir = Multiply(-1.0f, railDir);
    }
    float initYaw = (Length(railDir) > 0.001f) ? std::atan2(railDir.x, railDir.z) : 0.0f;
    float initPitch = -std::atan2(param.direction.y, std::abs(param.direction.x));
    object_->SetRotate({ initPitch, initYaw, 0.0f });

    // 初期位置を設定
    Vector3 railPos = railMover_->GetCurrentPosition();
    object_->SetTranslate({ railPos.x, worldY_, railPos.z });

    auto collider = std::make_unique<Collider>();
    collider->initialize(this, radius_);
    SetCollider(std::move(collider));
}

void Projectile::Update() {
#ifdef USE_IMGUI
    ImGuiDrawDebugInfo();
#endif

    if (isDead_) return;

    Vector3 prevPos = GetWorldPosition();

    // 1. 空気抵抗によるわずかな減速と、重力による弾道落下（Bullet Drop）
    speed_ *= s_config.drag;
    velocityY_ += s_config.gravity;

    // 2. レール上の位置と高度を更新
    railMover_->Advance(speed_);
    worldY_ += velocityY_;

    // 非ループのレール端（始点0.0 / 終点 GetMaxT()）に到達したら即座に消滅
    const RailPath* path = railMover_->GetRailPath();
    if (path && !path->IsLoop()) {
        float prog = railMover_->GetProgress();
        float maxT = path->GetMaxT();
        if (maxT > 0.0f && (prog <= 0.0005f || prog >= maxT - 0.0005f)) {
            isDead_ = true;
            return;
        }
    }

    // 3. 座標の合成
    Vector3 railPos = railMover_->GetCurrentPosition();
    Vector3 finalPos = { railPos.x, worldY_, railPos.z }; // 更新された worldY_ を使う

    // 4. 壁・地形との衝突判定（めり込む前に判定し、衝突した場合は潜った座標に更新せず消滅）
    if (CheckMapCollision(prevPos, finalPos)) {
        isDead_ = true;
        return;
    }

    // 5. 3D移動ベクトルから弾頭の向き（Yaw / Pitch）を算出して飛翔方向へ真っ直ぐ向ける
    Vector3 moveVec = Subtract(finalPos, prevPos);
    if (Length(moveVec) > 0.0001f) {
        float yaw = std::atan2(moveVec.x, moveVec.z);
        float horizontalDist = std::sqrt(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
        float pitch = -std::atan2(moveVec.y, (std::max)(horizontalDist, 0.0001f));
        object_->SetRotate({ pitch, yaw, 0.0f });
    }

    // 6. 消滅直前（残り6フレーム）は曳光剤が燃え尽きるように弾体を細く収束させる
    constexpr int kFadeFrames = 6;
    if (lifeTimer_ <= kFadeFrames) {
        float fadeRatio = static_cast<float>((std::max)(lifeTimer_, 0)) / static_cast<float>(kFadeFrames);
        object_->SetScale({
            s_config.scale.x * fadeRatio,
            s_config.scale.y * fadeRatio,
            s_config.scale.z
        });
    } else {
        object_->SetScale(s_config.scale);
    }

    object_->SetTranslate(finalPos);

    object_->Update();
    if (auto* col = GetCollider()) {
        col->Update();
    }

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

void Projectile::OnCollision(GameObject* other) {
    other; // 使わない場合は警告回避のために記述
    isDead_ = true; // 敵に当たったら1回で確実に消滅（貫通しない）
}

bool Projectile::CheckMapCollision(const Vector3& prevPos, const Vector3& finalPos) {
    if (!scene_ || isDead_) return false;

    const auto& triangles = scene_->GetTriangle();
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
