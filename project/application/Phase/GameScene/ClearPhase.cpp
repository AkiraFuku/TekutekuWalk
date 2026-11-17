#include "ClearPhase.h"
#include "Sprite.h"
#include "WinApp.h"
#include "SceneManager.h"
#include "Input.h"
#include "Fade.h"
#include "TextureManager.h"
#include "GameScene.h"
#include "Player.h"
#include "GoalObject.h"
#include "Enemy.h"
#include "CameraController.h"
#include "Camera.h"
#include "ParticleManager.h"
#include "MathFunction.h"
#include <cmath>
#include <algorithm>
#include <numbers>

void ClearPhase::Initialize(Scene* scene)
{
    step_ = Step::GoalZoom;
    timer_ = 0.0f;
    irisProgress_ = 0.0f;
    hasEmittedHoistEffect_ = false;
    goalYaw_ = 0.0f;

    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;
    GoalObject* goal = gameScene ? gameScene->GetGoal() : nullptr;
    CameraController* camCtrl = gameScene ? gameScene->GetCamera() : nullptr;

    if (player && player->GetObject3d()) {
        // 被弾点滅を解除してクリア演出中確実に表示
        player->ResetHitInvincibility();

        playerStartPos_ = player->GetWorldPosition();
        playerTargetPos_ = playerStartPos_;
        if (goal) {
            goalStartPos_ = goal->GetWorldPosition();
            if (goal->GetObject3d()) {
                goalBaseScale_ = goal->GetObject3d()->GetScale();
                goalYaw_ = goal->GetObject3d()->GetRotate().y;
            }
            // ゴールオブジェクトの位置にX/Zを合わせ、高さは現在の接地高さを維持
            playerTargetPos_.x = goalStartPos_.x;
            playerTargetPos_.z = goalStartPos_.z;
        }

        playerBaseScale_ = player->GetObject3d()->GetScale();
        startPlayerYaw_ = player->GetObject3d()->GetRotate().y;

        // カメラの位置とズーム目標位置を計算
        Vector3 camPos = (camCtrl && camCtrl->GetCamera())
            ? camCtrl->GetCamera()->GetTranslate()
            : Vector3{ playerTargetPos_.x, playerTargetPos_.y + 2.0f, playerTargetPos_.z - 8.5f };

        startCamPos_ = camPos;

        // プレイヤーからカメラへの水平方向ベクトル
        Vector3 toCam = camPos - playerTargetPos_;
        toCam.y = 0.0f;
        float len = Length(toCam);
        if (len > 0.001f) {
            toCam = toCam / len;
        } else {
            toCam = { 0.0f, 0.0f, -1.0f };
        }

        // プレイヤーがカメラ正面を向く目標Yaw角
        targetPlayerYaw_ = std::atan2(toCam.x, toCam.z);

        // 最短回転になるよう角度差を正規化
        const float twoPi = 2.0f * std::numbers::pi_v<float>;
        float diffYaw = std::fmod(targetPlayerYaw_ - startPlayerYaw_, twoPi);
        if (diffYaw > std::numbers::pi_v<float>) diffYaw -= twoPi;
        else if (diffYaw < -std::numbers::pi_v<float>) diffYaw += twoPi;
        targetPlayerYaw_ = startPlayerYaw_ + diffYaw;

        // プレイヤーと頭上に掲げたゴールが両方しっかり収まるカメラ目標位置・注視点
        startLookAt_ = playerStartPos_ + Vector3{ 0.0f, 1.0f, 0.0f };
        targetLookAt_ = playerTargetPos_ + Vector3{ 0.0f, 0.65f, 0.0f };
        targetCamPos_ = playerTargetPos_ + toCam * 3.8f + Vector3{ 0.0f, 0.85f, 0.0f };

        // 汎用サブパーツシステムでプレイヤーの独立パーツ（右手・左手・右足・左足）を自動検出＆バインド
        player->GetObject3d()->AutoDetectSubParts();
        player->GetObject3d()->BindSubPartByNearestPos("RightHand", {  0.505f, -0.162f, -0.024f });
        player->GetObject3d()->BindSubPartByNearestPos("LeftHand",  { -0.505f, -0.162f, -0.024f });
        player->GetObject3d()->BindSubPartByNearestPos("RightFoot", {  0.175f, -0.351f,  0.0f   });
        player->GetObject3d()->BindSubPartByNearestPos("LeftFoot",  { -0.175f, -0.351f,  0.0f   });
        player->GetObject3d()->ResetSubParts();

        // ゴール接触時の初期エフェクト
        ParticleManager::GetInstance()->Emit("GameEffects", "Hit", playerTargetPos_, 8);
    }

    // 放射状トランジションスプライトの初期化
    InitializeIrisSprites();

    // ゴール到達SEの再生
    goalSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
    if (goalSE_ != 0) {
        Audio::GetInstance()->PlayAudio(goalSE_, false, 0.8f, "SE");
    }
}

