#include "Player.h"
#include "EffectManager.h"
#include "Enemy.h"
#include "GameScene.h"
#include "Input.h"
#include "InputHandler.h"
#include "JsonManager.h"
#include "Logger.h"
#include "ModelManager.h"
#include "Object3D.h"
#include "Physics.h"
#include "PlayerBehavior.h"
#include "PlayerState.h"
#include "PrimitiveDrawer.h"
#include "RailMover.h"
#include "RailPath.h"
#include "Scene.h"
#include "collider.h"
#include "imgui.h"
#include <numbers>

Player::Player() = default;
Player::~Player() = default;

void Player::LoadConfig(const std::string &filePath) {
  auto jsonMgr = JsonManager::GetInstance();
  if (!jsonMgr->Load("PlayerConfig", filePath)) {
    Logger::Log("[Player] Could not open " + filePath +
                ". Using default config.\n");
    return;
  }

  const auto &j = jsonMgr->Get("PlayerConfig");

  // Paths
  if (j.contains("paths")) {
    const auto &jp = j["paths"];
    config_.paths.modelDir = jp.value("modelDir", config_.paths.modelDir);
    config_.paths.modelFile = jp.value("modelFile", config_.paths.modelFile);
    config_.paths.texture = jp.value("texture", config_.paths.texture);
    config_.paths.hitSE = jp.value("hitSE", config_.paths.hitSE);
    config_.paths.damageSE = jp.value("damageSE", config_.paths.damageSE);
  }

  // Movement
  if (j.contains("movement")) {
    const auto &jm = j["movement"];
    config_.movement.walkSpeed =
        jm.value("walkSpeed", config_.movement.walkSpeed);
    config_.movement.dashSpeed =
        jm.value("dashSpeed", config_.movement.dashSpeed);
    config_.movement.gravity = jm.value("gravity", config_.movement.gravity);
    config_.movement.jumpAcceleration =
        jm.value("jumpAcceleration", config_.movement.jumpAcceleration);
    config_.movement.coyoteDuration =
        jm.value("coyoteDuration", config_.movement.coyoteDuration);
    config_.movement.jumpBufferDuration =
        jm.value("jumpBufferDuration", config_.movement.jumpBufferDuration);
    config_.movement.maxSlopeCos =
        jm.value("maxSlopeCos", config_.movement.maxSlopeCos);
    config_.movement.maxStepHeight =
        jm.value("maxStepHeight", config_.movement.maxStepHeight);
    config_.movement.groundSnapDistance =
        jm.value("groundSnapDistance", config_.movement.groundSnapDistance);
    config_.movement.landingStaggerDuration = jm.value(
        "landingStaggerDuration", config_.movement.landingStaggerDuration);
    config_.movement.landingStaggerMinFallSpeed =
        jm.value("landingStaggerMinFallSpeed",
                 config_.movement.landingStaggerMinFallSpeed);
    config_.movement.landingSpeedMultiplier = jm.value(
        "landingSpeedMultiplier", config_.movement.landingSpeedMultiplier);
  }

  // Combat
  if (j.contains("combat")) {
    const auto &jc = j["combat"];
    config_.combat.hitInvincibilityDuration = jc.value(
        "hitInvincibilityDuration", config_.combat.hitInvincibilityDuration);
    config_.combat.knockbackDuration =
        jc.value("knockbackDuration", config_.combat.knockbackDuration);
    config_.combat.knockbackSpeed =
        jc.value("knockbackSpeed", config_.combat.knockbackSpeed);
    config_.combat.knockbackJumpForce =
        jc.value("knockbackJumpForce", config_.combat.knockbackJumpForce);
  }

  // Juice
  if (j.contains("juice")) {
    const auto &jj = j["juice"];
    config_.juice.jumpStretchIntensity =
        jj.value("jumpStretchIntensity", config_.juice.jumpStretchIntensity);
    config_.juice.jumpStretchDuration =
        jj.value("jumpStretchDuration", config_.juice.jumpStretchDuration);
    config_.juice.landSquashIntensityMax = jj.value(
        "landSquashIntensityMax", config_.juice.landSquashIntensityMax);
    config_.juice.landSquashDuration =
        jj.value("landSquashDuration", config_.juice.landSquashDuration);
  }

  // Collider
  if (j.contains("collider")) {
    const auto &jc = j["collider"];
    config_.collider.radius = jc.value("radius", config_.collider.radius);
    config_.collider.modelRadius =
        jc.value("modelRadius", config_.collider.modelRadius);
    config_.collider.heightOffset =
        jc.value("heightOffset", config_.collider.heightOffset);
    config_.collider.wallRayHeight =
        jc.value("wallRayHeight", config_.collider.wallRayHeight);

    if (jc.contains("head")) {
      config_.collider.headOffsetY =
          jc["head"].value("offsetY", config_.collider.headOffsetY);
      config_.collider.headRadius =
          jc["head"].value("radius", config_.collider.headRadius);
    }
    if (jc.contains("body")) {
      config_.collider.bodyOffsetY =
          jc["body"].value("offsetY", config_.collider.bodyOffsetY);
      config_.collider.bodyRadius =
          jc["body"].value("radius", config_.collider.bodyRadius);
    }
    if (jc.contains("attack")) {
      const auto &ja = jc["attack"];
      config_.collider.attackOffsetY =
          ja.value("offsetY", config_.collider.attackOffsetY);
      config_.collider.attackOffsetForward =
          ja.value("offsetForward", config_.collider.attackOffsetForward);
      config_.collider.attackRadius =
          ja.value("radius", config_.collider.attackRadius);
      if (ja.contains("halfSize") && ja["halfSize"].is_array() &&
          ja["halfSize"].size() >= 3) {
        config_.collider.attackHalfSize = {ja["halfSize"][0].get<float>(),
                                           ja["halfSize"][1].get<float>(),
                                           ja["halfSize"][2].get<float>()};
      }
    }
  }

  // Slide
  if (j.contains("slide")) {
    const auto &js = j["slide"];
    config_.slide.duration = js.value("duration", config_.slide.duration);
    config_.slide.initialSpeed =
        js.value("initialSpeed", config_.slide.initialSpeed);
    config_.slide.invincibilityAfter =
        js.value("invincibilityAfter", config_.slide.invincibilityAfter);
  }

  // Hover
  if (j.contains("hover")) {
    const auto &jh = j["hover"];
    config_.hover.maxDuration =
        jh.value("maxDuration", config_.hover.maxDuration);
    config_.hover.fallSpeed = jh.value("fallSpeed", config_.hover.fallSpeed);
    config_.hover.initialBoostY =
        jh.value("initialBoostY", config_.hover.initialBoostY);
  }

  // Guard
  if (j.contains("guard")) {
    const auto &jg = j["guard"];
    config_.guard.maxDuration =
        jg.value("maxDuration", config_.guard.maxDuration);
    config_.guard.bashDuration =
        jg.value("bashDuration", config_.guard.bashDuration);
    config_.guard.bashSpeed = jg.value("bashSpeed", config_.guard.bashSpeed);
    config_.guard.invincibilityAfter =
        jg.value("invincibilityAfter", config_.guard.invincibilityAfter);
  }

  // AttackAction
  if (j.contains("attackAction")) {
    const auto &ja = j["attackAction"];
    config_.attackAction.duration =
        ja.value("duration", config_.attackAction.duration);
    config_.attackAction.windupDuration =
        ja.value("windupDuration", config_.attackAction.windupDuration);
    config_.attackAction.rushDuration =
        ja.value("rushDuration", config_.attackAction.rushDuration);
    config_.attackAction.recoveryDuration =
        ja.value("recoveryDuration", config_.attackAction.recoveryDuration);
    config_.attackAction.rushSpeedMultiplier = ja.value(
        "rushSpeedMultiplier", config_.attackAction.rushSpeedMultiplier);
    config_.attackAction.invincibilityAfter =
        ja.value("invincibilityAfter", config_.attackAction.invincibilityAfter);
  }

  // RapidFire
  if (j.contains("rapidFire")) {
    const auto &jrf = j["rapidFire"];
    config_.rapidFire.fireInterval =
        jrf.value("fireInterval", config_.rapidFire.fireInterval);
    config_.rapidFire.bulletSpeed =
        jrf.value("bulletSpeed", config_.rapidFire.bulletSpeed);
    config_.rapidFire.bulletSpreadY =
        jrf.value("bulletSpreadY", config_.rapidFire.bulletSpreadY);
  }

  // DropThrough
  if (j.contains("dropThrough")) {
    const auto &jd = j["dropThrough"];
    config_.dropThrough.duration =
        jd.value("duration", config_.dropThrough.duration);
    config_.dropThrough.downVelocity =
        jd.value("downVelocity", config_.dropThrough.downVelocity);
  }
}

