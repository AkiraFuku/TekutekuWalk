#pragma once
#include "Scene.h"
#include "Camera.h"
#include "SkyBox.h"
#include <memory>
#include <string>
#include <vector>

/// <summary>
/// ステージセレクトシーン
/// </summary>
class StageSelectScene : public Scene
{
public:
    StageSelectScene();
    ~StageSelectScene() override;

    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

private:
    struct StageInfo {
        std::string name;        // 表示名 (例: "Stage 1")
        std::string filePath;    // JSONファイルパス (例: "resources/Stagemap/stage1.json")
        std::string description; // ステージ説明
    };

    std::vector<StageInfo> stageList_;
    int selectedStageIndex_ = 0;

    std::unique_ptr<Camera> camera_;
    std::unique_ptr<SkyBox> skyBox_;

    bool isTransitioning_ = false;
    std::string nextSceneName_ = "";
};
