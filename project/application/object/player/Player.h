#pragma once
#include "Animation.h"
#include "Audio.h"
#include "Behavior/PlayerState.h"
#include "GameObject.h"
#include "Object3d.h"
#include "RailMover.h"
#include "Transform.h"
#include <memory>
#include <string>

/// <summary>
/// プレイヤーの設定パラメータ・アセットパスを保持するデータ構造体
/// </summary>
struct PlayerConfig {
  struct Paths {
    std::string modelDir = "resources/player/";
    std::string modelFile = "player.obj";
    std::string texture = "resources/player/player.png";
    std::string hitSE = "resources/Audio/SE/Hit.mp3";
    std::string damageSE = "resources/Audio/SE/Damage.mp3";
  } paths;

  struct Movement {
    float walkSpeed = 7.5f;               // 通常歩き速度 (m/s)
    float dashSpeed = 15.0f;              // ダッシュ走り速度 (m/s)
    float gravity = -50.0f;               // 重力加速度 (m/s^2)
    float jumpAcceleration = 17.0f;       // ジャンプ初速度 (従来24.0の半分の高さ)
    float coyoteDuration = 0.12f;         // 崖落ち後ジャンプ猶予時間 (秒)
    float jumpBufferDuration = 0.15f;     // 着地前ジャンプ先行入力猶予時間 (秒)
    float maxSlopeCos = 0.65f;            // 登れる最大傾斜（cos約49度）
    float maxStepHeight = 0.35f;          // 乗り越えられる段差の最大高さ (m)
    float groundSnapDistance = 0.35f;     // 地面吸着距離 (m)
    float landingStaggerDuration = 0.12f; // 着地時の怯み（硬直）時間 (秒)
    float landingStaggerMinFallSpeed =
        8.0f; // 着地怯みが発生する最小落下速度 (m/s)
    float landingSpeedMultiplier =
        0.25f; // 着地怯み中の移動速度倍率 (0.25 = 75%減速)
  } movement;

  struct Combat {
    float hitInvincibilityDuration = 1.5f; // 被弾後の無敵時間 (秒)
    float knockbackDuration = 0.25f;       // ノックバック持続時間 (秒)
    float knockbackSpeed = 7.0f;           // ノックバック初速 (m/s)
    float knockbackJumpForce = 6.0f;       // ノックバック時の跳ね上げ初速 (m/s)
  } combat;

  struct Juice {
    float jumpStretchIntensity = 0.25f;   // ジャンプ時縦伸び量 (1.25倍)
    float jumpStretchDuration = 0.22f;    // ジャンプ伸縮時間 (秒)
    float landSquashIntensityMax = 0.32f; // 着地時最大縦潰れ量 (0.68倍)
    float landSquashDuration = 0.25f;     // 着地伸縮時間 (秒)
  } juice;

  struct ColliderConfig {
    float radius = 0.50f;       // コライダー基本半径（1m立方体に合わせ0.5m）
    float modelRadius = 0.50f;  // 壁判定用モデル半径（1m立方体の半幅0.5m）
    float heightOffset = 0.50f; // 足元オフセット（1m立方体の底面が接地）
    float wallRayHeight = 0.0f; // 壁検知レイ発射高さ
    float headOffsetY = 0.0f;   // 互換用
    float headRadius = 0.50f;   // 互換用
    float bodyOffsetY = 0.0f;   // 胴体スフィア高さ（立方体中心）
    float bodyRadius = 0.50f;   // 胴体スフィア半径（0.5m）
    float attackOffsetY = 0.0f; // 攻撃AABB中心高さ（立方体中心と同じ高さ）
    float attackOffsetForward =
        0.95f; // 攻撃AABB前方オフセット（プレイヤー前方へ突き出す距離）
    float attackRadius = 0.50f; // 互換用
    Vector3 attackHalfSize = {
        0.55f, 0.55f,
        0.70f}; // 攻撃AABBの半径サイズ (横幅/2, 高さ/2, 前方リーチ長/2)
  } collider;

