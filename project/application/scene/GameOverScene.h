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
    Vector2 gameOverLogoBaseSize_ = { 500.0f, 272.0f };

    // 遷移先選択ボタンスプライト
    std::unique_ptr<Sprite> stageSelectButtonSprite_; // resources/GAMOVER/StageSerect.png
    std::unique_ptr<Sprite> titleButtonSprite_;       // resources/GAMOVER/GoToTitle.png
    Vector2 stageSelectBaseSize_ = { 330.0f, 106.0f };
    Vector2 titleBaseSize_ = { 300.0f, 126.0f };

    float blinkTimer_ = 0.0f;
    MenuItem selectedMenu_ = MenuItem::StageSelect;

    Audio::SoundHandle decisionSE_ = 0;
    bool isTransitioning_ = false;
};
