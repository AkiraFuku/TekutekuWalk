// Collider.h
#pragma once
#include "Vector3.h"
#include "GameObject.h"
#include "DrawFunction.h"
#include "Quaternion.h"
#include <vector>
#include <string>

class GameObject;

// コライダーを構成する球体パーツの定義
struct CollisionSphereDef {
    std::string name;                                        // パーツ名（"Head", "Body" など）
    Vector3 offset = { 0.0f, 0.0f, 0.0f };                   // オーナー中心からのローカルオフセット
    float radius = 1.0f;                                     // 球の半径
    Quaternion localRotate = { 0.0f, 0.0f, 0.0f, 1.0f };     // ローカル回転
};

class Collider {
public:
    Collider() = default;
    ~Collider() = default;

    void initialize(GameObject* owner, float radius);
    void Update();
    void Draw();

    void SetOffset(const Vector3& offset) {
        Offset_ = offset;
    }
    const Vector3& GetOffset() const { return Offset_; }

    void SetPosition(const Vector3& position) {
        position_ = position;
    }

    void SetRotation(const Quaternion& rotation) {
        rotation_ = rotation;
    }
    const Quaternion& GetRotation() const { return rotation_; }

    // 持ち主（GameObject）から座標を取得
    Vector3 GetWorldPosition() const;

    float GetRadius() const { return radius_; }
    void SetRadius(float radius) { radius_ = radius; }

    CollisionCategory GetCategory() const { return category_; }
    void SetCategory(CollisionCategory category) { category_ = category; }

    GameObject* GetOwner() const { return owner_; }

    // 衝突検知時に呼び出す（処理はオーナーに委譲）
    void OnCollision(Collider* other);

    bool IsCollide() const {
        return isCollide_;
    }
    void SetCollide(bool collide) {
        isCollide_ = collide;
    }

    // ─── Sphere 構造体によるモデル形状対応（マルチスフィア） ───
    void ClearSpheres() { sphereDefs_.clear(); }
    void AddSphere(const std::string& name, const Vector3& offset, float radius, const Quaternion& localRotate = { 0.0f, 0.0f, 0.0f, 1.0f });
    bool SetSphereOffset(const std::string& name, const Vector3& offset);
    bool SetSphereRadius(const std::string& name, float radius);
    const std::vector<CollisionSphereDef>& GetSphereDefs() const { return sphereDefs_; }

    // ワールド座標系での全 Sphere（center, radius, rotate）を取得
    std::vector<Sphere> GetWorldSpheres() const;

private:
    Vector3 position_ = {};
    Vector3 Offset_ = {};
    Quaternion rotation_ = { 0.0f, 0.0f, 0.0f, 1.0f };
    GameObject* owner_ = nullptr;
    CollisionCategory category_ = CollisionCategory::Player;
    float radius_ = 1.0f;
    bool isCollide_ = true; // 衝突可能か？

    std::vector<CollisionSphereDef> sphereDefs_; // モデル形状に合わせた球体定義リスト
};