void Player::SaveConfig(const std::string &filePath) {
  nlohmann::json j;

  j["paths"] = {{"modelDir", config_.paths.modelDir},
                {"modelFile", config_.paths.modelFile},
                {"texture", config_.paths.texture},
                {"hitSE", config_.paths.hitSE},
                {"damageSE", config_.paths.damageSE}};

  j["movement"] = {
      {"walkSpeed", config_.movement.walkSpeed},
      {"dashSpeed", config_.movement.dashSpeed},
      {"gravity", config_.movement.gravity},
      {"jumpAcceleration", config_.movement.jumpAcceleration},
      {"coyoteDuration", config_.movement.coyoteDuration},
      {"jumpBufferDuration", config_.movement.jumpBufferDuration},
      {"maxSlopeCos", config_.movement.maxSlopeCos},
      {"maxStepHeight", config_.movement.maxStepHeight},
      {"groundSnapDistance", config_.movement.groundSnapDistance},
      {"landingStaggerDuration", config_.movement.landingStaggerDuration},
      {"landingStaggerMinFallSpeed",
       config_.movement.landingStaggerMinFallSpeed},
      {"landingSpeedMultiplier", config_.movement.landingSpeedMultiplier}};

  j["combat"] = {
      {"hitInvincibilityDuration", config_.combat.hitInvincibilityDuration},
      {"knockbackDuration", config_.combat.knockbackDuration},
      {"knockbackSpeed", config_.combat.knockbackSpeed},
      {"knockbackJumpForce", config_.combat.knockbackJumpForce}};

  j["juice"] = {
      {"jumpStretchIntensity", config_.juice.jumpStretchIntensity},
      {"jumpStretchDuration", config_.juice.jumpStretchDuration},
      {"landSquashIntensityMax", config_.juice.landSquashIntensityMax},
      {"landSquashDuration", config_.juice.landSquashDuration}};

  j["collider"] = {
      {"radius", config_.collider.radius},
      {"modelRadius", config_.collider.modelRadius},
      {"heightOffset", config_.collider.heightOffset},
      {"wallRayHeight", config_.collider.wallRayHeight},
      {"head",
       {{"offsetY", config_.collider.headOffsetY},
        {"radius", config_.collider.headRadius}}},
      {"body",
       {{"offsetY", config_.collider.bodyOffsetY},
        {"radius", config_.collider.bodyRadius}}},
      {"attack",
       {{"offsetY", config_.collider.attackOffsetY},
        {"offsetForward", config_.collider.attackOffsetForward},
        {"radius", config_.collider.attackRadius},
        {"halfSize",
         {config_.collider.attackHalfSize.x, config_.collider.attackHalfSize.y,
          config_.collider.attackHalfSize.z}}}}};

  j["slide"] = {{"duration", config_.slide.duration},
                {"initialSpeed", config_.slide.initialSpeed},
                {"invincibilityAfter", config_.slide.invincibilityAfter}};

  j["hover"] = {{"maxDuration", config_.hover.maxDuration},
                {"fallSpeed", config_.hover.fallSpeed},
                {"initialBoostY", config_.hover.initialBoostY}};

  j["guard"] = {{"maxDuration", config_.guard.maxDuration},
                {"bashDuration", config_.guard.bashDuration},
                {"bashSpeed", config_.guard.bashSpeed},
                {"invincibilityAfter", config_.guard.invincibilityAfter}};

  j["attackAction"] = {
      {"duration", config_.attackAction.duration},
      {"windupDuration", config_.attackAction.windupDuration},
      {"rushDuration", config_.attackAction.rushDuration},
      {"recoveryDuration", config_.attackAction.recoveryDuration},
      {"rushSpeedMultiplier", config_.attackAction.rushSpeedMultiplier},
      {"invincibilityAfter", config_.attackAction.invincibilityAfter}};

  j["rapidFire"] = {{"fireInterval", config_.rapidFire.fireInterval},
                    {"bulletSpeed", config_.rapidFire.bulletSpeed},
                    {"bulletSpreadY", config_.rapidFire.bulletSpreadY}};

  j["dropThrough"] = {{"duration", config_.dropThrough.duration},
                      {"downVelocity", config_.dropThrough.downVelocity}};

  JsonManager::GetInstance()->Save(filePath, j);
}

void Player::ApplyColliderConfig() {
  if (auto *col = GetCollider()) {
    col->initialize(this, config_.collider.radius);
    col->SetOffset({0.0f, 0.0f, 0.0f});
    col->ClearSpheres();
    col->AddSphere("Body", {0.0f, config_.collider.bodyOffsetY, 0.0f},
                   config_.collider.radius);
  }
  if (attackCollider_) {
    attackCollider_->initialize(this, config_.collider.attackRadius);
    attackCollider_->SetCategory(CollisionCategory::PlayerAttack);
    attackCollider_->SetCollide(isAttackHitboxActive_);
    attackCollider_->ClearSpheres();
    attackCollider_->ClearAABBs();
    attackCollider_->AddAABB("Attack",
                             {0.0f, config_.collider.attackOffsetY,
                              config_.collider.attackOffsetForward},
                             config_.collider.attackHalfSize);
  }
}

