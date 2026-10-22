#include "Player.h"
#include "Object3D.h"
#include "ModelManager.h"
#include "RailMover.h"
#include "RailPath.h"
#include "InputHandler.h"
#include "imgui.h"
#include "PlayerState.h"
#include "PlayerBehavior.h"
#include "Enemy.h"
#include "Scene.h"
#include "PrimitiveDrawer.h"
#include <numbers>
#include "GameScene.h"
#include "EffectManager.h"
#include "collider.h"
#include "Physics.h"
#include "Input.h"
#include "JsonManager.h"
#include "Logger.h"

Player::Player() = default;
Player::~Player() = default;

void Player::LoadConfig(const std::string& filePath)
{
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("PlayerConfig", filePath)) {
        Logger::Log("[Player] Could not open " + filePath + ". Using default config.\n");
        return;
    }

    const auto& j = jsonMgr->Get("PlayerConfig");

    // Paths
    if (j.contains("paths")) {
        const auto& jp = j["paths"];
        config_.paths.modelDir = jp.value("modelDir", config_.paths.modelDir);
        config_.paths.modelFile = jp.value("modelFile", config_.paths.modelFile);
        config_.paths.texture = jp.value("texture", config_.paths.texture);
        config_.paths.hitSE = jp.value("hitSE", config_.paths.hitSE);
        config_.paths.damageSE = jp.value("damageSE", config_.paths.damageSE);
    }

    // Movement
    if (j.contains("movement")) {
        const auto& jm = j["movement"];
        config_.movement.walkSpeed = jm.value("walkSpeed", config_.movement.walkSpeed);
        config_.movement.dashSpeed = jm.value("dashSpeed", config_.movement.dashSpeed);
        config_.movement.gravity = jm.value("gravity", config_.movement.gravity);
        config_.movement.jumpAcceleration = jm.value("jumpAcceleration", config_.movement.jumpAcceleration);
        config_.movement.coyoteDuration = jm.value("coyoteDuration", config_.movement.coyoteDuration);
        config_.movement.jumpBufferDuration = jm.value("jumpBufferDuration", config_.movement.jumpBufferDuration);
        config_.movement.maxSlopeCos = jm.value("maxSlopeCos", config_.movement.maxSlopeCos);
        config_.movement.maxStepHeight = jm.value("maxStepHeight", config_.movement.maxStepHeight);
        config_.movement.groundSnapDistance = jm.value("groundSnapDistance", config_.movement.groundSnapDistance);
    }

    // Combat
    if (j.contains("combat")) {
        const auto& jc = j["combat"];
        config_.combat.hitInvincibilityDuration = jc.value("hitInvincibilityDuration", config_.combat.hitInvincibilityDuration);
        config_.combat.knockbackDuration = jc.value("knockbackDuration", config_.combat.knockbackDuration);
        config_.combat.knockbackSpeed = jc.value("knockbackSpeed", config_.combat.knockbackSpeed);
        config_.combat.knockbackJumpForce = jc.value("knockbackJumpForce", config_.combat.knockbackJumpForce);
    }

    // Juice
    if (j.contains("juice")) {
        const auto& jj = j["juice"];
        config_.juice.jumpStretchIntensity = jj.value("jumpStretchIntensity", config_.juice.jumpStretchIntensity);
        config_.juice.jumpStretchDuration = jj.value("jumpStretchDuration", config_.juice.jumpStretchDuration);
        config_.juice.landSquashIntensityMax = jj.value("landSquashIntensityMax", config_.juice.landSquashIntensityMax);
        config_.juice.landSquashDuration = jj.value("landSquashDuration", config_.juice.landSquashDuration);
    }

    // Collider
    if (j.contains("collider")) {
        const auto& jc = j["collider"];
        config_.collider.radius = jc.value("radius", config_.collider.radius);
        config_.collider.modelRadius = jc.value("modelRadius", config_.collider.modelRadius);
        config_.collider.heightOffset = jc.value("heightOffset", config_.collider.heightOffset);
        config_.collider.wallRayHeight = jc.value("wallRayHeight", config_.collider.wallRayHeight);

        if (jc.contains("head")) {
            config_.collider.headOffsetY = jc["head"].value("offsetY", config_.collider.headOffsetY);
            config_.collider.headRadius = jc["head"].value("radius", config_.collider.headRadius);
        }
        if (jc.contains("body")) {
            config_.collider.bodyOffsetY = jc["body"].value("offsetY", config_.collider.bodyOffsetY);
            config_.collider.bodyRadius = jc["body"].value("radius", config_.collider.bodyRadius);
        }
        if (jc.contains("attack")) {
            config_.collider.attackOffsetY = jc["attack"].value("offsetY", config_.collider.attackOffsetY);
            config_.collider.attackOffsetForward = jc["attack"].value("offsetForward", config_.collider.attackOffsetForward);
            config_.collider.attackRadius = jc["attack"].value("radius", config_.collider.attackRadius);
        }
    }
}

void Player::SaveConfig(const std::string& filePath)
{
    nlohmann::json j;

    j["paths"] = {
        { "modelDir", config_.paths.modelDir },
        { "modelFile", config_.paths.modelFile },
        { "texture", config_.paths.texture },
        { "hitSE", config_.paths.hitSE },
        { "damageSE", config_.paths.damageSE }
    };

    j["movement"] = {
        { "walkSpeed", config_.movement.walkSpeed },
        { "dashSpeed", config_.movement.dashSpeed },
        { "gravity", config_.movement.gravity },
        { "jumpAcceleration", config_.movement.jumpAcceleration },
        { "coyoteDuration", config_.movement.coyoteDuration },
        { "jumpBufferDuration", config_.movement.jumpBufferDuration },
        { "maxSlopeCos", config_.movement.maxSlopeCos },
        { "maxStepHeight", config_.movement.maxStepHeight },
        { "groundSnapDistance", config_.movement.groundSnapDistance }
    };

    j["combat"] = {
        { "hitInvincibilityDuration", config_.combat.hitInvincibilityDuration },
        { "knockbackDuration", config_.combat.knockbackDuration },
        { "knockbackSpeed", config_.combat.knockbackSpeed },
        { "knockbackJumpForce", config_.combat.knockbackJumpForce }
    };

    j["juice"] = {
        { "jumpStretchIntensity", config_.juice.jumpStretchIntensity },
        { "jumpStretchDuration", config_.juice.jumpStretchDuration },
        { "landSquashIntensityMax", config_.juice.landSquashIntensityMax },
        { "landSquashDuration", config_.juice.landSquashDuration }
    };

    j["collider"] = {
        { "radius", config_.collider.radius },
        { "modelRadius", config_.collider.modelRadius },
        { "heightOffset", config_.collider.heightOffset },
        { "wallRayHeight", config_.collider.wallRayHeight },
        { "head", {
            { "offsetY", config_.collider.headOffsetY },
            { "radius", config_.collider.headRadius }
        }},
        { "body", {
            { "offsetY", config_.collider.bodyOffsetY },
            { "radius", config_.collider.bodyRadius }
        }},
        { "attack", {
            { "offsetY", config_.collider.attackOffsetY },
            { "offsetForward", config_.collider.attackOffsetForward },
            { "radius", config_.collider.attackRadius }
        }}
    };

    JsonManager::GetInstance()->Save(filePath, j);
}

