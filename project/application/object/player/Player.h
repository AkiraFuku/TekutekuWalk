#pragma once
#include <memory>
#include "Transform.h"
#include "Object3d.h"
#include "Behavior/PlayerState.h"
#include "GameObject.h"
#include "Animation.h"
#include "RailMover.h"
#include "Audio.h"

class InputHandler;
class Input;
class Camera;
class RailMover;
class RailPath;
class IPlayerBehavior;
class Robot;
class Enemy;
class Scene;

class Player : public GameObject
{
public:
    Player();
    ~Player();

    void OnCollision(GameObject* other) override;

    Vector3 GetWorldPosition() const override {
        return object_->GetTranslate();
    }

    Vector3 GetCenterPosition() const {
        if (object_) {
            Vector3 pos = object_->GetTranslate();
            pos.y += 0.80f; // 足元からモデル中心（身長約1.65mの中間）
            return pos;
        }
        return { 0.0f, 0.0f, 0.0f };
    }

    CollisionCategory GetCategory() const override {

        return CollisionCategory::Player;
    }

    void Initialize();
    void Update();
    void UpdateTransform();
    void UpdateRailPath();
    void Draw();

    void SetCamera(Camera* camera) {
        camera_ = camera;
        if (object_) {
            object_->SetCamera(camera);
        }
    }
    void SetRailPosition(const Vector2& position);

    void SetPosition(const Vector3& position) {
        if (object_) {
            object_->SetTranslate(position);
        }
    }
    void SetTransform(const EulerTransform& transform) {
        if (object_) {
            object_->SetScale(transform.scale);
            object_->SetRotate(transform.rotate);
            object_->SetTranslate(transform.translate);
        }
    }
    EulerTransform GetTransform() const {
        if (object_) {
            return { object_->GetScale(), object_->GetRotate(), object_->GetTranslate() };
        }
        return {};
    }
    Vector3 GetVelocity() const {
        return velocity_;
    }
    void SetVelocity(const Vector3& velocity) {
        velocity_ = velocity;
    }

    IPlayerBehavior* GetBehavior() const {
        return baseState_ ? baseState_->GetBehavior() : nullptr;
    }
    IPlayerState* GetState() const {
        return baseState_.get();
    }

    void SetAngle(float angle) {
        playerAngle_ = angle;
    }
    void AddVelocity(Vector3 v);

    void SetRail(RailPath* rail);
    void ChangeState(std::unique_ptr<IPlayerState> newState);
    void ChangeBehavior(std::unique_ptr<IPlayerBehavior> newBehavior);


    void Move(float ratio);
    void Jump();
    bool TryExecuteBufferedJump();

    // スクワッシュ＆ストレッチ（伸縮演出）のトリガーと更新
    void TriggerSquashStretch(float intensityY, float duration);
    void UpdateSquashStretch();

    void Attack();

    float GetRailProgress() const;
    float GetCurrentDistance() const;
    const RailPath* GetRailPath() const;

    const RailMover* GetRailMover() const {
        return railMover_.get();
    }
    bool IsGround() const {
        return isGrounded_;
    }

    enum class MoveState {
        Idle, // 待機
        Walk, // 歩き
        Dash  // 走り（ダッシュ）
    };

    MoveState GetMoveState() const { return moveState_; }
    bool IsDashing() const { return isDashing_; }
    void SetDashing(bool dashing) { isDashing_ = dashing; }
    bool IsMoving() const { return isMoving_; }
    float GetWalkSpeed() const { return kWalkSpeed_; }
    float GetDashSpeed() const { return kDashSpeed_; }

    Vector3 GetDirection() const;
    int GetMoveDirection() const;
    void CheckGroundCollision();
    void UpdateGravity();
    void RayCastUpdate() override; // 互換性のため残し、中でUpdateRayCollisionsを呼ぶ
    const CollisionRayInfo* GetRayInfo(const std::string& name) const;

    Collider* GetAttackCollider() const {
        return attackCollider_.get();
    }
    void SetAttackHitboxActive(bool active);
    bool IsAttackHitboxActive() const;
    bool IsAttacking() const;
    void OnAttackHit(GameObject* target);

    bool IsHit() const {
        return isDamaged_;
    }
    float GetHitVisualTimer() const {
        return hitInvincibilityTimer_;
    }
    void SetInvincible(bool invincible) {
        isInvincible_ = invincible;
    }
    void TriggerInvincibility(float duration) {
        hitInvincibilityTimer_ = (std::max)(hitInvincibilityTimer_, duration);
    }

