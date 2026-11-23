#include "CameraController.h"
#include "Player.h"  
#include "Enemy.h"
#include "MathFunction.h"  
#include "Camera.h"
#include "transform.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "RailMover.h"
#include "RailPath.h"
#include "Imgui.h"
#include <cstdlib>
#include "JsonManager.h"
#include "Logger.h"

using namespace std;

CameraController::CameraController() = default;
CameraController::~CameraController() = default;

void CameraController::LoadConfig(const std::string& filePath)
{
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("CameraConfig", filePath)) {
        Logger::Log("[Camera] Could not open " + filePath + ". Using default config.\n");
        return;
    }

    const auto& j = jsonMgr->Get("CameraConfig");

    if (j.contains("mode")) {
        std::string modeStr = j.value("mode", "AutoOffset");
        if (modeStr == "RailCamera") {
            mode_ = CameraMode::RailCamera;
            config_.mode = 1;
        } else {
            mode_ = CameraMode::AutoOffset;
            config_.mode = 0;
        }
    }

    if (j.contains("autoOffset")) {
        const auto& ja = j["autoOffset"];
        config_.autoOffset.distance = ja.value("distance", config_.autoOffset.distance);
        config_.autoOffset.height = ja.value("height", config_.autoOffset.height);
        config_.autoOffset.lookAtHeight = ja.value("lookAtHeight", config_.autoOffset.lookAtHeight);
        config_.autoOffset.smoothSpeed = ja.value("smoothSpeed", config_.autoOffset.smoothSpeed);
        config_.autoOffset.flipSide = ja.value("flipSide", config_.autoOffset.flipSide);
    }

    if (j.contains("rendering")) {
        const auto& jr = j["rendering"];
        config_.rendering.drawDistance = jr.value("drawDistance", config_.rendering.drawDistance);
    }

    if (j.contains("clearPhase")) {
        const auto& jc = j["clearPhase"];
        if (jc.contains("targetOffsetGoal") && jc["targetOffsetGoal"].is_array() && jc["targetOffsetGoal"].size() >= 3) {
            config_.clearPhase.targetOffsetGoal = {
                jc["targetOffsetGoal"][0].get<float>(),
                jc["targetOffsetGoal"][1].get<float>(),
                jc["targetOffsetGoal"][2].get<float>()
            };
        }
        if (jc.contains("clearOffset") && jc["clearOffset"].is_array() && jc["clearOffset"].size() >= 3) {
            config_.clearPhase.clearOffset = {
                jc["clearOffset"][0].get<float>(),
                jc["clearOffset"][1].get<float>(),
                jc["clearOffset"][2].get<float>()
            };
        }
    }

    if (j.contains("enemyFraming")) {
        const auto& je = j["enemyFraming"];
        config_.enemyFraming.enabled = je.value("enabled", config_.enemyFraming.enabled);
        config_.enemyFraming.detectRange = je.value("detectRange", config_.enemyFraming.detectRange);
        config_.enemyFraming.detectRangeY = je.value("detectRangeY", config_.enemyFraming.detectRangeY);
        config_.enemyFraming.blendStartRatio = je.value("blendStartRatio", config_.enemyFraming.blendStartRatio);
        config_.enemyFraming.focusWeight = je.value("focusWeight", config_.enemyFraming.focusWeight);
        config_.enemyFraming.focusWeightY = je.value("focusWeightY", config_.enemyFraming.focusWeightY);
        config_.enemyFraming.camYFollowRatio = je.value("camYFollowRatio", config_.enemyFraming.camYFollowRatio);
        config_.enemyFraming.minCamOffsetY = je.value("minCamOffsetY", config_.enemyFraming.minCamOffsetY);
        config_.enemyFraming.extraHeightRatio = je.value("extraHeightRatio", config_.enemyFraming.extraHeightRatio);
        config_.enemyFraming.paddingHorizontal = je.value("paddingHorizontal", config_.enemyFraming.paddingHorizontal);
        config_.enemyFraming.paddingVertical = je.value("paddingVertical", config_.enemyFraming.paddingVertical);
        config_.enemyFraming.maxExtraDistance = je.value("maxExtraDistance", config_.enemyFraming.maxExtraDistance);
        config_.enemyFraming.focusSmoothSpeed = je.value("focusSmoothSpeed", config_.enemyFraming.focusSmoothSpeed);
        config_.enemyFraming.focusSmoothSpeedY = je.value("focusSmoothSpeedY", config_.enemyFraming.focusSmoothSpeedY);
        config_.enemyFraming.zoomSmoothSpeed = je.value("zoomSmoothSpeed", config_.enemyFraming.zoomSmoothSpeed);
    }
}