void ClearPhase::InitializeIrisSprites()
{
    TextureManager::GetInstance()->LoadTexture("resources/white.png");
    TextureManager::GetInstance()->LoadTexture("resources/circle2.png");

    irisBlades_.clear();
    irisBlades_.reserve(kIrisBladeCount_);

    const Vector2 center = {
        static_cast<float>(WinApp::kClientWidth) * 0.5f,
        static_cast<float>(WinApp::kClientHeight) * 0.5f
    };
    const float twoPi = 2.0f * std::numbers::pi_v<float>;

    for (int i = 0; i < kIrisBladeCount_; ++i) {
        auto blade = std::make_unique<Sprite>();
        blade->Initialize("resources/white.png");
        blade->SetAnchorPoint({ 0.5f, 1.0f }); // 中心から外側へ伸びる扇ブレード
        blade->SetPosition(center);
        float angle = (twoPi / static_cast<float>(kIrisBladeCount_)) * static_cast<float>(i);
        blade->SetRotation(angle);

        // クリア画面の暖色テーマ（サンセットオレンジ＆ゴールデンクリーム）に合わせた交互配色
        Vector4 bladeColor = (i % 2 == 0)
            ? Vector4{ 0.98f, 0.68f, 0.42f, 1.0f }
            : Vector4{ 1.00f, 0.85f, 0.58f, 1.0f };
        blade->SetColor(bladeColor);
        blade->SetSize({ 0.0f, 0.0f });
        blade->Update();
        irisBlades_.push_back(std::move(blade));
    }

    irisCenterCircle_ = std::make_unique<Sprite>();
    irisCenterCircle_->Initialize("resources/circle2.png");
    irisCenterCircle_->SetAnchorPoint({ 0.5f, 0.5f });
    irisCenterCircle_->SetPosition(center);
    irisCenterCircle_->SetColor({ 0.96f, 0.58f, 0.35f, 1.0f });
    irisCenterCircle_->SetSize({ 0.0f, 0.0f });
    irisCenterCircle_->Update();
}

void ClearPhase::UpdateIrisSprites(float progress)
{
    float p = std::clamp(progress, 0.0f, 1.0f);
    // EaseInOutCubic で気持ちよく閉じる
    float ease = (p < 0.5f)
        ? (4.0f * p * p * p)
        : (1.0f - std::pow(-2.0f * p + 2.0f, 3.0f) * 0.5f);

    const float maxRadius = 920.0f; // 画面対角線を完全に覆う半径
    const float currentLen = maxRadius * ease;
    const float maxBladeWidth = 260.0f;
    const float currentWidth = maxBladeWidth * ease;

    const Vector2 center = {
        static_cast<float>(WinApp::kClientWidth) * 0.5f,
        static_cast<float>(WinApp::kClientHeight) * 0.5f
    };
    const float twoPi = 2.0f * std::numbers::pi_v<float>;

    for (int i = 0; i < static_cast<int>(irisBlades_.size()); ++i) {
        float angle = (twoPi / static_cast<float>(kIrisBladeCount_)) * static_cast<float>(i);
        // 外周側から中心に向かって迫り出してくる動きをミックス
        float outerDist = (1.0f - ease) * 760.0f;
        Vector2 bladePos = {
            center.x + std::sin(angle) * outerDist,
            center.y - std::cos(angle) * outerDist
        };
        irisBlades_[i]->SetPosition(bladePos);
        irisBlades_[i]->SetRotation(angle);
        irisBlades_[i]->SetSize({ currentWidth, currentLen });
        irisBlades_[i]->Update();
    }

    if (irisCenterCircle_) {
        // 中央の円が最後にキュッと現れる（先輩の動画 00:09 の中心サークル表現）
        float circleScale = std::clamp((ease - 0.35f) / 0.65f, 0.0f, 1.0f);
        float circleSize = 180.0f * circleScale;
        irisCenterCircle_->SetPosition(center);
        irisCenterCircle_->SetSize({ circleSize, circleSize });
        irisCenterCircle_->Update();
    }
}

