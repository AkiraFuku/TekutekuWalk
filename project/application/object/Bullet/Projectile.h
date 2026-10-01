#pragma once
#include "Vector3.h"
#include "Object3d.h"
#include <memory>
#include "GameObject.h"
class RailMover;
class RailPath;
class Camera;
class Scene;
/// <summary>
/// プレイヤーが射出する弾の基底クラス
/// </summary>
class Projectile : public GameObject {
public:
    enum class ProjectileOwner {
        Unknown,
        Player,
        Enemy
    };
    // Projectile.h 辺りに追加
    struct ProjectileSpawnParam {
        Vector2 position;  // 生成位置 (RailProgress t, WorldY)
        Vector2 direction; // 飛んでいく方向ベクトル
        float speed;       // 弾の速さ
    };

    Projectile();
    virtual ~Projectile();
    // コリジョン
  Vector3 GetWorldPosition() const override;
    float GetRadius() const {
        return radius_;
    }
    void OnCollision(GameObject* other) override;
    CollisionCategory GetCategory() const override;

    // 初期化: 走行するレール、開始地点(t)、速度(正なら終点方向、負なら始点方向)
   virtual void Initialize(const RailPath* path, const ProjectileSpawnParam& param, ProjectileOwner owner);
    virtual void Update();
    virtual void Draw();

    bool IsDead() const {
        return isDead_;
    }
  
    void SetCamera(Camera* camera) {
        camera_ = camera;
        if (object_) {
            object_->SetCamera(camera);
        }
    }
    void SetScene(Scene* scene) { scene_ = scene; }
    ProjectileOwner GetOwner() const { return owner_; }
protected:
    std::unique_ptr<RailMover> railMover_;
    std::unique_ptr<Object3d> object_;
    Camera* camera_ = nullptr;
    float worldY_ = 0.0f; // 追加：弾の高度を保持
    float velocityY_ = 0.0f;  // 追加：Y方向の移動速度
    float speed_ = 0.0f;
    float radius_ = 0.5f;
    bool isDead_ = false;
    int lifeTimer_ = 180; // 3秒程度(60fps)
    Scene* scene_ = nullptr;

    // 地形・壁との衝突判定
    bool CheckMapCollision(const Vector3& prevPos, const Vector3& finalPos);

private:
    ProjectileOwner owner_=ProjectileOwner::Unknown; // 持ち主を保持
};