#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "DrawFunction.h"
#include "PrimitiveDrawer.h"
#include <string>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

class Object3d;

/// <summary>
/// Object3d に動的に着脱できる汎用コンポーネント基底クラス
/// </summary>
class IEditorComponent
{
public:
    virtual ~IEditorComponent() = default;
    virtual std::string GetComponentName() const = 0;
    virtual void OnUpdate(Object3d* owner) { (void)owner; }
    virtual void OnDraw(Object3d* owner) { (void)owner; }
    virtual void OnInspectorGUI() {}

    bool enabled = true;
};

/// <summary>
/// 自動回転コンポーネント
/// </summary>
class RotatorComponent : public IEditorComponent
{
public:
    std::string GetComponentName() const override { return "Auto Rotator"; }
    void OnUpdate(Object3d* owner) override;
    void OnInspectorGUI() override {
#ifdef USE_IMGUI
        ImGui::DragFloat3("Rotate Speed (rad/s)", &speed.x, 0.05f);
#endif
    }
    Vector3 speed = { 0.0f, 1.0f, 0.0f };
};

/// <summary>
/// サイン波で往復移動するコンポーネント
/// </summary>
class OscillatorComponent : public IEditorComponent
{
public:
    std::string GetComponentName() const override { return "Oscillator"; }
    void OnUpdate(Object3d* owner) override;
    void OnInspectorGUI() override {
#ifdef USE_IMGUI
        ImGui::DragFloat3("Amplitude", &amplitude.x, 0.05f);
        ImGui::DragFloat("Frequency", &frequency, 0.05f, 0.0f, 20.0f);
#endif
    }
    Vector3 amplitude = { 0.0f, 0.5f, 0.0f };
    float frequency = 2.0f;
    float time_ = 0.0f;
    Vector3 basePos_ = { 0.0f, 0.0f, 0.0f };
    bool initialized_ = false;
};

/// <summary>
/// デバッグ用バウンディングボックス描画コンポーネント
/// </summary>
class BoxColliderDebugComponent : public IEditorComponent
{
public:
    std::string GetComponentName() const override { return "Debug Box Collider"; }
    void OnDraw(Object3d* owner) override;
    void OnInspectorGUI() override {
#ifdef USE_IMGUI
        ImGui::DragFloat3("Half Size", &halfSize.x, 0.05f, 0.05f, 50.0f);
        ImGui::ColorEdit4("Wire Color", &color.x);
#endif
    }
    Vector3 halfSize = { 0.5f, 0.5f, 0.5f };
    Vector4 color = { 0.2f, 1.0f, 0.4f, 1.0f };
};