void Player::Initialize() {
  // JSON設定の読み込み
  LoadConfig();

  inputHandler_ = std::make_unique<InputHandler>();
  object_ = std::make_unique<Object3d>();
  ModelManager::GetInstance()->LoadModel(config_.paths.modelDir, config_.paths.modelFile);
  object_->Initialize();
  object_->SetModel(config_.paths.modelFile);
  object_->SetTexture(config_.paths.texture);
  /*   animation = std::make_unique<Animation>();

     animation->Initialize(config_.paths.modelDir, config_.paths.modelFile);
     animation->SetCurrentTime(0.0f);
     object_->SetAnimations(animation.get());*/

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

  lastGroundY_ = worldY_ - config_.collider.heightOffset;
  hitPoints_.value = hitPoints_.max;

  HitSE_ = Audio::GetInstance()->LoadAudio(config_.paths.hitSE);
  DamageSE_ = Audio::GetInstance()->LoadAudio(config_.paths.damageSE);
}
void Player::Update() {
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

  // 着地怯み（硬直）タイマーの減衰
  if (landingRecoveryTimer_ > 0.0f) {
    landingRecoveryTimer_ -= deltaTime_;
    if (landingRecoveryTimer_ < 0.0f) {
      landingRecoveryTimer_ = 0.0f;
    }
  }

  // 先行入力タイマーの減衰（着地怯み中はホールドして立ち上がりジャンプを保証）
  if (jumpBufferTimer_ > 0.0f && !IsLandingStaggered()) {
    jumpBufferTimer_ -= deltaTime_;
    if (jumpBufferTimer_ < 0.0f) {
      jumpBufferTimer_ = 0.0f;
    }
  }

  // 連射クールダウン＆射撃姿勢タイマーの減衰
  if (shootCooldownTimer_ > 0.0f) {
    shootCooldownTimer_ -= deltaTime_;
    if (shootCooldownTimer_ < 0.0f) {
      shootCooldownTimer_ = 0.0f;
    }
  }
  if (shootSlowdownTimer_ > 0.0f) {
    shootSlowdownTimer_ -= deltaTime_;
    if (shootSlowdownTimer_ < 0.0f) {
      shootSlowdownTimer_ = 0.0f;
    }
  }

  // 1.
  // 毎フレームの移動状態リセット＆入力・ノックバック・ビヘイビア更新（レール進行確定）
  frameMoveDir_ = 0;
  HandleDamage();
  HandleKnockback();
  HandleInput();

  if (baseState_) {
    baseState_->Update(this);
  }

  // 2. 今フレームの移動結果を踏まえて1回だけレイキャスト＆重力＆地面吸着を実行
  RayCastUpdate();
  UpdateGravity();

  // 3. 接地確定後に先行入力が残っていれば即座にジャンプを実行
  if (isGrounded_ && jumpBufferTimer_ > 0.0f) {
    TryExecuteBufferedJump();
  }

  // 4. 接地状態と移動入力から歩き・ダッシュ・待機の移動状態を決定
  if (!isGrounded_ || !isMoving_) {
    moveState_ = MoveState::Idle;
  } else if (isDashing_) {
    moveState_ = MoveState::Dash;
  } else {
    moveState_ = MoveState::Walk;
  }

  // 5. スクワッシュ演出＆レール座標・3Dモデル・コライダーの最終同期
  UpdateSquashStretch();
  UpdateRailPath();
  ImGuiDrawDebugInfo();
}

void Player::UpdateTransform() {
  RayCastUpdate();
  UpdateRailPath();
  if (auto *col = GetCollider()) {
    col->Update();
  }
  if (attackCollider_) {
    attackCollider_->Update();
  }
}

void Player::Draw() {
  auto drawDebugColliders = [this]() {
    if (auto *col = GetCollider()) {
      col->Draw();
    }
    if (attackCollider_ && IsAttackHitboxActive()) {
      attackCollider_->Draw();
    }
  };

  // 被弾無敵時間中のモデル点滅（0.08秒ごとに表示/非表示を切り替え、近接攻撃時等は点滅しない）
  if (isAlive_ && isDamaged_ && hitInvincibilityTimer_ > 0.0f) {
    const float kBlinkInterval = 0.08f;
    if (fmodf(hitInvincibilityTimer_, kBlinkInterval * 2.0f) < kBlinkInterval) {
      drawDebugColliders();
      return;
    }
  }

  object_->Draw();
  drawDebugColliders();
}

