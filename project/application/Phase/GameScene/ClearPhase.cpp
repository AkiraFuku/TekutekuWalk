#include "ClearPhase.h"
#include "Sprite.h"
#include "WinApp.h"
#include "SceneManager.h"
#include "Input.h"
#include "Fade.h"
#include "TextureManager.h"
#include "GameScene.h"
#include "Player.h"
#include "GoalObject.h"
#include "Enemy.h"
#include "CameraController.h"
#include "Camera.h"
#include "ParticleManager.h"
#include "MathFunction.h"
#include <cmath>
#include <algorithm>
#include <numbers>
#include <random>

void ClearPhase::Initialize(Scene* scene)
{
    step_ = Step::GoalZoom;
    timer_ = 0.0f;
    irisProgress_ = 0.0f;
    hasEmittedHoistEffect_ = false;
    auraEmitTimer_ = 0.0f;
    goalYaw_ = 0.0f;

    InitializeClearEffects();
    InitializeConfetti();

    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;
    GoalObject* goal = gameScene ? gameScene->GetGoal() : nullptr;
    CameraController* camCtrl = gameScene ? gameScene->GetCamera() : nullptr;

    // ステージBGMを停止
    if (gameScene) {
        Audio::VoiceHandle bgmHandle = gameScene->getBGMPlayHundle();
        if (bgmHandle != 0 && Audio::GetInstance()->IsPlaying(bgmHandle)) {
            Audio::GetInstance()->StopAudio(bgmHandle);
        }
    }

    if (player && player->GetObject3d()) {
        // 被弾点滅を解除してクリア演出中確実に表示
        player->ResetHitInvincibility();

        playerStartPos_ = player->GetWorldPosition();
        playerTargetPos_ = playerStartPos_;
        if (goal) {
            goalStartPos_ = goal->GetWorldPosition();
            if (goal->GetObject3d()) {
                goalBaseScale_ = goal->GetObject3d()->GetScale();
                goalYaw_ = goal->GetObject3d()->GetRotate().y;
            }
            // ゴールオブジェクトの位置にX/Zを合わせ、高さは現在の接地高さを維持
            playerTargetPos_.x = goalStartPos_.x;
            playerTargetPos_.z = goalStartPos_.z;
        }

        playerBaseScale_ = player->GetObject3d()->GetScale();
        startPlayerYaw_ = player->GetObject3d()->GetRotate().y;

        // カメラの位置とズーム目標位置を計算
        Vector3 camPos = (camCtrl && camCtrl->GetCamera())
            ? camCtrl->GetCamera()->GetTranslate()
            : Vector3{ playerTargetPos_.x, playerTargetPos_.y + 2.0f, playerTargetPos_.z - 8.5f };

        startCamPos_ = camPos;

        // プレイヤーからカメラへの水平方向ベクトル
        Vector3 toCam = camPos - playerTargetPos_;
        toCam.y = 0.0f;
        float len = Length(toCam);
        if (len > 0.001f) {
            toCam = toCam / len;
        } else {
            toCam = { 0.0f, 0.0f, -1.0f };
        }

        // プレイヤーがカメラ正面を向く目標Yaw角
        targetPlayerYaw_ = std::atan2(toCam.x, toCam.z);

        // 最短回転になるよう角度差を正規化
        const float twoPi = 2.0f * std::numbers::pi_v<float>;
        float diffYaw = std::fmod(targetPlayerYaw_ - startPlayerYaw_, twoPi);
        if (diffYaw > std::numbers::pi_v<float>) diffYaw -= twoPi;
        else if (diffYaw < -std::numbers::pi_v<float>) diffYaw += twoPi;
        targetPlayerYaw_ = startPlayerYaw_ + diffYaw;

        // プレイヤーと頭上に掲げたゴールが両方しっかり収まるカメラ目標位置・注視点
        startLookAt_ = playerStartPos_ + Vector3{ 0.0f, 1.0f, 0.0f };
        targetLookAt_ = playerTargetPos_ + Vector3{ 0.0f, 0.65f, 0.0f };
        targetCamPos_ = playerTargetPos_ + toCam * 3.8f + Vector3{ 0.0f, 0.85f, 0.0f };

        // 汎用サブパーツシステムでプレイヤーの独立パーツ（右手・左手・右足・左足）を自動検出＆バインド
        player->GetObject3d()->AutoDetectSubParts();
        player->GetObject3d()->BindSubPartByNearestPos("RightHand", {  0.505f, -0.162f, -0.024f });
        player->GetObject3d()->BindSubPartByNearestPos("LeftHand",  { -0.505f, -0.162f, -0.024f });
        player->GetObject3d()->BindSubPartByNearestPos("RightFoot", {  0.175f, -0.351f,  0.0f   });
        player->GetObject3d()->BindSubPartByNearestPos("LeftFoot",  { -0.175f, -0.351f,  0.0f   });
        player->GetObject3d()->ResetSubParts();

        // ゴール接触時の初期エフェクト（祝福の光柱・キラキラ・星くず）
        ParticleManager::GetInstance()->Emit("ClearEffects", "LightRay", playerTargetPos_, 8);
        ParticleManager::GetInstance()->Emit("ClearEffects", "Sparkle", playerTargetPos_, 16);
        ParticleManager::GetInstance()->Emit("ClearEffects", "StarDust", playerTargetPos_, 12);
    }

    // 放射状トランジションスプライトの初期化
    InitializeIrisSprites();

    // ゴール到達SE・クリアBGMの再生
    goalSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
    if (goalSE_ != 0) {
        Audio::GetInstance()->PlayAudio(goalSE_, false, 0.7f, "SE");
    }

    clearBGM_ = Audio::GetInstance()->LoadAudio("resources/Audio/BGM/Stageclear.mp3");
    if (clearBGM_ != 0) {
        clearBGMVoice_ = Audio::GetInstance()->PlayAudio(clearBGM_, false, 0.35f, "BGM");
    }
}

