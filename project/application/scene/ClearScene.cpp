#include "ClearScene.h"
#include "Phase.h"
#include "Sprite.h"
#include "WinApp.h"
#include "SceneManager.h"
#include "Input.h"
#include "Fade.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "LightManager.h"
#include "Object3dCommon.h"
#include "PrimitiveDrawer.h"
#include <cmath>
#include <algorithm>
#include <numbers>
#include <random>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

ClearScene::ClearScene() = default;
ClearScene::~ClearScene() = default;

void ClearScene::Initialize()
{
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 0.25f, -5.2f });
    camera_->SetRotate({ 0.06f, 0.0f, 0.0f });
    cameraMap_["Main"] = std::move(camera_);
    ChangeActiveCamera(cameraMap_["Main"].get());

    // 温かみのある太陽光ライティング
    LightManager::GetInstance()->AddDirectionalLight(
        { 1.0f, 0.97f, 0.92f, 1.0f },
        { 0.1f, -0.8f, 0.6f },
        1.15f
    );

    TextureManager::GetInstance()->LoadTexture("resources/white.png");
    TextureManager::GetInstance()->LoadTexture("resources/circle2.png");
    TextureManager::GetInstance()->LoadTexture("resources/StageClear/tekutekuClear.png");
    TextureManager::GetInstance()->LoadTexture("resources/GAMOVER/StageSerect.png");
    TextureManager::GetInstance()->LoadTexture("resources/GAMOVER/GoToTitle.png");

    skyBox_ = std::make_unique<SkyBox>();
    skyBox_->Initialize();
    skyBox_->SetCamera(activeCamera_);
    skyBox_->SetTextureByFilePath("resources/output_skybox.dds");
    Object3dCommon::GetInstance()->SetDefaultSkyBox(skyBox_.get());

    // 1. 背景パネル（温かみのあるクリームイエロー・最背面 Z=12.0f に配置）
    ModelManager::GetInstance()->CreateBoxModel("ClearBackdropBox");
    backdropObject_ = std::make_unique<Object3d>();
    backdropObject_->Initialize();
    backdropObject_->SetModel("ClearBackdropBox");
    backdropObject_->SetTexture("resources/white.png");
    backdropObject_->SetEnableLighting(false);
    backdropObject_->SetModelColor(backdropColor_);
    backdropObject_->SetScale({ 55.0f, 35.0f, 0.5f });
    backdropObject_->SetTranslate({ 0.0f, 0.0f, 12.0f });
    backdropObject_->SetCamera(activeCamera_);
    backdropObject_->Update();

    // 2. ステージクリア横断幕 (PbdCloth2D: 背景と3Dモデルの間の深度 Z=99.0f に配置)
    clearClothBanner_ = std::make_unique<PbdCloth2D>();
    clearClothBanner_->SetAnchorPoint(bannerAnchor_);
    clearClothBanner_->Initialize(bannerPos_, bannerSize_, 20, 14, bannerStiffness_, bannerPinMode_);
    clearClothBanner_->SetGravity(bannerGravity_);
    clearClothBanner_->SetWindEnabled(bannerWindEnabled_);
    clearClothBanner_->SetWindStrength(bannerWindStrength_);
    clearClothBanner_->SetDepthZ(bannerDepthZ_);
    clearClothBanner_->SetTexture("resources/StageClear/tekutekuClear.png");

    // 3. ステージ台座（サンセットオレンジ）
    ModelManager::GetInstance()->CreateBoxModel("ClearStageFloor");
    stageFloorObject_ = std::make_unique<Object3d>();
    stageFloorObject_->Initialize();
    stageFloorObject_->SetModel("ClearStageFloor");
    stageFloorObject_->SetTexture("resources/white.png");
    stageFloorObject_->SetModelColor(floorColor_);
    stageFloorObject_->SetScale(floorScale_);
    stageFloorObject_->SetTranslate(floorPos_);
    stageFloorObject_->SetCamera(activeCamera_);
    stageFloorObject_->Update();

    // 4. プレイヤーモデル（正面を向き、汎用サブパーツシステムで手足を分離バインド）
    ModelManager::GetInstance()->LoadModel("resources/player/", "player.obj");
    playerObject_ = std::make_unique<Object3d>();
    playerObject_->Initialize();
    playerObject_->SetModel("player.obj");
    playerObject_->SetTexture("resources/player/player.png");
    playerObject_->SetTranslate(playerBasePos_);
    playerObject_->SetRotate({ 0.0f, std::numbers::pi_v<float>, 0.0f });
    playerObject_->SetScale(playerBaseScale_);
    playerObject_->SetCamera(activeCamera_);

    // 汎用サブパーツ自動検出と名前バインド
    playerObject_->AutoDetectSubParts();
    playerObject_->BindSubPartByNearestPos("RightHand", {  0.505f, -0.162f, -0.024f });
    playerObject_->BindSubPartByNearestPos("LeftHand",  { -0.505f, -0.162f, -0.024f });
    playerObject_->BindSubPartByNearestPos("RightFoot", {  0.175f, -0.351f,  0.0f   });
    playerObject_->BindSubPartByNearestPos("LeftFoot",  { -0.175f, -0.351f,  0.0f   });
    playerObject_->ResetSubParts();
    playerObject_->SetSubPartTranslate("RightHand", rightHandRaiseOffset_);
    playerObject_->Update();

    // 5. 右手の先に掲げる星（ゴールオブジェクト）モデル
    ModelManager::GetInstance()->LoadModel("resources/Goal/", "goal.obj");
    goalObject_ = std::make_unique<Object3d>();
    goalObject_->Initialize();
    goalObject_->SetModel("goal.obj");
    Vector3 initHandPos = playerObject_->GetSubPartWorldPosition("RightHand");
    goalObject_->SetTranslate(initHandPos + starOffsetFromHand_);
    goalObject_->SetCamera(activeCamera_);
    goalObject_->Update();
    goalYaw_ = 0.0f;

    // 6. 暖色系紙吹雪（Confetti）の初期化
    InitializeConfetti();

    // 7. 遷移先選択ボタン（ステージセレクト / タイトル）
    stageSelectButtonSprite_ = std::make_unique<Sprite>();
    stageSelectButtonSprite_->Initialize("resources/GAMOVER/StageSerect.png");
    stageSelectButtonSprite_->SetSize(stageSelectBaseSize_);
    stageSelectButtonSprite_->SetPosition({
        static_cast<float>(WinApp::kClientWidth) * 0.34f,
        static_cast<float>(WinApp::kClientHeight) * 0.84f
    });
    stageSelectButtonSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    stageSelectButtonSprite_->Update();

    titleButtonSprite_ = std::make_unique<Sprite>();
    titleButtonSprite_->Initialize("resources/GAMOVER/GoToTitle.png");
    titleButtonSprite_->SetSize(titleBaseSize_);
    titleButtonSprite_->SetPosition({
        static_cast<float>(WinApp::kClientWidth) * 0.66f,
        static_cast<float>(WinApp::kClientHeight) * 0.84f
    });
    titleButtonSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    titleButtonSprite_->Update();

    // 8. 放射状アイリスオープン演出の初期化
    InitializeIrisOpen();

    isTransitioning_ = false;
    selectedMenu_ = MenuItem::StageSelect;
    timer_ = 0.0f;
    autoReturnTimer_ = 0.0f;
    prevStickLeft_ = false;
    prevStickRight_ = false;

    // ステージクリアBGM・SE
    clearBGM_ = Audio::GetInstance()->LoadAudio("resources/Audio/BGM/Stageclear.mp3");
    clearBGMVoice_ = Audio::GetInstance()->PlayAudio(clearBGM_, false, 0.35f, "SE");
    decisionSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void ClearScene::InitializeConfetti()
{
    confettiList_.clear();
    confettiList_.reserve(kConfettiCount_);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distX(40.0f, static_cast<float>(WinApp::kClientWidth) - 40.0f);
    std::uniform_real_distribution<float> distY(-400.0f, static_cast<float>(WinApp::kClientHeight) * 0.65f);
    std::uniform_real_distribution<float> distW(16.0f, 34.0f);
    std::uniform_real_distribution<float> distH(12.0f, 24.0f);
    std::uniform_real_distribution<float> distFall(85.0f, 185.0f);
    std::uniform_real_distribution<float> distSwayAmp(18.0f, 55.0f);
    std::uniform_real_distribution<float> distSwayFreq(1.6f, 3.8f);
    std::uniform_real_distribution<float> distPhase(0.0f, 2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> distRotSpeed(-2.8f, 2.8f);
    std::uniform_real_distribution<float> distFlipSpeed(3.2f, 7.5f);

    // 暖色系（赤・オレンジ・黄色・ピンク・白）中心の華やかなカラーパレット
    const Vector4 palette[] = {
        { 0.98f, 0.42f, 0.42f, 0.95f }, // コーラルレッド
        { 1.00f, 0.64f, 0.32f, 0.95f }, // サンセットオレンジ
        { 1.00f, 0.88f, 0.38f, 0.95f }, // ゴールデンイエロー
        { 0.98f, 0.68f, 0.80f, 0.95f }, // ブロッサムピンク
        { 1.00f, 0.78f, 0.52f, 0.95f }, // ピーチゴールド
        { 0.96f, 0.52f, 0.66f, 0.95f }, // ルビーピンク
        { 1.00f, 0.98f, 0.92f, 0.95f }, // クリームホワイト
    };
    const int paletteSize = static_cast<int>(std::size(palette));

    for (int i = 0; i < kConfettiCount_; ++i) {
        ConfettiPiece piece;
        piece.sprite = std::make_unique<Sprite>();
        piece.sprite->Initialize("resources/white.png");
        piece.sprite->SetAnchorPoint({ 0.5f, 0.5f });

        piece.basePos = { distX(rng), distY(rng) };
        piece.baseSize = { distW(rng), distH(rng) };
        piece.fallSpeed = distFall(rng);
        piece.swayAmp = distSwayAmp(rng);
        piece.swayFreq = distSwayFreq(rng);
        piece.swayPhase = distPhase(rng);
        piece.rotation = distPhase(rng);
        piece.rotSpeed = distRotSpeed(rng);
        piece.flipPhase = distPhase(rng);
        piece.flipSpeed = distFlipSpeed(rng);
        piece.color = palette[i % paletteSize];

        piece.sprite->SetColor(piece.color);
        piece.sprite->SetPosition(piece.basePos);
        piece.sprite->SetSize(piece.baseSize);
        piece.sprite->SetRotation(piece.rotation);
        piece.sprite->Update();

        confettiList_.push_back(std::move(piece));
    }
}

void ClearScene::UpdateConfetti(float dt)
{
    const float screenH = static_cast<float>(WinApp::kClientHeight);
    const float screenW = static_cast<float>(WinApp::kClientWidth);

    for (auto& piece : confettiList_) {
        piece.basePos.y += piece.fallSpeed * dt;
        piece.swayPhase += piece.swayFreq * dt;
        piece.rotation += piece.rotSpeed * dt;
        piece.flipPhase += piece.flipSpeed * dt;

        // 画面下端を超えたら上部へループさせて絶え間なく降らせる
        if (piece.basePos.y > screenH + 40.0f) {
            piece.basePos.y = -40.0f;
            piece.basePos.x = std::fmod(piece.basePos.x + 173.0f, screenW - 80.0f) + 40.0f;
        }

        float currentX = piece.basePos.x + std::sin(piece.swayPhase) * piece.swayAmp;

        // 紙片がクルクルと3D的に裏返りながら舞い落ちる見た目を再現
        float flipX = 0.22f + 0.78f * std::abs(std::cos(piece.flipPhase));
        float flipY = 0.55f + 0.45f * std::abs(std::sin(piece.flipPhase * 0.7f));

        piece.sprite->SetPosition({ currentX, piece.basePos.y });
        piece.sprite->SetSize({ piece.baseSize.x * flipX, piece.baseSize.y * flipY });
        piece.sprite->SetRotation(piece.rotation);
        piece.sprite->Update();
    }
}

void ClearScene::InitializeIrisOpen()
{
    irisTimer_ = 0.0f;
    isIrisOpenFinished_ = false;
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
        blade->SetAnchorPoint({ 0.5f, 1.0f });
        blade->SetPosition(center);
        float angle = (twoPi / static_cast<float>(kIrisBladeCount_)) * static_cast<float>(i);
        blade->SetRotation(angle);

        // サンセットオレンジ＆ゴールデンクリームの暖色グラデーション
        Vector4 bladeColor = (i % 2 == 0)
            ? Vector4{ 0.98f, 0.68f, 0.42f, 1.0f }
            : Vector4{ 1.00f, 0.85f, 0.58f, 1.0f };
        blade->SetColor(bladeColor);
        blade->SetSize({ 260.0f, 920.0f });
        blade->Update();
        irisBlades_.push_back(std::move(blade));
    }

    irisCenterCircle_ = std::make_unique<Sprite>();
    irisCenterCircle_->Initialize("resources/circle2.png");
    irisCenterCircle_->SetAnchorPoint({ 0.5f, 0.5f });
    irisCenterCircle_->SetPosition(center);
    irisCenterCircle_->SetColor({ 0.96f, 0.58f, 0.35f, 1.0f });
    irisCenterCircle_->SetSize({ 180.0f, 180.0f });
    irisCenterCircle_->Update();
}