    InputHandler* GetInputHandler() const {
        return inputHandler_.get();
    }
    const char* GetStateName() const;
    const char* GetBehaviorName() const;

    void SetScene(Scene* scene);
    Scene* GetScene() const {
        return scene_;
    }

    float GetWorldY() const {
        return worldY_;
    }

    bool IsAlive() const {
        return isAlive_;
    }
    bool IsDead() const {
        return !isAlive_;
    }
    void SetAlive(bool alive) {
        isAlive_ = alive;
    }
    void Die() {
        isAlive_ = false;
    }

    bool IsActive() const {
        return isActive_;
    }
    bool SetActive(bool active) {
        isActive_ = active;
        return isActive_;
    }
    bool IsGrounded() const {
        return isGrounded_;
    }
    bool IsRayHit() const {
        return GameObject::IsRayHit();
    }

    const Triangle& GetRayHitTriangle() const {
        return GameObject::GetRayHitTriangle();
    }
    int GetHitPoints() const {
        return hitPoints_.value;
    }
    int GetMaxHitPoints() const {
        return hitPoints_.max;
    }

    void SetDeltaTime(float deltaTime = DXCommon::kDeltaTime) {
        deltaTime_ = deltaTime;
    }
    float GetDeltaTime() const {
        return deltaTime_;
    }

    void SetGravityScale(float scale) {
        gravityScale_ = scale;
    }

    void TakeDamage(int knockbackDirection = -1);
    bool IsKnockback() const {
        return isKnockback_;
    }


    void PlayHitSE() {
        playHundle_ = Audio::GetInstance()->PlayAudio(HitSE_, false, 1.0f);

    }

private:
    float deltaTime_ = DXCommon::kDeltaTime;

    std::unique_ptr<IPlayerState> baseState_;
    Scene* scene_ = nullptr;
    std::unique_ptr<InputHandler> inputHandler_;
    std::unique_ptr<Object3d> object_;
    std::unique_ptr<Animation> animation;

    const float kWalkSpeed_ = 7.5f;   // 通常歩き速度 (m/s)
    const float kDashSpeed_ = 15.0f;  // ダッシュ走り速度 (m/s)
    const float kMoveSpeed_ = 12.0f;  // 互換用

    MoveState moveState_ = MoveState::Idle;
    bool isDashing_ = false;
    bool isMoving_ = false;

    // スクワッシュ＆ストレッチ（Squash & Stretch）演出パラメータ
    struct SquashStretchState {
        float timer = 0.0f;
        float duration = 0.0f;
        float intensityY = 0.0f; // 正: 縦伸び(ジャンプ), 負: 縦潰れ(着地)
        bool isActive = false;
    };
    SquashStretchState squashStretch_;
    bool wasGrounded_ = true;           // 前フレームの接地フラグ（着地瞬間検知用）
    float landingFallSpeed_ = 0.0f;     // 着地直前の落下速度
    Vector3 baseScale_ = { 1.0f, 1.0f, 1.0f }; // 基本スケール
    float jumpStretchIntensity_ = 0.25f;   // ジャンプ時縦伸び量 (1.25倍)
    float jumpStretchDuration_ = 0.22f;    // ジャンプ伸縮時間(秒)
    float landSquashIntensityMax_ = 0.32f; // 着地時最大縦潰れ量 (0.68倍)
    float landSquashDuration_ = 0.25f;     // 着地伸縮時間(秒)

    // コヨーテタイム & 先行入力用タイマー
    float coyoteTimer_ = 0.0f;
    float jumpBufferTimer_ = 0.0f;
    const float kCoyoteDuration_ = 0.12f;      // 崖落ち後ジャンプ猶予時間 (秒)
    const float kJumpBufferDuration_ = 0.15f;  // 着地前ジャンプ先行入力猶予時間 (秒)

    void HandleInput();
    void HandleDamage();
    void HandleKnockback();
    void HandleAlive();

    void InitializeRays();
    void UpdateRayCollisions();

    Camera* camera_ = nullptr;
    void ImGuiDrawDebugInfo();

    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    Vector3 wallPushOffset_ = { 0.0f, 0.0f, 0.0f }; // 壁からの押し返し用オフセット
    float worldY_ = 0.0f;

