#include "GameEngine.h"
#include "PrimitiveDrawer.h"
#include "OffScreen.h"
#include "Fade.h"
#include "Object3dCommon.h"
void GameEngine::Initialize() {

    Framework::Initialize();



    sceneFactory_ = std::make_unique< SceneFactory>();


    SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());

    Fade::GetInstance()->Initialize();
    //   SceneManager::GetInstance()->ChangeScene("GameScene");
    SceneManager::GetInstance()->ChangeScene("TitleScene");
    PrimitiveDrawer::GetInstance()->Initialize();
    OffScreen::GetInstance()->SetEffectFlags(
        PostEffectFlag::DepthOutline
    );
};
void GameEngine::Finalize() {
    SceneManager::GetInstance()->Finalize();

    Framework::Finalize();
};
void GameEngine::Update() {
    Framework::Update();
    SceneManager::GetInstance()->Update();
    Fade::GetInstance()->Update();

}
void GameEngine::PreDraw()
{
    Framework::PreDraw();
}
;
void GameEngine::Draw() {
    Object3dCommon::GetInstance()->SetRenderPassIndex(0);
    SceneManager::GetInstance()->Draw();

    Framework::Draw();
}