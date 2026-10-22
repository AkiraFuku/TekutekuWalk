#include "Enemy.h"
#include "Camera.h"
#include "Object3D.h"
#include "ModelManager.h"
#include "RailMover.h"
#include "RailPath.h"
#include <cmath>
#include "enemyBehavior.h"
#include "EnemyAction.h"
#include "EnemyState.h"
#include "Player.h"
#include "imgui.h"
#include "Robot.h"
#include "ParticleEmitter.h"
#include "GameScene.h"
#include "CameraController.h"
#include "PrimitiveDrawer.h"
#include "Collider.h"
#include "JsonManager.h"
#include "Logger.h"

Enemy::Enemy() = default;
Enemy::~Enemy() = default;

void Enemy::SetRobot(std::unique_ptr<Robot> robot) {
    robot_ = std::move(robot);
}

void Enemy::LoadConfig(const std::string& filePath)
{
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("EnemyConfig", filePath)) {
        Logger::Log("[Enemy] Could not open " + filePath + ". Using default config.\n");
        return;
    }

    const auto& j = jsonMgr->Get("EnemyConfig");

    // Paths
    if (j.contains("paths")) {
        const auto& jp = j["paths"];
        config_.paths.modelDir = jp.value("modelDir", config_.paths.modelDir);
        config_.paths.modelFile = jp.value("modelFile", config_.paths.modelFile);
        config_.paths.texture = jp.value("texture", config_.paths.texture);
    }

    // Transform
    if (j.contains("transform")) {
        const auto& jt = j["transform"];
        if (jt.contains("scale") && jt["scale"].is_array() && jt["scale"].size() >= 3) {
            config_.transform.scale = {
                jt["scale"][0].get<float>(),
                jt["scale"][1].get<float>(),
                jt["scale"][2].get<float>()
            };
        }
        if (jt.contains("rotate") && jt["rotate"].is_array() && jt["rotate"].size() >= 3) {
            config_.transform.rotate = {
                jt["rotate"][0].get<float>(),
                jt["rotate"][1].get<float>(),
                jt["rotate"][2].get<float>()
            };
        }
    }

    // Movement
    if (j.contains("movement")) {
        const auto& jm = j["movement"];
        config_.movement.moveSpeed = jm.value("moveSpeed", config_.movement.moveSpeed);
        config_.movement.gravity = jm.value("gravity", config_.movement.gravity);
        config_.movement.gravityScale = jm.value("gravityScale", config_.movement.gravityScale);
        config_.movement.turnCooldown = jm.value("turnCooldown", config_.movement.turnCooldown);
        config_.movement.heightOffset = jm.value("heightOffset", config_.movement.heightOffset);
    }

    // Combat
    if (j.contains("combat")) {
        const auto& jc = j["combat"];
        config_.combat.hitInvincibilityDuration = jc.value("hitInvincibilityDuration", config_.combat.hitInvincibilityDuration);
        config_.combat.knockbackDuration = jc.value("knockbackDuration", config_.combat.knockbackDuration);
        config_.combat.knockbackSpeed = jc.value("knockbackSpeed", config_.combat.knockbackSpeed);
        config_.combat.knockbackJumpForce = jc.value("knockbackJumpForce", config_.combat.knockbackJumpForce);
    }

    // Collider
    if (j.contains("collider")) {
        const auto& jc = j["collider"];
        config_.collider.radius = jc.value("radius", config_.collider.radius);
    }
}