void ClearScene::UpdateIrisOpen(float dt)
{
    if (isIrisOpenFinished_) return;

    irisTimer_ += dt;
    float t = std::clamp(irisTimer_ / kIrisOpenDuration_, 0.0f, 1.0f);
    // 1.0 (閉じた状態) -> 0.0 (開いた状態)
    float remain = 1.0f - (t * t * (3.0f - 2.0f * t));

    const Vector2 center = {
        static_cast<float>(WinApp::kClientWidth) * 0.5f,
        static_cast<float>(WinApp::kClientHeight) * 0.5f
    };
    const float twoPi = 2.0f * std::numbers::pi_v<float>;

    for (int i = 0; i < static_cast<int>(irisBlades_.size()); ++i) {
        float angle = (twoPi / static_cast<float>(kIrisBladeCount_)) * static_cast<float>(i);
        float outerDist = (1.0f - remain) * 820.0f;
        Vector2 bladePos = {
            center.x + std::sin(angle) * outerDist,
            center.y - std::cos(angle) * outerDist
        };
        irisBlades_[i]->SetPosition(bladePos);
        irisBlades_[i]->SetSize({ 260.0f * remain, 920.0f * remain });
        irisBlades_[i]->Update();
    }

    if (irisCenterCircle_) {
        float circleSize = 180.0f * std::clamp(remain * 1.4f, 0.0f, 1.0f);
        irisCenterCircle_->SetSize({ circleSize, circleSize });
        irisCenterCircle_->Update();
    }

    if (t >= 1.0f) {
        isIrisOpenFinished_ = true;
    }
}