void ClearPhase::InitializeClearEffects()
{
    auto* pm = ParticleManager::GetInstance();
    pm->ReleaseParticleGroupSet("ClearEffects");

    TextureManager::GetInstance()->LoadTexture("resources/circle2.png");
    TextureManager::GetInstance()->LoadTexture("resources/gradationLine.png");
    TextureManager::GetInstance()->LoadTexture("resources/white.png");

    // 1. ふんわり舞い上がるキラキラ光粒（爆発ではなく上方向へ優しく昇り明滅）
    ParticleManager::ParticleEmitterFunc sparkleInit = [](const Vector3& emitterPos, std::mt19937& rng) -> ParticleManager::Particle {
        std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> distRadius(0.15f, 0.95f);
        std::uniform_real_distribution<float> distUp(1.2f, 2.8f);
        std::uniform_real_distribution<float> distScale(0.10f, 0.24f);
        std::uniform_real_distribution<float> distLife(0.75f, 1.35f);
        std::uniform_int_distribution<int> distColor(0, 3);

        float angle = distAngle(rng);
        float radius = distRadius(rng);

        ParticleManager::Particle p{};
        float s = distScale(rng);
        p.transform.scale = { s, s, s };
        p.transform.rotate = { 0.0f, 0.0f, 0.0f };
        p.transform.translate = emitterPos + Vector3{
            std::cos(angle) * radius * 0.4f,
            -0.2f,
            std::sin(angle) * radius * 0.4f
        };
        p.velocity = {
            std::cos(angle) * radius * 0.8f,
            distUp(rng),
            std::sin(angle) * radius * 0.8f
        };

        const Vector4 palette[] = {
            { 1.00f, 0.90f, 0.40f, 1.0f }, // ゴールデンイエロー
            { 1.00f, 0.72f, 0.35f, 1.0f }, // サンセットオレンジ
            { 1.00f, 0.98f, 0.85f, 1.0f }, // クリームホワイト
            { 0.98f, 0.65f, 0.78f, 1.0f }, // ブロッサムピンク
        };
        p.color = palette[distColor(rng)];
        p.lifeTime = distLife(rng);
        p.currentTime = 0.0f;
        return p;
    };

    ParticleManager::ParticleUpdateFunc sparkleUpdate = [](ParticleManager::Particle& p, float dt) {
        p.velocity.x *= (1.0f - 1.2f * dt);
        p.velocity.z *= (1.0f - 1.2f * dt);
        p.transform.translate += p.velocity * dt;

        float t = std::clamp(p.currentTime / p.lifeTime, 0.0f, 1.0f);
        // ふんわり明滅しながら消える
        float twinkle = 0.65f + 0.35f * std::sin(p.currentTime * 18.0f);
        p.color.w = (1.0f - (t * t)) * twinkle;
    };

    pm->CreateParticleGroup("ClearEffects", "Sparkle", "resources/circle2.png", ParticleManager::EffectType::Plane, sparkleInit, sparkleUpdate);

    // 2. 上昇キラキラオーラ（喜びバウンス中の継続演出）
    ParticleManager::ParticleEmitterFunc auraInit = [](const Vector3& emitterPos, std::mt19937& rng) -> ParticleManager::Particle {
        std::uniform_real_distribution<float> distOffset(-0.65f, 0.65f);
        std::uniform_real_distribution<float> distUp(1.2f, 2.8f);
        std::uniform_real_distribution<float> distScale(0.09f, 0.20f);
        std::uniform_real_distribution<float> distLife(0.6f, 1.1f);
        std::uniform_int_distribution<int> distColor(0, 2);

        ParticleManager::Particle p{};
        float s = distScale(rng);
        p.transform.scale = { s, s, s };
        p.transform.rotate = { 0.0f, 0.0f, 0.0f };
        p.transform.translate = emitterPos + Vector3{ distOffset(rng), distOffset(rng) * 0.4f - 0.3f, distOffset(rng) };
        p.velocity = { distOffset(rng) * 0.4f, distUp(rng), distOffset(rng) * 0.4f };

        const Vector4 palette[] = {
            { 1.00f, 0.92f, 0.45f, 1.0f },
            { 1.00f, 0.72f, 0.35f, 1.0f },
            { 1.00f, 1.00f, 0.88f, 1.0f },
        };
        p.color = palette[distColor(rng)];
        p.lifeTime = distLife(rng);
        p.currentTime = 0.0f;
        return p;
    };

    ParticleManager::ParticleUpdateFunc auraUpdate = [](ParticleManager::Particle& p, float dt) {
        p.transform.translate += p.velocity * dt;
        float t = std::clamp(p.currentTime / p.lifeTime, 0.0f, 1.0f);
        p.color.w = 1.0f - (t * t);
    };

    pm->CreateParticleGroup("ClearEffects", "Aura", "resources/circle2.png", ParticleManager::EffectType::Plane, auraInit, auraUpdate);

    // 3. 祝福の光柱・シャワー（縦長の光の帯が上空へ立ち昇る）
    ParticleManager::ParticleEmitterFunc rayInit = [](const Vector3& emitterPos, std::mt19937& rng) -> ParticleManager::Particle {
        std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> distRadius(0.2f, 1.1f);
        std::uniform_real_distribution<float> distWidth(0.08f, 0.18f);
        std::uniform_real_distribution<float> distHeight(1.2f, 2.4f);
        std::uniform_real_distribution<float> distUp(2.2f, 3.8f);
        std::uniform_real_distribution<float> distLife(0.65f, 1.1f);
        std::uniform_int_distribution<int> distColor(0, 1);

        float angle = distAngle(rng);
        float radius = distRadius(rng);

        ParticleManager::Particle p{};
        p.transform.scale = { distWidth(rng), distHeight(rng), 1.0f };
        p.transform.rotate = { 0.0f, 0.0f, 0.0f };
        p.transform.translate = emitterPos + Vector3{
            std::cos(angle) * radius,
            -0.4f,
            std::sin(angle) * radius
        };
        p.velocity = { 0.0f, distUp(rng), 0.0f };
        p.color = (distColor(rng) == 0)
            ? Vector4{ 1.00f, 0.90f, 0.45f, 0.85f }
            : Vector4{ 1.00f, 0.98f, 0.88f, 0.85f };
        p.lifeTime = distLife(rng);
        p.currentTime = 0.0f;
        return p;
    };

    ParticleManager::ParticleUpdateFunc rayUpdate = [](ParticleManager::Particle& p, float dt) {
        p.transform.translate += p.velocity * dt;
        float t = std::clamp(p.currentTime / p.lifeTime, 0.0f, 1.0f);
        // 途中で最大透明度になり、後半すっと消える
        float alphaCurve = std::sin(t * std::numbers::pi_v<float>);
        p.color.w = 0.85f * alphaCurve;
    };

    pm->CreateParticleGroup("ClearEffects", "LightRay", "resources/gradationLine.png", ParticleManager::EffectType::Plane, rayInit, rayUpdate);

    // 4. 3D星くず・紙吹雪（小さな煌めきが回転しながら舞う）
    ParticleManager::ParticleEmitterFunc dustInit = [](const Vector3& emitterPos, std::mt19937& rng) -> ParticleManager::Particle {
        std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> distRadius(0.3f, 1.3f);
        std::uniform_real_distribution<float> distUp(1.0f, 2.4f);
        std::uniform_real_distribution<float> distScale(0.06f, 0.13f);
        std::uniform_real_distribution<float> distRot(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution<float> distLife(0.9f, 1.5f);
        std::uniform_int_distribution<int> distColor(0, 3);

        float angle = distAngle(rng);
        float radius = distRadius(rng);
        float s = distScale(rng);

        ParticleManager::Particle p{};
        p.transform.scale = { s * 1.4f, s, s };
        p.transform.rotate = { 0.0f, 0.0f, distRot(rng) };
        p.transform.translate = emitterPos + Vector3{
            std::cos(angle) * radius * 0.5f,
            0.0f,
            std::sin(angle) * radius * 0.5f
        };
        p.velocity = {
            std::cos(angle) * 0.9f,
            distUp(rng),
            std::sin(angle) * 0.9f
        };

        const Vector4 palette[] = {
            { 1.00f, 0.88f, 0.35f, 1.0f },
            { 1.00f, 0.62f, 0.35f, 1.0f },
            { 0.98f, 0.68f, 0.82f, 1.0f },
            { 1.00f, 0.98f, 0.90f, 1.0f },
        };
        p.color = palette[distColor(rng)];
        p.lifeTime = distLife(rng);
        p.currentTime = 0.0f;
        return p;
    };

    ParticleManager::ParticleUpdateFunc dustUpdate = [](ParticleManager::Particle& p, float dt) {
        p.velocity.y -= 1.8f * dt; // ゆっくり重力で落ち始める
        p.transform.translate += p.velocity * dt;
        p.transform.rotate.z += 4.5f * dt;
        float t = std::clamp(p.currentTime / p.lifeTime, 0.0f, 1.0f);
        p.color.w = 1.0f - (t * t);
    };

    pm->CreateParticleGroup("ClearEffects", "StarDust", "resources/white.png", ParticleManager::EffectType::Plane, dustInit, dustUpdate);
}

void ClearPhase::InitializeConfetti()
{
    TextureManager::GetInstance()->LoadTexture("resources/white.png");
    confettiList_.clear();
    confettiList_.reserve(kConfettiCount_);

    std::mt19937 rng(108);
    std::uniform_real_distribution<float> distX(30.0f, static_cast<float>(WinApp::kClientWidth) - 30.0f);
    std::uniform_real_distribution<float> distY(-260.0f, static_cast<float>(WinApp::kClientHeight) * 0.25f);
    std::uniform_real_distribution<float> distW(14.0f, 28.0f);
    std::uniform_real_distribution<float> distH(10.0f, 20.0f);
    std::uniform_real_distribution<float> distFall(140.0f, 260.0f);
    std::uniform_real_distribution<float> distSwayAmp(18.0f, 48.0f);
    std::uniform_real_distribution<float> distSwayFreq(2.0f, 4.2f);
    std::uniform_real_distribution<float> distPhase(0.0f, 2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> distRotSpeed(-3.2f, 3.2f);
    std::uniform_real_distribution<float> distFlipSpeed(3.5f, 8.0f);

    const Vector4 palette[] = {
        { 0.98f, 0.42f, 0.42f, 0.92f },
        { 1.00f, 0.64f, 0.32f, 0.92f },
        { 1.00f, 0.88f, 0.38f, 0.92f },
        { 0.98f, 0.68f, 0.80f, 0.92f },
        { 1.00f, 0.78f, 0.52f, 0.92f },
        { 1.00f, 0.98f, 0.92f, 0.92f },
    };
    const int paletteSize = static_cast<int>(std::size(palette));

    for (int i = 0; i < kConfettiCount_; ++i) {
        ConfettiPiece piece;
        piece.sprite = std::make_unique<Sprite>();
        piece.sprite->Initialize("resources/white.png");
        piece.sprite->SetAnchorPoint({ 0.5f, 0.5f });

        piece.basePos = { distX(rng), distY(rng) };
        piece.baseSize = { distW(rng), distH(rng) };
        piece.fallSpeed = distFall(rng);
        piece.swayAmp = distSwayAmp(rng);
        piece.swayFreq = distSwayFreq(rng);
        piece.swayPhase = distPhase(rng);
        piece.rotation = distPhase(rng);
        piece.rotSpeed = distRotSpeed(rng);
        piece.flipPhase = distPhase(rng);
        piece.flipSpeed = distFlipSpeed(rng);
        piece.color = palette[i % paletteSize];

        piece.sprite->SetColor(piece.color);
        piece.sprite->SetPosition(piece.basePos);
        piece.sprite->SetSize(piece.baseSize);
        piece.sprite->SetRotation(piece.rotation);
        piece.sprite->Update();

        confettiList_.push_back(std::move(piece));
    }
}

void ClearPhase::UpdateConfetti(float dt)
{
    const float screenH = static_cast<float>(WinApp::kClientHeight);
    const float screenW = static_cast<float>(WinApp::kClientWidth);

    for (auto& piece : confettiList_) {
        piece.basePos.y += piece.fallSpeed * dt;
        piece.swayPhase += piece.swayFreq * dt;
        piece.rotation += piece.rotSpeed * dt;
        piece.flipPhase += piece.flipSpeed * dt;

        if (piece.basePos.y > screenH + 40.0f) {
            piece.basePos.y = -40.0f;
            piece.basePos.x = std::fmod(piece.basePos.x + 173.0f, screenW - 60.0f) + 30.0f;
        }

        float currentX = piece.basePos.x + std::sin(piece.swayPhase) * piece.swayAmp;
        float flipX = 0.25f + 0.75f * std::abs(std::cos(piece.flipPhase));
        float flipY = 0.55f + 0.45f * std::abs(std::sin(piece.flipPhase * 0.7f));

        piece.sprite->SetPosition({ currentX, piece.basePos.y });
        piece.sprite->SetSize({ piece.baseSize.x * flipX, piece.baseSize.y * flipY });
        piece.sprite->SetRotation(piece.rotation);
        piece.sprite->Update();
    }
}

void ClearPhase::InitializeIrisSprites()
{
    TextureManager::GetInstance()->LoadTexture("resources/white.png");
    TextureManager::GetInstance()->LoadTexture("resources/circle2.png");

    irisBlades_.clear();
    irisBlades_.reserve(kIrisBladeCount_);

    const Vector2 center = {
        static_cast<float>(WinApp::kClientWidth) * 0.5f,
        static_cast<float>(WinApp::kClientHeight) * 0.5f
    };
    const float twoPi = 2.0f * std::numbers::pi_v<float>;

    for (int i = 0; i < kIrisBladeCount_; ++i) {
        auto blade = std::make_unique<Sprite>();
        blade->Initialize("resources/white.png");
        blade->SetAnchorPoint({ 0.5f, 1.0f }); // 中心から外側へ伸びる扇ブレード
        blade->SetPosition(center);
        float angle = (twoPi / static_cast<float>(kIrisBladeCount_)) * static_cast<float>(i);
        blade->SetRotation(angle);

        // クリア画面の暖色テーマ（サンセットオレンジ＆ゴールデンクリーム）に合わせた交互配色
        Vector4 bladeColor = (i % 2 == 0)
            ? Vector4{ 0.98f, 0.68f, 0.42f, 1.0f }
            : Vector4{ 1.00f, 0.85f, 0.58f, 1.0f };
        blade->SetColor(bladeColor);
        blade->SetSize({ 0.0f, 0.0f });
        blade->Update();
        irisBlades_.push_back(std::move(blade));
    }

    irisCenterCircle_ = std::make_unique<Sprite>();
    irisCenterCircle_->Initialize("resources/circle2.png");
    irisCenterCircle_->SetAnchorPoint({ 0.5f, 0.5f });
    irisCenterCircle_->SetPosition(center);
    irisCenterCircle_->SetColor({ 0.96f, 0.58f, 0.35f, 1.0f });
    irisCenterCircle_->SetSize({ 0.0f, 0.0f });
    irisCenterCircle_->Update();
}

void ClearPhase::UpdateIrisSprites(float progress)
{
    float p = std::clamp(progress, 0.0f, 1.0f);
    // EaseInOutCubic で気持ちよく閉じる
    float ease = (p < 0.5f)
        ? (4.0f * p * p * p)
        : (1.0f - std::pow(-2.0f * p + 2.0f, 3.0f) * 0.5f);

    const float maxRadius = 920.0f; // 画面対角線を完全に覆う半径
    const float currentLen = maxRadius * ease;
    const float maxBladeWidth = 260.0f;
    const float currentWidth = maxBladeWidth * ease;

    const Vector2 center = {
        static_cast<float>(WinApp::kClientWidth) * 0.5f,
        static_cast<float>(WinApp::kClientHeight) * 0.5f
    };
    const float twoPi = 2.0f * std::numbers::pi_v<float>;

    for (int i = 0; i < static_cast<int>(irisBlades_.size()); ++i) {
        float angle = (twoPi / static_cast<float>(kIrisBladeCount_)) * static_cast<float>(i);
        // 外周側から中心に向かって迫り出してくる動きをミックス
        float outerDist = (1.0f - ease) * 760.0f;
        Vector2 bladePos = {
            center.x + std::sin(angle) * outerDist,
            center.y - std::cos(angle) * outerDist
        };
        irisBlades_[i]->SetPosition(bladePos);
        irisBlades_[i]->SetRotation(angle);
        irisBlades_[i]->SetSize({ currentWidth, currentLen });
        irisBlades_[i]->Update();
    }

    if (irisCenterCircle_) {
        // 中央の円が最後にキュッと現れる（先輩の動画 00:09 の中心サークル表現）
        float circleScale = std::clamp((ease - 0.35f) / 0.65f, 0.0f, 1.0f);
        float circleSize = 180.0f * circleScale;
        irisCenterCircle_->SetPosition(center);
        irisCenterCircle_->SetSize({ circleSize, circleSize });
        irisCenterCircle_->Update();
    }
}

void ClearPhase::UpdateCameraAndPlayer(Scene* scene, float dt)
{
    GameScene* gameScene = static_cast<GameScene*>(scene);
    if (!gameScene) return;

    Player* player = gameScene->GetPlayer();
    GoalObject* goal = gameScene->GetGoal();
    CameraController* camCtrl = gameScene->GetCamera();

    float t = std::clamp(timer_ / kZoomDuration_, 0.0f, 1.0f);
    // EaseOutCubic で滑らかにカメラが寄る
    float easeOut = 1.0f - std::pow(1.0f - t, 3.0f);
    // プレイヤーの振り向きは最初の0.28秒で完了させる
    float turnT = std::clamp(timer_ / 0.28f, 0.0f, 1.0f);
    float easeTurn = 1.0f - std::pow(1.0f - turnT, 3.0f);

    Vector3 currentPlayerPos = Lerp(playerStartPos_, playerTargetPos_, easeTurn);
    float squashX = 1.0f;
    float stretchY = 1.0f;
    float poseRoll = 0.0f;

    // ── フェーズ1 (0.00s〜0.28s): 予備動作のしゃがみ込み（ゴールを掴むタメ） ──
    if (timer_ < 0.28f) {
        float crouchPhase = std::sin((timer_ / 0.28f) * std::numbers::pi_v<float>);
        stretchY = 1.0f - 0.25f * crouchPhase;
        squashX = 1.0f + 0.14f * crouchPhase;
        currentPlayerPos.y -= 0.12f * crouchPhase;
    }
    // ── フェーズ2 (0.28s〜0.65s): ゴールを頭上へ勢いよく持ち上げて伸び上がる ──
    else if (timer_ < 0.65f) {
        float liftP = (timer_ - 0.28f) / (0.65f - 0.28f);
        float stretchPulse = std::sin(liftP * std::numbers::pi_v<float>);
        stretchY = 1.0f + 0.22f * stretchPulse;
        squashX = 1.0f - 0.10f * stretchPulse;
        currentPlayerPos.y += 0.20f * stretchPulse;
    }
    // ── フェーズ3 (0.65s〜): 頭上にゴールを掲げたまま喜びバウンス ──
    else {
        float celebTime = timer_ - 0.65f;
        float hopWave = std::abs(std::sin(celebTime * 7.5f));
        currentPlayerPos.y += hopWave * 0.30f;

        float squashWave = std::sin(celebTime * 15.0f);
        stretchY = 1.0f + 0.12f * squashWave;
        squashX = 1.0f - 0.07f * squashWave;
        poseRoll = 0.07f * std::sin(celebTime * 7.5f);
    }

    float currentYaw = Lerp(startPlayerYaw_, targetPlayerYaw_, easeTurn);

    // ── 右手パーツ("RightHand")で星を頭上に突き上げるサブパーツアニメーション ──
    Vector3 rightHandOffset = { 0.0f, 0.0f, 0.0f };
    Vector3 leftHandOffset  = { 0.0f, 0.0f, 0.0f };

    if (timer_ < 0.28f) {
        // 予備動作：右手を少し下げて星を掴む構え
        float p = timer_ / 0.28f;
        rightHandOffset = { -0.10f * p, -0.15f * p, 0.15f * p };
        leftHandOffset  = { -0.05f * p, -0.08f * p, 0.0f };
    }
    else if (timer_ < 0.65f) {
        // 右手を頭上高く突き上げる（EaseOutBack）
        float p = std::clamp((timer_ - 0.28f) / (0.65f - 0.28f), 0.0f, 1.0f);
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        float easeBack = 1.0f + c3 * std::pow(p - 1.0f, 3.0f) + c1 * std::pow(p - 1.0f, 2.0f);

        rightHandOffset = Lerp(
            Vector3{ -0.10f, -0.15f, 0.15f },
            Vector3{ -0.08f,  0.78f, 0.05f },
            easeBack
        );
        leftHandOffset = { -0.08f, -0.05f * (1.0f - p), 0.0f };
    }
    else {
        // 右手を高く掲げたまま喜びバウンス
        float celebTime = timer_ - 0.65f;
        float handBob = std::sin(celebTime * 15.0f) * 0.06f;
        rightHandOffset = { -0.08f, 0.78f + handBob, 0.05f };
        leftHandOffset  = { -0.06f, -0.06f * std::sin(celebTime * 7.5f), 0.0f };
    }

    if (player && player->GetObject3d()) {
        player->GetObject3d()->SetSubPartTranslate("RightHand", rightHandOffset);
        player->GetObject3d()->SetSubPartTranslate("LeftHand",  leftHandOffset);
        player->GetObject3d()->SetTranslate(currentPlayerPos);
        player->GetObject3d()->SetRotate({ 0.0f, currentYaw, poseRoll });
        player->GetObject3d()->SetScale({
            playerBaseScale_.x * squashX,
            playerBaseScale_.y * stretchY,
            playerBaseScale_.z * squashX
        });
        player->GetObject3d()->Update();
    }

    // ── ゴールオブジェクト（星）を突き上げた右手のすぐ上に追従させて掲げる ──
    if (goal && goal->GetObject3d()) {
        Vector3 rightHandWorldPos = (player && player->GetObject3d())
            ? player->GetObject3d()->GetSubPartWorldPosition("RightHand")
            : currentPlayerPos;

        Vector3 hoistTargetPos = rightHandWorldPos + Vector3{ 0.0f, 0.58f, 0.0f };

        Vector3 goalPos = goalStartPos_;
        Vector3 goalScale = goalBaseScale_;
        float goalTiltZ = 0.0f;

        if (timer_ < 0.28f) {
            float p = timer_ / 0.28f;
            goalPos = Lerp(goalStartPos_, hoistTargetPos, p);
            goalYaw_ += 3.0f * dt;
        }
        else if (timer_ < 0.65f) {
            float p = std::clamp((timer_ - 0.28f) / (0.65f - 0.28f), 0.0f, 1.0f);
            goalPos = hoistTargetPos;
            float popScale = 1.0f + 0.20f * std::sin(p * std::numbers::pi_v<float>);
            goalScale = goalBaseScale_ * popScale;
            goalYaw_ += 10.0f * dt;
        }
        else {
            if (!hasEmittedHoistEffect_) {
                hasEmittedHoistEffect_ = true;
                ParticleManager::GetInstance()->Emit("ClearEffects", "LightRay", hoistTargetPos, 10);
                ParticleManager::GetInstance()->Emit("ClearEffects", "Sparkle", hoistTargetPos, 24);
                ParticleManager::GetInstance()->Emit("ClearEffects", "StarDust", hoistTargetPos, 18);
            }
            auraEmitTimer_ += dt;
            if (auraEmitTimer_ >= 0.08f) {
                auraEmitTimer_ = 0.0f;
                ParticleManager::GetInstance()->Emit("ClearEffects", "Aura", hoistTargetPos, 3);
            }
            goalPos = hoistTargetPos;
            goalYaw_ += 6.5f * dt;
            goalTiltZ = poseRoll;
        }

        goal->GetObject3d()->SetTranslate(goalPos);
        goal->GetObject3d()->SetRotate({ 0.0f, goalYaw_, goalTiltZ });
        goal->GetObject3d()->SetScale(goalScale);
        goal->GetObject3d()->Update();
    }

    if (camCtrl && camCtrl->GetCamera()) {
        Camera* camera = camCtrl->GetCamera();
        Vector3 camPos = Lerp(startCamPos_, targetCamPos_, easeOut);
        Vector3 lookAt = Lerp(startLookAt_, targetLookAt_, easeOut);

        Vector3 dir = lookAt - camPos;
        float len = Length(dir);
        if (len > 0.001f) {
            dir = dir / len;
            float angleY = std::atan2(dir.x, dir.z);
            float distXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);
            float angleX = std::atan2(-dir.y, distXZ);
            camera->SetTranslate(camPos);
            camera->SetRotate({ angleX, angleY, 0.0f });
            camera->Update();
        }
    }

    // カメラ移動に伴うステージ内エネミーのWVP行列同期
    for (const auto& enemy : gameScene->GetEnemies()) {
        if (enemy) {
            enemy->UpdateMatrix();
        }
    }
}

void ClearPhase::Update(Scene* scene)
{
    const float dt = 1.0f / 60.0f;
    timer_ += dt;
    UpdateConfetti(dt);

    Input* input = Input::GetInstance();
    bool isSkip = input && (
        input->TriggerKeyDown(DIK_SPACE) ||
        input->TriggerKeyDown(DIK_RETURN) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_A));

    if (step_ == Step::GoalZoom) {
        UpdateCameraAndPlayer(scene, dt);

        if (timer_ >= kZoomDuration_ || isSkip) {
            step_ = Step::IrisClose;
            timer_ = 0.0f;
        }
    }
    else if (step_ == Step::IrisClose) {
        irisProgress_ = std::clamp(timer_ / kIrisDuration_, 0.0f, 1.0f);
        UpdateIrisSprites(irisProgress_);

        if (irisProgress_ >= 1.0f) {
            step_ = Step::ChangeScene;
            SceneManager::GetInstance()->ChangeScene("ClearScene");
            return;
        }
    }
}

void ClearPhase::Draw(Scene* scene)
{
    (void)scene;
    for (auto& piece : confettiList_) {
        if (piece.sprite) {
            piece.sprite->Draw();
        }
    }
    if (step_ >= Step::IrisClose && irisProgress_ > 0.001f) {
        for (auto& blade : irisBlades_) {
            if (blade) {
                blade->Draw();
            }
        }
        if (irisCenterCircle_) {
            irisCenterCircle_->Draw();
        }
    }
}

void ClearPhase::Finalize(Scene* scene)
{
    if (clearBGMVoice_ != 0 && Audio::GetInstance()->IsPlaying(clearBGMVoice_)) {
        Audio::GetInstance()->StopAudio(clearBGMVoice_);
    }
    clearBGMVoice_ = 0;

    ParticleManager::GetInstance()->ReleaseParticleGroupSet("ClearEffects");

    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;
    if (player && player->GetObject3d()) {
        player->GetObject3d()->ResetSubParts();
        player->GetObject3d()->SetScale(playerBaseScale_);
    }
    confettiList_.clear();
    irisBlades_.clear();
    irisCenterCircle_.reset();
}

