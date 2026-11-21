#include "SceneManager.h"
#include <cassert>
#include <memory>
#include "LightManager.h"
#include "PrimitiveDrawer.h"
#include "OffScreen.h"
#include "ParticleManager.h"
#include "EffectManager.h"
#include "Fade.h"
#include "EditorManager.h"
#include "Object3dCommon.h"

// 静的メンバ変数の実体
std::unique_ptr<SceneManager> SceneManager::instance = nullptr;

SceneManager* SceneManager::GetInstance() {
    if (instance == nullptr) {
        // privateコンストラクタを呼び出せるヘルパー構造体
        struct Helper : public SceneManager {
            Helper() : SceneManager() {}
        };
        instance = std::make_unique<Helper>();
    }
    return instance.get();
}

void SceneManager::Finalize() {
    // staticなhelperInstanceはdeleteしない
    instance = nullptr;
}

SceneManager::~SceneManager()
{
    if (scene_) {
        scene_->Finalize();
        scene_ = nullptr;
    }
}

void SceneManager::Update() {
    // シーン切り替え処理
    if (nextScene_) {
        if (scene_) {
            EditorManager::GetInstance()->ClearObjects();
            scene_->Finalize();
            ParticleManager::GetInstance()->ReleaseAllParticleGroupSets();
            scene_ = nullptr;
        }
        scene_ = std::move(nextScene_);
        scene_->SetSceneManager(this);

        scene_->Initialize();

        OffScreen::GetInstance()->SetCamera(scene_->GetActiveCamera());
    }
    if (scene_) {
        // ゲームカメラは常にシーン自身の activeCamera_ を維持（デバッグカメラと完全分離）
        if (Camera* gameCam = scene_->GetActiveCamera()) {
            Object3dCommon::GetInstance()->SetDefaultCamera(gameCam);
            ParticleManager::GetInstance()->SetCamera(gameCam);
            PrimitiveDrawer::GetInstance()->SetCamera(gameCam);
        }

        if (EditorManager::GetInstance()->ShouldUpdateGame()) {
            scene_->Update();
            LightManager::GetInstance()->Update();
            if (!scene_->IsGameTimeStopped()) {
                ParticleManager::GetInstance()->Update();
                EffectManager::GetInstance()->Update();
            }
            EditorManager::GetInstance()->ConsumeStepFrame();
        } else {
            scene_->UpdateInEditor();
            LightManager::GetInstance()->Update();
        }
        EditorManager::GetInstance()->UpdateSpawnedObjects(scene_->GetActiveCamera());
    }
}

void SceneManager::Draw() {
    if (scene_) {

        scene_->Draw();
        EditorManager::GetInstance()->DrawSpawnedObjects();
        PrimitiveDrawer::GetInstance()->Draw(); // シーンの描画後にプリミティブ描画を実行   
    }
    Fade::GetInstance()->Draw();

}

void SceneManager::DrawSceneView() {
    if (scene_) {
        EditorManager::GetInstance()->DrawSceneGrid();
        scene_->DrawSceneView();
        EditorManager::GetInstance()->DrawSpawnedObjects();
        PrimitiveDrawer::GetInstance()->DrawSceneView();
    }
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
    assert(sceneFactory_);
    nextScene_ = sceneFactory_->CreateScene(sceneName);
}