void Enemy::SaveConfig(const std::string& filePath)
{
    nlohmann::json j;

    j["paths"] = {
        { "modelDir", config_.paths.modelDir },
        { "modelFile", config_.paths.modelFile },
        { "texture", config_.paths.texture }
    };

    j["transform"] = {
        { "scale", { config_.transform.scale.x, config_.transform.scale.y, config_.transform.scale.z } },
        { "rotate", { config_.transform.rotate.x, config_.transform.rotate.y, config_.transform.rotate.z } }
    };

    j["movement"] = {
        { "moveSpeed", config_.movement.moveSpeed },
        { "gravity", config_.movement.gravity },
        { "gravityScale", config_.movement.gravityScale },
        { "turnCooldown", config_.movement.turnCooldown },
        { "heightOffset", config_.movement.heightOffset }
    };

    j["combat"] = {
        { "hitInvincibilityDuration", config_.combat.hitInvincibilityDuration },
        { "knockbackDuration", config_.combat.knockbackDuration },
        { "knockbackSpeed", config_.combat.knockbackSpeed },
        { "knockbackJumpForce", config_.combat.knockbackJumpForce }
    };

    j["collider"] = {
        { "radius", config_.collider.radius }
    };

    JsonManager::GetInstance()->Save(filePath, j);
}

void Enemy::ApplyColliderConfig()
{
    radius_ = config_.collider.radius;
    if (auto* col = GetCollider()) {
        col->initialize(this, radius_);
    }
}

void Enemy::Initialize()
{
    // JSON設定の読み込み
    LoadConfig();

    object_ = std::make_unique<Object3d>();
    ModelManager::GetInstance()->CreateSphereModel("Enemy");
    ModelManager::GetInstance()->LoadModel(config_.paths.modelDir, config_.paths.modelFile);

    object_->Initialize();
    object_->SetModel(config_.paths.modelFile);
    object_->SetTexture(config_.paths.texture);
    object_->SetScale(config_.transform.scale);
    object_->SetRotate(config_.transform.rotate);

    moveSpeed_ = config_.movement.moveSpeed;
    gravityScale_ = config_.movement.gravityScale;
    radius_ = config_.collider.radius;

    railMover_ = std::make_unique<RailMover>();
    ChangeBehavior(std::make_unique<EnemyBehaviorPatrol>());
    ChangeState(std::make_unique<StateEnemyNormal>());
    hitParticle_ = std::make_unique<ParticleEmitter>("GameEffects", "", object_->GetTransform(), 3, 10.0f, 0.0f);

    auto collider = std::make_unique<Collider>();
    collider->initialize(this, radius_);
    SetCollider(std::move(collider));

    object_->Update();
}

void Enemy::Update()
{
    if (turnCooldownTimer_ > 0.0f) {
        turnCooldownTimer_ -= deltaTime_;
        if (turnCooldownTimer_ < 0.0f) {
            turnCooldownTimer_ = 0.0f;
        }
    }

    if (hitInvincibilityTimer_ > 0.0f) {
        hitInvincibilityTimer_ -= deltaTime_; // 60FPSを想定した減算

        if (hitInvincibilityTimer_ <= 0.0f) {
            hitInvincibilityTimer_ = 0.0f;
            isDamaged_ = false; // クールダウン終了
        }
    }
    RayCastUpdate();

    if (isKnockback_) {
        HandleKnockback();
    } else {
        // 1. ノックバック中でない場合のみ現在の状態を更新
        if (state_) {
            state_->Update(this);
        }
    }

    // 3. 物理計算とレール座標の更新
    UpdatePhysics();

    if (auto* col = GetCollider()) {
        col->Update();
    }

}

void Enemy::UpdateTransform()
{
    RayCastUpdate();
    if (IsRayHit()) {
        worldY_ = GetRayHitPoint().y + config_.movement.heightOffset;
    }
    UpdatePhysics();
    if (auto* col = GetCollider()) {
        col->Update();
    }
}

