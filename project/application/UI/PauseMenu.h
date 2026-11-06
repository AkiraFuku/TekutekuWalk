#pragma once
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

class GameScene;

class PauseMenu
{
public:
    enum class MenuAction {
        Resume = 0,
        Retry,
        Title,
        Count
    };

    void Initialize(GameScene* gameScene);
    void Update();
    void Draw();

    bool IsPaused() const { return isPaused_; }
    void SetPaused(bool paused);

private:
    GameScene* gameScene_ = nullptr;
    bool isPaused_ = false;

    int selectedIndex_ = 0;
    const std::vector<std::string> menuLabels_ = {
        "RESUME",
        "RETRY",
        "TITLE"
    };

    uint32_t selectSE_ = 0;
    uint32_t decideSE_ = 0;
};