void CameraController::SaveConfig(const std::string& filePath)
{
    nlohmann::json j;

    j["mode"] = (mode_ == CameraMode::RailCamera) ? "RailCamera" : "AutoOffset";

    j["autoOffset"] = {
        { "distance", config_.autoOffset.distance },
        { "height", config_.autoOffset.height },
        { "lookAtHeight", config_.autoOffset.lookAtHeight },
        { "smoothSpeed", config_.autoOffset.smoothSpeed },
        { "flipSide", config_.autoOffset.flipSide }
    };

    j["rendering"] = {
        { "drawDistance", config_.rendering.drawDistance }
    };

    j["clearPhase"] = {
        { "targetOffsetGoal", { config_.clearPhase.targetOffsetGoal.x, config_.clearPhase.targetOffsetGoal.y, config_.clearPhase.targetOffsetGoal.z } },
        { "clearOffset", { config_.clearPhase.clearOffset.x, config_.clearPhase.clearOffset.y, config_.clearPhase.clearOffset.z } }
    };

    j["enemyFraming"] = {
        { "enabled", config_.enemyFraming.enabled },
        { "detectRange", config_.enemyFraming.detectRange },
        { "detectRangeY", config_.enemyFraming.detectRangeY },
        { "blendStartRatio", config_.enemyFraming.blendStartRatio },
        { "focusWeight", config_.enemyFraming.focusWeight },
        { "focusWeightY", config_.enemyFraming.focusWeightY },
        { "camYFollowRatio", config_.enemyFraming.camYFollowRatio },
        { "minCamOffsetY", config_.enemyFraming.minCamOffsetY },
        { "extraHeightRatio", config_.enemyFraming.extraHeightRatio },
        { "paddingHorizontal", config_.enemyFraming.paddingHorizontal },
        { "paddingVertical", config_.enemyFraming.paddingVertical },
        { "maxExtraDistance", config_.enemyFraming.maxExtraDistance },
        { "focusSmoothSpeed", config_.enemyFraming.focusSmoothSpeed },
        { "focusSmoothSpeedY", config_.enemyFraming.focusSmoothSpeedY },
        { "zoomSmoothSpeed", config_.enemyFraming.zoomSmoothSpeed }
    };

    JsonManager::GetInstance()->Save(filePath, j);
}

void CameraController::Initialize(Camera* camera) {
    camera_ = camera;
    railMover_ = std::make_unique<RailMover>();

    // JSON設定の読み込み
    LoadConfig();

    if (camera_) {
        camera_->SetDrawDistance(config_.rendering.drawDistance);
    }
}

void CameraController::SetDrawDistance(float distance) {
    config_.rendering.drawDistance = distance;
    if (camera_) {
        camera_->SetDrawDistance(distance);
    }
}