void ClearPhase::UpdateCameraAndPlayer(Scene* scene, float dt)
{
    GameScene* gameScene = static_cast<GameScene*>(scene);
    if (!gameScene) return;

    Player* player = gameScene->GetPlayer();
    GoalObject* goal = gameScene->GetGoal();
    CameraController* camCtrl = gameScene->GetCamera();

    float t = std::clamp(timer_ / kZoomDuration_, 0.0f, 1.0f);
    // EaseOutCubic で滑らかにカメラが寄る
    float easeOut = 1.0f - std::pow(1.0f - t, 3.0f);
    // プレイヤーの振り向きは最初の0.28秒で完了させる
    float turnT = std::clamp(timer_ / 0.28f, 0.0f, 1.0f);
    float easeTurn = 1.0f - std::pow(1.0f - turnT, 3.0f);

    Vector3 currentPlayerPos = Lerp(playerStartPos_, playerTargetPos_, easeTurn);
    float squashX = 1.0f;
    float stretchY = 1.0f;
    float poseRoll = 0.0f;

    // ── フェーズ1 (0.00s〜0.28s): 予備動作のしゃがみ込み（ゴールを掴むタメ） ──
    if (timer_ < 0.28f) {
        float crouchPhase = std::sin((timer_ / 0.28f) * std::numbers::pi_v<float>);
        stretchY = 1.0f - 0.25f * crouchPhase;
        squashX = 1.0f + 0.14f * crouchPhase;
        currentPlayerPos.y -= 0.12f * crouchPhase;
    }
    // ── フェーズ2 (0.28s〜0.65s): ゴールを頭上へ勢いよく持ち上げて伸び上がる ──
    else if (timer_ < 0.65f) {
        float liftP = (timer_ - 0.28f) / (0.65f - 0.28f);
        float stretchPulse = std::sin(liftP * std::numbers::pi_v<float>);
        stretchY = 1.0f + 0.22f * stretchPulse;
        squashX = 1.0f - 0.10f * stretchPulse;
        currentPlayerPos.y += 0.20f * stretchPulse;
    }
    // ── フェーズ3 (0.65s〜): 頭上にゴールを掲げたまま喜びバウンス ──
    else {
        float celebTime = timer_ - 0.65f;
        float hopWave = std::abs(std::sin(celebTime * 7.5f));
        currentPlayerPos.y += hopWave * 0.30f;

        float squashWave = std::sin(celebTime * 15.0f);
        stretchY = 1.0f + 0.12f * squashWave;
        squashX = 1.0f - 0.07f * squashWave;
        poseRoll = 0.07f * std::sin(celebTime * 7.5f);
    }

    float currentYaw = Lerp(startPlayerYaw_, targetPlayerYaw_, easeTurn);

    // ── 右手パーツ("RightHand")で星を頭上に突き上げるサブパーツアニメーション ──
    Vector3 rightHandOffset = { 0.0f, 0.0f, 0.0f };
    Vector3 leftHandOffset  = { 0.0f, 0.0f, 0.0f };

    if (timer_ < 0.28f) {
        // 予備動作：右手を少し下げて星を掴む構え
        float p = timer_ / 0.28f;
        rightHandOffset = { -0.10f * p, -0.15f * p, 0.15f * p };
        leftHandOffset  = { -0.05f * p, -0.08f * p, 0.0f };
    }
    else if (timer_ < 0.65f) {
        // 右手を頭上高く突き上げる（EaseOutBack）
        float p = std::clamp((timer_ - 0.28f) / (0.65f - 0.28f), 0.0f, 1.0f);
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        float easeBack = 1.0f + c3 * std::pow(p - 1.0f, 3.0f) + c1 * std::pow(p - 1.0f, 2.0f);

        rightHandOffset = Lerp(
            Vector3{ -0.10f, -0.15f, 0.15f },
            Vector3{ -0.08f,  0.78f, 0.05f },
            easeBack
        );
        leftHandOffset = { -0.08f, -0.05f * (1.0f - p), 0.0f };
    }
    else {
        // 右手を高く掲げたまま喜びバウンス
        float celebTime = timer_ - 0.65f;
        float handBob = std::sin(celebTime * 15.0f) * 0.06f;
        rightHandOffset = { -0.08f, 0.78f + handBob, 0.05f };
        leftHandOffset  = { -0.06f, -0.06f * std::sin(celebTime * 7.5f), 0.0f };
    }

    if (player && player->GetObject3d()) {
        player->GetObject3d()->SetSubPartTranslate("RightHand", rightHandOffset);
        player->GetObject3d()->SetSubPartTranslate("LeftHand",  leftHandOffset);
        player->GetObject3d()->SetTranslate(currentPlayerPos);
        player->GetObject3d()->SetRotate({ 0.0f, currentYaw, poseRoll });
        player->GetObject3d()->SetScale({
            playerBaseScale_.x * squashX,
            playerBaseScale_.y * stretchY,
            playerBaseScale_.z * squashX
        });
        player->GetObject3d()->Update();
    }

    // ── ゴールオブジェクト（星）を突き上げた右手のすぐ上に追従させて掲げる ──
    if (goal && goal->GetObject3d()) {
        Vector3 rightHandWorldPos = (player && player->GetObject3d())
            ? player->GetObject3d()->GetSubPartWorldPosition("RightHand")
            : currentPlayerPos;

        Vector3 hoistTargetPos = rightHandWorldPos + Vector3{ 0.0f, 0.58f, 0.0f };

        Vector3 goalPos = goalStartPos_;
        Vector3 goalScale = goalBaseScale_;
        float goalTiltZ = 0.0f;

        if (timer_ < 0.28f) {
            float p = timer_ / 0.28f;
            goalPos = Lerp(goalStartPos_, hoistTargetPos, p);
            goalYaw_ += 3.0f * dt;
        }
        else if (timer_ < 0.65f) {
            float p = std::clamp((timer_ - 0.28f) / (0.65f - 0.28f), 0.0f, 1.0f);
            goalPos = hoistTargetPos;
            float popScale = 1.0f + 0.20f * std::sin(p * std::numbers::pi_v<float>);
            goalScale = goalBaseScale_ * popScale;
            goalYaw_ += 10.0f * dt;
        }
        else {
            if (!hasEmittedHoistEffect_) {
                hasEmittedHoistEffect_ = true;
                ParticleManager::GetInstance()->Emit("GameEffects", "Hit", hoistTargetPos, 14);
            }
            goalPos = hoistTargetPos;
            goalYaw_ += 6.5f * dt;
            goalTiltZ = poseRoll;
        }

        goal->GetObject3d()->SetTranslate(goalPos);
        goal->GetObject3d()->SetRotate({ 0.0f, goalYaw_, goalTiltZ });
        goal->GetObject3d()->SetScale(goalScale);
        goal->GetObject3d()->Update();
    }

    if (camCtrl && camCtrl->GetCamera()) {
        Camera* camera = camCtrl->GetCamera();
        Vector3 camPos = Lerp(startCamPos_, targetCamPos_, easeOut);
        Vector3 lookAt = Lerp(startLookAt_, targetLookAt_, easeOut);

        Vector3 dir = lookAt - camPos;
        float len = Length(dir);
        if (len > 0.001f) {
            dir = dir / len;
            float angleY = std::atan2(dir.x, dir.z);
            float distXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);
            float angleX = std::atan2(-dir.y, distXZ);
            camera->SetTranslate(camPos);
            camera->SetRotate({ angleX, angleY, 0.0f });
            camera->Update();
        }
    }

    // カメラ移動に伴うステージ内エネミーのWVP行列同期
    for (const auto& enemy : gameScene->GetEnemies()) {
        if (enemy) {
            enemy->UpdateMatrix();
        }
    }
}