void Enemy::ImGuiDrawDebugInfo()
{
#ifdef USE_IMGUI
    ImGui::Begin("Debug/Enemy");

    // 敵の基本情報
    if (object_) {
        Vector3 pos = object_->GetTranslate();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    }

    // レールの進捗
    if (railMover_) {
        ImGui::Text("Rail Progress: %.2f", railMover_->GetProgress());
        Vector3 dir = railMover_->GetCurrentDirection();
        ImGui::Text("Direction: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);
    }

    ImGui::Separator();
    ImGui::Text("--- Config File Actions ---");
    if (ImGui::Button("💾 Save Enemy Config to JSON")) {
        SaveConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("🔄 Reload from JSON")) {
        LoadConfig();
        ApplyColliderConfig();
    }

    ImGui::Separator();
    ImGui::Text("--- Movement Parameters ---");
    if (ImGui::SliderFloat("Move Speed", &config_.movement.moveSpeed, 0.5f, 30.0f, "%.1f m/s")) {
        moveSpeed_ = config_.movement.moveSpeed;
    }
    ImGui::SliderFloat("Gravity", &config_.movement.gravity, -120.0f, -10.0f, "%.1f");
    ImGui::SliderFloat("Gravity Scale", &config_.movement.gravityScale, 0.1f, 5.0f, "%.2f");
    ImGui::SliderFloat("Turn Cooldown", &config_.movement.turnCooldown, 0.05f, 2.0f, "%.2f s");
    ImGui::SliderFloat("Height Offset", &config_.movement.heightOffset, -0.5f, 2.0f, "%.2f m");

    ImGui::Separator();
    ImGui::Text("--- Combat Parameters ---");
    ImGui::SliderFloat("Invincibility Duration", &config_.combat.hitInvincibilityDuration, 0.1f, 3.0f, "%.2f s");
    ImGui::SliderFloat("Knockback Duration", &config_.combat.knockbackDuration, 0.1f, 2.0f, "%.2f s");
    ImGui::SliderFloat("Knockback Speed", &config_.combat.knockbackSpeed, 1.0f, 25.0f, "%.1f m/s");
    ImGui::SliderFloat("Knockback Jump Force", &config_.combat.knockbackJumpForce, 1.0f, 30.0f, "%.1f");

    ImGui::Separator();
    ImGui::Text("--- Collider Parameters ---");
    if (ImGui::SliderFloat("Collider Radius", &config_.collider.radius, 0.1f, 3.0f, "%.2f m")) {
        ApplyColliderConfig();
    }

    ImGui::Separator();
    ImGui::Text("--- Enemy States ---");

    // 敵の状態表示
    if (state_) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "State: %s", state_->GetName());
    } else {
        ImGui::Text("State: None");
    }

    // 敵の行動表示
    if (behavior_) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Behavior: %s", behavior_->GetName());
    } else {
        ImGui::Text("Behavior: None");
    }

    ImGui::Separator();

    // 物理情報
    ImGui::Text("IsGrounded: %s", isGrounded_ ? "True" : "False");
    ImGui::Text("WorldY: %.2f", worldY_);
    ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);

    ImGui::Separator();
    if (isDamaged_) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Status: CoolDown (Hit!)");
    } else {
        ImGui::Text("Status: Ready");
    }
    ImGui::ProgressBar(hitInvincibilityTimer_ / config_.combat.hitInvincibilityDuration, ImVec2(0, 0), "CD Timer");

    ImGui::End();
#endif // USE_IMGUI
}

void Enemy::Draw()
{
    if (object_) {
        object_->Draw();
    }
    if (auto* col = GetCollider()) {
        col->Draw();
    }

    ImGuiDrawDebugInfo();
}

void Enemy::SetRailPosition(const Vector2& position)
{
    if (railMover_) {
        railMover_->SetProgress(position.x);
        worldY_ = position.y;

        // 2. 物理座標の同期
        UpdatePhysics();

        // 3. 地面へのレイキャスト判定と高度補正（※シーンが設定済みの場合）
        if (scene_) {
            RayCastUpdate();
            if (IsRayHit()) {
                worldY_ = GetRayHitPoint().y + config_.movement.heightOffset;
                UpdatePhysics(); // 重力補正後の高度で再度トランスフォーム更新
                object_->Update();
            }
        }

    }
}