void ClearScene::Update()
{
    const float dt = 1.0f / 60.0f;
    timer_ += dt;

    if (activeCamera_) {
        activeCamera_->Update();
    }
    if (skyBox_) {
        skyBox_->SetTranslate(activeCamera_->GetTranslate());
        skyBox_->Update();
    }

    // 1. 放射状アイリスオープンの更新
    UpdateIrisOpen(dt);

    // 2. 紙吹雪の更新
    UpdateConfetti(dt);

    // 3. ステージクリア横断幕（PbdCloth2D）の更新
    if (clearClothBanner_) {
        clearClothBanner_->SetPosition(bannerPos_);
        clearClothBanner_->SetSize(bannerSize_);
        clearClothBanner_->SetRotation(bannerRotation_);
        clearClothBanner_->SetDepthZ(bannerDepthZ_);
        clearClothBanner_->SetGravity(bannerGravity_);
        clearClothBanner_->SetWindEnabled(bannerWindEnabled_);
        clearClothBanner_->SetWindStrength(bannerWindStrength_);
        clearClothBanner_->Update();
    }

    // 4. 右手で星を掲げて喜ぶプレイヤーのサブパーツ＆バウンス更新
    if (playerObject_) {
        float bouncePhase = timer_ * 6.5f;
        float hop = std::abs(std::sin(bouncePhase));
        float squashWave = std::sin(bouncePhase * 2.0f);

        Vector3 pos = playerBasePos_;
        pos.y += hop * 0.30f;

        float scaleX = playerBaseScale_.x * (1.0f - 0.07f * squashWave);
        float scaleY = playerBaseScale_.y * (1.0f + 0.12f * squashWave);
        float rollZ = std::sin(bouncePhase * 0.5f) * 0.08f;

        // 掲げる手（"RightHand" または画面向かって右の "LeftHand"）を高く突き上げ、反対の手はリズムよく振る
        const std::string raisedHandName = useScreenRightHand_ ? "LeftHand" : "RightHand";
        const std::string otherHandName  = useScreenRightHand_ ? "RightHand" : "LeftHand";

        float handBob = std::sin(bouncePhase * 2.0f) * 0.06f;
        Vector3 raisedOffset = {
            useScreenRightHand_ ? -rightHandRaiseOffset_.x : rightHandRaiseOffset_.x,
            rightHandRaiseOffset_.y + handBob,
            rightHandRaiseOffset_.z
        };
        Vector3 otherOffset = {
            0.0f,
            -0.08f * std::sin(bouncePhase),
            0.06f * std::cos(bouncePhase)
        };

        playerObject_->SetSubPartTranslate(raisedHandName, raisedOffset);
        playerObject_->SetSubPartTranslate(otherHandName, otherOffset);

        playerObject_->SetTranslate(pos);
        playerObject_->SetScale({ scaleX, scaleY, scaleX });
        playerObject_->SetRotate({ 0.0f, std::numbers::pi_v<float>, rollZ });
        playerObject_->Update();

        // 突き上げた手のワールド座標を取得し、そのすぐ上に星（goalObject_）を追従＆回転
        if (goalObject_) {
            goalYaw_ += goalRotateSpeed_ * dt;
            Vector3 handWorldPos = playerObject_->GetSubPartWorldPosition(raisedHandName);
            Vector3 goalPos = handWorldPos + starOffsetFromHand_;
            goalObject_->SetTranslate(goalPos);
            goalObject_->SetRotate({ 0.0f, goalYaw_, rollZ });
            goalObject_->Update();
        }
    }

    if (backdropObject_) {
        backdropObject_->SetModelColor(backdropColor_);
        backdropObject_->Update();
    }
    if (stageFloorObject_) {
        stageFloorObject_->SetModelColor(floorColor_);
        stageFloorObject_->SetTranslate(floorPos_);
        stageFloorObject_->SetScale(floorScale_);
        stageFloorObject_->Update();
    }

    // 5. 選択ボタン（ステージセレクト / タイトル）のパルスハイライト更新
    float pulse = 1.0f + 0.045f * std::sin(timer_ * 6.0f);
    if (stageSelectButtonSprite_) {
        bool isSelected = (selectedMenu_ == MenuItem::StageSelect);
        float scale = isSelected ? (1.12f * pulse) : 0.94f;
        stageSelectButtonSprite_->SetSize(stageSelectBaseSize_ * scale);
        stageSelectButtonSprite_->SetColor(isSelected
            ? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }
            : Vector4{ 0.55f, 0.55f, 0.55f, 0.78f });
        stageSelectButtonSprite_->Update();
    }
    if (titleButtonSprite_) {
        bool isSelected = (selectedMenu_ == MenuItem::Title);
        float scale = isSelected ? (1.12f * pulse) : 0.94f;
        titleButtonSprite_->SetSize(titleBaseSize_ * scale);
        titleButtonSprite_->SetColor(isSelected
            ? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }
            : Vector4{ 0.55f, 0.55f, 0.55f, 0.78f });
        titleButtonSprite_->Update();
    }

    Input* input = Input::GetInstance();