  struct Slide {
    float duration = 0.38f;           // スライディング持続時間 (秒)
    float initialSpeed = 22.0f;       // スライディング初速 (m/s)
    float invincibilityAfter = 0.15f; // 終了後無敵余韻 (秒)
  } slide;

  struct BackStep {
    float duration = 0.28f;           // バックステップ持続時間 (秒)
    float initialSpeed = 16.5f;       // バックステップ初速 (m/s)
    float jumpForce = 6.5f;           // 軽く後ろへ跳び退くY初速 (m/s)
    float invincibilityAfter = 0.15f; // 終了後無敵余韻 (秒)
  } backStep;

  struct Hover {
    float maxDuration = 2.0f;          // ホバー最大合計持続時間 (秒)
    float ascendSpeed = 4.5f;          // 緩やかな上昇速度 (m/s)
    float maxTotalHeight = 5.76f;      // ジャンプと合わせた最大上昇高度 (m)
    float holdTriggerDuration = 0.15f; // ホバー発動に必要な長押し時間 (秒)
    float triggerMaxVelY = 3.0f;       // ホバー移行を許可する最大Y速度 (m/s)
  } hover;

  struct Guard {
    float maxDuration = 3.0f;         // ガード最大継続時間 (秒)
    float bashDuration = 0.28f;       // バッシュ突進時間 (秒)
    float bashSpeed = 24.0f;          // バッシュ突進速度 (m/s)
    float invincibilityAfter = 0.20f; // ガード解除後無敵余韻 (秒)
  } guard;

  struct AttackAction {
    float duration = 0.36f;         // 攻撃全体の持続時間 (秒)
    float windupDuration = 0.04f;   // 近接攻撃の構え（予備動作）時間 (秒)
    float rushDuration = 0.22f;     // 打撃AABBヒットボックス持続時間 (秒)
    float recoveryDuration = 0.10f; // 打撃後のフォロースルー硬直時間 (秒)
    float rushSpeedMultiplier =
        0.0f; // 攻撃時の前進移動倍率（0.0 =
              // 体当たりせずその場でヒットボックス発生）
    float invincibilityAfter = 0.20f; // 攻撃終了後無敵余韻 (秒)
  } attackAction;

  struct DashAttack {
    float windupDuration = 0.03f;     // ダッシュ攻撃の予備動作時間 (秒)
    float rushDuration = 0.24f;       // 突進打撃AABBヒットボックス持続時間 (秒)
    float recoveryDuration = 0.10f;   // フォロースルー硬直時間 (秒)
    float rushSpeed = 24.0f;          // ダッシュ攻撃時の突進速度 (m/s)
    float invincibilityAfter = 0.20f; // 終了後無敵余韻 (秒)
  } dashAttack;

  struct RapidFire {
    float fireInterval = 0.09f;     // 連射間隔 (秒: 0.09 ≒ 秒間11発)
    float bulletSpeed = 0.72f;      // 実弾機関砲の高初速
    float bulletSpreadY = 0.025f;   // 連射時の上下ブレ幅
    float maxHeat = 1.0f;           // オーバーヒート最大熱量
    float heatPerShot = 0.08f;      // 1発あたりの発熱量
    float heatCoolRate = 0.35f;     // 通常時の冷却速度 (/秒)
    float overheatCoolRate = 0.45f; // オーバーヒート時のクールダウン速度 (/秒)
  } rapidFire;

  struct DropThrough {
    float duration = 0.30f;     // すり抜け足場床判定無視時間 (秒)
    float downVelocity = -6.0f; // すり抜け降下初速 (m/s)
  } dropThrough;
};

class InputHandler;
class Input;
class Camera;
class RailMover;
class RailPath;
class IPlayerBehavior;
class Robot;
class Enemy;
class Scene;

class Player : public GameObject {
public:
  Player();
  ~Player();