void Enemy::SetRail(RailPath* rail)
{
    if (rail && railMover_) {
        railMover_->SetPath(rail);
    }
}

void Enemy::Move(float ratio)
{
    if (railMover_) {
        // 毎フレームの移動量を計算して進める（進行方向 moveDirection_ を適用）
        railMover_->Advance(ratio * moveDirection_ * (moveSpeed_ * deltaTime_));
    }
}
void Enemy::ChangeBehavior(std::unique_ptr<IEnemyBehavior> newBehavior) {
    if (behavior_) behavior_->Finalize(this);
    behavior_ = std::move(newBehavior);
    if (behavior_) behavior_->Initialize(this);
}

void Enemy::ChangeState(std::unique_ptr<IEnemyState> newState) {
    if (state_) state_->Finalize(this);
    state_ = std::move(newState);
    if (state_) state_->Initialize(this);
}

void Enemy::UpdatePhysics() {
    Vector3 railPos = railMover_->GetCurrentPosition();
    Vector3 finalPos = { railPos.x, worldY_, railPos.z };
    object_->SetTranslate(finalPos);

    // 回転処理（逆走時は進行方向を反転）
    Vector3 dir = railMover_->GetCurrentDirection();
    if (moveDirection_ < 0.0f) {
        dir = Multiply(-1.0f, dir);
    }
    float angle = atan2f(dir.x, dir.z);

    // 進行方向の向きに加えて、のけぞり角度（ピッチ）と回転（ロール）を合成
    Vector3 currentRot = initialRotationOffset_ + Vector3{ knockbackTilt_, angle, knockbackRoll_ };
    object_->SetRotate(currentRot);
    object_->Update();
}

void Enemy::TakeDamage(float knockbackDirection, bool isDeadly)
{
    if (isDamaged_ || isDeathFinished_) return;

    isDamaged_ = true;
    hitInvincibilityTimer_ = config_.combat.hitInvincibilityDuration;
    isKnockback_ = true;
    knockbackDirection_ = knockbackDirection;
    knockbackTimer_ = config_.combat.knockbackDuration;

    // 上方向に初速を与えて放物線ジャンプを開始
    velocity_.y = config_.combat.knockbackJumpForce;
    isGrounded_ = false;

    // 後ろへののけぞり姿勢を設定（仰け反る角度）
    knockbackTilt_ = -35.0f;
    knockbackRoll_ = 0.0f;

    PlayHitEffect();

    if (isDeadly) {
        ChangeState(std::make_unique<StateEnemyDead>());
    }
}

