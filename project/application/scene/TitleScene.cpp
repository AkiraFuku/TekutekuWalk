#include "TitleScene.h"
#include "ModelManager.h"
#include "Input.h"
#include "imgui.h"
#include "SceneManager.h"
#include "ParticleManager.h"//フレームワークに移植
#include "PSOManager.h"
#include "LightManager.h"
#include "PrimitiveDrawer.h"
#include "Phase.h"
#include "Fade.h"
#include "TitlePhase.h"
#include "PbdCloth2D.h"
#include <cmath>


TitleScene::TitleScene() = default;
TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {

    // 1. メインカメラの生成
    camera = std::make_unique<Camera>();
    camera->SetTranslate({ 0.0f, 0.0f, -5.0f });
    cameraMap_["Main"] = std::move(camera);

    // 2. デバッグ用カメラの生成
    auto debugCamera = std::make_unique<Camera>();
    debugCamera->SetTranslate({ 0.0f, 10.0f, -20.0f });
    cameraMap_["Debug"] = std::move(debugCamera);

    // 3. 最初はメインカメラをセット
    ChangeActiveCamera(cameraMap_["Main"].get());


    handle_ = Audio::GetInstance()->LoadAudio("resources/fanfare.mp3");

    // Audio::GetInstance()->PlayAudio(handle_, true);

    TextureManager::GetInstance()->LoadTexture("resources/Stagemap/863603.png");
    TextureManager::GetInstance()->LoadTexture("resources/Title/tekutekuTitle.png");

    LightManager::GetInstance()->AddDirectionalLight({ 0.0f,-1.0f,0.0f }, { 1.0f,1.0f,1.0f }, 1.0f);

    skyBox = std::make_unique<SkyBox>();
    skyBox->Initialize();
    skyBox->SetCamera(activeCamera_);
    skyBox->SetTextureByFilePath("resources/output_skybox.dds");
    Object3dCommon::GetInstance()->SetDefaultSkyBox(skyBox.get());

    Fade::GetInstance()->StartFadeIn(5.0f);

    // 道（地面ブロック）の生成と配置
    ModelManager::GetInstance()->CreateBoxModel("TitleRoad");
    roadObject_ = std::make_unique<Object3d>();
    roadObject_->Initialize();
    roadObject_->SetModel("TitleRoad");
    roadObject_->SetTexture("resources/Stagemap/863603.png");
    roadObject_->SetScale(roadScale_);
    roadObject_->SetTranslate({ characterPos_.x, characterPos_.y - 0.532f, characterPos_.z });
    roadObject_->SetCamera(activeCamera_);

    object3d = std::make_unique<Object3d>();
    ModelManager::GetInstance()->LoadModel("resources/player/", "player.obj");
    object3d->Initialize();
    //object_->SetModel("player.obj");
    object3d->SetModel("player.obj");
    object3d->SetTexture("resources/player/player.png");
    object3d->SetTranslate(characterPos_);
    object3d->SetCamera(activeCamera_);







    // 2D PBD 布（画面全体・四隅固定タイトル幕）オブジェクトの初期化
    cloth2D_ = std::make_unique<PbdCloth2D>();
    cloth2D_->SetAnchorPoint(cloth2DAnchor_);
    cloth2D_->Initialize(cloth2DPos_, cloth2DSize_, 20, 16, cloth2DK_, cloth2DPinMode_);
    cloth2D_->SetGravity(cloth2DGravity_);
    cloth2D_->SetTexture("resources/Title/tekutekuTitle.png");
    ChangePhase(std::make_unique<TitlePhase>());

    handle_ = Audio::GetInstance()->LoadAudio("resources/Audio/BGM/bgm.mp3");

    BGMHandle_= Audio::GetInstance()->PlayAudio(handle_, true, 0.5f ,"BGM");

}
void TitleScene::Finalize() {
    if (Audio::GetInstance()->IsPlaying(BGMHandle_))
    {
        Audio::GetInstance()->StopAudio(BGMHandle_);
    }

}
void TitleScene::Update() {
    // カメラの旋回（オービット）計算
    if (activeCamera_ && !isDebugCamera_) {
        cameraOrbitAngle_ += cameraOrbitSpeed_ * (1.0f / 60.0f);

        // キャラクター中心の円軌道上のカメラ位置
        Vector3 camPos;
        camPos.x = characterPos_.x + std::sin(cameraOrbitAngle_) * cameraOrbitRadius_;
        camPos.z = characterPos_.z + std::cos(cameraOrbitAngle_) * cameraOrbitRadius_;
        camPos.y = characterPos_.y + cameraHeight_;
        activeCamera_->SetTranslate(camPos);

        // キャラクターの注視点
        Vector3 lookTarget = { characterPos_.x, characterPos_.y + cameraTargetHeight_, characterPos_.z };
        Vector3 diff = { lookTarget.x - camPos.x, lookTarget.y - camPos.y, lookTarget.z - camPos.z };
        float horizontalDist = std::sqrt(diff.x * diff.x + diff.z * diff.z);

        // 注視点に向く回転角（Yaw, Pitch）
        float yaw = std::atan2(diff.x, diff.z);
        float pitch = std::atan2(-diff.y, horizontalDist);
        activeCamera_->SetRotate({ pitch, yaw, 0.0f });
    }

    if (activeCamera_) {
        activeCamera_->Update();
        activeCamera_->UpdateViewProjection();
    }
    skyBox->Update();

    // 2D PBD 布の更新
    if (cloth2D_) {
        cloth2D_->SetPosition(cloth2DPos_);
        cloth2D_->SetSize(cloth2DSize_);
        cloth2D_->SetRotation(cloth2DRotation_);
        cloth2D_->SetGravity(cloth2DGravity_);
        cloth2D_->SetWindEnabled(cloth2DWindEnabled_);
        cloth2D_->SetWindStrength(cloth2DWindStrength_);
        cloth2D_->Update();
    }

    // 道（地面）の更新
    if (roadObject_) {
        roadObject_->SetCamera(activeCamera_);
        roadObject_->SetScale(roadScale_);
        roadObject_->SetTranslate({ characterPos_.x, characterPos_.y - 0.532f, characterPos_.z });
        roadObject_->Update();
    }

    object3d->SetCamera(activeCamera_);
    object3d->SetTranslate(characterPos_);
    object3d->Update();
    currentPhase_->Update(this);

#ifdef USE_IMGUI
    ImGui::Begin("Title Scene Camera & Road");
    if (ImGui::Button("Go to Stage Select Scene", ImVec2(200, 30))) {
        SceneManager::GetInstance()->ChangeScene("StageSelectScene");
    }
    ImGui::Separator();
    ImGui::DragFloat("Orbit Speed", &cameraOrbitSpeed_, 0.01f, -2.0f, 2.0f);
    ImGui::DragFloat("Orbit Radius", &cameraOrbitRadius_, 0.1f, 1.0f, 30.0f);
    ImGui::DragFloat("Camera Height", &cameraHeight_, 0.1f, -5.0f, 15.0f);
    ImGui::DragFloat("Target Height", &cameraTargetHeight_, 0.1f, 0.0f, 5.0f);
    ImGui::Separator();
    ImGui::DragFloat3("Character Pos", &characterPos_.x, 0.1f);
    ImGui::DragFloat3("Road Scale", &roadScale_.x, 0.1f);
    ImGui::End();

    ImGui::Begin("Title 2D Cloth Debug");
    ImGui::DragFloat2("Position", &cloth2DPos_.x, 1.0f, 0.0f, 1280.0f);
    ImGui::DragFloat2("Size", &cloth2DSize_.x, 1.0f, 50.0f, 1280.0f);
    ImGui::DragFloat("Rotation", &cloth2DRotation_, 0.02f, -3.14f, 3.14f);

    const char* pinModes[] = { "Top Corners Only (2 Points)", "Top Edge All", "Four Corners (4 Points)" };
    if (ImGui::Combo("Pin Mode", &cloth2DPinMode_, pinModes, IM_ARRAYSIZE(pinModes))) {
        if (cloth2D_) {
            cloth2D_->SetPinMode(cloth2DPinMode_);
        }
    }

    if (ImGui::SliderFloat("Stiffness (k)", &cloth2DK_, 0.05f, 1.0f)) {
        if (cloth2D_) {
            cloth2D_->SetK(cloth2DK_);
        }
    }

    ImGui::DragFloat2("Base Gravity", &cloth2DGravity_.x, 5.0f, -1000.0f, 1000.0f);
    ImGui::Checkbox("Enable Wind", &cloth2DWindEnabled_);
    ImGui::SliderFloat("Wind Strength", &cloth2DWindStrength_, 0.0f, 800.0f);

    if (ImGui::Button("Reset Cloth")) {
        if (cloth2D_) {
            cloth2D_->SetAnchorPoint(cloth2DAnchor_);
            cloth2D_->Initialize(cloth2DPos_, cloth2DSize_, 20, 16, cloth2DK_, cloth2DPinMode_);
            cloth2D_->SetGravity(cloth2DGravity_);
            cloth2D_->SetTexture("resources/Title/tekutekuTitle.png");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Gust Left (-X)")) {
        if (cloth2D_) {
            cloth2D_->ApplyGust(-400.0f);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Gust Right (+X)")) {
        if (cloth2D_) {
            cloth2D_->ApplyGust(400.0f);
        }
    }
    ImGui::End();
#endif
}
void TitleScene::Draw() {
    skyBox->Draw();
    if (roadObject_) {
        roadObject_->Draw();
    }
    object3d->Draw();
    if (cloth2D_) {
        cloth2D_->Draw();
    }
    if (currentPhase_) {
        currentPhase_->Draw(this);
    }
   
}


