#pragma once
#include "Vector3.h"
#include <string>

/// <summary>
/// Unity の Hierarchy / Inspector に表示するための共通インターフェース
/// </summary>
class IEditorInspectable
{
public:
    virtual ~IEditorInspectable() = default;

    /// <summary>Hierarchy に表示するオブジェクト名</summary>
    virtual std::string GetInspectorName() const = 0;

    /// <summary>Hierarchy でのグループ分けカテゴリ名</summary>
    virtual std::string GetInspectorCategory() const { return "GameObject"; }

    /// <summary>位置ベクトルのポインタ（編集・ImGuizmo対応用、不要なら nullptr）</summary>
    virtual Vector3* GetInspectorPositionPtr() { return nullptr; }

    /// <summary>回転ベクトル(ラジアン)のポインタ（編集・ImGuizmo対応用、不要なら nullptr）</summary>
    virtual Vector3* GetInspectorRotationPtr() { return nullptr; }

    /// <summary>スケールベクトルのポインタ（編集・ImGuizmo対応用、不要なら nullptr）</summary>
    virtual Vector3* GetInspectorScalePtr() { return nullptr; }

    /// <summary>表示オンオフフラグのポインタ（不要なら nullptr）</summary>
    virtual bool* GetInspectorVisiblePtr() { return nullptr; }

    /// <summary>Inspector ウィンドウに固有パラメータを描画する</summary>
    virtual void OnInspectorGUI() {}

    /// <summary>Inspector や ImGuizmo で Transform が変更された直後に呼ばれる通知</summary>
    virtual void OnTransformModified() {}
};