void Player::ApplyColliderConfig()
{
    if (auto* col = GetCollider()) {
        col->initialize(this, config_.collider.radius);
        col->SetOffset({ 0.0f, 0.0f, 0.0f });
        col->AddSphere("Head", { 0.0f, config_.collider.headOffsetY, 0.0f }, config_.collider.headRadius);
        col->AddSphere("Body", { 0.0f, config_.collider.bodyOffsetY, 0.0f }, config_.collider.bodyRadius);
    }
    if (attackCollider_) {
        attackCollider_->initialize(this, config_.collider.attackRadius);
        attackCollider_->SetCategory(CollisionCategory::PlayerAttack);
        attackCollider_->SetCollide(isAttackHitboxActive_);
        attackCollider_->AddSphere("Attack", { 0.0f, config_.collider.attackOffsetY, config_.collider.attackOffsetForward }, config_.collider.attackRadius);
    }
}

void Player::Initialize()
{
    // JSON設定の読み込み
    LoadConfig();

    inputHandler_ = std::make_unique<InputHandler>();
    object_ = std::make_unique<Object3d>();
    ModelManager::GetInstance()->LoadModel(config_.paths.modelDir, config_.paths.modelFile);
    object_->Initialize();
    object_->SetModel(config_.paths.modelFile);
    object_->SetTexture(config_.paths.texture);
    animation = std::make_unique<Animation>();

    animation->Initialize(config_.paths.modelDir, config_.paths.modelFile);
    animation->SetCurrentTime(0.0f);
    object_->SetAnimations(animation.get());

    railMover_ = std::make_unique<RailMover>();

    // Stateの初期化のみ行い、Behaviorの初期化はState内部で行う
    ChangeState(PlayerStateFactory::CreateState(PlayerFormType::Normal));

    InitializeRays();

    // コライダーの初期化
    auto collider = std::make_unique<Collider>();
    SetCollider(std::move(collider));
    attackCollider_ = std::make_unique<Collider>();

    ApplyColliderConfig();

    object_->Update();

    HitSE_ = Audio::GetInstance()->LoadAudio(config_.paths.hitSE);
    DamageSE_ = Audio::GetInstance()->LoadAudio(config_.paths.damageSE);
}
void Player::Update()
{
    //   if (!isActive_) return;

    HandleAlive();

    if (!isAlive_) {
        ChangeState(std::make_unique<StateDead>());
    }

    if (dropThroughTimer_ > 0.0f) {
        dropThroughTimer_ -= deltaTime_;
        if (dropThroughTimer_ < 0.0f) {
            dropThroughTimer_ = 0.0f;
        }
    }

    // コヨーテタイマーの更新（接地中は常に満タン、離れたらカウントダウン）
    if (isGrounded_) {
        coyoteTimer_ = config_.movement.coyoteDuration;
    } else if (coyoteTimer_ > 0.0f) {
        coyoteTimer_ -= deltaTime_;
        if (coyoteTimer_ < 0.0f) {
            coyoteTimer_ = 0.0f;
        }
    }

    // 先行入力タイマーの減衰
    if (jumpBufferTimer_ > 0.0f) {
        jumpBufferTimer_ -= deltaTime_;
        if (jumpBufferTimer_ < 0.0f) {
            jumpBufferTimer_ = 0.0f;
        }
    }

    HandleDamage();
    HandleKnockback();
    HandleInput();
    UpdateSquashStretch();
    RayCastUpdate();
    if (auto* col = GetCollider()) {
        col->Update();
    }
    if (attackCollider_) {
        attackCollider_->SetCollide(IsAttackHitboxActive());
        attackCollider_->Update();
    }

    if (baseState_) baseState_->Update(this);

    // 接地時に先行入力が残っていれば即座にジャンプを実行
    if (isGrounded_ && jumpBufferTimer_ > 0.0f) {
        TryExecuteBufferedJump();
    }

    // 接地状態と移動入力から歩き・ダッシュ・待機の移動状態を決定
    if (!isGrounded_ || !isMoving_) {
        moveState_ = MoveState::Idle;
    } else if (isDashing_) {
        moveState_ = MoveState::Dash;
    } else {
        moveState_ = MoveState::Walk;
    }

    UpdateRailPath();
    ImGuiDrawDebugInfo();
}

void Player::UpdateTransform()
{
    RayCastUpdate();
    UpdateRailPath();
    if (auto* col = GetCollider()) {
        col->Update();
    }
    if (attackCollider_) {
        attackCollider_->Update();
    }
}

void Player::Draw()
{
    //    if (!isActive_) return;

        // 無敵時間中のモデル点滅（0.08秒ごとに表示/非表示を切り替え）
    if (hitInvincibilityTimer_ > 0.0f) {
        const float kBlinkInterval = 0.08f;
        if (fmodf(hitInvincibilityTimer_, kBlinkInterval * 2.0f) < kBlinkInterval) {
            if (auto* col = GetCollider()) {
                col->Draw();
            }
            return;
        }
    }

    object_->Draw();

    if (auto* col = GetCollider()) {
        col->Draw();
    }

    // 攻撃ヒットボックスが有効な場合、オレンジ色のワイヤーフレーム球体を描画
    if (attackCollider_ && IsAttackHitboxActive()) {
#ifdef USE_LINE
        auto attackSpheres = attackCollider_->GetWorldSpheres();
        for (const auto& sphere : attackSpheres) {
            PrimitiveDrawer::GetInstance()->DrawSphere(sphere, { 1.0f, 0.6f, 0.0f, 1.0f });
        }
#endif // USE_LINE
    }
}

void Player::SetRailPosition(const Vector2& position)
{
    if (railMover_) {
        // 1. レールの進捗を設定
        railMover_->SetProgress(position.x);

        // 2. 高度(Y)の設定（Y座標の管理変数 worldY_ も同期）
        worldY_ = position.y;

        // 3. 即座にトランスフォーム（位置・回転）を更新
        UpdateRailPath();

        // 4. 地面へのレイキャスト判定と高度補正（シーンが設定済みの場合）
        if (scene_) {
            RayCastUpdate();
            if (IsRayHit()) {
                worldY_ = GetRayHitPoint().y + config_.collider.heightOffset;
                UpdateRailPath(); // 重力補正後の高度で再度トランスフォーム更新
                object_->Update();
            }
        }
    }
}

