#include "SceneManager.h"
#include <cassert>
#include <memory>
#include "LightManager.h"
#include "PrimitiveDrawer.h"
#include "OffScreen.h"
#include "ParticleManager.h"
#include "GpuParticleManager.h"
#include "EffectManager.h"
#include "Fade.h"

void SceneManager::Finalize() {
    Singleton<SceneManager>::Finalize();
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
            scene_->Finalize();
            LightManager::GetInstance()->ClearLights();
            ParticleManager::GetInstance()->ReleaseAllParticleGroupSets();
            GpuParticleManager::GetInstance()->ResetParticles();
            GpuParticleManager::GetInstance()->SetCamera(nullptr);
            scene_ = nullptr;
        }
        scene_ = std::move(nextScene_);
        scene_->SetSceneManager(this);

        scene_->Initialize();

        OffScreen::GetInstance()->SetCamera(scene_->GetActiveCamera());
    }
    if (scene_) {
        scene_->Update();
        LightManager::GetInstance()->Update();
        if (!scene_->IsGameTimeStopped()) {
            ParticleManager::GetInstance()->Update();
            GpuParticleManager::GetInstance()->Update();
            EffectManager::GetInstance()->Update();
        }
    }
}

void SceneManager::Draw() {
    if (scene_) {

        scene_->Draw();
        PrimitiveDrawer::GetInstance()->Draw(); // シーンの描画後にプリミティブ描画を実行   
    }
    Fade::GetInstance()->Draw();

}

void SceneManager::ChangeScene(const std::string& sceneName)
{
    assert(sceneFactory_);
    nextScene_ = sceneFactory_->CreateScene(sceneName);
}