#ifdef USE_IMGUI
    ImGui::Begin("Clear Scene Settings");
    ImGui::Text("=== STAGE CLEAR SCENE ===");

    bool triggerStageSelect = false;
    bool triggerTitle = false;
    bool triggerGoalDemo = false;

    const char* menuNames[] = { "Stage Select Scene", "Title Scene", "Goal Demo Scene" };
    int currentItem = static_cast<int>(selectedMenu_);
    if (ImGui::Combo("Next Destination", &currentItem, menuNames, IM_ARRAYSIZE(menuNames))) {
        selectedMenu_ = static_cast<MenuItem>(currentItem);
    }

    ImGui::Separator();
    ImGui::Text("--- Colors (Warm Sunset & Cream) ---");
    ImGui::ColorEdit4("Backdrop Color", &backdropColor_.x);
    ImGui::ColorEdit4("Stage Floor Color", &floorColor_.x);

    ImGui::Separator();
    ImGui::Text("--- Right Hand & Star Pose ---");
    ImGui::Checkbox("Use Screen-Right Hand", &useScreenRightHand_);
    ImGui::DragFloat3("Hand Raise Offset", &rightHandRaiseOffset_.x, 0.02f, -1.5f, 2.0f);
    ImGui::DragFloat3("Star Offset from Hand", &starOffsetFromHand_.x, 0.02f, -1.5f, 2.0f);
    ImGui::DragFloat("Star Rotate Speed", &goalRotateSpeed_, 0.1f, 0.0f, 20.0f);
    ImGui::DragFloat3("Player Pos", &playerBasePos_.x, 0.05f);
    ImGui::DragFloat3("Floor Pos", &floorPos_.x, 0.05f);
    ImGui::DragFloat3("Floor Scale", &floorScale_.x, 0.1f);

    ImGui::Separator();
    ImGui::Text("--- Cloth Banner (PbdCloth2D) ---");
    ImGui::DragFloat2("Banner Pos", &bannerPos_.x, 1.0f);
    if (ImGui::DragFloat2("Banner Size", &bannerSize_.x, 1.0f, 100.0f, 1280.0f)) {
        if (clearClothBanner_) {
            clearClothBanner_->SetSize(bannerSize_);
            clearClothBanner_->Reset();
        }
    }
    ImGui::DragFloat("Banner Depth Z", &bannerDepthZ_, 0.1f, 0.0f, 99.9f);
    const char* pinModes[] = { "Top Corners (2 Points)", "Top Edge All", "Four Corners (4 Points)" };
    if (ImGui::Combo("Banner Pin Mode", &bannerPinMode_, pinModes, IM_ARRAYSIZE(pinModes))) {
        if (clearClothBanner_) {
            clearClothBanner_->SetPinMode(bannerPinMode_);
        }
    }
    if (ImGui::SliderFloat("Banner Stiffness", &bannerStiffness_, 0.05f, 1.0f)) {
        if (clearClothBanner_) {
            clearClothBanner_->SetK(bannerStiffness_);
        }
    }
    ImGui::Checkbox("Banner Wind", &bannerWindEnabled_);
    ImGui::SliderFloat("Wind Strength", &bannerWindStrength_, 0.0f, 600.0f);

    ImGui::Separator();
    if (ImGui::Button("Go to Stage Select", ImVec2(180, 36))) {
        selectedMenu_ = MenuItem::StageSelect;
        triggerStageSelect = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Go to Title", ImVec2(180, 36))) {
        selectedMenu_ = MenuItem::Title;
        triggerTitle = true;
    }
    if (ImGui::Button("Go to Goal Demo (R)", ImVec2(368, 36))) {
        selectedMenu_ = MenuItem::GoalDemo;
        triggerGoalDemo = true;
    }
    ImGui::Checkbox("Auto Return to Goal Demo", &s_autoReturnToGoalDemo_);
    if (s_autoReturnToGoalDemo_) {
        ImGui::SliderFloat("Auto Return Delay (s)", &s_autoReturnDelay_, 0.5f, 10.0f, "%.1fs");
        ImGui::Text("Auto Return: %.1f / %.1f s", autoReturnTimer_, s_autoReturnDelay_);
    }
    ImGui::End();
