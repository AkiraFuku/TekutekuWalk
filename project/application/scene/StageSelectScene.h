#pragma once
#include "Scene.h"
#include "Camera.h"
#include "SkyBox.h"
#include "Object3d.h"
#include "Animation.h"
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
    enum class ItemType {
        Stage,
        BackToTitle
    };

    struct SelectItem {
        ItemType type;
        std::string name;        // 表示名 (例: "Stage 1", "タイトルに戻る")
        std::string filePath;    // JSONファイルパス (ステージの場合)
        std::string description; // 説明
        std::string textureName; // stageTitle内の画像ファイル名 (例: "Stage1.png")
    };

    // 選択項目リスト
    std::vector<SelectItem> itemList_;
    int selectedIndex_ = 0;

    // 3Dキューブオブジェクト
    std::vector<std::unique_ptr<Object3d>> cubeObjects_;

    // カルーセル回転用変数・パラメータ
    float currentAngle_ = 0.0f; // 現在の回転角度（ラジアン）
    float targetAngle_ = 0.0f;  // 目標回転角度（ラジアン）
    Vector3 ringCenter_ = { 0.0f, 2.2f, 1.0f }; // 画面上部リング中心
    float ringRadius_ = 2.6f;                   // リング半径
    float cubeSelectedScale_ = 0.42f;           // 選択中キューブのスケール
    float cubeNormalScale_ = 0.25f;             // 非選択キューブのスケール

    // 決定時演出用変数
    bool isDecided_ = false;
    float decisionBlinkTimer_ = 0.0f;

    // プレイヤー退場演出用変数
    enum class ExitDirection {
        None,
        Right, // ステージ決定時（右へ退場）
        Left   // タイトル戻り時（左へ振り返って退場）
    };
    ExitDirection exitDirection_ = ExitDirection::None;
    float exitTimer_ = 0.0f;
    bool isFadeStarted_ = false;
    float playerBaseRotateY_ = 1.5707963f; // 通常時の向き (右向き)

    // プレイヤーオブジェクト＆アニメーション (カメラから離し、右側面がカメラ側を向く設定)
    std::unique_ptr<Object3d> playerObject_;
    std::unique_ptr<Animation> playerAnimation_;
    Vector3 playerPos_ = { 0.0f, -0.2f, 2.5f };
    Vector3 playerRotate_ = { 0.0f, 1.5707963f, 0.0f }; // 右側がカメラに向く角度 (+90度)

    // 道（地面）オブジェクト (横方向に伸ばす設定)
    std::unique_ptr<Object3d> roadObject_;
    Vector3 roadScale_ = { 3.0f, 0.2f, 25.0f };
    Vector3 roadPos_ = { 0.0f, -0.3f, 2.5f };
    Vector3 roadRotate_ = { 0.0f, 1.5707963f, 0.0f };   // 横向きに配置 (90度)

    std::unique_ptr<Camera> camera_;
    std::unique_ptr<SkyBox> skyBox_;

    bool isTransitioning_ = false;
    std::string nextSceneName_ = "";
};
