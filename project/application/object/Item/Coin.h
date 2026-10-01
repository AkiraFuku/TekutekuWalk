#pragma once
#include "GameObject.h"
#include "Object3d.h"
#include "Vector3.h"
#include "Vector2.h"
#include <memory>
#include <string>

class RailPath;
class RailMover;
class Camera;

/// <summary>
/// 収集アイテム（コイン）クラス
/// プレイヤーが接触すると取得状態になり、スコア加算や消滅演出等を行う
/// </summary>
class Coin : public GameObject {
public:
    Coin();
    ~Coin() override;

    /// <summary>
    /// 初期化処理
    /// </summary>
    /// <param name="modelName">3Dモデルファイル名（空文字の場合はデバッグ球体描画のみ）</param>
    void Initialize(const std::string& modelName = "");

    /// <summary>
    /// 毎フレーム更新処理（自転・上下浮遊アニメーション、取得後消滅演出）
    /// </summary>
    void Update();

    /// <summary>
    /// 描画処理（モデル描画＋デバッグ用コライダー描画）
    /// </summary>
    void Draw();

    // ─── 配置設定 ───
    /// <summary>レールパスを設定</summary>
    void SetRail(RailPath* rail);

    /// <summary>レール上の位置（x: 進行度 0.0~1.0, y: 高さ）を設定</summary>
    void SetRailPosition(const Vector2& position);

    /// <summary>ワールド座標で直接配置</summary>
    void SetPosition(const Vector3& position);

    /// <summary>描画カメラの設定</summary>
    void SetCamera(Camera* camera);

    // ─── 衝突判定・GameObjectインターフェース ───
    void OnCollision(GameObject* other) override;

    Vector3 GetWorldPosition() const override {
        return position_;
    }

    CollisionCategory GetCategory() const override {
        return CollisionCategory::Collectible;
    }

    // ─── コイン固有のゲッター・セッター ───
    /// <summary>取得されたかどうか</summary>
    bool IsCollected() const { return isCollected_; }

    /// <summary>完全に消滅して破棄可能か（取得後演出が終了したか）</summary>
    bool IsDead() const { return isDead_; }

    /// <summary>取得時の獲得スコア値</summary>
    int32_t GetScoreValue() const { return scoreValue_; }
    void SetScoreValue(int32_t score) { scoreValue_ = score; }

    /// <summary>モデルの設定・差し替え（後からモデルを作成した際に使用）</summary>
    void SetModel(const std::string& modelName);

    /// <summary>当たり判定半径</summary>
    float GetRadius() const { return radius_; }
    void SetRadius(float radius);

private:
    std::unique_ptr<Object3d> object_;
    std::unique_ptr<RailMover> railMover_;

    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 basePosition_ = { 0.0f, 0.0f, 0.0f }; // 浮遊アニメーションの基準座標
    float railWorldY_ = 0.0f;
    bool isRailMode_ = false;

    // ─── アニメーションパラメータ ───
    float rotateSpeed_ = 3.0f;     // 自転速度 (rad/s)
    float currentRotationY_ = 0.0f;
    float bobbingTimer_ = 0.0f;    // 上下浮遊タイマー
    float bobbingSpeed_ = 4.0f;    // 浮遊速度
    float bobbingHeight_ = 0.2f;   // 浮遊の振幅

    // ─── コライダー・ステータス ───
    float radius_ = 0.6f;          // コライダー半径
    int32_t scoreValue_ = 100;     // 獲得スコア
    bool isCollected_ = false;     // 取得フラグ
    bool isDead_ = false;          // 破棄可能フラグ

    // ─── 取得時エフェクト演出 ───
    float collectAnimTimer_ = 0.0f;
    const float kCollectAnimDuration_ = 0.3f; // 取得後の上昇・縮小演出時間（秒）
    bool hasModel_ = false;
};
