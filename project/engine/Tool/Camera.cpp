#include "Camera.h"
#include "MathFunction.h"
#include "ImGuI.h"
#include "ImGuiManager.h"
#include "DXCommon.h"
#include "Object3dCommon.h"
#include <cmath>
Camera::Camera()
    :worldTransform_({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} })
    , fovY(1.047f) // 60度 (約1.047 rad)
    , aspect(static_cast<float>(WinApp::kClientWidth) / static_cast<float>(WinApp::kClientHeight))
    , nearCrip(0.1f)
    , farCrip(1000.0f)
    , worldMatrix(MakeAffineMatrix(worldTransform_.scale, worldTransform_.rotate, worldTransform_.translate))
    , viewMatrix(Inverse(worldMatrix))
    , projectionMatrix(MakePerspectiveFovMatrix(fovY, aspect, nearCrip, farCrip))
    , viewProtectionMatrix(Multiply(viewMatrix, projectionMatrix))
{
#ifdef USE_IMGUI
    initialTransform_ = worldTransform_;
#endif
    // カメラバッファ作成
    CreateCameraBuffer();
}

#ifdef USE_IMGUI
void Camera::UpdateEditorCamera() {
    ImGuiIO& io = ImGui::GetIO();

    bool isMouseDownRight = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    bool isMouseDownMiddle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    // ドラッグ開始判定：新規クリック時にImGuiのUI操作中(WantCaptureMouse)なら開始しない
    if (!isRightDragging_ && isMouseDownRight) {
        if (!io.WantCaptureMouse) {
            isRightDragging_ = true;
        }
    } else if (!isMouseDownRight) {
        isRightDragging_ = false;
    }

    if (!isMiddleDragging_ && isMouseDownMiddle) {
        if (!io.WantCaptureMouse) {
            isMiddleDragging_ = true;
        }
    } else if (!isMouseDownMiddle) {
        isMiddleDragging_ = false;
    }

    // カメラの各軸ベクトルを worldMatrix から取得
    Vector3 right = { worldMatrix.m[0][0], worldMatrix.m[0][1], worldMatrix.m[0][2] };
    Vector3 up = { worldMatrix.m[1][0], worldMatrix.m[1][1], worldMatrix.m[1][2] };
    Vector3 forward = { worldMatrix.m[2][0], worldMatrix.m[2][1], worldMatrix.m[2][2] };

    if (Length(right) > 0.0001f) {
        right = Normalize(right);
    }
    if (Length(up) > 0.0001f) {
        up = Normalize(up);
    }
    if (Length(forward) > 0.0001f) {
        forward = Normalize(forward);
    }

    // 1. 右クリックホールド：視点回転（Pitch / Yaw）＆ WASD 移動（Unity Flythrough）
    if (isRightDragging_) {
        float deltaX = io.MouseDelta.x;
        float deltaY = io.MouseDelta.y;

        // 視点回転
        worldTransform_.rotate.y += deltaX * rotateSpeed_;
        worldTransform_.rotate.x += deltaY * rotateSpeed_;

        // ピッチ角制限（首が真上・真下を超えて反転しないように制限）
        const float kMaxPitch = 1.55f;
        if (worldTransform_.rotate.x > kMaxPitch) {
            worldTransform_.rotate.x = kMaxPitch;
        }
        if (worldTransform_.rotate.x < -kMaxPitch) {
            worldTransform_.rotate.x = -kMaxPitch;
        }

        // 右ドラッグ中のホイール回転：移動速度の調整
        if (std::abs(io.MouseWheel) > 0.01f) {
            moveSpeed_ += io.MouseWheel * 0.05f;
            if (moveSpeed_ < 0.05f) {
                moveSpeed_ = 0.05f;
            }
        }

        // WASD / QE による移動
        float currentSpeed = moveSpeed_;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
            currentSpeed *= 3.0f; // Shiftで高速移動
        }

        Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            moveDir = moveDir + forward;
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            moveDir = moveDir - forward;
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            moveDir = moveDir + right;
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            moveDir = moveDir - right;
        }
        if (ImGui::IsKeyDown(ImGuiKey_E)) {
            moveDir = moveDir + Vector3{ 0.0f, 1.0f, 0.0f };
        } // ワールド上昇
        if (ImGui::IsKeyDown(ImGuiKey_Q)) {
            moveDir = moveDir - Vector3{ 0.0f, 1.0f, 0.0f };
        } // ワールド下降

        if (Length(moveDir) > 0.0001f) {
            worldTransform_.translate = worldTransform_.translate + Normalize(moveDir) * currentSpeed;
        }
    }

    // 2. 中クリックドラッグ：パン移動（画面平行移動）
    if (isMiddleDragging_) {
        float deltaX = io.MouseDelta.x;
        float deltaY = io.MouseDelta.y;
        float panSpeed = moveSpeed_ * 0.05f;

        worldTransform_.translate = worldTransform_.translate - right * (deltaX * panSpeed);
        worldTransform_.translate = worldTransform_.translate + up * (deltaY * panSpeed);
    }

    // 3. 通常時のホイール回転：前後ズーム
    if (!isRightDragging_ && !io.WantCaptureMouse && std::abs(io.MouseWheel) > 0.01f) {
        float zoomSpeed = moveSpeed_ * 2.0f;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) {
            zoomSpeed *= 3.0f;
        }
        worldTransform_.translate = worldTransform_.translate + forward * (io.MouseWheel * zoomSpeed);
    }
}
#endif // USE_IMGUI

