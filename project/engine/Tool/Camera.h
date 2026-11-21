#pragma once
#include "Vector4.h"
#include "WinApp.h"
#include "Transform.h"
#include "RenderTypes.h"
#include "IInspectable.h"
#include <wrl/client.h>
#include <d3d12.h>

class Camera : public IEditorInspectable
{
public:

    Camera ();
    virtual ~Camera() override;

    // ─── IEditorInspectable ──────────────────────────────────────────
    std::string GetInspectorName() const override { return editorName_.empty() ? "Camera" : editorName_; }
    std::string GetInspectorCategory() const override { return "Camera"; }
    Vector3* GetInspectorPositionPtr() override { return &worldTransform_.translate; }
    Vector3* GetInspectorRotationPtr() override { return &worldTransform_.rotate; }
    void OnInspectorGUI() override;
    void SetEditorName(const std::string& name) { editorName_ = name; }

    void Update();

    void UpdateView();

    void UpdateViewProjection();


    void SetRotate(const Vector3& rotate) {
        worldTransform_.rotate = rotate;
    }
    void SetTranslate(const Vector3& translate) {
        worldTransform_.translate = translate;
    }
    void SetTransform(const EulerTransform& transForm) {
        worldTransform_ = transForm;
    }
    void SetFovY(const float fovY) {
        this->fovY = fovY;
    }
    float GetFovY() const { return fovY; }
    void SetAspectRatio(const float aspect) {
        this->aspect = aspect;
    }
    float GetAspectRatio() const { return aspect; }
    void SetNearCrip(const float nearCrip) {
        this->nearCrip = nearCrip;
    }
    void SetFarCrip(const float farCrip) {
        this->farCrip = farCrip;
    }
    void SetViewMatrix(const Matrix4x4& viewMatrix) {
        this->viewMatrix = viewMatrix;
    }
    // 描画範囲の設定（FarClipおよび距離カリング用）
    void SetDrawDistance(float distance) {
        drawDistance_ = distance;
        farCrip = distance;
    }
    float GetDrawDistance() const { return drawDistance_; }


    const Vector3& GetRotate()const{return worldTransform_.rotate;}
    const Vector3& GetTranslate()const{return worldTransform_.translate;}
    const EulerTransform& GetTransform()const{ return worldTransform_; };

    const Matrix4x4& GetWorldMatrix()const{return worldMatrix;};
    const Matrix4x4& GetViewMatrix()const{return viewMatrix;};
    const Matrix4x4& GetProjectionMatrix()const{return projectionMatrix;};
    const Matrix4x4& GetViewProtectionMatrix()const{return viewProtectionMatrix;};
    float GetFarCrip() const { return farCrip; }

    // カメラバッファ管理メソッド
    ID3D12Resource* GetCameraBufferResource() const { return cameraBufferResource_.Get(); }
    bool HasCameraBuffer() const { return cameraBufferResource_ != nullptr; }

    EulerTransform worldTransform_;

#ifdef USE_IMGUI
    // --- エディタカメラ（SceneCamera）機能 ---
    void SetEditorMode(bool enable) { isEditorMode_ = enable; }
    bool IsEditorMode() const { return isEditorMode_; }
    void SetAllowViewportInput(bool allow) { allowViewportInput_ = allow; }
    void SetShowDebugWindow(bool show) { showDebugWindow_ = show; }
    void SetMoveSpeed(float speed) { moveSpeed_ = speed; }
    float GetMoveSpeed() const { return moveSpeed_; }
    void SetRotateSpeed(float speed) { rotateSpeed_ = speed; }
    float GetRotateSpeed() const { return rotateSpeed_; }
#endif

private:
#ifdef USE_IMGUI
    void UpdateEditorCamera();

    bool isEditorMode_ = false;       // エディタ操作が有効かどうか
    bool allowViewportInput_ = false; // Viewportウィンドウ上での入力を許可するか
    bool showDebugWindow_ = false;    // 従来の単独Cameraウィンドウ表示フラグ（エディタ統合のためデフォルトOFF）
    float moveSpeed_ = 0.5f;          // 移動速度
    float rotateSpeed_ = 0.003f;      // マウス回転感度
    bool isRightDragging_ = false;    // 右ボタンドラッグ中か
    bool isMiddleDragging_ = false;   // 中ボタンドラッグ中か
    EulerTransform initialTransform_; // 初期リセット用
#endif

    Matrix4x4 worldMatrix;
    Matrix4x4 viewMatrix;
    Matrix4x4 projectionMatrix;
    Matrix4x4 viewProtectionMatrix;
    float fovY ;
    float aspect ;
    float nearCrip ;
    float farCrip ;
    float drawDistance_ = 100.0f;
    std::string editorName_ = "Camera";

    // カメラバッファ管理
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraBufferResource_;
    CameraForGPU* cameraData_ = nullptr;
    void CreateCameraBuffer();
    void UpdateCameraBuffer();

};
