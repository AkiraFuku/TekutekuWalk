#pragma once
#include "Vector3.h"
#include "DrawFunction.h"
#include <memory>
#include <string>
#include <vector>

// 1本のレイの定義と判定結果
struct CollisionRayInfo {
    std::string name;         // レイの識別名（"Floor", "FrontWall" など）
    Ray ray;                  // レイの起点と方向
    bool isColide = false;    // 衝突したかどうか
    Vector3 crossPoint = {};  // 交差点（衝突地点）
    Vector3 hitNormal = {};   // 衝突面の法線（壁の押し返しなどに使用）
    float distance = FLT_MAX; // 衝突距離
    Triangle hitTriangle = {}; // 衝突したポリゴン（三角形）
};

enum class CollisionCategory {
    Player=0,// プレイヤー
    Enemy=10,// 敵
    PlayerProjectile=20,// プレイヤーの弾
    PlayerAttack=25,// プレイヤーの攻撃ヒットボックス
    EnemyProjectile=30,// 敵の弾
    Goal=40,// ゴール
    Collectible=50,// 収集アイテム
    CollisionObject=60, // めり込めないオブジェクト
    Attackable=70, // 攻撃可能オブジェクト
    InvincibleEnemy=80, // 攻撃（無敵）不可エネミー
    // 必要に応じて他のカテゴリも追加

};
class Collider;
class GameObject
{
public:
    virtual ~GameObject();

    // 衝突時に呼ばれる通知関数
    virtual void OnCollision(GameObject* other) {};

    // 判定に必要な情報のゲッター
    virtual Vector3 GetWorldPosition() const = 0;

    // Colliderのゲッター・セッター
    virtual Collider* GetCollider() {
        return collider_.get();
    }
    virtual const Collider* GetCollider() const {
        return collider_.get();
    }
    void SetCollider(std::unique_ptr<Collider> collider);

    /// <summary>
    /// ぶつかった相手のカテゴリを識別するための関数
    /// </summary>
    /// <returns></returns>
    virtual CollisionCategory GetCategory() const = 0;

    /// <summary>
    /// レイキャスト（接地判定など）の更新処理
    /// デフォルトでは何もしないため、不要なオブジェクトは実装しなくてOK
    /// </summary>
    virtual void RayCastUpdate() {};

    // レイキャスト判定結果アクセサ
    bool IsRayHit() const { return isRayHit_; }
    void SetRayHit(bool hit) { isRayHit_ = hit; }
    const Vector3& GetRayHitPoint() const { return rayHitPoint_; }
    void SetRayHitPoint(const Vector3& point) { rayHitPoint_ = point; }
    float GetRayHitDistance() const { return rayHitDistance_; }
    void SetRayHitDistance(float distance) { rayHitDistance_ = distance; }
    const Triangle& GetRayHitTriangle() const { return rayHitTriangle_; }
    void SetRayHitTriangle(const Triangle& tri) { rayHitTriangle_ = tri; }
    RayTriangleCollisionResult GetRayCollisionResult() const { return result_; }
    void SetRayCollisionResult(RayTriangleCollisionResult res) { result_ = res; }
    const Ray& GetRay() const { return ray_; }
    void SetRay(const Ray& ray) { ray_ = ray; }
    void SetRayOrigin(const Vector3& origin) { ray_.origin = origin; }
    void SetRayDiff(const Vector3& diff) { ray_.diff = diff; }


private:
    Ray ray_ = {};
    bool isRayHit_ = false;
    Vector3 rayHitPoint_ = {};
    float rayHitDistance_ = 0.0f;
    Triangle rayHitTriangle_ = {};
    RayTriangleCollisionResult result_ = RayTriangleCollisionResult::NoCollision;

    // 自身のコライダー（必要に応じて派生クラスで初期化）
    std::unique_ptr<Collider> collider_;

};