#endif

    if (!isTransitioning_ && isIrisOpenFinished_) {
        if (s_autoReturnToGoalDemo_) {
            autoReturnTimer_ += 1.0f / 60.0f;
        }
        bool stickLeft = false;
        bool stickRight = false;
        XINPUT_STATE padState{};
        if (input && input->GetJoyStick(0, padState)) {
            float lx = static_cast<float>(padState.Gamepad.sThumbLX) / 32767.0f;
            stickLeft = (lx < -0.5f);
            stickRight = (lx > 0.5f);
        }
        bool triggerStickLeft = stickLeft && !prevStickLeft_;
        bool triggerStickRight = stickRight && !prevStickRight_;
        prevStickLeft_ = stickLeft;
        prevStickRight_ = stickRight;

        bool selectStage = input && (
            input->TriggerKeyDown(DIK_LEFT) || input->TriggerKeyDown(DIK_UP) ||
            input->TriggerKeyDown(DIK_A) || input->TriggerKeyDown(DIK_W) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP) ||
            triggerStickLeft);

        bool selectTitle = input && (
            input->TriggerKeyDown(DIK_RIGHT) || input->TriggerKeyDown(DIK_DOWN) ||
            input->TriggerKeyDown(DIK_D) || input->TriggerKeyDown(DIK_S) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN) ||
            triggerStickRight);

        if (selectStage) {
            if (selectedMenu_ != MenuItem::StageSelect && clearClothBanner_) {
                clearClothBanner_->ApplyGust(-260.0f); // 選択切り替え時に横断幕へ風を送る
            }
            selectedMenu_ = MenuItem::StageSelect;
        } else if (selectTitle) {
            if (selectedMenu_ != MenuItem::Title && clearClothBanner_) {
                clearClothBanner_->ApplyGust(260.0f); // 選択切り替え時に横断幕へ風を送る
            }
            selectedMenu_ = MenuItem::Title;
        }

        bool isGoalDemoShortcut = (input && (
            input->TriggerKeyDown(DIK_R) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_Y)));
        bool isAutoReturn = s_autoReturnToGoalDemo_ && (autoReturnTimer_ >= s_autoReturnDelay_);
        if (isGoalDemoShortcut || isAutoReturn) {
            selectedMenu_ = MenuItem::GoalDemo;
        }

        bool isPressed = (input && (
            input->TriggerKeyDown(DIK_SPACE) ||
            input->TriggerKeyDown(DIK_RETURN) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_A) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_START)))
            || isGoalDemoShortcut
            || isAutoReturn