void CameraController::Update() {
    if (!camera_)
    {
        return;
    }

#ifdef USE_IMGUI
    ImGui::Begin("Debug/camera");
    if (railMover_) {
        ImGui::Text("Rail Progress: %.2f", railMover_->GetProgress());
    }
    if (target_) {
        Vector3 pos = target_->GetTransform().translate;
        ImGui::Text("Player Pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    }

    ImGui::Separator();
    ImGui::Text("--- Config File Actions ---");
    if (ImGui::Button("💾 Save Camera Config to JSON")) {
        SaveConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("🔄 Reload from JSON")) {
        LoadConfig();
        if (camera_) {
            camera_->SetDrawDistance(config_.rendering.drawDistance);
        }
    }

    ImGui::Separator();
    // カメラモード切り替え
    const char* modeNames[] = { "AutoOffset (自動計算)", "RailCamera (カメラレール)" };
    int currentModeIdx = (mode_ == CameraMode::AutoOffset) ? 0 : 1;
    if (ImGui::Combo("Camera Mode", &currentModeIdx, modeNames, 2)) {
        mode_ = (currentModeIdx == 0) ? CameraMode::AutoOffset : CameraMode::RailCamera;
        config_.mode = currentModeIdx;
    }

    if (mode_ == CameraMode::AutoOffset) {
        ImGui::DragFloat("Offset Distance", &config_.autoOffset.distance, 0.5f, 5.0f, 100.0f);
        ImGui::DragFloat("Offset Height", &config_.autoOffset.height, 0.2f, -20.0f, 50.0f);
        ImGui::DragFloat("LookAt Height", &config_.autoOffset.lookAtHeight, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Smooth Speed", &config_.autoOffset.smoothSpeed, 0.01f, 0.01f, 1.0f);
        ImGui::Checkbox("Flip Side", &config_.autoOffset.flipSide);

        ImGui::Separator();
        ImGui::Text("--- Enemy Framing ---");
        ImGui::Checkbox("Enable Enemy Framing", &config_.enemyFraming.enabled);
        if (config_.enemyFraming.enabled) {
            ImGui::DragFloat("Detect Range (XZ)", &config_.enemyFraming.detectRange, 0.5f, 1.0f, 50.0f);
            ImGui::DragFloat("Detect Range (Y)", &config_.enemyFraming.detectRangeY, 0.5f, 1.0f, 50.0f);
            ImGui::SliderFloat("Blend Start Ratio", &config_.enemyFraming.blendStartRatio, 0.0f, 0.99f);
            ImGui::SliderFloat("Focus Weight (XZ)", &config_.enemyFraming.focusWeight, 0.0f, 0.5f);
            ImGui::SliderFloat("Focus Weight (Y)", &config_.enemyFraming.focusWeightY, 0.0f, 1.0f);
            ImGui::SliderFloat("Cam Y Follow Ratio", &config_.enemyFraming.camYFollowRatio, 0.0f, 1.0f);
            ImGui::DragFloat("Min Cam Offset Y", &config_.enemyFraming.minCamOffsetY, 0.1f, -5.0f, 0.0f);
            ImGui::SliderFloat("Extra Height Ratio", &config_.enemyFraming.extraHeightRatio, -0.5f, 1.0f);
            ImGui::DragFloat("Padding H", &config_.enemyFraming.paddingHorizontal, 0.2f, 0.0f, 20.0f);
            ImGui::DragFloat("Padding V", &config_.enemyFraming.paddingVertical, 0.2f, 0.0f, 20.0f);
            ImGui::DragFloat("Max Extra Distance", &config_.enemyFraming.maxExtraDistance, 0.5f, 0.0f, 30.0f);
            ImGui::DragFloat("Focus Smooth Speed (XZ)", &config_.enemyFraming.focusSmoothSpeed, 0.005f, 0.005f, 1.0f);
            ImGui::DragFloat("Focus Smooth Speed (Y)", &config_.enemyFraming.focusSmoothSpeedY, 0.005f, 0.005f, 1.0f);
            ImGui::DragFloat("Zoom Smooth Speed", &config_.enemyFraming.zoomSmoothSpeed, 0.005f, 0.005f, 1.0f);
        }
    }

    if (ImGui::DragFloat("Draw Distance (FarClip)", &config_.rendering.drawDistance, 1.0f, 20.0f, 500.0f)) {
        SetDrawDistance(config_.rendering.drawDistance);
    }
    ImGui::End();
#endif // USE_IMGUI

    if (mode_ == CameraMode::RailCamera && railMover_ && railMover_->isRailSet()) {
        RailCamera();
    } else {
        AutoOffsetCamera();
    }

    RotateCamera();
    // シェイクの更新
    UpdateShake();
    camera_->Update();
}

void CameraController::SetRailPath(const RailPath* path)
{
    if (railMover_) {
        railMover_->SetPath(path);
    }
}

void CameraController::SetRailProgress(float progress)
{
    if (railMover_)
    {
        railMover_->BindProgress(&progress);
    }
}

void CameraController::Reset() {
    const EulerTransform& targetWorldTransform = target_->GetTransform();
    camera_->SetTranslate(targetWorldTransform.translate + targetOffset_);
    isFocusInitialized_ = false;
    isYInitialized_ = false;
    currentExtraDistance_ = 0.0f;
    currentEnemyFocusOffset_ = { 0.0f, 0.0f, 0.0f };
    currentEnemyCamOffsetY_ = 0.0f;
}

void CameraController::RotateCamera() {
    if (!target_ || !camera_) return;

    Vector3 targetPos = currentFocusPos_;
    if (!isFocusInitialized_) {
        targetPos = target_->GetTransform().translate;
        targetPos.y += config_.autoOffset.lookAtHeight;
    }

    // LookAt関数の実装（ターゲットの方向を向くように回転角を計算）
    Vector3 cameraPos = camera_->GetTranslate();
    Vector3 direction = Normalize(targetPos - cameraPos);

    float angleY = std::atan2(direction.x, direction.z);
    float distanceXZ = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    float angleX = std::atan2(-direction.y, distanceXZ);

    camera_->SetRotate({ angleX, angleY, 0.0f });
}

void CameraController::RailCamera()
{
    if (!target_ || !railMover_ || !railMover_->isRailSet()) return;
    railMover_->SyncWith(target_->GetRailMover());
    camera_->SetTranslate(railMover_->GetCurrentPosition());
}

void CameraController::AutoOffsetCamera()
{
    if (!target_ || !camera_) return;

    Vector3 playerPos = target_->GetTransform().translate;
    if (!isYInitialized_) {
        smoothedFocusY_ = playerPos.y;
        isYInitialized_ = true;
    } else {
        float dy = playerPos.y - smoothedFocusY_;
        float yRate = (std::abs(dy) > 1.5f) ? 0.16f : 0.08f;
        smoothedFocusY_ += dy * yRate;
    }

    // 近くの敵をカメラに収めるための注視点オフセット・カメラ高さ・追加距離を算出
    float targetExtraDistance = 0.0f;
    Vector3 targetEnemyFocusOffset = { 0.0f, 0.0f, 0.0f };
    float targetEnemyCamOffsetY = 0.0f;

    if (config_.enemyFraming.enabled && enemies_ && !enemies_->empty()) {
        Vector3 minPos = playerPos;
        Vector3 maxPos = playerPos;
        float maxWeight = 0.0f;
        float detectRangeXZ = (std::max)(config_.enemyFraming.detectRange, 0.01f);
        float detectRangeY = (std::max)(config_.enemyFraming.detectRangeY, 0.01f);
        float blendStart = std::clamp(config_.enemyFraming.blendStartRatio, 0.0f, 0.99f);
        float fadeSpanNorm = (std::max)(1.0f - blendStart, 0.001f);

        for (const auto& enemy : *enemies_) {
            if (!enemy || enemy->IsDead()) {
                continue;
            }
            Vector3 enemyPos = enemy->GetWorldPosition();
            Vector3 diff = enemyPos - playerPos;
            float distXZ = std::hypot(diff.x, diff.z);
            float distY = std::abs(diff.y);
            float normDist = std::hypot(distXZ / detectRangeXZ, distY / detectRangeY);

            if (normDist <= 1.0f) {
                // 境界付近で0、近づくにつれて滑らかに1へ変化するSmoothstep重み
                float t = std::clamp((1.0f - normDist) / fadeSpanNorm, 0.0f, 1.0f);
                float weight = t * t * (3.0f - 2.0f * t);
                maxWeight = (std::max)(maxWeight, weight);

                Vector3 effectiveEnemyPos = Lerp(playerPos, enemyPos, weight);
                minPos.x = (std::min)(minPos.x, effectiveEnemyPos.x);
                minPos.y = (std::min)(minPos.y, effectiveEnemyPos.y);
                minPos.z = (std::min)(minPos.z, effectiveEnemyPos.z);
                maxPos.x = (std::max)(maxPos.x, effectiveEnemyPos.x);
                maxPos.y = (std::max)(maxPos.y, effectiveEnemyPos.y);
                maxPos.z = (std::max)(maxPos.z, effectiveEnemyPos.z);
            }
        }

        if (maxWeight > 0.0f) {
            Vector3 boundingCenter = (minPos + maxPos) * 0.5f;
            targetEnemyFocusOffset.x = (boundingCenter.x - playerPos.x) * config_.enemyFraming.focusWeight;
            targetEnemyFocusOffset.z = (boundingCenter.z - playerPos.z) * config_.enemyFraming.focusWeight;
            targetEnemyFocusOffset.y = (boundingCenter.y - smoothedFocusY_) * config_.enemyFraming.focusWeightY;

            // カメラ位置のY追従は注視点より控えめにし、下方向への潜り込みを制限（自然なチルトを実現）
            targetEnemyCamOffsetY = (std::max)(
                targetEnemyFocusOffset.y * config_.enemyFraming.camYFollowRatio,
                config_.enemyFraming.minCamOffsetY * maxWeight
            );

            float actualFocusX = playerPos.x + targetEnemyFocusOffset.x;
            float actualFocusZ = playerPos.z + targetEnemyFocusOffset.z;
            float actualFocusY = smoothedFocusY_ + targetEnemyFocusOffset.y + config_.autoOffset.lookAtHeight;

            // 注視点から最も遠い端までの片側スパンを算出（重み付けによる偏りも考慮）
            float halfSpanH = (std::max)(
                std::hypot(actualFocusX - minPos.x, actualFocusZ - minPos.z),
                std::hypot(maxPos.x - actualFocusX, maxPos.z - actualFocusZ)
            ) + config_.enemyFraming.paddingHorizontal * 0.5f * maxWeight;

            float halfSpanV = (std::max)(
                std::abs(actualFocusY - minPos.y),
                std::abs(maxPos.y - actualFocusY)
            ) + config_.enemyFraming.paddingVertical * 0.5f * maxWeight;

            float halfFovY = camera_->GetFovY() * 0.5f;
            float aspect = camera_->GetAspectRatio();
            float tanHalfFovY = std::tan(halfFovY);
            float tanHalfFovX = tanHalfFovY * aspect;

            float reqDistH = (tanHalfFovX > 1e-4f) ? (halfSpanH / tanHalfFovX) : config_.autoOffset.distance;
            float reqDistV = (tanHalfFovY > 1e-4f) ? (halfSpanV / tanHalfFovY) : config_.autoOffset.distance;
            float requiredDist = (std::max)(reqDistH, reqDistV);

            if (requiredDist > config_.autoOffset.distance) {
                targetExtraDistance = (std::min)(requiredDist - config_.autoOffset.distance, config_.enemyFraming.maxExtraDistance);
            }
        }
    }

    currentExtraDistance_ = Lerp(currentExtraDistance_, targetExtraDistance, config_.enemyFraming.zoomSmoothSpeed);
    currentEnemyFocusOffset_.x = Lerp(currentEnemyFocusOffset_.x, targetEnemyFocusOffset.x, config_.enemyFraming.focusSmoothSpeed);
    currentEnemyFocusOffset_.z = Lerp(currentEnemyFocusOffset_.z, targetEnemyFocusOffset.z, config_.enemyFraming.focusSmoothSpeed);
    currentEnemyFocusOffset_.y = Lerp(currentEnemyFocusOffset_.y, targetEnemyFocusOffset.y, config_.enemyFraming.focusSmoothSpeedY);
    currentEnemyCamOffsetY_ = Lerp(currentEnemyCamOffsetY_, targetEnemyCamOffsetY, config_.enemyFraming.focusSmoothSpeedY);
    float effectiveDistance = config_.autoOffset.distance + currentExtraDistance_;
    float effectiveHeight = config_.autoOffset.height + currentExtraDistance_ * config_.enemyFraming.extraHeightRatio + currentEnemyCamOffsetY_;

    Vector3 focusBasePos = { playerPos.x, smoothedFocusY_, playerPos.z };
    Vector3 railFwd = { 0.0f, 0.0f, 1.0f };

    const RailMover* pMover = target_->GetRailMover();
    const RailPath* rail = (pMover && pMover->isRailSet()) ? pMover->GetRailPath() : stageRail_;

    if (rail && rail->GetPointCount() >= 2) {
        if (!rail->IsLoop() && rail->GetTotalLength() > 0.0f) {
            // カメラの視野角(FOV)とアスペクト比、距離から画面端に映るレール上の半幅を算出
            float d = effectiveDistance;
            float h = effectiveHeight;
            float hLook = config_.autoOffset.lookAtHeight;
            float dh = h - hLook;
            float lookDist = std::sqrt(d * d + dh * dh);
            float railDepth = (lookDist > 1e-4f) ? ((d * d + h * dh) / lookDist) : d;

            float halfFovY = camera_->GetFovY() * 0.5f;
            float aspect = camera_->GetAspectRatio();
            float halfVisibleLength = railDepth * std::tan(halfFovY) * aspect;

            float totalLen = rail->GetTotalLength();
            float playerDist = pMover ? pMover->GetCurrentDistance() : 0.0f;

            float clampedDist = playerDist;
            if (totalLen > halfVisibleLength * 2.0f) {
                clampedDist = std::clamp(playerDist, halfVisibleLength, totalLen - halfVisibleLength);
            } else {
                clampedDist = totalLen * 0.5f;
            }

            float clampedT = rail->GetTFromDistance(clampedDist);
            Vector3 clampedRailPos = rail->GetPosition(clampedT);
            focusBasePos = { clampedRailPos.x, smoothedFocusY_, clampedRailPos.z };
            railFwd = rail->GetDirection(clampedT);
        } else {
            railFwd = (pMover && pMover->isRailSet()) ? pMover->GetCurrentDirection() : rail->GetDirection(0.0f);
        }
    }

    // 水平進行方向を抽出
    railFwd.y = 0.0f;
    float len = Length(railFwd);
    if (len > 1e-4f) {
        railFwd = railFwd * (1.0f / len);
    } else {
        railFwd = { 0.0f, 0.0f, 1.0f };
    }

    Vector3 up = { 0.0f, 1.0f, 0.0f };
    // レール進行方向に対して右側（外側）ベクトル
    Vector3 right = Cross(up, railFwd);
    float rLen = Length(right);
    if (rLen > 1e-4f) {
        right = right * (1.0f / rLen);
    } else {
        right = { 1.0f, 0.0f, 0.0f };
    }

    if (config_.autoOffset.flipSide) {
        right = right * -1.0f;
    }

    // 目標カメラ位置と注視点: クランプ済み基準位置 + 敵フレーミングオフセット + 進行方向のわずかな先読み + オフセット
    Vector3 lookAhead = railFwd * 0.6f;
    Vector3 framedCamBasePos = {
        focusBasePos.x + currentEnemyFocusOffset_.x,
        focusBasePos.y,
        focusBasePos.z + currentEnemyFocusOffset_.z
    };
    Vector3 framedFocusBasePos = focusBasePos + currentEnemyFocusOffset_;
    Vector3 targetCamPos = framedCamBasePos + lookAhead * 0.5f + right * effectiveDistance + up * effectiveHeight;
    Vector3 targetFocusPos = framedFocusBasePos + lookAhead + up * config_.autoOffset.lookAtHeight;

    Vector3 currentCamPos = camera_->GetTranslate();
    if (!isFocusInitialized_ || Length(currentCamPos) < 1e-4f) {
        currentCamPos = targetCamPos;
        currentFocusPos_ = targetFocusPos;
        isFocusInitialized_ = true;
    }

    Vector3 newCamPos = Lerp(currentCamPos, targetCamPos, config_.autoOffset.smoothSpeed);
    currentFocusPos_ = Lerp(currentFocusPos_, targetFocusPos, config_.autoOffset.smoothSpeed);
    camera_->SetTranslate(newCamPos);
}


void CameraController::RequestShake(float duration, float power, std::function<float(float)> easingFunc) {
    if (duration <= 0.0f) return;
    
    shakeTimer_ = duration;
    shakeDuration_ = duration; // 最大時間を保存
    shakePower_ = power;
    shakeEasing_ = easingFunc; // イージング関数を登録
}

void CameraController::UpdateShake()
{
    if (shakeTimer_ > 0.0f)
    {
        // 残り時間の割合 (1.0 から始まって 0.0 に近づく)
        float progress = shakeTimer_ / shakeDuration_;

        // イージングを適用して現在のパワーを計算
        float currentPower = shakePower_;
        if (shakeEasing_) {
            currentPower = shakePower_ * shakeEasing_(progress);
        }

        // 減衰したパワーでランダムなズレを計算
        float rx = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * currentPower;
        float ry = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * currentPower;

        shakeOffset_ = { rx, ry, 0.0f };

        // 現在のカメラ座標にシェイクを足す
        Vector3 currentPos = camera_->GetTranslate();
        camera_->SetTranslate(currentPos + shakeOffset_);

        // タイマー更新
        shakeTimer_ -= DXCommon::kDeltaTime;
        if (shakeTimer_ <= 0.0f)
        {
            shakeTimer_ = 0.0f;
            shakeDuration_ = 0.0f;
            shakeOffset_ = { 0.0f, 0.0f, 0.0f };
            shakeEasing_ = nullptr;
        }
    }
    else
    {
        shakeOffset_ = { 0.0f, 0.0f, 0.0f };
    }
}