  void OnCollision(GameObject *other) override;

  Vector3 GetWorldPosition() const override { return object_->GetTranslate(); }

  Vector3 GetCenterPosition() const {
    if (object_) {
      return object_->GetTranslate(); // 1m立方体の原点はモデル中心
    }
    return {0.0f, 0.0f, 0.0f};
  }

  CollisionCategory GetCategory() const override {

    return CollisionCategory::Player;
  }

  void Initialize();
  void Update();
  void UpdateTransform();
  void UpdateRailPath();
  void Draw();

  void SetCamera(Camera *camera) {
    camera_ = camera;
    if (object_) {
      object_->SetCamera(camera);
    }
    if (weaponObject_) {
      weaponObject_->SetCamera(camera);
    }
  }
  void SetRailPosition(const Vector2 &position);

  void SetPosition(const Vector3 &position) {
    if (object_) {
      object_->SetTranslate(position);
    }
  }
  void AddPosition(const Vector3 &delta) override {
    if (object_) {
      object_->SetTranslate(object_->GetTranslate() + delta);
    }
  }
  void SetTransform(const EulerTransform &transform) {
    if (object_) {
      object_->SetScale(transform.scale);
      object_->SetRotate(transform.rotate);
      object_->SetTranslate(transform.translate);
    }
  }
  EulerTransform GetTransform() const {
    if (object_) {
      return {object_->GetScale(), object_->GetRotate(),
              object_->GetTranslate()};
    }
    return {};
  }
  Vector3 GetVelocity() const { return velocity_; }
  void SetVelocity(const Vector3 &velocity) { velocity_ = velocity; }

  IPlayerBehavior *GetBehavior() const {
    return baseState_ ? baseState_->GetBehavior() : nullptr;
  }
  IPlayerState *GetState() const { return baseState_.get(); }

  void SetAngle(float angle) { playerAngle_ = angle; }
  void AddVelocity(Vector3 v);

  void SetRail(RailPath *rail);
  void ChangeState(std::unique_ptr<IPlayerState> newState);
  void ChangeBehavior(std::unique_ptr<IPlayerBehavior> newBehavior);

  void Move(float ratio);
  void MoveAtSpeed(float speedMetersPerSec);
  void Jump();
  bool TryExecuteBufferedJump();

  // スクワッシュ＆ストレッチ（伸縮演出）のトリガーと更新
  void TriggerSquashStretch(float intensityY, float duration);
  void UpdateSquashStretch();

  // サブパーツ（手足）アニメーション制御
  void StartAttackAnimation();
  void StopAttackAnimation();
  void SetDashStepActive(bool active) {
    isDashStepActive_ = active;
    if (!active) {
      dashStepProgress_ = 0.0f;
    }
  }
  bool IsDashStepActive() const { return isDashStepActive_; }
  void SetDashStepProgress(float progress) {
    dashStepProgress_ = std::clamp(progress, 0.0f, 1.0f);
  }
  float GetDashStepRecoveryRatio() const {
    return isDashStepActive_ ? dashStepProgress_ : 1.0f;
  }
  void UpdateSubPartAnimation();

  void Attack();

  bool CanShoot() const {
    return shootCooldownTimer_ <= 0.0f && !isShootOverheated_;
  }
  void ResetShootCooldown();
  float GetShootEnergyRatio() const {
    if (config_.rapidFire.maxHeat <= 0.0f) {
      return 1.0f;
    }
    return std::clamp(1.0f - (shootHeat_ / config_.rapidFire.maxHeat), 0.0f, 1.0f);
  }
  bool IsShootOverheated() const { return isShootOverheated_; }
  bool IsLandingStaggered() const { return landingRecoveryTimer_ > 0.0f; }
  void CancelLandingStagger() { landingRecoveryTimer_ = 0.0f; }
  float GetLandingRecoveryTimer() const { return landingRecoveryTimer_; }
  bool IsShootingStance() const { return shootSlowdownTimer_ > 0.0f; }

