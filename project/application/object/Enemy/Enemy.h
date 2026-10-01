#pragma once
#include <memory>
#include "Transform.h"
#include "Object3d.h"
#include "GameObject.h"
#include "Scene.h"

class Camera;
class RailMover;
class RailPath;
class IEnemyBehavior; // 前方宣言
class IEnemyState; // 前方宣言
class Player; // Enemy.h の場合
class Robot; // Enemy.h の場合
class ParticleEmitter; // Enemy.h の場合

class Enemy : public GameObject
{
public:

    enum class EnemyType
    {
        Normal,
        Bound,
        Chase,
        Hover,
        Shield,
    };


    Enemy();
    virtual ~Enemy();
    //コリジョン

    float GetRadius() const {
        return radius_;
    }
    virtual void OnCollision(GameObject* other) override; // Enemy側
    virtual void OnCollideWithPlayer(Player* player);     // プレイヤーと通常接触（体当たり）した際のコールバック
    Vector3 GetWorldPosition() const override {
        return object_->GetTranslate();
    }
    CollisionCategory GetCategory() const override {
        return CollisionCategory::Enemy;
    }


    virtual void Initialize();
    virtual void Update();
    virtual void UpdateTransform();
    virtual void Draw();

    void SetCamera(Camera* camera) {
        camera_ = camera;
        if (object_) {
            object_->SetCamera(camera);
        }
    }

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
    void SetRailPosition(const Vector2& position);


    void SetRail(RailPath* rail);
    void Move(float ratio); // 進行させるメソッド

    // 行動（Behavior）を切り替えるメソッド
    void ChangeBehavior(std::unique_ptr<IEnemyBehavior> newBehavior);

    // 状態（State）を切り替えるメソッド
    void ChangeState(std::unique_ptr<IEnemyState> newState);

    // プレイヤーと同様に、IsGround() などの判定があると Behavior 側で便利です
    bool IsGround() const {
        return isGrounded_;
    }

    void SetMoveSpeed(float speed) {
        moveSpeed_ = speed;
    }
    float GetMoveSpeed() const {
        return moveSpeed_;
    }

    //レイキャスト判定処理
    void RayCastUpdate()override;
    void UpdateGravity(); // 重力の更新処理
    // enemy状態取得
    IEnemyState* GetState() const {
        return state_.get();
    }
    // enemy行動ビヘイビア取得
    IEnemyBehavior* GetBehavior() const {
        return behavior_.get();
    };
    //状態名称取得
    const char* GetStateName() const;
    // 行動ビヘイビアの名称取得
    const char* GetBehaviorName() const;
    // 敵が死んでいるかどうかを判定
    bool IsDead() const;
    bool IsKnockback() const { return isKnockback_; }
    void TakeDamage(float knockbackDirection = -1.0f, bool isDeadly = true);
    void HandleKnockback();

    // ロボットを設定するメソッド（外部または派生クラスのInitializeで呼ぶ）
    void SetRobot(std::unique_ptr<Robot> robot);
    Robot* GetRobot() const {
        return robot_.get();
    }

    void SetScene(Scene* scene) {
        scene_ = scene;
    }
    Scene* GetScene() const {
        if (scene_)
        {
            return scene_;
        }
        return nullptr;
    }

    // 外部（GameSceneのUpdate等）から毎フレームのdeltaTimeを受け取るためのセッター
    void SetDeltaTime(float deltaTime) {
        deltaTime_ = deltaTime;
    }

    void SetVelocity(Vector3 velocity) {
        velocity_ = velocity;
        if (velocity_.y > 0.0f) {
            isGrounded_ = false;
        }
    }
    void AddVelocity(Vector3 velocity) {

        velocity_ += velocity;
    }
    Vector3 GetVelocity() const {
        return velocity_;
    }

    //重力の落下スケール
    void SetGravityScale(float Scale = 1.0f) {

        gravityScale_ = Scale;
    };
    float GetCurrentDistance() const;
    const RailMover* GetRailMover() const;
    float GetDeltaTime() const {
        return deltaTime_;
    }

    float GetGroundY() const {

        return rayHitParam_.groundY;
    }

    float GetMoveDirection() const { return moveDirection_; }
    void SetMoveDirection(float dir) { moveDirection_ = (dir >= 0.0f) ? 1.0f : -1.0f; }
    void Turn() { moveDirection_ *= -1.0f; turnCooldownTimer_ = kTurnCooldown_; }
    bool IsWallHit() const { return isWallHit_; }

