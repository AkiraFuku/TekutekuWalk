#include "StageSelectScene.h"
#include "Phase.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "TextureManager.h"
#include "LightManager.h"
#include "Object3dCommon.h"
#include "ModelManager.h"
#include "Input.h"
#include "Fade.h"
#include "imgui.h"
#include <numbers>
#include <cmath>

StageSelectScene::StageSelectScene() = default;
StageSelectScene::~StageSelectScene() = default;

void StageSelectScene::Initialize()
{
    // カメラ初期化 (画面上部のキューブ群と下部のプレイヤーが見渡せるアングル)
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 1.8f, -6.5f });
    cameraMap_["Main"] = std::move(camera_);
    ChangeActiveCamera(cameraMap_["Main"].get());

    // ライト初期化
    LightManager::GetInstance()->AddDirectionalLight({ 0.0f, -1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f);

    // テクスチャ事前読み込み
    TextureManager::GetInstance()->LoadTexture("resources/Stagemap/863603.png");
    TextureManager::GetInstance()->LoadTexture("resources/Yellow.png");

    // スカイボックス初期化
    skyBox_ = std::make_unique<SkyBox>();
    skyBox_->Initialize();
    skyBox_->SetCamera(activeCamera_);
    skyBox_->SetTextureByFilePath("resources/output_skybox.dds");
    Object3dCommon::GetInstance()->SetDefaultSkyBox(skyBox_.get());

    // ステージ＆タイトル戻る項目の構築
    const std::string kTextureDir = "resources/stageTitle/";
    itemList_ = {
        { ItemType::Stage, "Stage 1", "resources/Stagemap/stage1.json", "チュートリアル・基本ステージ", "Stage1.png" },
        { ItemType::Stage, "Stage 2", "resources/Stagemap/stage2.json", "中級ステージ 2", "Stage2.png" },
        { ItemType::Stage, "Stage 3", "resources/Stagemap/stage3.json", "上級ステージ 3", "Stage3.png" },
        { ItemType::Stage, "Stage 4", "resources/Stagemap/stage4.json", "高難易度ステージ 4", "Stage4.png" },
        { ItemType::Stage, "Stage 5", "resources/Stagemap/stage5.json", "巨大ステージ 5", "Stage5.png" },
        { ItemType::Stage, "Test Stage", "resources/Stagemap/test_stage.json", "開発・テスト用ステージ", "Test.png" },
        { ItemType::BackToTitle, "Title に戻る", "", "タイトル画面に戻ります", "Back.png" },
    };

    // ModelManager を使ってキューブモデルを生成
    ModelManager::GetInstance()->CreateBoxModel("StageSelectCube");

    // 各項目に対応するキューブオブジェクトを生成
    cubeObjects_.clear();
    for (size_t i = 0; i < itemList_.size(); ++i) {
        auto cube = std::make_unique<Object3d>();
        cube->Initialize();
        cube->SetModel("StageSelectCube");
        if (!itemList_[i].textureName.empty()) {
            cube->SetTexture(kTextureDir + itemList_[i].textureName);
        }
        cube->SetCamera(activeCamera_);
        cubeObjects_.push_back(std::move(cube));
    }

    // --- 道（地面）オブジェクトの生成 ---
    ModelManager::GetInstance()->CreateBoxModel("StageSelectRoad");
    roadObject_ = std::make_unique<Object3d>();
    roadObject_->Initialize();
    roadObject_->SetModel("StageSelectRoad");
    roadObject_->SetTexture("resources/Stagemap/863603.png");
    roadObject_->SetScale(roadScale_);
    roadObject_->SetTranslate(roadPos_);
    roadObject_->SetRotate(roadRotate_);
    roadObject_->SetCamera(activeCamera_);

    // --- プレイヤーモデルとアニメーションの生成 ---
    ModelManager::GetInstance()->LoadModel("resources/human/", "walk.gltf");
    playerObject_ = std::make_unique<Object3d>();
    playerObject_->Initialize();
    playerObject_->SetModel("walk.gltf");
    playerObject_->SetTexture("resources/Yellow.png");
    playerObject_->SetTranslate(playerPos_);
    playerObject_->SetRotate(playerRotate_);
    playerObject_->SetCamera(activeCamera_);

    playerAnimation_ = std::make_unique<Animation>();
    playerAnimation_->Initialize("resources/human", "walk.gltf");
    playerAnimation_->SetCurrentTime(0.0f);
    playerObject_->SetAnimations(playerAnimation_.get());

    selectedIndex_ = 0;
    currentAngle_ = 0.0f;
    targetAngle_ = 0.0f;
    isDecided_ = false;
    decisionBlinkTimer_ = 0.0f;
    exitDirection_ = ExitDirection::None;
    exitTimer_ = 0.0f;
    isFadeStarted_ = false;

    // フェードイン開始
    Fade::GetInstance()->StartFadeIn(0.5f);
    isTransitioning_ = false;
}

