#include "SceneFactory.h"
#include "GameScene.h"
#include "TitleScene.h"
#include "ResultScene.h"
#include "StageSelectScene.h"
#include "GameOverScene.h"
std::unique_ptr<Scene> SceneFactory::CreateScene(const std::string& sceneName)
{
    std::unique_ptr<Scene> scene = nullptr;

    if (sceneName == "TitleScene") {
         scene = std::make_unique<TitleScene>();
    }
    else if (sceneName == "StageSelectScene") {
         scene = std::make_unique<StageSelectScene>();
    }
    else if (sceneName == "GameScene") {
         scene = std::make_unique<GameScene>();
    }
    else if (sceneName == "ResultScene") {
        // scene = std::make_unique<ResultScene>();
    }
    else if (sceneName == "GameOverScene") {
         scene = std::make_unique<GameOverScene>();
    }

    return scene;
}