void Enemy::HandleKnockback()
{
    if (!isKnockback_) return;

    // 残り時間に応じた速度減衰でレール沿いに後退
    float ratio = (config_.combat.knockbackDuration > 0.0f) ? (knockbackTimer_ / config_.combat.knockbackDuration) : 0.0f;
    float moveAmount = knockbackDirection_ * config_.combat.knockbackSpeed * ratio * deltaTime_;
    if (railMover_) {
        railMover_->Advance(moveAmount);
    }

    // 重力計算（放物線を描いて落下）
    UpdateGravity();

    // 死亡時はきりもみ回転、生存時は徐々にのけぞり姿勢から復帰
    if (state_ && strcmp(state_->GetName(), "Dead") == 0) {
        knockbackRoll_ += 360.0f * deltaTime_;
    } else {
        knockbackTilt_ += (0.0f - knockbackTilt_) * (8.0f * deltaTime_);
    }

    knockbackTimer_ -= deltaTime_;

    // 地面に着地したか、または滞空時間が終了したらノックバック完了
    if (knockbackTimer_ <= 0.0f || (isGrounded_ && velocity_.y <= 0.0f && knockbackTimer_ < config_.combat.knockbackDuration * 0.7f)) {
        isKnockback_ = false;
        knockbackTilt_ = 0.0f;
        if (state_ && strcmp(state_->GetName(), "Dead") == 0) {
            isDeathFinished_ = true; // 演出完了により消滅可能フラグON
        }
    }
}
// Enemy.cpp
void Enemy::OnCollision(GameObject* other) {

    // ぶつかった相手の有効性チェック（被弾中・ノックバック中・死亡演出後はスキップ）
    if (!other || isDamaged_ || isKnockback_ || isDeathFinished_) return;

    if (other->GetCategory() == CollisionCategory::Player || other->GetCategory() == CollisionCategory::PlayerAttack) {
        Player* player = dynamic_cast<Player*>(other);
        if (!player) {
            return;
        }

        // プレイヤーの攻撃ヒットボックスに当たった場合、または攻撃中だった場合
        bool isHitByAttack = (other->GetCategory() == CollisionCategory::PlayerAttack) ||
                             player->IsAttacking();

        if (isHitByAttack) {
            // シーンのポリモーフィックなヒットストップとシェイクを要求
            if (scene_) {
                scene_->TriggerHitStop(0.1f);
                scene_->RequestCameraShake(0.09f, 3.0f);
            }

            // プレイヤー側に攻撃ヒット（突進停止＆硬直解除）を通知
            player->OnAttackHit(this);

            if (robot_) {
                // 1. ロボットからプレイヤー用 Factory を取得
                auto factory = robot_->CreatePlayerFactory();

                if (factory) {
                    // 2. Factory を使って State を生成
                    auto rideOnState = factory->CreateState();

                    // 3. プレイヤーの State を切り替える
                    player->ChangeState(std::move(rideOnState));
                }
            }

            // プレイヤーから見て後ろへ吹き飛ぶ向きを計算
            float hitDir = -1.0f;
            if (railMover_ && player->GetRailMover()) {
                float enemyDist = railMover_->GetCurrentDistance();
                float playerDist = player->GetRailMover()->GetCurrentDistance();
                hitDir = (enemyDist >= playerDist) ? 1.0f : -1.0f;
            }

            TakeDamage(hitDir, true);
            return;
        }

        // プレイヤーの攻撃以外での接触（体当たりなど）
        OnCollideWithPlayer(player);
    }
    // 弾カテゴリの判定
    if (other->GetCategory() == CollisionCategory::PlayerProjectile)
    {
        float hitDir = -moveDirection_;
        TakeDamage(hitDir, true);
    }
}

void Enemy::OnCollideWithPlayer(Player* player) {
    (void)player; // 基底クラスでは何もしない
}