void StageSelectScene::Finalize()
{
    cubeObjects_.clear();
    playerObject_.reset();
    playerAnimation_.reset();
    roadObject_.reset();
}

void StageSelectScene::Update()
{
    if (activeCamera_) {
        activeCamera_->Update();
    }

    auto* input = Input::GetInstance();
    const int itemCount = static_cast<int>(itemList_.size());

    // 決定前かつフェード中でなければ入力を受け付ける
    if (!isDecided_ && !Fade::GetInstance()->IsFading() && itemCount > 0) {
        // 左右キー / 上下キー / ゲームパッドで選択切り替え
        if (input->TriggerKeyDown(DIK_LEFT) || input->TriggerKeyDown(DIK_UP) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP)) {
            selectedIndex_ = (selectedIndex_ - 1 + itemCount) % itemCount;
        }
        if (input->TriggerKeyDown(DIK_RIGHT) || input->TriggerKeyDown(DIK_DOWN) ||
            input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN)) {
            selectedIndex_ = (selectedIndex_ + 1) % itemCount;
        }

        // 決定 (Enter / Space / Aボタン)
        if (input->TriggerKeyDown(DIK_RETURN) || input->TriggerKeyDown(DIK_SPACE) || input->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
            isDecided_ = true;
            decisionBlinkTimer_ = 0.0f;
            exitTimer_ = 0.0f;
            isFadeStarted_ = false;
            if (itemList_[selectedIndex_].type == ItemType::Stage) {
                nextSceneName_ = "GameScene";
                GameScene::SetSelectedStagePath(itemList_[selectedIndex_].filePath);
                exitDirection_ = ExitDirection::Right; // 右へ退場
            } else {
                nextSceneName_ = "TitleScene";
                exitDirection_ = ExitDirection::Left;  // 方向転換して左へ退場
            }
        }

        // 戻る (Escape / Bボタン) は直接タイトル戻り
        if (input->TriggerKeyDown(DIK_ESCAPE) || input->TriggerPadDown(0, XINPUT_GAMEPAD_B)) {
            isDecided_ = true;
            decisionBlinkTimer_ = 0.0f;
            exitTimer_ = 0.0f;
            isFadeStarted_ = false;
            nextSceneName_ = "TitleScene";
            exitDirection_ = ExitDirection::Left; // 方向転換して左へ退場
        }
    }

    // --- プレイヤー退場演出の更新 ---
    if (exitDirection_ != ExitDirection::None) {
        exitTimer_ += 1.0f / 60.0f;
        const float exitSpeed = 7.5f; // 画面外へ走る速度

        if (exitDirection_ == ExitDirection::Right) {
            // 右へ前進
            playerRotate_.y = playerBaseRotateY_;
            playerPos_.x += exitSpeed * (1.0f / 60.0f);
        } else if (exitDirection_ == ExitDirection::Left) {
            // 逆方向（左）へ方向転換して前進
            playerRotate_.y = -playerBaseRotateY_;
            playerPos_.x -= exitSpeed * (1.0f / 60.0f);
        }

        // 一定時間経過（画面外へ向かった頃）でフェードアウト開始
        if (!isFadeStarted_ && exitTimer_ >= 0.8f) {
            isFadeStarted_ = true;
            Fade::GetInstance()->StartFadeOut(0.5f);
        }
    }

    // フェード完了後のシーン遷移
    if (isFadeStarted_ && !Fade::GetInstance()->IsFading()) {
        SceneManager::GetInstance()->ChangeScene(nextSceneName_);
        return;
    }

    // 決定時点滅タイマーの更新
    if (isDecided_) {
        decisionBlinkTimer_ += 1.0f / 60.0f;
    }

    // --- 環状キューブ配置 & カルーセルアニメーション計算 ---
    if (itemCount > 0) {
        const float twoPi = 2.0f * std::numbers::pi_v<float>;
        const float angleStep = twoPi / static_cast<float>(itemCount);
        // 選択されたインデックスが手前(正面)に来る目標角度
        targetAngle_ = -static_cast<float>(selectedIndex_) * angleStep;

        // 角度のスムーズ補間 (最短回転補間: -π 〜 π に正規化して大回転を防止)
        float diff = targetAngle_ - currentAngle_;
        diff = std::fmod(diff, twoPi);
        if (diff > std::numbers::pi_v<float>) {
            diff -= twoPi;
        } else if (diff < -std::numbers::pi_v<float>) {
            diff += twoPi;
        }
        currentAngle_ += diff * 0.15f;

        for (int i = 0; i < itemCount; ++i) {
            float itemAngle = currentAngle_ + static_cast<float>(i) * angleStep;

            // XZ平面上の円周配置 (手前が -Z 側)
            float x = ringCenter_.x + ringRadius_ * std::sin(itemAngle);
            float z = ringCenter_.z - ringRadius_ * std::cos(itemAngle);
            float y = ringCenter_.y;

            cubeObjects_[i]->SetTranslate({ x, y, z });

            // 選択中のキューブは強調表示、非選択は小型化
            if (i == selectedIndex_) {
                float scale = isDecided_ ? cubeSelectedScale_ * 1.15f : cubeSelectedScale_;
                cubeObjects_[i]->SetScale({ scale, scale, scale });
                // 選択中の自転アニメーション (決定時は高速回転)
                static float spin = 0.0f;
                spin += isDecided_ ? 0.1f : 0.03f;
                cubeObjects_[i]->SetRotate({ 0.3f, spin, 0.0f });
            } else {
                cubeObjects_[i]->SetScale({ cubeNormalScale_, cubeNormalScale_, cubeNormalScale_ });
                // 外側を向くように回転
                cubeObjects_[i]->SetRotate({ 0.0f, itemAngle, 0.0f });
            }

            cubeObjects_[i]->Update();
        }
    }

    // --- 道オブジェクトの更新 ---
    if (roadObject_) {
        roadObject_->SetCamera(activeCamera_);
        roadObject_->SetScale(roadScale_);
        roadObject_->SetTranslate(roadPos_);
        roadObject_->SetRotate(roadRotate_);
        roadObject_->Update();
    }

    // --- プレイヤーオブジェクトの更新 ---
    if (playerObject_) {
        playerObject_->SetCamera(activeCamera_);
        playerObject_->SetTranslate(playerPos_);
        playerObject_->SetRotate(playerRotate_);
        playerObject_->Update();
    }

    // --- ImGui デバッグ & ステージ選択 UI ---
