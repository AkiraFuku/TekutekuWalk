#include "Coin.h"
#include "RailMover.h"
#include "ModelManager.h"
#include "PrimitiveDrawer.h"
#include "Collider.h"
#include "Camera.h"
#include <cmath>

Coin::Coin() = default;
Coin::~Coin() = default;

void Coin::Initialize(const std::string& modelName) {
    object_ = std::make_unique<Object3d>();
    object_->Initialize();

    if (!modelName.empty()) {
        SetModel(modelName);
    }

    railMover_ = std::make_unique<RailMover>();

    // コライダーの初期化（Collectibleカテゴリ設定）
    auto collider = std::make_unique<Collider>();
    collider->initialize(this, radius_);
    collider->SetCategory(CollisionCategory::Collectible);
    SetCollider(std::move(collider));
}

void Coin::SetModel(const std::string& modelName) {
    if (object_ && !modelName.empty()) {
        object_->SetModel(modelName);
        hasModel_ = true;
    }
}

void Coin::SetRadius(float radius) {
    radius_ = radius;
    if (auto* col = GetCollider()) {
        col->SetRadius(radius_);
    }
}

void Coin::SetRail(RailPath* rail) {
    if (railMover_) {
        railMover_->SetPath(rail);
        isRailMode_ = true;
        basePosition_ = railMover_->GetCurrentPosition();
        basePosition_.y = railWorldY_;
        position_ = basePosition_;
        if (object_) {
            object_->SetTranslate(position_);
        }
    }
}

void Coin::SetRailPosition(const Vector2& position) {
    if (railMover_) {
        isRailMode_ = true;
        float progress = position.x;
        railWorldY_ = position.y;

        railMover_->SetProgress(progress);
        Vector3 railPos = railMover_->GetCurrentPosition();
        basePosition_ = { railPos.x, railWorldY_, railPos.z };
        position_ = basePosition_;
        if (object_) {
            object_->SetTranslate(position_);
            object_->Update();
        }
    }
}

void Coin::SetPosition(const Vector3& position) {
    isRailMode_ = false;
    basePosition_ = position;
    position_ = basePosition_;
    if (object_) {
        object_->SetTranslate(position_);
        object_->Update();
    }
}

void Coin::SetCamera(Camera* camera) {
    if (object_) {
        object_->SetCamera(camera);
    }
}

void Coin::Update() {
    if (isDead_) return;

    const float deltaTime = 1.0f / 60.0f; // 約60FPS想定

    // ─── 取得後の上昇・縮小演出 ───
    if (isCollected_) {
        collectAnimTimer_ += deltaTime;
        float progress = collectAnimTimer_ / kCollectAnimDuration_;
        if (progress >= 1.0f) {
            isDead_ = true;
            return;
        }

        // 上昇しながら回転加速・縮小
        position_.y += 3.0f * deltaTime;
        currentRotationY_ += rotateSpeed_ * 3.0f * deltaTime;

        float scale = 1.0f - progress;
        if (object_) {
            object_->SetScale({ scale, scale, scale });
            object_->SetRotate({ 0.0f, currentRotationY_, 0.0f });
            object_->SetTranslate(position_);
            object_->Update();
        }
        return;
    }

    // ─── 通常時のアニメーション（自転 ＋ 上下浮遊） ───
    currentRotationY_ += rotateSpeed_ * deltaTime;
    bobbingTimer_ += bobbingSpeed_ * deltaTime;
    float bobbingOffset = std::sin(bobbingTimer_) * bobbingHeight_;

    if (isRailMode_ && railMover_) {
        Vector3 railPos = railMover_->GetCurrentPosition();
        basePosition_ = { railPos.x, railWorldY_, railPos.z };
    }

    position_ = basePosition_;
    position_.y += bobbingOffset;

    if (object_) {
        object_->SetTranslate(position_);
        object_->SetRotate({ 0.0f, currentRotationY_, 0.0f });
        object_->Update();
    }

    if (auto* col = GetCollider()) {
        col->SetPosition(position_);
    }
}

void Coin::Draw() {
    if (isDead_) return;

    // 3Dモデルがあれば描画
    if (hasModel_ && object_) {
        object_->Draw();
    }

    // デバッグ用コライダー描画（ゴールド色の球体）
    // モデル未作成時でもコインの位置とサイズがゲーム画面で一目で確認可能
    Vector4 coinColor = isCollected_ ? Vector4{ 1.0f, 1.0f, 1.0f, 0.5f } : Vector4{ 1.0f, 0.85f, 0.0f, 0.8f };
    Sphere collisionSphere = { position_, radius_, { 0, 0, 0, 1 } };
    PrimitiveDrawer::GetInstance()->DrawSphere(collisionSphere, coinColor);
}

void Coin::OnCollision(GameObject* other) {
    if (!other || isCollected_) return;

    // プレイヤーと接触したら取得
    if (other->GetCategory() == CollisionCategory::Player) {
        isCollected_ = true;
        // 二重取得防止のためコライダーを即時無効化
        if (auto* col = GetCollider()) {
            col->SetCollide(false);
        }
    }
}
