#pragma once
#include "Scene.h"
#include "AbstractSceneFactory.h"
#include <memory>
#include "Singleton.h"
class SceneManager : public Singleton<SceneManager>
{
protected:
    // コンストラクタ・デストラクタをprotectedにする
    SceneManager() = default;
    ~SceneManager() override;

public:
    friend class Singleton<SceneManager>;
    friend struct std::default_delete<SceneManager>;
    // 終了処理（インスタンスの破棄）
    void Finalize();


    void Update();
    void Draw();

    void SetSceneFactory(AbstractSceneFactory* sceneFactory) {
        sceneFactory_ = sceneFactory;
    }
    void ChangeScene(const std::string& sceneName);
    Scene* GetCurrentScene() const { return scene_.get(); }
private:
    std::unique_ptr<Scene> scene_ = nullptr;
    std::unique_ptr<Scene> nextScene_ = nullptr;

    AbstractSceneFactory* sceneFactory_ = nullptr;

};