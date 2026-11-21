#include "Scene.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "PrimitiveDrawer.h"
#include "OffScreen.h"
#include "Phase.h"
#include "EditorManager.h"

// Sceneクラスに共通関数として作ると便利です
void Scene::ChangeActiveCamera(Camera* targetCamera) {
    activeCamera_ = targetCamera;
    if (activeCamera_) {
        activeCamera_->SetEditorName("Main Camera");
        EditorManager::GetInstance()->RegisterObject(activeCamera_);
    }

    // 各マネージャーに新しいカメラを通知
    Object3dCommon::GetInstance()->SetDefaultCamera(activeCamera_);
    ParticleManager::GetInstance()->SetCamera(activeCamera_);
    PrimitiveDrawer::GetInstance()->SetCamera(activeCamera_);
    OffScreen::GetInstance()->SetCamera(activeCamera_);


}

void Scene::ChangePhase(std::unique_ptr<Phase> nextPhase)
{
    // 現在のフェーズをクリア
    if (currentPhase_) {
        currentPhase_->Finalize(this);
        currentPhase_.reset();
    }

    // 新しいフェーズを設定して初期化
    currentPhase_ = std::move(nextPhase);
    if (currentPhase_) {
        currentPhase_->Initialize(this);
    }
}

void Scene::PrayBGM(Audio::SoundHandle bgmHundle)
{

    BGMHandle_ = Audio::GetInstance()->PlayAudio(bgmHundle, true, 0.25f, "BGM");

}