void Enemy::RayCastUpdate()
{
    if (!scene_) return;

    const std::vector<Triangle>& triangles = scene_->GetTriangle();
    if (triangles.empty()) {
        SetRayHit(false);
        SetRayHitDistance(FLT_MAX);
        return;
    }

    // 始点を敵の位置から少し高めに持ち上げる（めり込み時でも確実に地面の上から下向きに照射）
    Vector3 rayOrigin = object_->GetTranslate();
    rayOrigin.y += 2.0f;
    Vector3 rayDiff = { 0.0f, -25.0f, 0.0f };
    SetRay({ rayOrigin, rayDiff });

    // 毎フレーム初期化
    SetRayHit(false);
    SetRayHitDistance(FLT_MAX);
    SetRayCollisionResult(RayTriangleCollisionResult::NoCollision);
    SetRayHitPoint({ 0.0f, 0.0f, 0.0f });
    SetRayHitTriangle(Triangle{});

    for (const auto& tri : triangles) {
        Vector3 tmpHit = {};
        float dist = 0.0f;
        RayTriangleCollisionResult result;

        if (CheckRayTriangle(GetRay(), tri, &dist, &tmpHit, &result)) {
            // FrontFace と BackFace の両方を対象にする（左手系・モデルの巻き順差異への対応）
            if (result == RayTriangleCollisionResult::FrontFace || result == RayTriangleCollisionResult::BackFace) {

                // 面法線を計算して上向き（床面）であることを確認（垂直な壁や急崖は除外）
                Vector3 v01 = Subtract(tri.vertices[1], tri.vertices[0]);
                Vector3 v12 = Subtract(tri.vertices[2], tri.vertices[1]);
                Vector3 normal = Cross(v01, v12);
                float nLen = Length(normal);
                if (nLen > 0.0001f) {
                    normal = Normalize(normal);
                    if (result == RayTriangleCollisionResult::BackFace) {
                        normal = Multiply(-1.0f, normal);
                    }
                    if (normal.y < 0.2f) {
                        continue; // 壁や天井面は床判定から除外
                    }
                }

                // 最も近い（最も高い位置にある）床を選択
                if (dist < GetRayHitDistance()) {
                    SetRayHitDistance(dist);
                    SetRayHitTriangle(tri);
                    SetRayHitPoint(tmpHit);
                    SetRayCollisionResult(result);
                    SetRayHit(true);
                }
            }
        }
    }

    // デバッグ描画
    PrimitiveDrawer::GetInstance()->DrawLine(GetRay().origin, Add(GetRay().origin, GetRay().diff),
        IsRayHit() ? Vector4{ 1,0,0,1 } : Vector4{ 0,1,0,1 });
    if (IsRayHit()) {
        PrimitiveDrawer::GetInstance()->DrawSphere({ GetRayHitPoint(), 0.05f, {} }, { 0,0,1,1 });
    }

    // ─── 前方壁の判定と押し戻し・反転処理 ───
    isWallHit_ = false;
    wallRayDistance_ = FLT_MAX;

    if (railMover_) {
        Vector3 railDir = railMover_->GetCurrentDirection();
        Vector3 forwardDir = (Length(railDir) > 0.001f) ? Normalize(railDir) : Vector3{ 0.0f, 0.0f, 1.0f };
        if (moveDirection_ < 0.0f) {
            forwardDir = Multiply(-1.0f, forwardDir);
        }

        float wallLength = config_.collider.radius + 0.3f;
        Ray wallRay;
        wallRay.origin = object_->GetTranslate();
        wallRay.origin.y += config_.movement.heightOffset;
        wallRay.diff = Multiply(wallLength, forwardDir);

        Vector3 wallHitPoint = {};
        float closestWallDist = FLT_MAX;
        bool hitWall = false;

        for (const auto& tri : triangles) {
            Vector3 tmpHit = {};
            float dist = 0.0f;
            RayTriangleCollisionResult result;
            if (CheckRayTriangle(wallRay, tri, &dist, &tmpHit, &result)) {
                if (result == RayTriangleCollisionResult::FrontFace || result == RayTriangleCollisionResult::BackFace) {
                    Vector3 v01 = Subtract(tri.vertices[1], tri.vertices[0]);
                    Vector3 v12 = Subtract(tri.vertices[2], tri.vertices[1]);
                    Vector3 normal = Normalize(Cross(v01, v12));
                    if (result == RayTriangleCollisionResult::BackFace) {
                        normal = Multiply(-1.0f, normal);
                    }
                    // 垂直に近い面のみ壁とする（床や緩やかな坂は除外）
                    if (std::abs(normal.y) < 0.7f && dist < closestWallDist) {
                        closestWallDist = dist;
                        wallHitPoint = tmpHit;
                        hitWall = true;
                    }
                }
            }
        }

        isWallHit_ = hitWall;
        wallRayDistance_ = closestWallDist;

        // 壁レイのデバッグ描画
        Vector3 wallEnd = Add(wallRay.origin, wallRay.diff);
        PrimitiveDrawer::GetInstance()->DrawLine(wallRay.origin, wallEnd,
            hitWall ? Vector4{ 1.0f, 0.0f, 0.0f, 1.0f } : Vector4{ 0.0f, 1.0f, 1.0f, 1.0f });
        if (hitWall) {
            PrimitiveDrawer::GetInstance()->DrawSphere({ wallHitPoint, 0.08f, {} }, { 1.0f, 0.5f, 0.0f, 1.0f });

            // めり込み防止（押し戻し補正）
            float penetration = wallLength - closestWallDist;
            if (penetration > 0.0f) {
                float currentDist = railMover_->GetCurrentDistance();
                float targetDist = currentDist - (penetration * moveDirection_);
                const RailPath* path = railMover_->GetRailPath();
                if (path) {
                    float targetT = path->GetTFromDistance(targetDist);
                    railMover_->SetProgress(targetT);
                }
            }

            // 壁衝突時の自動反転（クールダウン付き）
            if (turnCooldownTimer_ <= 0.0f) {
                Turn();
            }
        }
    }
}