void Player::SetRailPosition(const Vector2 &position) {
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

void Player::AddVelocity(Vector3 v) { velocity_ += v; }

void Player::SetRail(RailPath *rail) {
  if (!rail || !railMover_)
    return;

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

void Player::Move(float ratio) {
  if (std::abs(ratio) > 0.001f) {
    isMoving_ = true;
    frameMoveDir_ = (ratio > 0.0f) ? 1 : -1;
  }
  // 着地怯み中、または機関砲の連射反動中はダッシュを解除して腰を据える
  if (IsLandingStaggered() || IsShootingStance()) {
    isDashing_ = false;
  }
  float speed =
      isDashing_ ? config_.movement.dashSpeed : config_.movement.walkSpeed;
  if (IsLandingStaggered()) {
    speed *= config_.movement.landingSpeedMultiplier;
  }
  railMover_->Advance(ratio * (speed * deltaTime_));
}

void Player::MoveAtSpeed(float speedMetersPerSec) {
  if (std::abs(speedMetersPerSec) > 0.001f) {
    isMoving_ = true;
    frameMoveDir_ = (speedMetersPerSec > 0.0f) ? 1 : -1;
  }
  railMover_->Advance(speedMetersPerSec * deltaTime_);
}

void Player::Jump() {
  // ジャンプボタンが押されたら先行入力タイマーをチャージ
  jumpBufferTimer_ = config_.movement.jumpBufferDuration;
  TryExecuteBufferedJump();
}

bool Player::TryExecuteBufferedJump() {
  if (jumpBufferTimer_ <= 0.0f || IsLandingStaggered()) {
    return false;
  }

  // 接地中、または崖を踏み外した直後（コヨーテタイム中）ならジャンプ許可
  bool canJump = isGrounded_ || (coyoteTimer_ > 0.0f && !isJumping_);
  if (!canJump) {
    return false;
  }

  // すり抜け足場の上で「下入力＋ジャンプ」を行った場合は下層へすり抜け降下
  Input *input = Input::GetInstance();
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
    dropThroughTimer_ =
        config_.dropThrough.duration; // すり抜け足場の床判定を無視
    velocity_.y =
        config_.dropThrough.downVelocity; // 下向き初速を与えてスムーズに降りる
    worldY_ -= 0.1f;
    coyoteTimer_ = 0.0f;
    jumpBufferTimer_ = 0.0f;
    return true;
  } else {
    // 通常ジャンプ実行！
    velocity_.y = config_.movement.jumpAcceleration;
    isGrounded_ = false;
    isJumping_ = true;
    coyoteTimer_ = 0.0f;     // コヨーテタイムを消費
    jumpBufferTimer_ = 0.0f; // 先行入力を消費
    TriggerSquashStretch(
        config_.juice.jumpStretchIntensity,
        config_.juice.jumpStretchDuration); // ジャンプ縦伸び発動！

    // ジャンプビヘイビアへ遷移
    if (baseState_) {
      if (auto factory = baseState_->GetFactory()) {
        baseState_->ChangeBehavior(this,
                                   factory->CreateBehavior(BehaviorType::Jump));
      }
    }
    return true;
  }
}

void Player::Attack() {}

float Player::GetRailProgress() const { return railMover_->GetProgress(); }
float Player::GetCurrentDistance() const {
  return railMover_->GetCurrentDistance();
}
const RailPath *Player::GetRailPath() const {
  return railMover_->GetRailPath();
}

void Player::UpdateRailPath() {
  Vector3 railPos = railMover_->GetCurrentPosition();
  Vector3 railDir = railMover_->GetCurrentDirection();

  Vector3 finalPos = {railPos.x, worldY_ + squashGroundedOffsetY_, railPos.z};
  finalPos = Add(finalPos, wallPushOffset_);
  object_->SetTranslate(finalPos);

  // ノックバック中は向きを変更せず直前の向きを固定
  if (!isKnockback_) {
    float currentFrameAngle = 0.0f;
    if (Length(railDir) > 0.001f) {
      currentFrameAngle = atan2f(railDir.x, railDir.z);
      if (railMover_->GetMoveDirection() ==
          RailMover::MoveDirection::Backward) {
        currentFrameAngle += std::numbers::pi_v<float>;
      }
    }
    currentAngle_ = currentFrameAngle;
  }

  object_->SetRotate({0.0f, currentAngle_, 0.0f});
  object_->Update();

  // コライダーの回転（Quaternion）とワールド座標の同期（オフセットや半径の初期設定はApplyColliderConfigで1回のみ実施）
  if (auto *col = GetCollider()) {
    col->SetRotation(EulerToQuaternion({0.0f, currentAngle_, 0.0f}));
    col->Update();
  }

  // 攻撃用AABBヒットボックスの回転・有効状態の同期
  if (attackCollider_) {
    attackCollider_->SetRotation(
        EulerToQuaternion({0.0f, currentAngle_, 0.0f}));
    attackCollider_->SetCollide(IsAttackHitboxActive());
    attackCollider_->Update();
  }
}

void Player::CheckGroundCollision() {
  auto floorRay = GetRayInfo("Floor");
  bool hitFloor = floorRay && floorRay->isColide;

  float playerBottomY = worldY_ - config_.collider.heightOffset;

  if (hitFloor) {
    // 急すぎる斜面（崖・壁）は地面として扱わない（登れる傾斜角の制限:
    // cos約49度以上で歩行可能）
    bool isWalkableSlope =
        (floorRay->hitNormal.y >= config_.movement.maxSlopeCos);
    rayHitParam_.groundY = floorRay->crossPoint.y;

    if (isWalkableSlope) {
      const float kGroundEpsilon = 0.05f;

      // 1. 通常の接地（足元が地面付近、またはめり込んでいる場合）
      if (playerBottomY <= rayHitParam_.groundY + kGroundEpsilon &&
          velocity_.y <= 0.0f) {
        isGrounded_ = true;
        isJumping_ = false;
      }
      // 2. 下り坂吸着（Ground Snapping）:
      // 直前に接地しており、現在ジャンプ中でなく、落下初速がない
      // かつ、直前フレームの地面からの高低差が段差制限（maxStepHeight）以内の緩やかなスロープでのみ吸着
      // （プレハブブロックや崖など急激に地面が落ちる場所では吸着させず自然落下に移行）
      else if (isGrounded_ && !isJumping_ && velocity_.y <= 0.0f &&
               (playerBottomY - rayHitParam_.groundY) <=
                   config_.movement.groundSnapDistance &&
               (lastGroundY_ - rayHitParam_.groundY) <=
                   config_.movement.maxStepHeight) {
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
  if (!hitFloor &&
      worldY_ <= rayHitParam_.minY + config_.collider.heightOffset) {
    worldY_ = rayHitParam_.minY + config_.collider.heightOffset;
    velocity_.y = 0.0f;
    isGrounded_ = true;
    isJumping_ = false;
  }

  // 接地中の確定地面高さを記録（段差降り・崖落ちの判定に使用）
  if (isGrounded_ && hitFloor) {
    lastGroundY_ = rayHitParam_.groundY;
  }
}

void Player::UpdateGravity() {
  // 落下速度を着地直前判定用に記録
  landingFallSpeed_ = (velocity_.y < 0.0f) ? -velocity_.y : 0.0f;

  // 1.
  // 重力加速度と垂直速度の積分（先に移動してから床判定を行うことで1フレームの地面めり込みを防止）
  if (!isGrounded_) {
    velocity_.y += (config_.movement.gravity * gravityScale_) * deltaTime_;
  } else if (velocity_.y < 0.0f) {
    velocity_.y = 0.0f;
  }

  worldY_ += velocity_.y * deltaTime_;

  // 2. 地面との当たり判定・めり込み押し上げ・下り坂スナップ
  CheckGroundCollision();

  // 3.
  // 着地（空中から地面に触れた瞬間）を検知してスクワッシュ（縦潰れ）＆着地怯みを発動！
  if (!wasGrounded_ && isGrounded_) {
    const float kMinFallSpeed = 1.2f;
    if (landingFallSpeed_ >= kMinFallSpeed) {
      // 落下速度が大きいほど潰れを強くする（最大1.0）
      float speedFactor =
          std::clamp((landingFallSpeed_ - kMinFallSpeed) / 12.0f, 0.35f, 1.0f);
      float squashIntensity =
          -config_.juice.landSquashIntensityMax * speedFactor;
      TriggerSquashStretch(squashIntensity, config_.juice.landSquashDuration);
    }

    // 重量級メカの着地怯み（硬直・ブレーキ・カメラ微振動）
    if (landingFallSpeed_ >= config_.movement.landingStaggerMinFallSpeed) {
      float staggerFactor = std::clamp(
          (landingFallSpeed_ - config_.movement.landingStaggerMinFallSpeed) /
              16.0f,
          0.6f, 1.25f);
      landingRecoveryTimer_ =
          config_.movement.landingStaggerDuration * staggerFactor;
      isDashing_ = false;
      if (scene_) {
        scene_->RequestCameraShake(0.08f, 0.35f * staggerFactor);
      }
    }
  }
  wasGrounded_ = isGrounded_;
}

void Player::InitializeRays() {
  rayList_.clear();

  CollisionRayInfo floorRay;
  floorRay.name = "Floor";
  // 実際のoriginやdiffは毎フレーム更新
  floorRay.ray.diff = {0.0f, -10.0f - rayHitParam_.rayOffset, 0.0f};
  rayList_.push_back(floorRay);

  float wallLength = config_.collider.radius + 0.2f;

  CollisionRayInfo frontRay;
  frontRay.name = "FrontWall";
  frontRay.ray.diff = {0.0f, 0.0f, wallLength};
  rayList_.push_back(frontRay);

  CollisionRayInfo backRay;
  backRay.name = "BackWall";
  backRay.ray.diff = {0.0f, 0.0f, -wallLength};
  rayList_.push_back(backRay);

  CollisionRayInfo leftRay;
  leftRay.name = "LeftWall";
  leftRay.ray.diff = {-wallLength, 0.0f, 0.0f};
  rayList_.push_back(leftRay);

  CollisionRayInfo rightRay;
  rightRay.name = "RightWall";
  rightRay.ray.diff = {wallLength, 0.0f, 0.0f};
  rayList_.push_back(rightRay);
}

const CollisionRayInfo *Player::GetRayInfo(const std::string &name) const {
  for (const auto &rayInfo : rayList_) {
    if (rayInfo.name == name) {
      return &rayInfo;
    }
  }
  return nullptr;
}

void Player::RayCastUpdate() { UpdateRayCollisions(); }

void Player::UpdateRayCollisions() {
  if (!scene_ || !railMover_)
    return;

  const std::vector<Triangle> &triangles = scene_->GetTriangle();

  // 1フレーム遅れの object_->GetTranslate() ではなく、現在のレール進捗と
  // worldY_ から最新中心座標を取得
  Vector3 railPos = railMover_->GetCurrentPosition();
  Vector3 center = {railPos.x, worldY_, railPos.z};

  // 壁レイの発射方向は坂道レールでも地面を撃たないよう水平面（XZ平面）に投影
  Vector3 railDir = railMover_->GetCurrentDirection();
  Vector3 horizDir = {railDir.x, 0.0f, railDir.z};
  Vector3 forwardDir = (Length(horizDir) > 0.001f) ? Normalize(horizDir)
                                                   : Vector3{0.0f, 0.0f, 1.0f};
  Vector3 rightDir = Normalize(Cross({0.0f, 1.0f, 0.0f}, forwardDir));

  float wallLength = 1.5f;

  // ノックバックやスライドも含めた今フレームの実移動方向（静止時は向いている方向）
  int moveDir = (frameMoveDir_ != 0) ? frameMoveDir_ : GetMoveDirection();

  float maxPushBackProgress = 0.0f;

  // ─── Phase 1: 壁レイ（FrontWall / BackWall / LeftWall / RightWall）の判定
  // ───
  for (auto &rayInfo : rayList_) {
    if (rayInfo.name == "Floor")
      continue;

    rayInfo.isColide = false;
    rayInfo.distance = FLT_MAX;
    rayInfo.crossPoint = {};
    rayInfo.hitNormal = {};
    rayInfo.hitTriangle = {};

    rayInfo.ray.origin = center;
    rayInfo.ray.origin.y = center.y + config_.collider.wallRayHeight;

    if (rayInfo.name == "FrontWall") {
      rayInfo.ray.diff = Multiply(wallLength, forwardDir);
    } else if (rayInfo.name == "BackWall") {
      rayInfo.ray.diff = Multiply(-wallLength, forwardDir);
    } else if (rayInfo.name == "LeftWall") {
      rayInfo.ray.diff = Multiply(-wallLength, rightDir);
    } else if (rayInfo.name == "RightWall") {
      rayInfo.ray.diff = Multiply(wallLength, rightDir);
    }

    if (!triangles.empty()) {
      auto hits = Physics::RaycastAll(rayInfo.ray, triangles, 1000.0f,
                                      CollisionLayer::All);
      for (const auto &hit : hits) {
        if (hit.triangle.isOneway) {
          continue; // 壁レイはすり抜け足場を無視
        }
        if (hit.normal.y >= config_.movement.maxSlopeCos) {
          continue; // 歩行可能な緩斜面は壁から除外
        }

        rayInfo.distance = hit.distance;
        rayInfo.crossPoint = hit.point;
        rayInfo.hitNormal = hit.normal;
        rayInfo.hitTriangle = hit.triangle;
        rayInfo.isColide = true;
        break;
      }
    }

    float drawLen = config_.collider.modelRadius + 0.05f;
    Vector3 dir = Normalize(rayInfo.ray.diff);
    Vector3 drawEnd = Add(rayInfo.ray.origin, Multiply(drawLen, dir));
    bool isWallNear = rayInfo.isColide && (rayInfo.distance <= drawLen);
    PrimitiveDrawer::GetInstance()->DrawLine(rayInfo.ray.origin, drawEnd,
                                             isWallNear ? Vector4{1, 0, 0, 1}
                                                        : Vector4{0, 1, 0, 1});

    if (rayInfo.isColide) {
      float targetWallDist = config_.collider.modelRadius + 0.005f;
      if (rayInfo.name == "FrontWall" && moveDir >= 0) {
        float penetration = targetWallDist - rayInfo.distance;
        if (penetration > 0.0f) {
          penetration = (std::min)(penetration, 0.25f);
          maxPushBackProgress = (std::max)(maxPushBackProgress, penetration);
        }
      } else if (rayInfo.name == "BackWall" && moveDir < 0) {
        float penetration = targetWallDist - rayInfo.distance;
        if (penetration > 0.0f) {
          penetration = (std::min)(penetration, 0.25f);
          maxPushBackProgress = (std::min)(maxPushBackProgress, -penetration);
        }
      }

      if (rayInfo.distance <= (config_.collider.modelRadius + 0.05f)) {
        PrimitiveDrawer::GetInstance()->DrawSphere(
            {rayInfo.crossPoint, 0.05f, {}}, {0, 0, 1, 1});
      }
    }
  }

  // ─── Phase 2: 段差ステップアップ（Step-up）判定 ───
  // FrontWallに当たっていても、maxStepHeight
  // 以下の上面があり、かつその上の胸高さに壁がなければ段差として登る
  if (isGrounded_ && !isJumping_ && !triangles.empty() && moveDir != 0) {
    bool isSideWallBlocked = false;
    const auto *lw = GetRayInfo("LeftWall");
    const auto *rw = GetRayInfo("RightWall");
    if ((lw && lw->isColide &&
         lw->distance <= (config_.collider.modelRadius + 0.08f)) ||
        (rw && rw->isColide &&
         rw->distance <= (config_.collider.modelRadius + 0.08f))) {
      isSideWallBlocked = true;
    }

    if (!isSideWallBlocked) {
      Vector3 stepForward =
          (moveDir > 0) ? forwardDir : Multiply(-1.0f, forwardDir);
      float stepCheckDist = config_.collider.modelRadius + 0.08f;
      float playerBottomY = worldY_ - config_.collider.heightOffset;

      // 段差より上の高さ（maxStepHeight + 0.25m）に高い壁がないか確認
      Ray upperWallRay;
      upperWallRay.origin = {
          center.x, playerBottomY + config_.movement.maxStepHeight + 0.25f,
          center.z};
      upperWallRay.diff = Multiply(stepCheckDist + 0.05f, stepForward);
      RaycastHit upperWallHit;
      bool hasUpperWall =
          Physics::Raycast(upperWallRay, triangles, &upperWallHit,
                           stepCheckDist + 0.05f, CollisionLayer::All) &&
          !upperWallHit.triangle.isOneway &&
          (upperWallHit.normal.y < config_.movement.maxSlopeCos);

      if (!hasUpperWall) {
        Ray stepRay;
        stepRay.origin = Add(center, Multiply(stepCheckDist, stepForward));
        stepRay.origin.y =
            playerBottomY + config_.movement.maxStepHeight + 0.05f;
        stepRay.diff = {0.0f, -(config_.movement.maxStepHeight + 0.2f), 0.0f};

        RaycastHit stepHit;
        bool hitStep = Physics::Raycast(
            stepRay, triangles, &stepHit, config_.movement.maxStepHeight + 0.2f,
            CollisionLayer::Ground | CollisionLayer::OneWay);
        if (hitStep && stepHit.normal.y >= config_.movement.maxSlopeCos) {
          float stepHeight = stepHit.point.y - playerBottomY;
          if (stepHeight > 0.02f &&
              stepHeight <= config_.movement.maxStepHeight) {
            worldY_ = stepHit.point.y + config_.collider.heightOffset;
            center.y = worldY_;
            maxPushBackProgress = 0.0f; // 段差を登れた場合は壁押し戻しを解除
          }
        }
      }
    }
  }

  // ─── Phase 3:
  // 壁によるレール押し戻しを先に適用（壁の上面をFloorレイが誤検知してワープするのを防ぐ）
  // ───
  if (std::abs(maxPushBackProgress) > 0.0001f) {
    float currentDist = railMover_->GetCurrentDistance();
    float targetDist = currentDist - maxPushBackProgress;
    const RailPath *path = railMover_->GetRailPath();
    if (path) {
      float targetT = path->GetTFromDistance(targetDist);
      railMover_->SetProgress(targetT);
      railPos = railMover_->GetCurrentPosition();
      center = {railPos.x, worldY_, railPos.z};
    }
  }

  wallPushOffset_ = {0.0f, 0.0f, 0.0f};

  // ─── Phase 4: 押し戻し後の確定XZ座標で Floor レイを判定 ───
  float playerBottomY = worldY_ - config_.collider.heightOffset;
  float floorOriginOffsetY =
      config_.movement.maxStepHeight +
      0.35f; // 頭上の天井や高い壁の上面を拾わない安全な高さ

  for (auto &rayInfo : rayList_) {
    if (rayInfo.name != "Floor")
      continue;

    rayInfo.isColide = false;
    rayInfo.distance = FLT_MAX;
    rayInfo.crossPoint = {};
    rayInfo.hitNormal = {};
    rayInfo.hitTriangle = {};

    rayInfo.ray.origin = {center.x, playerBottomY + floorOriginOffsetY,
                          center.z};
    rayInfo.ray.diff = {0.0f, -15.0f, 0.0f};

    if (!triangles.empty()) {
      auto hits = Physics::RaycastAll(rayInfo.ray, triangles, 1000.0f,
                                      CollisionLayer::All);
      for (const auto &hit : hits) {
        if (hit.normal.y <= 0.0f) {
          continue;
        }
        if (hit.triangle.isOneway) {
          if (dropThroughTimer_ > 0.0f || velocity_.y > 0.0f) {
            continue;
          }
          if (playerBottomY < hit.point.y - 0.8f) {
            continue;
          }
        }
        // 足元より maxStepHeight + 0.1m
        // 以上高い面は天井/壁上面として除外（壁上ワープ防止）
        if (hit.point.y >
            playerBottomY + config_.movement.maxStepHeight + 0.10f) {
          continue;
        }

        rayInfo.distance = hit.distance;
        rayInfo.crossPoint = hit.point;
        rayInfo.hitNormal = hit.normal;
        rayInfo.hitTriangle = hit.triangle;
        rayInfo.isColide = true;
        break;
      }
    }

    Vector3 drawEnd = Add(rayInfo.ray.origin, rayInfo.ray.diff);
    PrimitiveDrawer::GetInstance()->DrawLine(
        rayInfo.ray.origin, drawEnd,
        rayInfo.isColide ? Vector4{1, 0, 0, 1} : Vector4{0, 1, 0, 1});
  }

  // 急斜面への前進阻止
  auto floorRayCheck = GetRayInfo("Floor");
  if (floorRayCheck && floorRayCheck->isColide &&
      floorRayCheck->hitNormal.y < config_.movement.maxSlopeCos &&
      floorRayCheck->hitNormal.y > 0.0f) {
    float dotDir = Dot(floorRayCheck->hitNormal, forwardDir) * float(moveDir);
    if (dotDir < -0.05f) {
      float currentDist = railMover_->GetCurrentDistance();
      float push = (moveDir >= 0) ? 0.05f : -0.05f;
      if (const RailPath *path = railMover_->GetRailPath()) {
        railMover_->SetProgress(path->GetTFromDistance(currentDist - push));
      }
    }
  }

  // 地面判定の更新
  auto floorRay = GetRayInfo("Floor");
  if (floorRay) {
    SetRayHit(floorRay->isColide);
    SetRayHitDistance(floorRay->distance);
    SetRayHitPoint(floorRay->crossPoint);
    SetRayHitTriangle(floorRay->hitTriangle);
    SetRayCollisionResult(RayTriangleCollisionResult::FrontFace);
    isCurrentGroundOneway_ =
        (floorRay->isColide && floorRay->hitTriangle.isOneway);
  } else {
    isCurrentGroundOneway_ = false;
  }
}

void Player::HandleInput() {
  // 毎フレームの入力処理開始時に移動フラグ・ダッシュフラグをリセット
  isDashing_ = false;
  isMoving_ = false;

  // ノックバック中は操作不能にする
  if (isKnockback_) {
    return;
  }

  auto commands = inputHandler_->HandleInput();
  for (auto &command : commands) {
    if (baseState_) {
      baseState_->HandleInput(this, command.get());
    }
  }
}

void Player::HandleDamage() {
  if (hitInvincibilityTimer_ > 0.0f) {
    hitInvincibilityTimer_ -= deltaTime_;
    if (hitInvincibilityTimer_ <= 0.0f) {
      hitInvincibilityTimer_ = 0.0f;
      isDamaged_ = false;
    }
  }
  if (actionInvincibilityTimer_ > 0.0f) {
    actionInvincibilityTimer_ -= deltaTime_;
    if (actionInvincibilityTimer_ <= 0.0f) {
      actionInvincibilityTimer_ = 0.0f;
    }
  }
}

void Player::HandleKnockback() {
  if (isKnockback_ && knockbackTimer_ > 0.0f) {
    // ノックバックの残り時間に応じて速度を減衰
    float progressRatio = knockbackTimer_ / config_.combat.knockbackDuration;
    float moveAmount = float(knockbackDirection_) *
                       config_.combat.knockbackSpeed * progressRatio *
                       deltaTime_;
    frameMoveDir_ = (knockbackDirection_ >= 0) ? 1 : -1;
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

void Player::TakeDamage(int knockbackDirection) {
  if (hitInvincibilityTimer_ <= 0.0f && actionInvincibilityTimer_ <= 0.0f &&
      !isInvincible_) {
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
      baseState_->ChangeBehavior(
          this, baseState_->GetFactory()->CreateBehavior(BehaviorType::Root));
    }
  }
}

void Player::HandleAlive() {
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
  ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y,
              velocity_.z);

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

  const char *moveStateStr = "Idle";
  if (moveState_ == MoveState::Walk)
    moveStateStr = "Walk";
  else if (moveState_ == MoveState::Dash)
    moveStateStr = "Dash (Running)";
  ImGui::Text("Move State: %s (Dashing: %s)", moveStateStr,
              isDashing_ ? "YES" : "NO");
  ImGui::Text("Current Speed: %.1f m/s", isDashing_
                                             ? config_.movement.dashSpeed
                                             : config_.movement.walkSpeed);
  ImGui::Separator();

  if (isKnockback_) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                       "STATUS: KNOCKBACK! (Timer: %.2f)", knockbackTimer_);
  } else if (isDamaged_) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       "STATUS: INVINCIBLE / DAMAGED!");
  } else {
    ImGui::Text("STATUS: Normal");
  }

  ImGui::Text("Hit Points: %d", hitPoints_.value);
  ImGui::ProgressBar(hitInvincibilityTimer_ /
                         config_.combat.hitInvincibilityDuration,
                     ImVec2(0, 0), "Hit Timer");

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
  ImGui::SliderFloat("Walk Speed", &config_.movement.walkSpeed, 1.0f, 20.0f,
                     "%.1f m/s");
  ImGui::SliderFloat("Dash Speed", &config_.movement.dashSpeed, 2.0f, 40.0f,
                     "%.1f m/s");
  ImGui::SliderFloat("Gravity", &config_.movement.gravity, -120.0f, -10.0f,
                     "%.1f");
  ImGui::SliderFloat("Jump Accel", &config_.movement.jumpAcceleration, 5.0f,
                     50.0f, "%.1f");
  ImGui::SliderFloat("Coyote Time", &config_.movement.coyoteDuration, 0.01f,
                     0.5f, "%.2f s");
  ImGui::SliderFloat("Jump Buffer", &config_.movement.jumpBufferDuration, 0.01f,
                     0.5f, "%.2f s");
  ImGui::SliderFloat("Max Step Height", &config_.movement.maxStepHeight, 0.05f,
                     1.0f, "%.2f m");
  ImGui::SliderFloat("Ground Snap Dist", &config_.movement.groundSnapDistance,
                     0.1f, 3.0f, "%.2f m");
  ImGui::SliderFloat("Max Slope Cos", &config_.movement.maxSlopeCos, 0.2f,
                     0.95f, "%.2f");
  ImGui::SliderFloat("Landing Stagger Duration",
                     &config_.movement.landingStaggerDuration, 0.0f, 0.5f,
                     "%.2f s");
  ImGui::SliderFloat("Landing Stagger Min Speed",
                     &config_.movement.landingStaggerMinFallSpeed, 1.0f, 25.0f,
                     "%.1f m/s");
  ImGui::SliderFloat("Landing Speed Mult",
                     &config_.movement.landingSpeedMultiplier, 0.0f, 1.0f,
                     "%.2f");

  ImGui::Separator();
  ImGui::Text("--- Combat Parameters ---");
  ImGui::SliderFloat("Invincibility Duration",
                     &config_.combat.hitInvincibilityDuration, 0.1f, 5.0f,
                     "%.2f s");
  ImGui::SliderFloat("Knockback Duration", &config_.combat.knockbackDuration,
                     0.05f, 1.0f, "%.2f s");
  ImGui::SliderFloat("Knockback Speed", &config_.combat.knockbackSpeed, 1.0f,
                     25.0f, "%.1f m/s");
  ImGui::SliderFloat("Knockback Jump Force", &config_.combat.knockbackJumpForce,
                     1.0f, 20.0f, "%.1f");

  ImGui::Separator();
  ImGui::Text("--- Squash & Stretch (Juice) ---");
  ImGui::SliderFloat("Jump Stretch", &config_.juice.jumpStretchIntensity, 0.05f,
                     0.60f, "%.2f");
  ImGui::SliderFloat("Jump Duration", &config_.juice.jumpStretchDuration, 0.05f,
                     0.50f, "%.2f s");
  ImGui::SliderFloat("Land Squash Max", &config_.juice.landSquashIntensityMax,
                     0.05f, 0.60f, "%.2f");
  ImGui::SliderFloat("Land Duration", &config_.juice.landSquashDuration, 0.05f,
                     0.50f, "%.2f s");
  if (ImGui::Button("Test Jump Stretch")) {
    TriggerSquashStretch(config_.juice.jumpStretchIntensity,
                         config_.juice.jumpStretchDuration);
  }
  ImGui::SameLine();
  if (ImGui::Button("Test Land Squash")) {
    TriggerSquashStretch(-config_.juice.landSquashIntensityMax,
                         config_.juice.landSquashDuration);
  }

  ImGui::Separator();
  ImGui::Text("--- Slide Action Settings ---");
  ImGui::SliderFloat("Slide Duration", &config_.slide.duration, 0.1f, 1.5f,
                     "%.2f s");
  ImGui::SliderFloat("Slide Initial Speed", &config_.slide.initialSpeed, 5.0f,
                     40.0f, "%.1f m/s");
  ImGui::SliderFloat("Slide Invincibility After",
                     &config_.slide.invincibilityAfter, 0.0f, 1.0f, "%.2f s");

  ImGui::Separator();
  ImGui::Text("--- Hover Form Settings ---");
  ImGui::SliderFloat("Hover Max Duration", &config_.hover.maxDuration, 0.5f,
                     6.0f, "%.2f s");
  ImGui::SliderFloat("Hover Fall Speed", &config_.hover.fallSpeed, -5.0f, 0.0f,
                     "%.2f m/s");
  ImGui::SliderFloat("Hover Initial Boost Y", &config_.hover.initialBoostY,
                     0.0f, 10.0f, "%.1f m/s");

  ImGui::Separator();
  ImGui::Text("--- Guard Form Settings ---");
  ImGui::SliderFloat("Guard Max Duration", &config_.guard.maxDuration, 0.5f,
                     8.0f, "%.2f s");
  ImGui::SliderFloat("Bash Duration", &config_.guard.bashDuration, 0.1f, 1.0f,
                     "%.2f s");
  ImGui::SliderFloat("Bash Speed", &config_.guard.bashSpeed, 5.0f, 40.0f,
                     "%.1f m/s");
  ImGui::SliderFloat("Guard Invincibility After",
                     &config_.guard.invincibilityAfter, 0.0f, 1.0f, "%.2f s");

  ImGui::Separator();
  ImGui::Text("--- Melee Attack Settings ---");
  ImGui::SliderFloat("Windup Duration", &config_.attackAction.windupDuration,
                     0.0f, 0.3f, "%.2f s");
  ImGui::SliderFloat("Strike Duration", &config_.attackAction.rushDuration,
                     0.05f, 0.5f, "%.2f s");
  ImGui::SliderFloat("Recovery Duration",
                     &config_.attackAction.recoveryDuration, 0.02f, 0.5f,
                     "%.2f s");
  ImGui::SliderFloat("Step Speed Multiplier",
                     &config_.attackAction.rushSpeedMultiplier, 0.2f, 3.0f,
                     "%.2f");
  ImGui::SliderFloat("Attack Invincibility After",
                     &config_.attackAction.invincibilityAfter, 0.0f, 1.0f,
                     "%.2f s");

  ImGui::Separator();
  ImGui::Text("--- Rapid Fire (Machine Gun) Settings ---");
  ImGui::SliderFloat("Fire Interval", &config_.rapidFire.fireInterval, 0.03f,
                     0.50f, "%.2f s");
  ImGui::SliderFloat("Bullet Speed", &config_.rapidFire.bulletSpeed, 0.10f,
                     1.50f, "%.2f");
  ImGui::SliderFloat("Bullet Spread Y", &config_.rapidFire.bulletSpreadY, 0.0f,
                     0.20f, "%.3f");

  ImGui::Separator();
  ImGui::Text("--- Drop Through Settings ---");
  ImGui::SliderFloat("Drop Through Ignore Duration",
                     &config_.dropThrough.duration, 0.1f, 1.0f, "%.2f s");
  ImGui::SliderFloat("Drop Through Down Velocity",
                     &config_.dropThrough.downVelocity, -20.0f, -1.0f,
                     "%.1f m/s");

  ImGui::Separator();
  ImGui::Text("--- Ground & Raycast Settings ---");
  ImGui::SliderFloat("Height Offset", &config_.collider.heightOffset, -0.5f,
                     1.0f, "%.3f m");
  ImGui::SliderFloat("Wall Ray Height", &config_.collider.wallRayHeight, 0.1f,
                     1.2f, "%.3f m");
  ImGui::SliderFloat("Model Wall Radius", &config_.collider.modelRadius, 0.05f,
                     1.0f, "%.3f m");

  ImGui::Separator();
  ImGui::Text("--- Sphere Collider Settings ---");
  if (ImGui::SliderFloat("Collider Radius", &config_.collider.radius, 0.1f,
                         1.5f, "%.3f m") ||
      ImGui::SliderFloat("Head Offset Y", &config_.collider.headOffsetY, 0.5f,
                         2.0f, "%.3f m") ||
      ImGui::SliderFloat("Head Radius", &config_.collider.headRadius, 0.1f,
                         0.8f, "%.3f m") ||
      ImGui::SliderFloat("Body Offset Y", &config_.collider.bodyOffsetY, 0.1f,
                         1.5f, "%.3f m") ||
      ImGui::SliderFloat("Body Radius", &config_.collider.bodyRadius, 0.1f,
                         0.8f, "%.3f m")) {
    ApplyColliderConfig();
  }

  ImGui::Separator();
  ImGui::Text("--- Attack AABB Hitbox Settings ---");
  ImGui::Checkbox("Debug: Force Enable Hitbox", &debugForceAttackHitbox_);
  ImGui::Text("Active: %s", IsAttackHitboxActive() ? "YES (Attacking!)" : "NO");
  if (ImGui::SliderFloat("Attack Offset Y", &config_.collider.attackOffsetY,
                         -0.5f, 1.5f, "%.3f m") ||
      ImGui::SliderFloat("Attack Offset Forward",
                         &config_.collider.attackOffsetForward, 0.2f, 2.5f,
                         "%.3f m") ||
      ImGui::SliderFloat3("Attack Box HalfSize (X,Y,Z)",
                          &config_.collider.attackHalfSize.x, 0.1f, 2.0f,
                          "%.2f m")) {
    ApplyColliderConfig();
  }

  ImGui::Separator();
  ImGui::Text("--- Raycast Info ---");
  for (const auto &rayInfo : rayList_) {
    if (rayInfo.isColide) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[%s] Hit! Dist: %.2f",
                         rayInfo.name.c_str(), rayInfo.distance);
    } else {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s] No Hit",
                         rayInfo.name.c_str());
    }
  }

  ImGui::End();