void ClearPhase::Update(Scene* scene)
{
    const float dt = 1.0f / 60.0f;
    timer_ += dt;

    Input* input = Input::GetInstance();
    bool isSkip = input && (
        input->TriggerKeyDown(DIK_SPACE) ||
        input->TriggerKeyDown(DIK_RETURN) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_A));

    if (step_ == Step::GoalZoom) {
        UpdateCameraAndPlayer(scene, dt);

        if (timer_ >= kZoomDuration_ || isSkip) {
            step_ = Step::IrisClose;
            timer_ = 0.0f;
        }
    }
    else if (step_ == Step::IrisClose) {
        irisProgress_ = std::clamp(timer_ / kIrisDuration_, 0.0f, 1.0f);
        UpdateIrisSprites(irisProgress_);

        if (irisProgress_ >= 1.0f) {
            step_ = Step::ChangeScene;
            SceneManager::GetInstance()->ChangeScene("ClearScene");
            return;
        }
    }
}

void ClearPhase::Draw(Scene* scene)
{
    (void)scene;
    if (step_ >= Step::IrisClose && irisProgress_ > 0.001f) {
        for (auto& blade : irisBlades_) {
            if (blade) {
                blade->Draw();
            }
        }
        if (irisCenterCircle_) {
            irisCenterCircle_->Draw();
        }
    }
}

void ClearPhase::Finalize(Scene* scene)
{
    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;
    if (player && player->GetObject3d()) {
        player->GetObject3d()->ResetSubParts();
        player->GetObject3d()->SetScale(playerBaseScale_);
    }
    irisBlades_.clear();
    irisCenterCircle_.reset();
}