void Enemy::UpdateGravity()
{
    if (IsRayHit()) {
        rayHitParam_.groundY = GetRayHitPoint().y;
        float targetY = rayHitParam_.groundY + config_.movement.heightOffset;
        float enemyBottomY = worldY_ - config_.movement.heightOffset;

        // 1. 上昇中（ジャンプ中 velocity_.y > 0.0f）の処理
        if (velocity_.y > 0.0f) {
            isGrounded_ = false;
            velocity_.y += (config_.movement.gravity * config_.movement.gravityScale) * deltaTime_;
            worldY_ += velocity_.y * deltaTime_;
        }
        else {
            // 2. 下降中または静止中（velocity_.y <= 0.0f）の着地・吸着処理
            // 地面以下に達した場合、または接地中の下り坂吸着範囲内にある場合
            if (worldY_ <= targetY + 0.05f || (isGrounded_ && enemyBottomY <= rayHitParam_.groundY + 0.5f)) {
                isGrounded_ = true;
                worldY_ = targetY; // 地面に確実にスナップ
                velocity_.y = 0.0f;
            } else {
                // 空中から落下中の場合
                isGrounded_ = false;
                velocity_.y += (config_.movement.gravity * config_.movement.gravityScale) * deltaTime_;
                worldY_ += velocity_.y * deltaTime_;

                // 落下した結果、地面を突き抜けた場合は地面で止める
                if (worldY_ <= targetY) {
                    worldY_ = targetY;
                    velocity_.y = 0.0f;
                    isGrounded_ = true;
                }
            }
        }
    } else {
        // 地面が検出されない場合（落下中または奈落）
        isGrounded_ = false;
        velocity_.y += (config_.movement.gravity * config_.movement.gravityScale) * deltaTime_;
        worldY_ += velocity_.y * deltaTime_;

        // 奈落の最低保証
        if (worldY_ <= rayHitParam_.minY + config_.movement.heightOffset) {
            worldY_ = rayHitParam_.minY + config_.movement.heightOffset;
            velocity_.y = 0.0f;
            isGrounded_ = true;
        }
    }
}
const char* Enemy::GetStateName() const
{
    return state_ ? state_->GetName() : "None";
}

const char* Enemy::GetBehaviorName() const
{
    return behavior_ ? behavior_->GetName() : "None";
}

bool Enemy::IsDead() const
{
    return isDeathFinished_ || (state_ && state_->IsDead());
}

bool Enemy::CanDamagePlayer() const
{
    return state_ ? state_->CanDamagePlayer() : false;
}

void Enemy::PlayHitEffect() {
    if (hitParticle_) {

        hitParticle_->SetTransform(object_->GetTransform());
        hitParticle_->Emit();
    }
}

float Enemy::GetCurrentDistance() const {
    return railMover_ ? railMover_->GetCurrentDistance() : 0.0f;
}

const RailMover* Enemy::GetRailMover() const {
    return railMover_.get();
}