void Camera::Update() {
#ifdef USE_IMGUI
    if (isEditorMode_) {
        UpdateEditorCamera();
    }
#endif // USE_IMGUI

    worldMatrix = MakeAffineMatrix(worldTransform_.scale, worldTransform_.rotate, worldTransform_.translate);

#ifdef USE_IMGUI
    ImGui::Begin("Camera");

    ImGui::Checkbox("Editor Camera Mode", &isEditorMode_);
    if (isEditorMode_) {
        ImGui::DragFloat("Move Speed", &moveSpeed_, 0.02f, 0.01f, 10.0f);
        ImGui::DragFloat("Rotate Speed", &rotateSpeed_, 0.0002f, 0.0005f, 0.05f);
        if (ImGui::Button("Reset Camera")) {
            worldTransform_ = initialTransform_;
        }
        ImGui::Separator();
        ImGui::TextDisabled("Controls:");
        ImGui::TextDisabled("- Right Drag + WASD / QE : Flythrough");
        ImGui::TextDisabled("- Shift : Boost Speed");
        ImGui::TextDisabled("- Middle Drag : Pan");
        ImGui::TextDisabled("- Wheel : Zoom / Speed Adj");
    }

    ImGui::Separator();
    float fovDegrees = fovY * (180.0f / 3.14159265f);
    if (ImGui::SliderFloat("FOV (Degree)", &fovDegrees, 20.0f, 100.0f, "%.1f deg")) {
        fovY = fovDegrees * (3.14159265f / 180.0f);
    }
    float globalCurvature = Object3dCommon::GetInstance()->GetGlobalCurvature();
    if (ImGui::SliderFloat("Curvature (空間湾曲)", &globalCurvature, 0.0f, 0.005f, "%.5f")) {
        Object3dCommon::GetInstance()->SetGlobalCurvature(globalCurvature);
    }
    ImGui::DragFloat3("Rotate", &(worldTransform_.rotate.x), 0.01f);
    ImGui::DragFloat3("Scale", &(worldTransform_.scale.x), 0.01f);
    ImGui::DragFloat3("Translate", &(worldTransform_.translate.x), 0.1f);
    ImGui::End();

#endif // USE_IMGUI

    UpdateView();
    UpdateViewProjection();
    UpdateCameraBuffer();
}

void Camera::UpdateView()
{
    viewMatrix = Inverse(worldMatrix);
}

void Camera::UpdateViewProjection()
{
    projectionMatrix = MakePerspectiveFovMatrix(fovY, aspect, nearCrip, farCrip);
    viewProtectionMatrix = Multiply(viewMatrix, projectionMatrix);
}

void Camera::CreateCameraBuffer()
{
    // CameraForGPU のサイズを 256 バイト境界にアライン
    size_t bufferSize = (sizeof(CameraForGPU) + 0xff) & ~0xff;

    // GPU リソース確保
    cameraBufferResource_ = DXCommon::GetInstance()->CreateBufferResource(bufferSize);

    // GPU メモリにマッピング
    cameraBufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

    // 初期値を設定
    if (cameraData_) {
        cameraData_->worldPosition = worldTransform_.translate;
        cameraData_->farClip = farCrip;

        // ワールド行列から前方ベクトル (Z軸) を抽出
        Vector3 forward = { worldMatrix.m[2][0], worldMatrix.m[2][1], worldMatrix.m[2][2] };
        cameraData_->cameraForward = Normalize(forward);
    }
}

void Camera::UpdateCameraBuffer()
{
    if (!cameraData_) {
        return;
    }

    // カメラ位置を更新
    cameraData_->worldPosition = worldTransform_.translate;

    // far値を更新
    cameraData_->farClip = farCrip;

    // ワールド行列から前方ベクトル (Z軸) を抽出・正規化
    Vector3 forward = { worldMatrix.m[2][0], worldMatrix.m[2][1], worldMatrix.m[2][2] };
    cameraData_->cameraForward = Normalize(forward);
}