void Player::AddVelocity(Vector3 v) {
    velocity_ += v;
}

void Player::SetRail(RailPath* rail)
{
    if (!rail || !railMover_) return;
    if (!rail || !railMover_) return;

    // レールパスを適用
    railMover_->SetPath(rail);

    // 物理座標の同期
    UpdateRailPath();

    // 地面へのレイキャスト判定と高度補正（シーンが設定済みの場合）
    if (scene_) {
        RayCastUpdate();
        if (IsRayHit()) {
            worldY_ = GetRayHitPoint().y + config_.collider.heightOffset;
            UpdateRailPath(); // 重力補正後の高度で再度トランスフォーム更新

        }
    }
}

void Player::Move(float ratio)
{
    if (std::abs(ratio) > 0.001f) {
        isMoving_ = true;
    }
    float speed = isDashing_ ? config_.movement.dashSpeed : config_.movement.walkSpeed;
    railMover_->Advance(ratio * (speed * deltaTime_));
}

void Player::Jump()
{
    // ジャンプボタンが押されたら先行入力タイマーをチャージ
    jumpBufferTimer_ = config_.movement.jumpBufferDuration;
    TryExecuteBufferedJump();
}

bool Player::TryExecuteBufferedJump()
{
    if (jumpBufferTimer_ <= 0.0f) {
        return false;
    }

    // 接地中、または崖を踏み外した直後（コヨーテタイム中）ならジャンプ許可
    bool canJump = isGrounded_ || (coyoteTimer_ > 0.0f && !isJumping_);
    if (!canJump) {
        return false;
    }

    // すり抜け足場の上で「下入力＋ジャンプ」を行った場合は下層へすり抜け降下
    Input* input = Input::GetInstance();
    bool isDownPressed = false;
    if (input) {
        if (input->PushedKeyDown(DIK_S) || input->PushedKeyDown(DIK_DOWN)) {
            isDownPressed = true;
        }
        XINPUT_STATE state;
        if (input->GetJoyStick(0, state)) {
            float rawY = (float)state.Gamepad.sThumbLY / 32767.0f;
            if (rawY < -0.5f) {
                isDownPressed = true;
            }
        }
        if (input->PushPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN)) {
            isDownPressed = true;
        }
    }

    if (isDownPressed && isCurrentGroundOneway_) {
        // 下層へすり抜け降下
        isGrounded_ = false;
        isJumping_ = false;
        dropThroughTimer_ = 0.3f; // 0.3秒間すり抜け足場の床判定を無視
        velocity_.y = -6.0f;     // 下向き初速を与えてスムーズに降りる
        worldY_ -= 0.1f;
        coyoteTimer_ = 0.0f;
        jumpBufferTimer_ = 0.0f;
        return true;
    } else {
        // 通常ジャンプ実行！
        velocity_.y = config_.movement.jumpAcceleration;
        isGrounded_ = false;
        isJumping_ = true;
        coyoteTimer_ = 0.0f;      // コヨーテタイムを消費
        jumpBufferTimer_ = 0.0f;  // 先行入力を消費
        TriggerSquashStretch(config_.juice.jumpStretchIntensity, config_.juice.jumpStretchDuration); // ジャンプ縦伸び発動！

        // ジャンプビヘイビアへ遷移
        if (baseState_) {
            if (auto factory = baseState_->GetFactory()) {
                baseState_->ChangeBehavior(this, factory->CreateBehavior(BehaviorType::Jump));
            }
        }
        return true;
    }
}

void Player::Attack() {}

float Player::GetRailProgress() const {
    return railMover_->GetProgress();
}
float Player::GetCurrentDistance() const {
    return railMover_->GetCurrentDistance();
}
const RailPath* Player::GetRailPath() const {
    return railMover_->GetRailPath();
}

void Player::UpdateRailPath()
{
    Vector3 railPos = railMover_->GetCurrentPosition();
    Vector3 railDir = railMover_->GetCurrentDirection();

    Vector3 finalPos = { railPos.x, worldY_, railPos.z };
    finalPos = Add(finalPos, wallPushOffset_);
    object_->SetTranslate(finalPos);

    // ノックバック中は向きを変更せず直前の向きを固定
    if (!isKnockback_) {
        float currentFrameAngle = 0.0f;
        if (Length(railDir) > 0.001f) {
            currentFrameAngle = atan2f(railDir.x, railDir.z);
            if (railMover_->GetMoveDirection() == RailMover::MoveDirection::Backward) {
                currentFrameAngle += std::numbers::pi_v<float>;
            }
        }
        currentAngle_ = currentFrameAngle;
    }

    object_->SetRotate({ 0.0f, currentAngle_, 0.0f });
    object_->Update();

    // コライダーの回転（Quaternion）およびモデル形状スフィアの同期
    if (auto* col = GetCollider()) {
        col->SetRotation(EulerToQuaternion({ 0.0f, currentAngle_, 0.0f }));
        col->SetSphereOffset("Head", { 0.0f, config_.collider.headOffsetY, 0.0f });
        col->SetSphereRadius("Head", config_.collider.headRadius);
        col->SetSphereOffset("Body", { 0.0f, config_.collider.bodyOffsetY, 0.0f });
        col->SetSphereRadius("Body", config_.collider.bodyRadius);
        col->Update();
    }

    // 攻撃用ヒットボックスの回転・位置の同期（前方に突き出し配置）
    if (attackCollider_) {
        attackCollider_->SetRotation(EulerToQuaternion({ 0.0f, currentAngle_, 0.0f }));
        attackCollider_->SetSphereOffset("Attack", { 0.0f, config_.collider.attackOffsetY, config_.collider.attackOffsetForward });
        attackCollider_->SetSphereRadius("Attack", config_.collider.attackRadius);
        attackCollider_->SetCollide(IsAttackHitboxActive());
        attackCollider_->Update();
    }
}