#endif
}

void Player::ChangeState(std::unique_ptr<IPlayerState> newState) {
  if (baseState_)
    baseState_->Finalize(this);
  baseState_ = std::move(newState);
  if (baseState_)
    baseState_->Initialize(this);
}

void Player::ChangeBehavior(std::unique_ptr<IPlayerBehavior> newBehavior) {
  if (baseState_) {
    baseState_->ChangeBehavior(this, std::move(newBehavior));
  }
}

void Player::OnCollision(GameObject *other) {
  if (!other || isInvincible_)
    return;

  if (other->GetCategory() == CollisionCategory::Enemy) {
    Enemy *enemy = dynamic_cast<Enemy *>(other);
    if (!enemy)
      return;

    // プレイヤーが攻撃中、または攻撃ヒットボックス展開中は接触ダメージを受けない
    // （敵への攻撃ヒット処理は Enemy::OnCollision
    // 側で厳密にAABB交差判定を行って通知する）
    if (IsAttacking() || IsAttackHitboxActive()) {
      return;
    }

    // 敵が通常状態の場合、ダメージおよびノックバック
    if (enemy->CanDamagePlayer()) {
      if (hitInvincibilityTimer_ <= 0.0f && actionInvincibilityTimer_ <= 0.0f) {
        // レール上の位置関係からノックバック方向を決定
        int knockDir = -1;
        float playerDist = GetCurrentDistance();
        float enemyDist = enemy->GetCurrentDistance();
        if (playerDist < enemyDist) {
          knockDir = -1; // プレイヤーが手前 → 手前側(後退)へノックバック
        } else if (playerDist > enemyDist) {
          knockDir = 1; // プレイヤーが奥 → 奥側(前進)へノックバック
        } else {
          // 同一地点の場合はプレイヤーの移動向きの逆方向
          knockDir = (GetMoveDirection() == 1) ? -1 : 1;
        }
        playHundle_ = Audio::GetInstance()->PlayAudio(DamageSE_, false, 0.75);

        if (scene_) {
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

const char *Player::GetStateName() const {
  return baseState_ ? baseState_->GetName() : "NoState";
}

const char *Player::GetBehaviorName() const {
  return (baseState_ && baseState_->GetBehavior())
             ? baseState_->GetBehavior()->GetName()
             : "NoBehavior";
}

void Player::SetScene(Scene *scene) { scene_ = scene; }

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

void Player::OnAttackHit(GameObject *target) {
  (void)target;
  velocity_.x = 0.0f;
  velocity_.z = 0.0f;

  // 近接攻撃ヒット時は無敵余韻を付与（ヒットボックス自体は打撃フェーズ終了まで表示・維持する）
  TriggerInvincibility(0.3f);
}

void Player::TriggerSquashStretch(float intensityY, float duration) {
  squashStretch_.intensityY = intensityY;
  squashStretch_.duration = duration;
  squashStretch_.timer = 0.0f;
  squashStretch_.isActive = true;
}

void Player::UpdateSquashStretch() {
  if (!object_)
    return;

  float currentOffsetY = 0.0f;

  if (squashStretch_.isActive && squashStretch_.duration > 0.0f) {
    squashStretch_.timer += deltaTime_;
    float progress = squashStretch_.timer / squashStretch_.duration;

    if (progress >= 1.0f) {
      squashStretch_.isActive = false;
      currentOffsetY = 0.0f;
    } else if (squashStretch_.intensityY < 0.0f) {
      // 着地・踏み込み時：油圧サスペンションのように素早く沈み込み、重々しく立ち上がる
      if (progress < 0.22f) {
        float t = progress / 0.22f;
        currentOffsetY = squashStretch_.intensityY *
                         std::sin(t * std::numbers::pi_v<float> * 0.5f);
      } else {
        float t = (progress - 0.22f) / 0.78f;
        float easeOut = (1.0f - t) * (1.0f - t);
        currentOffsetY = squashStretch_.intensityY * easeOut;
      }
    } else {
      // ジャンプ時：減衰サイン波
      float damp = expf(-4.2f * progress);
      float wave = cosf(progress * std::numbers::pi_v<float> * 2.5f) * damp;
      currentOffsetY = squashStretch_.intensityY * wave;
    }
  } else if (!isGrounded_ && velocity_.y < -6.0f) {
    // 空中高速落下中の風圧ストレッチ（微小な縦伸び予兆）
    float fallFactor = std::clamp((-velocity_.y - 6.0f) / 15.0f, 0.0f, 1.0f);
    currentOffsetY = 0.10f * fallFactor;
  }

  float targetScaleY = (std::max)(1.0f + currentOffsetY, 0.35f);
  // メカ筐体らしく、沈み込み時の横膨張を抑える（ゴムのような過度な膨張を防ぐ）
  float targetScaleXZ = (currentOffsetY < 0.0f)
                            ? (1.0f - currentOffsetY * 0.18f)
                            : sqrtf(1.0f / targetScaleY);

  Vector3 finalScale = {baseScale_.x * targetScaleXZ,
                        baseScale_.y * targetScaleY,
                        baseScale_.z * targetScaleXZ};
  object_->SetScale(finalScale);

  // 1m立方体（中心原点）が着地で縮む際、底面が地面から浮かないよう足元基準にY座標を補正
  if (isGrounded_ && currentOffsetY < 0.0f) {
    squashGroundedOffsetY_ =
        (finalScale.y - baseScale_.y) * config_.collider.heightOffset;
  } else {
    squashGroundedOffsetY_ = 0.0f;
  }
}