#ifdef USE_IMGUI
    ImGui::Begin("Stage Select Settings");
    ImGui::Text("Current Selection: %s", itemList_[selectedIndex_].name.c_str());
    ImGui::TextDisabled("Description: %s", itemList_[selectedIndex_].description.c_str());
    if (itemList_[selectedIndex_].type == ItemType::Stage) {
        ImGui::Text("File: %s", itemList_[selectedIndex_].filePath.c_str());
    }
    ImGui::Separator();

    for (int i = 0; i < itemCount; ++i) {
        if (ImGui::RadioButton(itemList_[i].name.c_str(), &selectedIndex_, i)) {
            // ImGui で選択された場合もカルーセルと連動
        }
    }

    ImGui::Separator();
    ImGui::Text("--- Layout Adjustments ---");
    ImGui::DragFloat3("Ring Center", &ringCenter_.x, 0.1f);
    ImGui::DragFloat("Ring Radius", &ringRadius_, 0.1f, 0.5f, 10.0f);
    ImGui::DragFloat("Cube Selected Scale", &cubeSelectedScale_, 0.02f, 0.05f, 2.0f);
    ImGui::DragFloat("Cube Normal Scale", &cubeNormalScale_, 0.02f, 0.05f, 2.0f);
    ImGui::Separator();
    ImGui::DragFloat3("Player Pos", &playerPos_.x, 0.1f);
    ImGui::DragFloat3("Player Rotate", &playerRotate_.x, 0.05f);
    ImGui::DragFloat3("Road Pos", &roadPos_.x, 0.1f);
    ImGui::DragFloat3("Road Scale", &roadScale_.x, 0.1f);
    ImGui::DragFloat3("Road Rotate", &roadRotate_.x, 0.05f);

    ImGui::Separator();
    if (ImGui::Button("Decide / Start", ImVec2(120, 30))) {
        if (!isDecided_) {
            isDecided_ = true;
            decisionBlinkTimer_ = 0.0f;
            exitTimer_ = 0.0f;
            isFadeStarted_ = false;
            if (itemList_[selectedIndex_].type == ItemType::Stage) {
                nextSceneName_ = "GameScene";
                GameScene::SetSelectedStagePath(itemList_[selectedIndex_].filePath);
                exitDirection_ = ExitDirection::Right;
            } else {
                nextSceneName_ = "TitleScene";
                exitDirection_ = ExitDirection::Left;
            }
        }
    }
    ImGui::End();
#endif
}

void StageSelectScene::Draw()
{
    if (skyBox_) {
        skyBox_->Draw();
    }

    // 道の描画
    if (roadObject_) {
        roadObject_->Draw();
    }

    // プレイヤーの描画
    if (playerObject_) {
        playerObject_->Draw();
    }

    // 環状キューブオブジェクトの描画
    for (size_t i = 0; i < cubeObjects_.size(); ++i) {
        if (cubeObjects_[i]) {
            // 決定演出中、選択されたキューブを高速点滅（チカチカ表示）
            if (isDecided_ && static_cast<int>(i) == selectedIndex_) {
                bool isBlinkVisible = (static_cast<int>(decisionBlinkTimer_ * 24.0f) % 2) == 0;
                if (!isBlinkVisible) {
                    continue; // 点滅の消灯フレームは描画をスキップ
                }
            }
            cubeObjects_[i]->Draw();
        }
    }
}