void Player::CheckGroundCollision()
{
    auto floorRay = GetRayInfo("Floor");
    bool hitFloor = floorRay && floorRay->isColide;

    float playerBottomY = worldY_ - config_.collider.heightOffset;

    if (hitFloor) {
        // 急すぎる斜面（崖・壁）は地面として扱わない（登れる傾斜角の制限: cos約49度以上で歩行可能）
        bool isWalkableSlope = (floorRay->hitNormal.y >= config_.movement.maxSlopeCos);
        rayHitParam_.groundY = floorRay->crossPoint.y;

        if (isWalkableSlope) {
            const float kGroundEpsilon = 0.05f;

            // 1. 通常の接地（足元が地面付近、またはめり込んでいる場合）
            if (playerBottomY <= rayHitParam_.groundY + kGroundEpsilon && velocity_.y <= 0.0f) {
                isGrounded_ = true;
                isJumping_ = false;
            }
            // 2. 下り坂・段差下り吸着（Ground Snapping）:
            // 直前に接地しており、現在ジャンプ中でなく、地面が下がった距離がスナップ範囲内であれば吸着
            else if (isGrounded_ && !isJumping_ && velocity_.y <= 0.0f &&
                     playerBottomY <= rayHitParam_.groundY + config_.movement.groundSnapDistance) {
                isGrounded_ = true;
            } else {
                isGrounded_ = false;
            }
        } else {
            // 急斜面のため歩行接地は不可
            isGrounded_ = false;
        }
    } else {
        isGrounded_ = false;
        rayHitParam_.groundY = -FLT_MAX;
    }

    // ─── 絶対的地面めり込み防止ガード ─────────────────────────
    // 窪みや急斜面、通常地面に関わらず、足元が地面ポリゴンの上面より下に潜っている場合は、
    // 重力落下による床抜けを絶対に防ぐため、即座にポリゴン上面の高さへ押し上げる！
    if (hitFloor && playerBottomY < floorRay->crossPoint.y) {
        worldY_ = floorRay->crossPoint.y + config_.collider.heightOffset;
        if (velocity_.y < 0.0f) {
            velocity_.y = 0.0f;
        }
        isGrounded_ = true;
        isJumping_ = false;
    }
    // めり込み補正・下り坂吸着補正（接地時に地面の高さに合わせる）
    else if (isGrounded_ && hitFloor) {
        worldY_ = rayHitParam_.groundY + config_.collider.heightOffset;
        velocity_.y = 0.0f;
    }

    // 奈落の最低保証（落下・死の防止処理：既存のコードを維持）
    if (!hitFloor && worldY_ <= rayHitParam_.minY + config_.collider.heightOffset) {
        worldY_ = rayHitParam_.minY + config_.collider.heightOffset;
        velocity_.y = 0.0f;
        isGrounded_ = true;
        isJumping_ = false;
    }
}

void Player::UpdateGravity()
{
    // 落下速度を着地直前判定用に記録
    landingFallSpeed_ = (velocity_.y < 0.0f) ? -velocity_.y : 0.0f;

    // 1. 地面の当たり判定
    CheckGroundCollision();

    // 着地（空中から地面に触れた瞬間）を検知してスクワッシュ（縦潰れ）を発動！
    if (!wasGrounded_ && isGrounded_) {
        const float kMinFallSpeed = 1.2f;
        if (landingFallSpeed_ >= kMinFallSpeed) {
            // 落下速度が大きいほど潰れを強くする（最大1.0）
            float speedFactor = std::clamp((landingFallSpeed_ - kMinFallSpeed) / 12.0f, 0.35f, 1.0f);
            float squashIntensity = -config_.juice.landSquashIntensityMax * speedFactor;
            TriggerSquashStretch(squashIntensity, config_.juice.landSquashDuration);
        }
    }
    wasGrounded_ = isGrounded_;

    // 2. 重力加速度の適用
    if (!isGrounded_) {
        velocity_.y += (config_.movement.gravity * gravityScale_) * deltaTime_;
    } else {
        if (velocity_.y < 0.0f) {
            velocity_.y = 0.0f;
        }
    }

    // 3. 速度による位置更新
    worldY_ += velocity_.y * deltaTime_;
}

void Player::InitializeRays() {
    rayList_.clear();

    CollisionRayInfo floorRay;
    floorRay.name = "Floor";
    // 実際のoriginやdiffは毎フレーム更新
    floorRay.ray.diff = { 0.0f, -10.0f - rayHitParam_.rayOffset, 0.0f };
    rayList_.push_back(floorRay);

    float wallLength = config_.collider.radius + 0.2f;

    CollisionRayInfo frontRay;
    frontRay.name = "FrontWall";
    frontRay.ray.diff = { 0.0f, 0.0f, wallLength };
    rayList_.push_back(frontRay);

    CollisionRayInfo backRay;
    backRay.name = "BackWall";
    backRay.ray.diff = { 0.0f, 0.0f, -wallLength };
    rayList_.push_back(backRay);

    CollisionRayInfo leftRay;
    leftRay.name = "LeftWall";
    leftRay.ray.diff = { -wallLength, 0.0f, 0.0f };
    rayList_.push_back(leftRay);

    CollisionRayInfo rightRay;
    rightRay.name = "RightWall";
    rightRay.ray.diff = { wallLength, 0.0f, 0.0f };
    rayList_.push_back(rightRay);
}

const CollisionRayInfo* Player::GetRayInfo(const std::string& name) const {
    for (const auto& rayInfo : rayList_) {
        if (rayInfo.name == name) {
            return &rayInfo;
        }
    }
    return nullptr;
}

void Player::RayCastUpdate()
{
    UpdateRayCollisions();
}

