#pragma once
#include "Scene.h"
#include "Camera.h"
#include "SkyBox.h"
#include "Audio.h"
#include <memory>

class Sprite;

/// <summary>
/// ゲームオーバー画面シーン
/// </summary>
class GameOverScene : public Scene
{
public:
    enum class MenuItem {
        StageSelect = 0,
        Title,
        Count
    };

    GameOverScene();
    ~GameOverScene() override;

    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<SkyBox> skyBox_;
    std::unique_ptr<Sprite> gameOverLogo_;

    MenuItem selectedMenu_ = MenuItem::StageSelect;

    Audio::SoundHandle decisionSE_ = 0;
    bool isTransitioning_ = false;
};