  // ホバージャンプ制御＆UI連携
  float GetJumpStartY() const { return jumpStartY_; }
  void SetJumpStartY(float y) { jumpStartY_ = y; }
  void SetHovering(bool hovering) { isHovering_ = hovering; }
  bool IsHovering() const { return isHovering_; }
  void ConsumeHoverEnergy(float deltaTime);
  void ResetHoverEnergy() { hoverEnergy_ = 1.0f; }
  float GetHoverEnergyRatio() const {
    return std::clamp(hoverEnergy_, 0.0f, 1.0f);
  }
  bool IsHoverConditionMet() const;
  bool IsHoverIconActive() const {
    return isHovering_ || IsHoverConditionMet();
  }

  // ダッシュ攻撃制御
  void SetDashAttackActive(bool active) { isDashAttackActive_ = active; }
  bool IsDashAttackActive() const { return isDashAttackActive_; }

  float GetRailProgress() const;
  float GetCurrentDistance() const;
  const RailPath *GetRailPath() const;

  const RailMover *GetRailMover() const { return railMover_.get(); }
  bool IsGround() const { return isGrounded_; }

  enum class MoveState {
    Idle, // 待機
    Walk, // 歩き
    Dash  // 走り（ダッシュ）
  };

  MoveState GetMoveState() const { return moveState_; }
  bool IsDashing() const { return isDashing_; }
  void SetDashing(bool dashing) { isDashing_ = dashing; }
  bool IsMoving() const { return isMoving_; }
  float GetWalkSpeed() const { return config_.movement.walkSpeed; }
  float GetDashSpeed() const { return config_.movement.dashSpeed; }

  // JSON設定の読み込み・保存・コライダー適用
  void LoadConfig(const std::string &filePath = "resources/config/player.json");
  void SaveConfig(const std::string &filePath = "resources/config/player.json");
  void ApplyColliderConfig();

  const PlayerConfig &GetConfig() const { return config_; }
  PlayerConfig &GetConfig() { return config_; }

  Vector3 GetDirection() const;
  int GetMoveDirection() const;
  void SetFacingDirection(int dir);
  void CheckGroundCollision();
  void UpdateGravity();
  void
  RayCastUpdate() override; // 互換性のため残し、中でUpdateRayCollisionsを呼ぶ
  const CollisionRayInfo *GetRayInfo(const std::string &name) const;

  Collider *GetAttackCollider() const { return attackCollider_.get(); }
  void SetAttackHitboxActive(bool active);
  bool IsAttackHitboxActive() const;
  bool IsAttacking() const;
  float GetMeleeRecoveryRatio() const {
    if (!IsAttacking()) {
      return 1.0f;
    }
    const float totalDuration =
        isDashAttackActive_
            ? (config_.dashAttack.windupDuration +
               config_.dashAttack.rushDuration +
               config_.dashAttack.recoveryDuration)
            : (config_.attackAction.windupDuration +
               config_.attackAction.rushDuration +
               config_.attackAction.recoveryDuration);
    if (totalDuration <= 0.0f) {
      return 1.0f;
    }
    return std::clamp(attackAnimTimer_ / totalDuration, 0.0f, 1.0f);
  }
  void OnAttackHit(GameObject *target);

  bool IsHit() const { return isDamaged_; }
  float GetHitVisualTimer() const { return hitInvincibilityTimer_; }
  void SetInvincible(bool invincible) { isInvincible_ = invincible; }
  void TriggerInvincibility(float duration) {
    actionInvincibilityTimer_ = (std::max)(actionInvincibilityTimer_, duration);
  }
  void ResetHitInvincibility() {
    hitInvincibilityTimer_ = 0.0f;
    actionInvincibilityTimer_ = 0.0f;
  }

  InputHandler *GetInputHandler() const { return inputHandler_.get(); }
  const char *GetStateName() const;
  const char *GetBehaviorName() const;