void Player::UpdateRayCollisions()
{
    if (!scene_) return;

    const std::vector<Triangle>& triangles = scene_->GetTriangle();

    Vector3 center = object_->GetTranslate();

    // 【修正点1】壁レイの発射方向を「現在の進行方向（ワールド絶対）」と「左右方向」にする
    // プレイヤーの回転行列依存にすると衝突時にレイも回転して挙動が不安定になるため、
    // レール進行方向(railDir)を基点にします。
    Vector3 railDir = railMover_->GetCurrentDirection();
    Vector3 forwardDir = (Length(railDir) > 0.001f) ? Normalize(railDir) : Vector3{ 0.0f, 0.0f, 1.0f };

    // ワールドY軸(0,1,0)との外積でプレイヤーの右方向ベクトルを求める
    Vector3 rightDir = Normalize(Cross({ 0.0f, 1.0f, 0.0f }, forwardDir));

    // トンネリング（移動速度によるすり抜け）を確実に防ぐため、内部レイ探索長は十分に確保
    float wallLength = 1.5f;

    // 現在のプレイヤー移動入力方向（1: 前進, -1: 後退, 0: 静止）
    int moveDir = GetMoveDirection();

    // 衝突によるレールの押し戻し量
    float maxPushBackProgress = 0.0f;

    for (auto& rayInfo : rayList_) {
        rayInfo.isColide = false;
        rayInfo.distance = FLT_MAX;
        rayInfo.crossPoint = {};
        rayInfo.hitNormal = {};
        rayInfo.hitTriangle = {};

        // レイの起点と方向を設定
        if (rayInfo.name == "Floor") {
            rayInfo.ray.origin = center;
            rayInfo.ray.origin.y += 1.5f; // 足元から十分高い位置から発射し、急な坂や窪みでも確実に上面を捉える
            rayInfo.ray.diff = { 0.0f, -15.0f, 0.0f };
        } else if (rayInfo.name == "FrontWall") {
            // 足元基準で wallRayHeight（膝〜腰）から水平に発射し、低い階段や段差の壁面も確実に検知
            rayInfo.ray.origin = center;
            rayInfo.ray.origin.y = center.y + config_.collider.wallRayHeight;
            rayInfo.ray.diff = Multiply(wallLength, forwardDir);
        } else if (rayInfo.name == "BackWall") {
            rayInfo.ray.origin = center;
            rayInfo.ray.origin.y = center.y + config_.collider.wallRayHeight;
            rayInfo.ray.diff = Multiply(-wallLength, forwardDir);
        } else if (rayInfo.name == "LeftWall") {
            rayInfo.ray.origin = center;
            rayInfo.ray.origin.y = center.y + config_.collider.wallRayHeight;
            rayInfo.ray.diff = Multiply(-wallLength, rightDir);
        } else if (rayInfo.name == "RightWall") {
            rayInfo.ray.origin = center;
            rayInfo.ray.origin.y = center.y + config_.collider.wallRayHeight;
            rayInfo.ray.diff = Multiply(wallLength, rightDir);
        }

        if (triangles.empty()) continue;

        // Unityスタイルの Physics::RaycastAll を使って候補となる衝突面を距離昇順で取得
        auto hits = Physics::RaycastAll(rayInfo.ray, triangles, 1000.0f, CollisionLayer::All);

        for (const auto& hit : hits) {
            // 【すり抜け足場（OneWay）の判定制御】
            if (hit.triangle.isOneway) {
                // 1. 壁判定（側面・下面）はすり抜け足場を無視（通過）
                if (rayInfo.name != "Floor") {
                    continue;
                }

                // 2. 下層へのすり抜け降下中は床判定を無視
                if (dropThroughTimer_ > 0.0f) {
                    continue;
                }

                // 3. 上昇中（ジャンプで飛び上がる最中）は下から通過するため床判定を無視
                if (velocity_.y > 0.0f) {
                    continue;
                }

                // 4. プレイヤーの足元が足場上面より大幅に下（0.8m以上）にめり込んでいる場合のみ通過
                float playerBottomY = worldY_ - config_.collider.heightOffset;
                if (playerBottomY < hit.point.y - 0.8f) {
                    continue;
                }
            }

            // 床レイ（Floor）は上向きの面（hit.normal.y > 0.0f）のみを検知（下向き面や裏面を排除）
            if (rayInfo.name == "Floor") {
                if (hit.normal.y <= 0.0f) {
                    continue;
                }
            } else {
                // 壁レイは登れる緩やかな地面（hit.normal.y >= maxSlopeCos）のみを除外し、
                // 垂直な壁および登れない急斜面（垂直に近い斜めの床）はすべて壁として検知する！
                if (hit.normal.y >= config_.movement.maxSlopeCos) {
                    continue;
                }
            }

            // 最も近い有効な交差面を採用
            rayInfo.distance = hit.distance;
            rayInfo.crossPoint = hit.point;
            rayInfo.hitNormal = hit.normal;
            rayInfo.hitTriangle = hit.triangle;
            rayInfo.isColide = true;
            break;
        }

        // デバッグ描画: 見た目は黄色の人型モデル幅（modelRadius）に合わせてスマートに表示
        Vector3 drawEnd;
        if (rayInfo.name == "Floor") {
            drawEnd = Add(rayInfo.ray.origin, rayInfo.ray.diff);
            PrimitiveDrawer::GetInstance()->DrawLine(rayInfo.ray.origin, drawEnd,
                rayInfo.isColide ? Vector4{ 1,0,0,1 } : Vector4{ 0,1,0,1 });
        } else {
            // 壁レイはモデル幅（+0.05m）の長さでコンパクトに描画
            float drawLen = config_.collider.modelRadius + 0.05f;
            Vector3 dir = Normalize(rayInfo.ray.diff);
            drawEnd = Add(rayInfo.ray.origin, Multiply(drawLen, dir));
            bool isWallNear = rayInfo.isColide && (rayInfo.distance <= drawLen);
            PrimitiveDrawer::GetInstance()->DrawLine(rayInfo.ray.origin, drawEnd,
                isWallNear ? Vector4{ 1,0,0,1 } : Vector4{ 0,1,0,1 });
        }

        if (rayInfo.isColide) {
            // 【修正点3】人型モデル実寸幅に合わせた押し戻し（モデルの表面が壁にピッタリ密着）
            float targetWallDist = config_.collider.modelRadius + 0.005f;
            if (rayInfo.name == "FrontWall") {
                // 前進中（moveDir >= 0）に前方の壁にぶつかったら進行方向と逆（後方）へ押し戻して止める
                if (moveDir >= 0) {
                    float penetration = targetWallDist - rayInfo.distance;
                    if (penetration > 0.0f) {
                        penetration = (std::min)(penetration, 0.15f); // 瞬間的な過剰押し出しを防止
                        maxPushBackProgress = (std::max)(maxPushBackProgress, penetration);
                    }
                }
            } else if (rayInfo.name == "BackWall") {
                // 後退中（moveDir < 0）に後方の壁にぶつかったら進行方向（前方）へ押し戻して止める
                // 前進中や停止中に背後の壁から前方に急激に突き飛ばされるのを完全に防止
                if (moveDir < 0) {
                    float penetration = targetWallDist - rayInfo.distance;
                    if (penetration > 0.0f) {
                        penetration = (std::min)(penetration, 0.15f); // 瞬間的な過剰押し出しを防止
                        maxPushBackProgress = (std::min)(maxPushBackProgress, -penetration);
                    }
                }
            }

            // モデル幅以内に接触している時だけヒット球を表示
            if (rayInfo.distance <= (config_.collider.modelRadius + 0.05f)) {
                PrimitiveDrawer::GetInstance()->DrawSphere({ rayInfo.crossPoint, 0.05f, {} }, { 0,0,1,1 });
            }
        }
    }

    // ─── 段差ステップアップ（Step-up / 階段・段差登り）処理 ──────────
    // プレイヤーが接地しており、ジャンプ中でない場合、足元の小さな段差（階段正面など）をスムーズに乗り越える
    if (isGrounded_ && !isJumping_ && !triangles.empty()) {

        // 階段の側面（横壁）からぶつかった場合に乗り上げてしまう不具合の防止：
        // 前方/後方壁レイ（FrontWall / BackWall）または側壁レイ（LeftWall / RightWall）が壁に衝突している場合、
        // それは乗り越えるべき小さなステップではなく「壁」なので、ステップアップを禁止する！
        bool isWallBlocked = false;
        if (moveDir > 0) {
            const auto* fw = GetRayInfo("FrontWall");
            if (fw && fw->isColide && fw->distance <= (config_.collider.modelRadius + 0.15f)) {
                isWallBlocked = true;
            }
        } else if (moveDir < 0) {
            const auto* bw = GetRayInfo("BackWall");
            if (bw && bw->isColide && bw->distance <= (config_.collider.modelRadius + 0.15f)) {
                isWallBlocked = true;
            }
        }

        // 側面の壁に強く接触している場合も階段側面からのよじ登りを禁止
        const auto* lw = GetRayInfo("LeftWall");
        const auto* rw = GetRayInfo("RightWall");
        if ((lw && lw->isColide && lw->distance <= (config_.collider.modelRadius + 0.10f)) ||
            (rw && rw->isColide && rw->distance <= (config_.collider.modelRadius + 0.10f))) {
            isWallBlocked = true;
        }

        if (moveDir != 0 && !isWallBlocked) {
            Vector3 stepForward = (moveDir > 0) ? forwardDir : Multiply(-1.0f, forwardDir);
            float stepCheckDist = config_.collider.modelRadius + 0.05f; // モデル前方検知距離

            Ray stepRay;
            stepRay.origin = Add(center, Multiply(stepCheckDist, stepForward));
            float playerBottomY = worldY_ - config_.collider.heightOffset;
            stepRay.origin.y = playerBottomY + config_.movement.maxStepHeight + 0.05f;
            stepRay.diff = { 0.0f, -(config_.movement.maxStepHeight + 0.2f), 0.0f };

            RaycastHit stepHit;
            // 地面（歩行可能面）レイヤーを対象に Raycast
            bool hitStep = Physics::Raycast(stepRay, triangles, &stepHit, config_.movement.maxStepHeight + 0.2f, CollisionLayer::Ground | CollisionLayer::OneWay);
            // 登れる緩やかな上面（法線角度制限）のみ段差として認識
            if (hitStep && stepHit.normal.y >= config_.movement.maxSlopeCos) {
                float stepHeight = stepHit.point.y - playerBottomY;
                // 足元より高く、かつ許容段差高さ以内であればステップアップ
                if (stepHeight > 0.02f && stepHeight <= config_.movement.maxStepHeight) {
                    worldY_ = stepHit.point.y + config_.collider.heightOffset;
                    // ステップアップ時は壁の押し戻しを無効化してスムーズに登らせる
                    maxPushBackProgress = 0.0f;
                }
            }
        }
    }

    // ─── 垂直に近い斜めの床（登れない急斜面）への前進阻止 ──────────
    // 足元の床レイが急斜面（maxSlopeCos未満）に触れており、その斜面に向かって前進しようとしている場合、
    // 斜面を壁とみなして前進を押し戻し・停止する
    auto floorRayCheck = GetRayInfo("Floor");
    if (floorRayCheck && floorRayCheck->isColide && floorRayCheck->hitNormal.y < config_.movement.maxSlopeCos && floorRayCheck->hitNormal.y > 0.0f) {
        float dotDir = Dot(floorRayCheck->hitNormal, forwardDir) * float(moveDir);
        if (dotDir < -0.05f) { // プレイヤーが急斜面に正面からぶつかって前進している
            float penetration = 0.05f;
            if (moveDir >= 0) {
                maxPushBackProgress = (std::max)(maxPushBackProgress, penetration);
            } else {
                maxPushBackProgress = (std::min)(maxPushBackProgress, -penetration);
            }
        }
    }

    // 【修正点4】めり込んだ分だけレールの進捗（Progress）を戻す
    if (std::abs(maxPushBackProgress) > 0.0001f) {
        // 壁にぶつかったら押し戻す（Advance）のではなく、壁の手前の座標を計算して直接セットし「止まる」挙動にする
        float currentDist = railMover_->GetCurrentDistance();
        float targetDist = currentDist - maxPushBackProgress;
        const RailPath* path = railMover_->GetRailPath();
        if (path) {
            float targetT = path->GetTFromDistance(targetDist);
            railMover_->SetProgress(targetT);
        }
    }

    // レール上ゲームのため、位置自体の直接的なオフセット加算は行わない（向きが狂う原因になるため）
    wallPushOffset_ = { 0.0f, 0.0f, 0.0f };

    // 地面判定の更新
    auto floorRay = GetRayInfo("Floor");
    if (floorRay) {
        SetRayHit(floorRay->isColide);
        SetRayHitDistance(floorRay->distance);
        SetRayHitPoint(floorRay->crossPoint);
        SetRayHitTriangle(floorRay->hitTriangle);
        SetRayCollisionResult(RayTriangleCollisionResult::FrontFace);
        isCurrentGroundOneway_ = (floorRay->isColide && floorRay->hitTriangle.isOneway);
    } else {
        isCurrentGroundOneway_ = false;
    }
}
void Player::HandleInput()
{
    // 毎フレームの入力処理開始時に移動フラグ・ダッシュフラグをリセット
    isDashing_ = false;
    isMoving_ = false;

    // ノックバック中は操作不能にする
    if (isKnockback_) {
        return;
    }

    auto commands = inputHandler_->HandleInput();
    for (auto& command : commands) {
        if (baseState_) {
            baseState_->HandleInput(this, command.get());
        }
    }
}

