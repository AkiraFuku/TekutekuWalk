#pragma once
#include "Scene.h"
#include "AbstractSceneFactory.h"
#include <memory>
class SceneManager
{
private:
    // シングルトン用静的インスタンス
    static std::unique_ptr<SceneManager> instance;

    // コンストラクタ・デストラクタをprivateにする
    SceneManager() = default;
    ~SceneManager();

    // コピー禁止
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

public:
    // インスタンス取得
    static SceneManager* GetInstance();
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