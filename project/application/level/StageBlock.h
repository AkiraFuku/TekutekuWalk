#pragma once
#include "Object3d.h"
#include "Camera.h"
#include "LevelData.h"
#include "MathFunction.h"
#include "IInspectable.h"
#include <memory>
#include <string>
#include <vector>

/// <summary>
/// 接地判定・壁判定を持つ汎用ステージブロック（プレハブ）クラス。
/// Blenderレベルエディターの kBlock / kTerrain タイプに対応。
/// Object3d を内包し、自身のワールドポリゴンを抽出して
/// GameScene の triangles_ リストへ提供する。
/// </summary>
class StageBlock : public IEditorInspectable
{
public:
    StageBlock() = default;
    ~StageBlock() override;

    // ─── IInspectable ─────────────────────────────────────────────────
    std::string GetInspectorName() const override { return name_.empty() ? "StageBlock" : name_; }
    std::string GetInspectorCategory() const override { return "Stage"; }
    Vector3* GetInspectorPositionPtr() override { return object_ ? object_->GetTranslatePtr() : nullptr; }
    Vector3* GetInspectorRotationPtr() override { return object_ ? object_->GetRotatePtr() : nullptr; }
    Vector3* GetInspectorScalePtr() override { return object_ ? object_->GetScalePtr() : nullptr; }
    bool* GetInspectorVisiblePtr() override { return &isVisible_; }
    void OnInspectorGUI() override;

    /// <summary>
    /// LevelObjectData から初期化する（レベルローダーから呼ばれる）
    /// </summary>
    void Initialize(const LevelObjectData& data, Camera* camera);

    /// <summary>
    /// 毎フレームの更新（ワールド三角形の再計算など）
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    // ─── セッター ─────────────────────────────────────────────────────
    void SetCamera(Camera* camera) { if (object_) object_->SetCamera(camera); }
    void SetDisabled(bool disabled) { disabled_ = disabled; }
    void SetCollisionEnabled(bool enabled) { isCollisionEnabled_ = enabled; }
    void SetOneway(bool oneway)            { isOneway_ = oneway; }
    void SetPrefabId(const std::string& id){ prefabId_ = id; }
    void SetTranslate(const Vector3& pos) { if (object_) object_->SetTranslate(pos); }
    void SetRotate(const Vector3& rot)    { if (object_) object_->SetRotate(rot); }
    void SetScale(const Vector3& scl)     { if (object_) object_->SetScale(scl); }

    // ─── ゲッター ─────────────────────────────────────────────────────
    bool IsDisabled()          const { return disabled_; }
    bool IsCollisionEnabled()  const { return isCollisionEnabled_; }
    bool IsOneway()            const { return isOneway_; }
    const std::string& GetPrefabId() const { return prefabId_; }
    const std::string& GetName() const { return name_; }
    LevelObjectType GetType()  const { return type_; }

    /// <summary>
    /// 接地/壁判定に使用するワールド三角形リストを返す。
    /// disabled_ または isCollisionEnabled_ == false の場合は空リストを返す。
    /// </summary>
    const std::vector<Triangle>& GetWorldTriangles() const { return triangles_; }

    Object3d* GetObject() const { return object_.get(); }
    Vector3 GetPosition() const { return object_ ? object_->GetTranslate() : Vector3{ 0.0f, 0.0f, 0.0f }; }

private:
    std::unique_ptr<Object3d> object_ = nullptr;

    std::string     name_               = "";
    std::string     prefabId_           = "";     ///< 適用されたプレハブID
    LevelObjectType type_               = LevelObjectType::kBlock;
    bool            disabled_           = false;  ///< 無効化フラグ（trueなら更新・描画・判定をスキップ）
    bool            isVisible_          = true;   ///< 表示オンオフフラグ
    bool            isCollisionEnabled_ = true;   ///< false にすると三角形判定を提供しない
    bool            isOneway_           = false;  ///< すり抜け足場フラグ（上から着地可・下から上昇通過可）

    /// キャッシュされたワールド空間三角形リスト
    mutable std::vector<Triangle> triangles_;
};