void Player::HandleDamage()
{
    if (hitInvincibilityTimer_ > 0.0f) {
        hitInvincibilityTimer_ -= deltaTime_;
        if (hitInvincibilityTimer_ <= 0.0f) {
            hitInvincibilityTimer_ = 0.0f;
            isDamaged_ = false;
        }
    }
}

void Player::HandleKnockback()
{
    if (isKnockback_ && knockbackTimer_ > 0.0f) {
        // ノックバックの残り時間に応じて速度を減衰
        float progressRatio = knockbackTimer_ / config_.combat.knockbackDuration;
        float moveAmount = float(knockbackDirection_) * config_.combat.knockbackSpeed * progressRatio * deltaTime_;
        railMover_->Advance(moveAmount);

        // Advance内でcurrentDir_が反転してしまわないよう、保存した移動向きを維持
        railMover_->SetMoveDirection(savedFacingDirection_);

        knockbackTimer_ -= deltaTime_;
        if (knockbackTimer_ <= 0.0f) {
            knockbackTimer_ = 0.0f;
            isKnockback_ = false;
        }
    }
}

void Player::TakeDamage(int knockbackDirection)
{
    if (hitInvincibilityTimer_ <= 0.0f && !isInvincible_) {
        isDamaged_ = true;
        hitInvincibilityTimer_ = config_.combat.hitInvincibilityDuration;
        hitPoints_.value--;

        // ノックバック開始
        isKnockback_ = true;
        knockbackTimer_ = config_.combat.knockbackDuration;
        knockbackDirection_ = knockbackDirection;
        velocity_.y = config_.combat.knockbackJumpForce;
        isGrounded_ = false;

        // ノックバック前の移動向きを保存
        if (railMover_) {
            savedFacingDirection_ = railMover_->GetMoveDirection();
        }

        // アタック中などの特殊状態を解除して通常行動に戻す
        if (baseState_ && baseState_->GetFactory()) {
            baseState_->ChangeBehavior(this, baseState_->GetFactory()->CreateBehavior(BehaviorType::Root));
        }
    }
}

