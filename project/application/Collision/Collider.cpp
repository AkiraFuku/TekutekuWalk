#include "Collider.h"
#include "GameObject.h"
#include "PrimitiveDrawer.h"
#include "MathFunction.h"
#include "RotateFunction.h"

void Collider::initialize(GameObject* owner, float radius)
{
    owner_ = owner;
    category_ = owner->GetCategory();
    radius_ = radius;

    Offset_ = { 0.0f, 0.0f, 0.0f };
    rotation_ = { 0.0f, 0.0f, 0.0f, 1.0f };
    sphereDefs_.clear();
    aabbDefs_.clear();

    position_ = owner_->GetWorldPosition();
}

void Collider::Update()
{
    if (owner_) {
        position_ = owner_->GetWorldPosition() + Offset_;
    }
}

void Collider::AddSphere(const std::string& name, const Vector3& offset, float radius, const Quaternion& localRotate)
{
    // 同名パーツがあれば更新、なければ追加
    for (auto& def : sphereDefs_) {
        if (def.name == name) {
            def.offset = offset;
            def.radius = radius;
            def.localRotate = localRotate;
            return;
        }
    }
    sphereDefs_.push_back({ name, offset, radius, localRotate });
}

bool Collider::SetSphereOffset(const std::string& name, const Vector3& offset)
{
    for (auto& def : sphereDefs_) {
        if (def.name == name) {
            def.offset = offset;
            return true;
        }
    }
    return false;
}

bool Collider::SetSphereRadius(const std::string& name, float radius)
{
    for (auto& def : sphereDefs_) {
        if (def.name == name) {
            def.radius = radius;
            return true;
        }
    }
    return false;
}

std::vector<Sphere> Collider::GetWorldSpheres() const
{
    std::vector<Sphere> result;
    Vector3 basePos = GetWorldPosition();

    if (sphereDefs_.empty()) {
        // AABBパーツが登録されている場合はデフォルト球体を生成しない
        if (!aabbDefs_.empty()) {
            return result;
        }
        // デフォルトの単一球
        Sphere s;
        s.center = basePos;
        s.radius = radius_;
        s.rotate = rotation_;
        result.push_back(s);
    } else {
        // モデル形状に合わせた各パーツ球
        for (const auto& def : sphereDefs_) {
            Sphere s;
            // ローカルオフセットをオーナーの回転で旋回させて加算
            Vector3 rotatedOffset = RotateVector(def.offset, rotation_);
            s.center = Add(basePos, rotatedOffset);
            s.radius = def.radius;
            // ローカル回転とオーナー回転を合成
            s.rotate = rotation_ * def.localRotate;
            result.push_back(s);
        }
    }
    return result;
}

void Collider::AddAABB(const std::string& name, const Vector3& offset, const Vector3& halfSize)
{
    for (auto& def : aabbDefs_) {
        if (def.name == name) {
            def.offset = offset;
            def.halfSize = halfSize;
            return;
        }
    }
    aabbDefs_.push_back({ name, offset, halfSize });
}

bool Collider::SetAABBOffset(const std::string& name, const Vector3& offset)
{
    for (auto& def : aabbDefs_) {
        if (def.name == name) {
            def.offset = offset;
            return true;
        }
    }
    return false;
}

bool Collider::SetAABBHalfSize(const std::string& name, const Vector3& halfSize)
{
    for (auto& def : aabbDefs_) {
        if (def.name == name) {
            def.halfSize = halfSize;
            return true;
        }
    }
    return false;
}

std::vector<AABB> Collider::GetWorldAABBs() const
{
    std::vector<AABB> result;
    Vector3 basePos = GetWorldPosition();

    // オーナーの回転基底ベクトルを取得（進行方向に合わせてボックスの縦横向きを反映）
    Vector3 right   = RotateVector({ 1.0f, 0.0f, 0.0f }, rotation_);
    Vector3 up      = RotateVector({ 0.0f, 1.0f, 0.0f }, rotation_);
    Vector3 forward = RotateVector({ 0.0f, 0.0f, 1.0f }, rotation_);

    for (const auto& def : aabbDefs_) {
        Vector3 rotatedOffset = RotateVector(def.offset, rotation_);
        Vector3 center = Add(basePos, rotatedOffset);

        Vector3 worldHalf = {
            std::abs(right.x) * def.halfSize.x + std::abs(up.x) * def.halfSize.y + std::abs(forward.x) * def.halfSize.z,
            std::abs(right.y) * def.halfSize.x + std::abs(up.y) * def.halfSize.y + std::abs(forward.y) * def.halfSize.z,
            std::abs(right.z) * def.halfSize.x + std::abs(up.z) * def.halfSize.y + std::abs(forward.z) * def.halfSize.z
        };

        AABB box;
        box.min = Subtract(center, worldHalf);
        box.max = Add(center, worldHalf);
        result.push_back(box);
    }
    return result;
}

void Collider::Draw()
{
#ifdef USE_LINE
    auto spheres = GetWorldSpheres();
    for (const auto& sphere : spheres) {
        // プレイヤーは水色・緑系、それ以外は赤系で描画
        Vector4 col = (category_ == CollisionCategory::Player) ? Vector4{ 0.2f, 0.8f, 1.0f, 1.0f } : Vector4{ 1.0f, 0.0f, 0.0f, 1.0f };
        PrimitiveDrawer::GetInstance()->DrawSphere(sphere, col);
    }
    auto aabbs = GetWorldAABBs();
    for (const auto& box : aabbs) {
        Vector4 col = (category_ == CollisionCategory::PlayerAttack)
            ? Vector4{ 1.0f, 0.55f, 0.0f, 1.0f }
            : ((category_ == CollisionCategory::Player) ? Vector4{ 0.2f, 0.8f, 1.0f, 1.0f } : Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
        auto* pd = PrimitiveDrawer::GetInstance();
        pd->DrawAABB(box, col);

        // 攻撃ヒットボックスは視認性を高めるため各面の対角クロスラインも描画
        if (category_ == CollisionCategory::PlayerAttack) {
            const Vector3& mn = box.min;
            const Vector3& mx = box.max;
            // 側面（左右）のX字
            pd->DrawLine({ mn.x, mn.y, mn.z }, { mn.x, mx.y, mx.z }, col);
            pd->DrawLine({ mn.x, mx.y, mn.z }, { mn.x, mn.y, mx.z }, col);
            pd->DrawLine({ mx.x, mn.y, mn.z }, { mx.x, mx.y, mx.z }, col);
            pd->DrawLine({ mx.x, mx.y, mn.z }, { mx.x, mn.y, mx.z }, col);
            // 上面・下面・前面・後面の対角線
            pd->DrawLine({ mn.x, mx.y, mn.z }, { mx.x, mx.y, mx.z }, col);
            pd->DrawLine({ mx.x, mx.y, mn.z }, { mn.x, mx.y, mx.z }, col);
            pd->DrawLine({ mn.x, mn.y, mx.z }, { mx.x, mx.y, mx.z }, col);
            pd->DrawLine({ mx.x, mn.y, mx.z }, { mn.x, mx.y, mx.z }, col);
        }
    }
#endif // USE_LINE
}

Vector3 Collider::GetWorldPosition() const
{
    if (owner_) {
        return owner_->GetWorldPosition() + Offset_;
    }
    return position_;
}

void Collider::OnCollision(Collider* other)
{
    if (owner_ && other && other->GetOwner()) {
        owner_->OnCollision(other->GetOwner());
    }
}