    // --- 必要なアクセサのみに整理 ---
    void SetTexture(const std::string& texturePath) { if (object_) { object_->SetTexture(texturePath); } }
    RailMover* GetRailMover() { return railMover_.get(); }
    Camera* GetCamera() const { return camera_; }
    float GetGravityScale() const { return gravityScale_; }
    bool IsDamaged() const { return isDamaged_; }
    bool IsDeathFinished() const { return isDeathFinished_; }
    void SetRadius(float radius) { radius_ = radius; }
    EnemyType GetEnemyType() const { return enemyType_; }
    void SetEnemyType(EnemyType type) { enemyType_ = type; }
    const Vector3& GetInitialRotationOffset() const { return initialRotationOffset_; }
    void SetInitialRotationOffset(const Vector3& offset) { initialRotationOffset_ = offset; }

protected:
    void UpdatePhysics(); // 重力やレール座標の合成処理
    void PlayHitEffect(); // パーティクルを発生させるヘルパー関数

private:
    float deltaTime_ = DXCommon::kDeltaTime; // フレームレートに合わせたデルタタイム

    float moveDirection_ = 1.0f;             // 進行方向 (1.0f: 順方向, -1.0f: 逆方向)
    float turnCooldownTimer_ = 0.0f;         // 連続反転防止タイマー
    const float kTurnCooldown_ = 0.5f;       // 反転クールダウン（秒）
    bool isWallHit_ = false;                 // 前方壁衝突フラグ
    float wallRayDistance_ = FLT_MAX;        // 前方壁までの距離

    std::unique_ptr<Object3d> object_;
    std::unique_ptr<RailMover> railMover_; // unique_ptrに変更
    Camera* camera_ = nullptr;

    float moveSpeed_ = 6.0f;
    std::unique_ptr<IEnemyBehavior> behavior_; // 現在の行動状態
    std::unique_ptr<IEnemyState> state_; // 現在の状態

    // 物理・移動関連の変数（Playerを参考に）
    bool isGrounded_ = true;
    float worldY_ = 0.0f;
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    float gravityScale_ = 1.0f; // 重力のスケール
    const float kGravity = -50.0f;           // 重力加速度（毎フレーム引く値）

    bool isDamaged_ = false;              // クールダウン中かどうかのフラグ
    float hitInvincibilityTimer_ = 0.0f;     // クールダウンタイマー
    const float kHitInvincibilityDuration_ = 1.0f; // クールダウン時間（秒単位にする場合はUpdateの計算に合わせる）

    // ノックバック・放物線・のけぞり演出用変数
    bool isKnockback_ = false;
    float knockbackTimer_ = 0.0f;
    const float kKnockbackDuration_ = 0.6f;  // 放物線滞空時間（秒）
    const float kKnockbackSpeed_ = 7.0f;     // 後退速度
    const float kKnockbackJumpForce_ = 13.0f;// 上方向への跳ね上がり力（放物線）
    float knockbackDirection_ = -1.0f;       // レール上の移動向き（1.0f: 前進, -1.0f: 後退）
    float knockbackTilt_ = 0.0f;             // 後ろへののけぞり角度（度数法）
    float knockbackRoll_ = 0.0f;             // 撃破時の回転角度
    bool isDeathFinished_ = false;           // やられ演出完了フラグ
    // 基底クラスで保持するように変更
    std::unique_ptr<Robot> robot_ = nullptr;

    // パーティクルエミッタの保持
    std::unique_ptr<ParticleEmitter> hitParticle_;

    //enemyの当たり判定
    float radius_ = 1.0f; // 当たり判定の半径

    Scene* scene_ = nullptr; // Enemyが所属するシーンへのポインタ

    // 地面レイキャスト判定パラメータ
    struct GroundRayParam {
        float groundY = 0.0f;
        float rayOffset = 1.0f;
        const float minY = -10.0f;     // 地面の最低Y座標
        const float kRayOffset = 2.0f; // レイの始点を上に持ち上げるオフセット
    };
    GroundRayParam rayHitParam_;
    const float kHeightOffset = 0.5f; // プレイヤーの高さオフセット（地面からの距離）
    EnemyType enemyType_ = EnemyType::Normal;
    Vector3 initialRotationOffset_ = { 0.0f, 0.0f, 0.0f };
};