void Player::HandleAlive()
{
    if (hitPoints_.value <= 0) {
        isAlive_ = false;
    }
}

void Player::ImGuiDrawDebugInfo() {
#ifdef USE_IMGUI
    ImGui::Begin("Debug/Player");
    ImGui::Text("Rail Progress: %.2f", railMover_->GetProgress());
    Vector3 pos = object_->GetTranslate();
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);

    ImGui::Separator();
    ImGui::Text("--- Player States ---");

    if (baseState_) {
        ImGui::Text("Base State: %s", baseState_->GetName());
    } else {
        ImGui::Text("Base State: None");
    }

    if (baseState_ && baseState_->GetBehavior()) {
        ImGui::Text("Behavior: %s", baseState_->GetBehavior()->GetName());
    } else {
        ImGui::Text("Behavior: None");
    }

    const char* moveStateStr = "Idle";
    if (moveState_ == MoveState::Walk) moveStateStr = "Walk";
    else if (moveState_ == MoveState::Dash) moveStateStr = "Dash (Running)";
    ImGui::Text("Move State: %s (Dashing: %s)", moveStateStr, isDashing_ ? "YES" : "NO");
    ImGui::Text("Current Speed: %.1f m/s", isDashing_ ? config_.movement.dashSpeed : config_.movement.walkSpeed);
    ImGui::Separator();

    if (isKnockback_) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "STATUS: KNOCKBACK! (Timer: %.2f)", knockbackTimer_);
    } else if (isDamaged_) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "STATUS: INVINCIBLE / DAMAGED!");
    } else {
        ImGui::Text("STATUS: Normal");
    }

    ImGui::Text("Hit Points: %d", hitPoints_.value);
    ImGui::ProgressBar(hitInvincibilityTimer_ / config_.combat.hitInvincibilityDuration, ImVec2(0, 0), "Hit Timer");

    ImGui::Separator();
    ImGui::Text("--- Config File Actions ---");
    if (ImGui::Button("💾 Save Config to JSON")) {
        SaveConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("🔄 Reload from JSON")) {
        LoadConfig();
        ApplyColliderConfig();
    }

    ImGui::Separator();
    ImGui::Text("--- Movement Parameters ---");
    ImGui::SliderFloat("Walk Speed", &config_.movement.walkSpeed, 1.0f, 20.0f, "%.1f m/s");
    ImGui::SliderFloat("Dash Speed", &config_.movement.dashSpeed, 2.0f, 40.0f, "%.1f m/s");
    ImGui::SliderFloat("Gravity", &config_.movement.gravity, -120.0f, -10.0f, "%.1f");
    ImGui::SliderFloat("Jump Accel", &config_.movement.jumpAcceleration, 5.0f, 50.0f, "%.1f");
    ImGui::SliderFloat("Coyote Time", &config_.movement.coyoteDuration, 0.01f, 0.5f, "%.2f s");
    ImGui::SliderFloat("Jump Buffer", &config_.movement.jumpBufferDuration, 0.01f, 0.5f, "%.2f s");
    ImGui::SliderFloat("Max Step Height", &config_.movement.maxStepHeight, 0.05f, 1.0f, "%.2f m");
    ImGui::SliderFloat("Ground Snap Dist", &config_.movement.groundSnapDistance, 0.1f, 3.0f, "%.2f m");
    ImGui::SliderFloat("Max Slope Cos", &config_.movement.maxSlopeCos, 0.2f, 0.95f, "%.2f");

    ImGui::Separator();
    ImGui::Text("--- Combat Parameters ---");
    ImGui::SliderFloat("Invincibility Duration", &config_.combat.hitInvincibilityDuration, 0.1f, 5.0f, "%.2f s");
    ImGui::SliderFloat("Knockback Duration", &config_.combat.knockbackDuration, 0.05f, 1.0f, "%.2f s");
    ImGui::SliderFloat("Knockback Speed", &config_.combat.knockbackSpeed, 1.0f, 25.0f, "%.1f m/s");
    ImGui::SliderFloat("Knockback Jump Force", &config_.combat.knockbackJumpForce, 1.0f, 20.0f, "%.1f");

    ImGui::Separator();
    ImGui::Text("--- Squash & Stretch (Juice) ---");
    ImGui::SliderFloat("Jump Stretch", &config_.juice.jumpStretchIntensity, 0.05f, 0.60f, "%.2f");
    ImGui::SliderFloat("Jump Duration", &config_.juice.jumpStretchDuration, 0.05f, 0.50f, "%.2f s");
    ImGui::SliderFloat("Land Squash Max", &config_.juice.landSquashIntensityMax, 0.05f, 0.60f, "%.2f");
    ImGui::SliderFloat("Land Duration", &config_.juice.landSquashDuration, 0.05f, 0.50f, "%.2f s");
    if (ImGui::Button("Test Jump Stretch")) {
        TriggerSquashStretch(config_.juice.jumpStretchIntensity, config_.juice.jumpStretchDuration);
    }
    ImGui::SameLine();
    if (ImGui::Button("Test Land Squash")) {
        TriggerSquashStretch(-config_.juice.landSquashIntensityMax, config_.juice.landSquashDuration);
    }

    ImGui::Separator();
    ImGui::Text("--- Ground & Raycast Settings ---");
    ImGui::SliderFloat("Height Offset", &config_.collider.heightOffset, -0.5f, 1.0f, "%.3f m");
    ImGui::SliderFloat("Wall Ray Height", &config_.collider.wallRayHeight, 0.1f, 1.2f, "%.3f m");
    ImGui::SliderFloat("Model Wall Radius", &config_.collider.modelRadius, 0.05f, 1.0f, "%.3f m");

    ImGui::Separator();
    ImGui::Text("--- Sphere Collider Settings ---");
    if (ImGui::SliderFloat("Collider Radius", &config_.collider.radius, 0.1f, 1.5f, "%.3f m") ||
        ImGui::SliderFloat("Head Offset Y", &config_.collider.headOffsetY, 0.5f, 2.0f, "%.3f m") ||
        ImGui::SliderFloat("Head Radius", &config_.collider.headRadius, 0.1f, 0.8f, "%.3f m") ||
        ImGui::SliderFloat("Body Offset Y", &config_.collider.bodyOffsetY, 0.1f, 1.5f, "%.3f m") ||
        ImGui::SliderFloat("Body Radius", &config_.collider.bodyRadius, 0.1f, 0.8f, "%.3f m")) {
        ApplyColliderConfig();
    }

    ImGui::Separator();
    ImGui::Text("--- Attack Hitbox Settings ---");
    ImGui::Checkbox("Debug: Force Enable Hitbox", &debugForceAttackHitbox_);
    ImGui::Text("Active: %s", IsAttackHitboxActive() ? "YES (Attacking!)" : "NO");
    if (ImGui::SliderFloat("Attack Offset Y", &config_.collider.attackOffsetY, 0.1f, 1.5f, "%.3f m") ||
        ImGui::SliderFloat("Attack Offset Forward", &config_.collider.attackOffsetForward, 0.1f, 1.5f, "%.3f m") ||
        ImGui::SliderFloat("Attack Radius", &config_.collider.attackRadius, 0.1f, 1.0f, "%.3f m")) {
        ApplyColliderConfig();
    }

    ImGui::Separator();
    ImGui::Text("--- Raycast Info ---");
    for (const auto& rayInfo : rayList_) {
        if (rayInfo.isColide) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[%s] Hit! Dist: %.2f", rayInfo.name.c_str(), rayInfo.distance);
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s] No Hit", rayInfo.name.c_str());
        }
    }

    ImGui::End();
