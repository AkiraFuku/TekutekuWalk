#pragma once
#include "Scene.h"
#include "Camera.h"
#include "SkyBox.h"
#include "Object3d.h"
#include "PbdCloth2D.h"
#include "Audio.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include <memory>
#include <vector>

class Sprite;

/// <summary>
/// ステージクリア演出・遷移先選択シーン
/// </summary>
class ClearScene : public Scene
{
public:
    enum class MenuItem {
        StageSelect = 0,
        Title,
        GoalDemo,
        Count
    };

    ClearScene();
    ~ClearScene() override;

    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

private:
    struct ConfettiPiece {
        std::unique_ptr<Sprite> sprite;
        Vector2 basePos = { 0.0f, 0.0f };
        Vector2 baseSize = { 24.0f, 16.0f };
        float fallSpeed = 120.0f;
        float swayAmp = 35.0f;
        float swayFreq = 2.5f;
        float swayPhase = 0.0f;
        float rotation = 0.0f;
        float rotSpeed = 1.8f;
        float flipPhase = 0.0f;
        float flipSpeed = 4.5f;
        Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    void InitializeConfetti();
    void UpdateConfetti(float dt);
    void InitializeIrisOpen();
    void UpdateIrisOpen(float dt);

    std::unique_ptr<Camera> camera_;
    std::unique_ptr<SkyBox> skyBox_;

    // 3D演出オブジェクト（暖色系背景パネル＋ステージ台＋右手で星を掲げて喜ぶプレイヤー）
    std::unique_ptr<Object3d> backdropObject_;
    std::unique_ptr<Object3d> stageFloorObject_;
    std::unique_ptr<Object3d> playerObject_;
    std::unique_ptr<Object3d> goalObject_;

    Vector4 backdropColor_ = { 1.00f, 0.92f, 0.78f, 1.0f }; // 温かみのあるクリームイエロー
    Vector4 floorColor_    = { 0.94f, 0.62f, 0.42f, 1.0f }; // サンセットオレンジ

    Vector3 playerBasePos_ = { 0.0f, -0.35f, 0.0f };
    Vector3 playerBaseScale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 rightHandRaiseOffset_ = { -0.08f, 0.78f, 0.05f }; // 右手パーツ突き上げ量
    Vector3 starOffsetFromHand_   = {  0.00f, 0.56f, 0.00f }; // 突き上げた右手から星までのオフセット
    bool useScreenRightHand_ = false; // false: キャラ自身の右手, true: 画面向かって右側の手
    float goalRotateSpeed_ = 4.5f;
    float goalYaw_ = 0.0f;
    Vector3 floorPos_ = { 0.0f, -1.35f, 0.5f };
    Vector3 floorScale_ = { 5.2f, 0.5f, 3.6f };

    // ステージクリア横断幕（PbdCloth2D：背景と3Dモデルの間に描画）
    std::unique_ptr<PbdCloth2D> clearClothBanner_;
    Vector2 bannerPos_ = { 640.0f, 75.0f };
    Vector2 bannerSize_ = { 760.0f, 380.0f };
    Vector2 bannerAnchor_ = { 0.5f, 0.0f };
    float bannerRotation_ = 0.0f;
    float bannerDepthZ_ = 99.0f; // 背景(NDC≈0.994)の手前、3Dモデル(NDC≈0.981)の奥
    bool bannerWindEnabled_ = true;
    float bannerWindStrength_ = 160.0f;
    int bannerPinMode_ = 0; // 0: 上辺両端2点吊り, 1: 上辺全体, 2: 四隅固定
    float bannerStiffness_ = 0.30f;
    Vector2 bannerGravity_ = { 0.0f, 480.0f };

    // 紙吹雪パーティクルスプライト群
    static constexpr int kConfettiCount_ = 72;
    std::vector<ConfettiPiece> confettiList_;

    // 遷移先選択ボタンスプライト
    std::unique_ptr<Sprite> stageSelectButtonSprite_; // resources/GAMOVER/StageSerect.png
    std::unique_ptr<Sprite> titleButtonSprite_;       // resources/GAMOVER/GoToTitle.png
    Vector2 stageSelectBaseSize_ = { 310.0f, 100.0f };
    Vector2 titleBaseSize_ = { 280.0f, 118.0f };

    // 放射状アイリスオープン演出
    static constexpr int kIrisBladeCount_ = 16;
    std::vector<std::unique_ptr<Sprite>> irisBlades_;
    std::unique_ptr<Sprite> irisCenterCircle_;
    float irisTimer_ = 0.0f;
    static constexpr float kIrisOpenDuration_ = 0.55f;
    bool isIrisOpenFinished_ = false;

    float timer_ = 0.0f;
    float autoReturnTimer_ = 0.0f;
    static inline bool s_autoReturnToGoalDemo_ = false;
    static inline float s_autoReturnDelay_ = 3.0f;
    MenuItem selectedMenu_ = MenuItem::StageSelect;
    bool prevStickLeft_ = false;
    bool prevStickRight_ = false;

    Audio::SoundHandle clearBGM_ = 0;
    Audio::VoiceHandle clearBGMVoice_ = 0;
    Audio::SoundHandle decisionSE_ = 0;
    bool isTransitioning_ = false;
};