  void SetScene(Scene *scene);
  Scene *GetScene() const { return scene_; }

  float GetWorldY() const { return worldY_; }

  bool IsAlive() const { return isAlive_; }
  bool IsDead() const { return !isAlive_; }
  void SetAlive(bool alive) { isAlive_ = alive; }
  void Die() { isAlive_ = false; }

  bool IsActive() const { return isActive_; }
  bool SetActive(bool active) {
    isActive_ = active;
    return isActive_;
  }
  bool IsGrounded() const { return isGrounded_; }
  bool HasGroundBelow() const { return rayHitParam_.groundY > -1e30f; }
  float GetGroundBelowY() const { return rayHitParam_.groundY; }
  bool IsRayHit() const { return GameObject::IsRayHit(); }

  const Triangle &GetRayHitTriangle() const {
    return GameObject::GetRayHitTriangle();
  }
  int GetHitPoints() const { return hitPoints_.value; }
  int GetMaxHitPoints() const { return hitPoints_.max; }

  void SetDeltaTime(float deltaTime = DXCommon::kDeltaTime) {
    deltaTime_ = deltaTime;
  }
  float GetDeltaTime() const { return deltaTime_; }

  void SetGravityScale(float scale) { gravityScale_ = scale; }

  void TakeDamage(int knockbackDirection = -1);
  bool IsKnockback() const { return isKnockback_; }

  void PlayHitSE() {
    playHundle_ = Audio::GetInstance()->PlayAudio(HitSE_, false, 1.0f);
  }

  Object3d *GetObject3d() const { return object_.get(); }

private:
  float deltaTime_ = DXCommon::kDeltaTime;

  std::unique_ptr<IPlayerState> baseState_;
  Scene *scene_ = nullptr;
  std::unique_ptr<InputHandler> inputHandler_;
  std::unique_ptr<Object3d> object_;
  std::unique_ptr<Object3d> weaponObject_;
  std::unique_ptr<Animation> animation;

  PlayerConfig config_;
  const float kWalkSpeed_ = 7.0f;  // 通常歩き速度 (m/s) ★演出速度と完全一致
  const float kDashSpeed_ = 13.0f; // ダッシュ走り速度 (m/s)
  const float kMoveSpeed_ = 12.0f; // 互換用

  MoveState moveState_ = MoveState::Idle;
  bool isDashing_ = false;
  bool isMoving_ = false;
  int frameMoveDir_ = 0; // 現フレームの実移動方向（1: 前進, -1: 後退, 0: 静止）

  // スクワッシュ＆ストレッチ（Squash & Stretch）演出パラメータ
  struct SquashStretchState {
    float timer = 0.0f;
    float duration = 0.0f;
    float intensityY = 0.0f; // 正: 縦伸び(ジャンプ), 負: 縦潰れ(着地)
    bool isActive = false;
  };
  SquashStretchState squashStretch_;
  bool wasGrounded_ = true;       // 前フレームの接地フラグ（着地瞬間検知用）
  float landingFallSpeed_ = 0.0f; // 着地直前の落下速度
  Vector3 baseScale_ = {1.0f, 1.0f, 1.0f}; // 基本スケール

  // コヨーテタイム & 先行入力用タイマー
  float coyoteTimer_ = 0.0f;
  float jumpBufferTimer_ = 0.0f;
  float jumpStartY_ = 0.0f;           // ジャンプ開始時のY座標（ホバー高度上限計算用）
  float hoverEnergy_ = 1.0f;          // ホバーゲージ残量 (0.0～1.0)
  bool isHovering_ = false;           // ホバージャンプ中フラグ
  bool isDashAttackActive_ = false;   // ダッシュ派生攻撃中フラグ
  float dashEffectTimer_ = 0.0f;      // ダッシュエフェクト発生間隔タイマー
  float shootCooldownTimer_ = 0.0f;   // 連射クールダウンタイマー
  float shootSlowdownTimer_ = 0.0f;   // 連射反動中の腰だめ歩行タイマー
  float shootRecoilTimer_ = 0.0f;     // 発砲1発ごとのリコイル振動タイマー
  float shootHeat_ = 0.0f;            // マシンガン熱量 (0.0～maxHeat)
  bool isShootOverheated_ = false;    // オーバーヒート中フラグ
  float landingRecoveryTimer_ = 0.0f; // 着地時の怯み（硬直）タイマー
  float squashGroundedOffsetY_ =
      0.0f; // 1m立方体の底面を地面に固定するスクワッシュY補正