#endif
}

void Player::ChangeState(std::unique_ptr<IPlayerState> newState) {
    if (baseState_) baseState_->Finalize(this);
    baseState_ = std::move(newState);
    if (baseState_) baseState_->Initialize(this);
}

void Player::ChangeBehavior(std::unique_ptr<IPlayerBehavior> newBehavior) {
    if (baseState_) {
        baseState_->ChangeBehavior(this, std::move(newBehavior));
    }
}

void Player::OnCollision(GameObject* other) {
    if (!other || isInvincible_) return;

    if (other->GetCategory() == CollisionCategory::Enemy) {
        Enemy* enemy = dynamic_cast<Enemy*>(other);
        if (!enemy) return;

        // プレイヤーが攻撃中の場合
        if (IsAttacking()) {
            if (!enemy->IsDead()) {
                OnAttackHit(enemy);
            }
            return;
        }

        // 敵が通常状態の場合、ダメージおよびノックバック
        if (enemy->CanDamagePlayer()) {
            if (hitInvincibilityTimer_ <= 0.0f) {
                // レール上の位置関係からノックバック方向を決定
                int knockDir = -1;
                float playerDist = GetCurrentDistance();
                float enemyDist = enemy->GetCurrentDistance();
                if (playerDist < enemyDist) {
                    knockDir = -1; // プレイヤーが手前 → 手前側(後退)へノックバック
                } else if (playerDist > enemyDist) {
                    knockDir = 1;  // プレイヤーが奥 → 奥側(前進)へノックバック
                } else {
                    // 同一地点の場合はプレイヤーの移動向きの逆方向
                    knockDir = (GetMoveDirection() == 1) ? -1 : 1;
                }
                playHundle_ = Audio::GetInstance()->PlayAudio(DamageSE_, false, 0.75);

                if (scene_)
                {
                    scene_->RequestCameraShake();
                }

                TakeDamage(knockDir);
            }
        }

    }
}

Vector3 Player::GetDirection() const {
    return railMover_->GetCurrentDirection();
}
int Player::GetMoveDirection() const {
    return int(railMover_->GetMoveDirection());
}

const char* Player::GetStateName() const {
    return baseState_ ? baseState_->GetName() : "NoState";
}

const char* Player::GetBehaviorName() const {
    return (baseState_ && baseState_->GetBehavior()) ? baseState_->GetBehavior()->GetName() : "NoBehavior";
}

void Player::SetScene(Scene* scene) {
    scene_ = scene;
}

void Player::SetAttackHitboxActive(bool active) {
    isAttackHitboxActive_ = active;
    if (attackCollider_) {
        attackCollider_->SetCollide(IsAttackHitboxActive());
    }
}

bool Player::IsAttackHitboxActive() const {
    return isAttackHitboxActive_ || debugForceAttackHitbox_;
}

bool Player::IsAttacking() const {
    if (baseState_ && baseState_->GetBehavior()) {
        return baseState_->GetBehavior()->IsAttacking();
    }
    return false;
}

void Player::OnAttackHit(GameObject* target) {
    velocity_.x = 0.0f;
    velocity_.z = 0.0f;

    // アタック成功時のBehavior復帰
    if (baseState_ && baseState_->GetFactory()) {
        baseState_->ChangeBehavior(this, baseState_->GetFactory()->CreateBehavior(BehaviorType::Root));
    }
    TriggerInvincibility(0.4f); // 撃破後0.4秒間の無敵余韻を付与
    SetAttackHitboxActive(false); // ヒット後は直ちに攻撃判定を解除
}

void Player::TriggerSquashStretch(float intensityY, float duration) {
    squashStretch_.intensityY = intensityY;
    squashStretch_.duration = duration;
    squashStretch_.timer = 0.0f;
    squashStretch_.isActive = true;
}

void Player::UpdateSquashStretch() {
    if (!object_) return;

    float currentOffsetY = 0.0f;

    if (squashStretch_.isActive && squashStretch_.duration > 0.0f) {
        squashStretch_.timer += deltaTime_;
        float progress = squashStretch_.timer / squashStretch_.duration;

        if (progress >= 1.0f) {
            squashStretch_.isActive = false;
            currentOffsetY = 0.0f;
        } else {
            // 減衰サイン波（ゴムのようにポヨンと揺れ戻って収束）
            float damp = expf(-4.2f * progress);
            float wave = cosf(progress * std::numbers::pi_v<float> * 2.5f) * damp;
            currentOffsetY = squashStretch_.intensityY * wave;
        }
    } else if (!isGrounded_ && velocity_.y < -6.0f) {
        // 空中高速落下中の風圧ストレッチ（微小な縦伸び予兆）
        float fallFactor = std::clamp((-velocity_.y - 6.0f) / 15.0f, 0.0f, 1.0f);
        currentOffsetY = 0.10f * fallFactor;
    }

    // 体積保存則: ScaleY が変化した分、ScaleX と ScaleZ を反比例させて体積を一定に保つ
    float targetScaleY = (std::max)(1.0f + currentOffsetY, 0.35f);
    float targetScaleXZ = sqrtf(1.0f / targetScaleY);

    Vector3 finalScale = {
        baseScale_.x * targetScaleXZ,
        baseScale_.y * targetScaleY,
        baseScale_.z * targetScaleXZ
    };
    object_->SetScale(finalScale);
}