#ifdef USE_IMGUI
            || triggerStageSelect || triggerTitle || triggerGoalDemo
#endif
            ;

        if (isPressed) {
            isTransitioning_ = true;
            if (decisionSE_ != 0) {
                Audio::GetInstance()->PlayAudio(decisionSE_, false, 0.8f, "SE");
            }
            Fade::GetInstance()->StartFadeOut(1.0f);
        }
    }
    else if (isTransitioning_) {
        if (!Fade::GetInstance()->IsFading()) {
            s_autoReturnToGoalDemo_ = false;
            if (selectedMenu_ == MenuItem::StageSelect) {
                SceneManager::GetInstance()->ChangeScene("StageSelectScene");
            } else if (selectedMenu_ == MenuItem::GoalDemo) {
                SceneManager::GetInstance()->ChangeScene("GoalDemoScene");
            } else {
                SceneManager::GetInstance()->ChangeScene("TitleScene");
            }
        }
    }
}

void ClearScene::Draw()
{
    // 1. 最背面：スカイボックス＆背景パネル
    if (skyBox_) {
        skyBox_->Draw();
    }
    if (backdropObject_) {
        backdropObject_->Draw();
    }

    // 2. 中間層（背景と3Dモデルの間）：ステージクリア横断幕 (PbdCloth2D, DepthZ = 99.0f)
    if (clearClothBanner_) {
        clearClothBanner_->Draw();
    }

    // 3. 前面層：3Dモデル群（ステージ台座・プレイヤー・右手の星）
    if (stageFloorObject_) {
        stageFloorObject_->Draw();
    }
    if (playerObject_) {
        playerObject_->Draw();
    }
    if (goalObject_) {
        goalObject_->Draw();
    }

    // 4. 最前面：紙吹雪＆選択ボタンUI＆トランジション
    for (auto& piece : confettiList_) {
        if (piece.sprite) {
            piece.sprite->Draw();
        }
    }

    if (stageSelectButtonSprite_) {
        stageSelectButtonSprite_->Draw();
    }
    if (titleButtonSprite_) {
        titleButtonSprite_->Draw();
    }

    if (!isIrisOpenFinished_) {
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

void ClearScene::Finalize()
{
    if (Audio::GetInstance()->IsPlaying(clearBGMVoice_)) {
        Audio::GetInstance()->StopAudio(clearBGMVoice_);
    }
    if (playerObject_) {
        playerObject_->ResetSubParts();
    }
    confettiList_.clear();
    irisBlades_.clear();
    irisCenterCircle_.reset();
    clearClothBanner_.reset();
    stageSelectButtonSprite_.reset();
    titleButtonSprite_.reset();
    goalObject_.reset();
    playerObject_.reset();
    stageFloorObject_.reset();
    backdropObject_.reset();
    skyBox_.reset();
}