  void HandleInput();
  void HandleDamage();
  void HandleKnockback();
  void HandleAlive();

  void InitializeRays();
  void UpdateRayCollisions();

  Camera *camera_ = nullptr;
  void ImGuiDrawDebugInfo();

  Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
  Vector3 wallPushOffset_ = {0.0f, 0.0f, 0.0f}; // 壁からの押し返し用オフセット
  float worldY_ = 0.0f;

  float gravityScale_ = 1.0f;
  bool isGrounded_ = true;
  bool isJumping_ = false;             // ジャンプ中フラグ（吸着解除用）
  float dropThroughTimer_ = 0.0f;      // すり抜け足場（OneWay）下層降下タイマー
  bool isCurrentGroundOneway_ = false; // 現在乗っている床がすり抜け足場かどうか
  float lastGroundY_ = 0.0f;           // 直前フレームの確定接地高さ（段差落下判定用）
  float prevSafeDistance_ = 0.0f;      // 前フレームの確定レール距離（壁スイープ判定用）
  bool hasPrevSafeDistance_ = false;   // 前フレーム距離が記録済みか

  // 地面レイキャスト判定パラメータ
  struct GroundRayParam {
    float groundY = 0.0f;
    float rayOffset = 1.0f;
    const float minY = -10.0f;     // 地面の最低Y座標
    const float kRayOffset = 2.0f; // レイの始点を上に持ち上げるオフセット
  };
  GroundRayParam rayHitParam_;

  std::unique_ptr<RailMover> railMover_;
  float playerAngle_ = -10.0f;

  // 攻撃用ヒットボックス（Hitbox）
  std::unique_ptr<Collider> attackCollider_;
  bool isAttackHitboxActive_ = false;   // 攻撃ヒットボックスが現在有効か
  bool debugForceAttackHitbox_ = false; // デバッグ用：常時有効化トグル

  bool isDamaged_ = false;
  Gauge hitPoints_ = {6, 6};
  float hitInvincibilityTimer_ = 0.0f; // 被弾時の無敵＆点滅タイマー
  float actionInvincibilityTimer_ =
      0.0f; // 攻撃・回避後の無敵余韻タイマー（点滅しない）

  bool isKnockback_ = false;
  float knockbackTimer_ = 0.0f;
  int knockbackDirection_ = -1; // -1: 手前(後退), 1: 奥(前進)
  float currentAngle_ = 0.0f;   // 現在の回転角度
  RailMover::MoveDirection savedFacingDirection_ =
      RailMover::MoveDirection::Forward;
  bool isInvincible_ = false;

  bool isAlive_ = true;
  bool isActive_ = true;

  std::vector<CollisionRayInfo> rayList_;

  Audio::SoundHandle HitSE_ = 0;
  Audio::SoundHandle DamageSE_ = 0;
  Audio::VoiceHandle playHundle_ = 0;

  // サブパーツ（手足）アニメーション状態
  float walkAnimTimer_ = 0.0f;
  bool isAttackAnimActive_ = false;
  bool isDashStepActive_ = false;
  float dashStepProgress_ = 0.0f;
  float attackAnimTimer_ = 0.0f;
  EulerTransform rightHandAnim_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  EulerTransform leftHandAnim_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  EulerTransform rightFootAnim_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  EulerTransform leftFootAnim_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
};