    float gravityScale_ = 1.0f;
    const float kGravity = -50.0f;
    const float kJumpAcceleration = 24.0f;

    // ジャンプ手触り（Juice）パラメータ
    const float kApexGravityScale = 0.45f;    // ジャンプ頂点付近の重力軽減倍率（浮遊感）
    const float kFallGravityScale = 1.35f;    // 下降中の重力増加倍率（キレのある落下）
    const float kApexThreshold = 3.5f;        // 頂点判定の垂直速度しきい値 (|velocity.y| < 3.5)

    bool isGrounded_ = true;
    bool isJumping_ = false; // ジャンプ中フラグ（吸着解除用）
    float dropThroughTimer_ = 0.0f;          // すり抜け足場（OneWay）下層降下タイマー
    bool isCurrentGroundOneway_ = false;     // 現在乗っている床がすり抜け足場かどうか

    // 地面レイキャスト判定パラメータ
    struct GroundRayParam {
        float groundY = 0.0f;
        float rayOffset = 1.0f;
        const float minY = -10.0f;     // 地面の最低Y座標
        const float kRayOffset = 2.0f; // レイの始点を上に持ち上げるオフセット
    };
    GroundRayParam rayHitParam_;
    float heightOffset_ = 0.0f;               // 地面からモデル原点（足元）までの高さオフセット
    float wallRayHeight_ = 0.40f;             // 壁検知レイの発射高さ（足元基準、膝〜腰の高さ）

    // 傾斜・段差対応パラメータ
    const float kMaxStepHeight = 0.35f;       // 乗り越えられる段差の最大高さ (m)
    const float kGroundSnapDistance = 1.2f;   // 下り坂・窪みで地面に吸着する最大距離 (m)
    const float kMaxSlopeCos = 0.65f;         // 登れる最大傾斜（cos約49度。これより急な崖は滑り落ち/壁判定）

    std::unique_ptr<RailMover> railMover_;
    float playerAngle_ = -10.0f;

    float radius_ = 0.38f;                    // コライダー互換用基本半径
    float modelRadius_ = 0.28f;               // 人型モデルの実際の幅（半径）

    // モデル形状（2頭身）にフィットさせた球体判定パラメータ
    float headOffsetY_ = 1.15f;               // 頭部スフィアの足元からの高さオフセット
    float headRadius_ = 0.38f;                // 頭部スフィアの半径
    float bodyOffsetY_ = 0.50f;               // 胴体・腰スフィアの足元からの高さオフセット
    float bodyRadius_ = 0.30f;                // 胴体・腰スフィアの半径

    // 攻撃用ヒットボックス（Hitbox）
    std::unique_ptr<Collider> attackCollider_;
    float attackOffsetY_ = 0.70f;             // 攻撃ヒットボックスの高さ（足元基準）
    float attackOffsetForward_ = 0.65f;       // 攻撃ヒットボックスの前方突き出しオフセット
    float attackRadius_ = 0.48f;              // 攻撃ヒットボックスの半径
    bool isAttackHitboxActive_ = false;       // 攻撃ヒットボックスが現在有効か
    bool debugForceAttackHitbox_ = false;     // デバッグ用：常時有効化トグル


    bool isDamaged_ = false;
    Gauge hitPoints_ = { 3, 3 };
    float hitInvincibilityTimer_ = 0.0f;
    const float kHitInvincibilityDuration_ = 1.5f; // 1.5秒間の無敵時間

    bool isKnockback_ = false;
    float knockbackTimer_ = 0.0f;
    const float kKnockbackDuration_ = 0.25f; // 0.25秒間のノックバック
    const float kKnockbackSpeed_ = 7.0f;     // ノックバックの初速
    const float kKnockbackJumpForce_ = 6.0f; // ノックバック時の軽い跳ね上げ
    int knockbackDirection_ = -1;            // -1: 手前(後退), 1: 奥(前進)
    float currentAngle_ = 0.0f;              // 現在の回転角度
    RailMover::MoveDirection savedFacingDirection_ = RailMover::MoveDirection::Forward;
    bool isInvincible_ = false;

    bool isAlive_ = true;
    bool isActive_ = true;

    std::vector<CollisionRayInfo> rayList_;

    Audio::SoundHandle HitSE_ = 0;
    Audio::SoundHandle DamageSE_ = 0;
    Audio::VoiceHandle playHundle_ = 0;
};