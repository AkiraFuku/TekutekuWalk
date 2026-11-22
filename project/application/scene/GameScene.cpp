#include "GameScene.h"
#include "application/level/StageManager.h"
#include "ModelManager.h"
#include "Input.h"
#include "imgui.h"
#include "PrimitiveDrawer.h"
#include "SceneManager.h"
#include "ParticleManager.h"
#include "GpuParticleManager.h"//フレームワークに移植
#include "ParticleEmitter.h"
#include "PSOManager.h"
#include "LightManager.h"
#include "RailPath.h"
#include "Object3d.h"
#include "Animation.h"
#include"Audio.h"
#include "TextureManager.h"
#include "RailPath.h"
#include "Enemy.h"
#include "EnemyFactory.h"
#include "CollisionManager.h"
#include "Physics.h"
#include "Projectile.h"
#include "PlayerState.h"
#include <numbers>
#include <Model.h>
#include "GoalObject.h"
#include "Phase.h"
#include "PlayPhase.h"
#include "ClearPhase.h"
#include "defeatPhase.h"
#include "PlayerHPUI.h"
#include "ScoreUI.h"
#include "StartPhase.h"
#include "Fade.h"
#include "PauseMenu.h"
#include "DamagePopupManager.h"

void GameScene::Initialize() {

    // 1. メインカメラの生成
    auto mainCamera = std::make_unique<Camera>();
    mainCamera->SetTranslate({ 0.0f, 0.0f, -5.0f });

    cameraMap_["Main"] = std::move(mainCamera);

#ifdef USE_IMGUI
    //// 2. デバッグ・エディタ用カメラの生成
    auto debugCamera = std::make_unique<Camera>();
    debugCamera->SetTranslate({ 0.0f, 10.0f, -20.0f });
    debugCamera->SetEditorMode(true);
    cameraMap_["Debug"] = std::move(debugCamera);
#endif

    // 3. 最初はメインカメラをセット
    ChangeActiveCamera(cameraMap_["Main"].get());


    handle_ = Audio::GetInstance()->LoadAudio("resources/Audio/BGM/bgm_Loop.mp3");

    BGMHandle_=Audio::GetInstance()->PlayAudio(handle_,true,0.25f ,"BGM");
    TextureManager::GetInstance()->LoadTexture("resources/grass.png");
    TextureManager::GetInstance()->LoadTexture("resources/gradationLine.png");
    TextureManager::GetInstance()->LoadTexture("resources/Efect.png");

    ParticleManager::ParticleEmitterFunc initializeFunc = [](const Vector3& emitterPosition, std::mt19937& randomEngine)-> ParticleManager::Particle {

        std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
        std::uniform_real_distribution<float> distTime(1.0f, 10.0f);
        ParticleManager::Particle particle;
        particle.transform.scale = { 1.0f,1.0f,1.0f };
        particle.transform.rotate = { 0.0f,0.0f,0.0f };
        Vector3 randomTranslate = { distribution(randomEngine),distribution(randomEngine) ,distribution(randomEngine) };
        particle.transform.translate = emitterPosition + randomTranslate;
        particle.velocity = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) };

        particle.color = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine),1.0f };

        particle.lifeTime = distTime(randomEngine);
        particle.currentTime = 0.0f;
        return particle;
        };
    ParticleManager::ParticleUpdateFunc updateFunc = [](ParticleManager::Particle& particle, float deltaTime) {
        // パーティクルの更新処理
        // 例: 速度に基づいて位置を更新し、寿命を減少させる
        particle.uvTransform.offset.x += deltaTime;
        particle.transform.translate += particle.velocity * deltaTime;
        };
    ParticleManager::ParticleEmitterFunc initialize = [](const Vector3& emitterPosition, std::mt19937& randomEngine)-> ParticleManager::Particle {

        std::uniform_real_distribution<float> rotation(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
        ParticleManager::Particle particle;
        particle.transform.scale = { 0.05f,1.0f,1.0f };
        particle.transform.rotate = { rotation(randomEngine),rotation(randomEngine),rotation(randomEngine) };
        particle.transform.translate = emitterPosition;
        particle.velocity = { 0.0f, 0.0f, 0.0f };

        particle.color = { 1.0f,1.0f,1.0f,1.5f };

        particle.lifeTime = 1.0f;
        particle.currentTime = 0.0f;
        return particle;
        };
    ParticleManager::ParticleUpdateFunc update = [](ParticleManager::Particle& particle, float deltaTime) {

        float progress = particle.currentTime / particle.lifeTime;
        if (progress > 1.0f) progress = 1.0f;

        // 5. イージング（Ease Out）を使って、最初はものすごい勢いで広がり、後半に少し減速させる
        // これにより「ドンッ」と弾けるような勢いが表現できます
        // (1 - (1 - t)^3) は Cubic Ease Out の式です
        float easeOut = 1.0f - std::pow(1.0f - progress, 3.0f);

        // 目標とする最大サイズ
        float maxScale = 2.5f;
        float currentScale = maxScale * easeOut;

        particle.transform.scale = { currentScale, currentScale, currentScale };

        particle.uvTransform.offset.y -= deltaTime * 0.08f; // UVを縦にスクロールさせる
        particle.uvTransform.offset.x -= deltaTime * 0.08f; // UVを縦にスクロールさせる


        // 6. 消え方もイージング（Ease In）をかけるか、後半に一気に消すとキレが出ます
        // 最初はくっきり、後半急激に消えるようにする例（(1 - progress)^2）
        particle.color.w = 1.0f - (progress * progress);


        };

    ParticleManager::GetInstance()->CreateParticleGroup("GameEffects", "Hit", "resources/Efect.png", ParticleManager::EffectType::Ring, initialize, update);
    ParticleManager::GetInstance()->SetCamera(activeCamera_);
    GpuParticleManager::GetInstance()->SetCamera(activeCamera_);
    {
        auto* gpuParticle = GpuParticleManager::GetInstance();
        gpuParticle->ResetParticles();

        // 1. 星のカービィ64風 敵撃破エフェクト ("EnemyDefeat")
        {
            // 中心の強い白い閃光（パッと弾けるコア）
            GpuParticleManager::EmitterConfig flash{};
            flash.count = 1;
            flash.lifeTimeMin = 0.14f;
            flash.lifeTimeMax = 0.18f;
            flash.startScaleMin = { 1.70f, 1.70f, 1.70f };
            flash.startScaleMax = { 2.00f, 2.00f, 2.00f };
            flash.endScaleMin = { 0.08f, 0.08f, 0.08f };
            flash.endScaleMax = { 0.14f, 0.14f, 0.14f };
            flash.startColorMin = { 1.8f, 1.75f, 1.35f, 1.0f };
            flash.startColorMax = { 2.0f, 1.95f, 1.60f, 1.0f };
            flash.endColorMin = { 1.0f, 0.85f, 0.35f, 0.0f };
            flash.endColorMax = { 1.0f, 0.90f, 0.45f, 0.0f };
            gpuParticle->RegisterEffectGroup("EnemyDefeat", "EnemyDefeat_Flash", "resources/circle2.png", flash, true, false, BlendMode::Add, GpuParticleManager::EffectType::Plane);

            // 黄金〜オレンジの衝撃波リング（外側へ一気に広がる）
            GpuParticleManager::EmitterConfig ring{};
            ring.count = 1;
            ring.lifeTimeMin = 0.26f;
            ring.lifeTimeMax = 0.32f;
            ring.startScaleMin = { 0.30f, 0.30f, 0.30f };
            ring.startScaleMax = { 0.38f, 0.38f, 0.38f };
            ring.endScaleMin = { 2.35f, 2.35f, 2.35f };
            ring.endScaleMax = { 2.70f, 2.70f, 2.70f };
            ring.angularVelocityMin = { 0.0f, 0.0f, -3.5f };
            ring.angularVelocityMax = { 0.0f, 0.0f,  3.5f };
            ring.startColorMin = { 1.5f, 1.25f, 0.45f, 1.0f };
            ring.startColorMax = { 1.7f, 1.45f, 0.65f, 1.0f };
            ring.endColorMin = { 1.0f, 0.50f, 0.10f, 0.0f };
            ring.endColorMax = { 1.0f, 0.60f, 0.20f, 0.0f };
            gpuParticle->RegisterEffectGroup("EnemyDefeat", "EnemyDefeat_Ring", "resources/Efect.png", ring, true, false, BlendMode::Add, GpuParticleManager::EffectType::Ring);

            // 追従する内側の白黄リング
            GpuParticleManager::EmitterConfig subRing{};
            subRing.count = 1;
            subRing.lifeTimeMin = 0.18f;
            subRing.lifeTimeMax = 0.22f;
            subRing.startScaleMin = { 0.18f, 0.18f, 0.18f };
            subRing.startScaleMax = { 0.24f, 0.24f, 0.24f };
            subRing.endScaleMin = { 1.50f, 1.50f, 1.50f };
            subRing.endScaleMax = { 1.75f, 1.75f, 1.75f };
            subRing.startColorMin = { 1.6f, 1.55f, 1.15f, 1.0f };
            subRing.startColorMax = { 1.8f, 1.75f, 1.35f, 1.0f };
            subRing.endColorMin = { 1.0f, 0.80f, 0.30f, 0.0f };
            subRing.endColorMax = { 1.0f, 0.88f, 0.40f, 0.0f };
            gpuParticle->RegisterEffectGroup("EnemyDefeat", "EnemyDefeat_SubRing", "resources/Efect.png", subRing, true, false, BlendMode::Add, GpuParticleManager::EffectType::Ring);

            // カービィ64らしいポフッと膨らんで消える丸い煙玉（通常ブレンド）
            GpuParticleManager::EmitterConfig smoke{};
            smoke.count = 9;
            smoke.emitRangeMin = { -0.25f, -0.20f, -0.25f };
            smoke.emitRangeMax = {  0.25f,  0.25f,  0.25f };
            smoke.lifeTimeMin = 0.35f;
            smoke.lifeTimeMax = 0.52f;
            smoke.startScaleMin = { 0.45f, 0.45f, 0.45f };
            smoke.startScaleMax = { 0.65f, 0.65f, 0.65f };
            smoke.endScaleMin = { 1.05f, 1.05f, 1.05f };
            smoke.endScaleMax = { 1.40f, 1.40f, 1.40f };
            smoke.velocityMin = { -4.2f, -1.5f, -4.2f };
            smoke.velocityMax = {  4.2f,  3.5f,  4.2f };
            smoke.startColorMin = { 1.00f, 0.96f, 0.86f, 0.92f };
            smoke.startColorMax = { 1.00f, 0.88f, 0.68f, 0.96f };
            smoke.endColorMin = { 0.95f, 0.78f, 0.58f, 0.0f };
            smoke.endColorMax = { 1.00f, 0.85f, 0.68f, 0.0f };
            smoke.drag = 7.0f;
            smoke.gravity = -0.6f;
            gpuParticle->RegisterEffectGroup("EnemyDefeat", "EnemyDefeat_SmokePuff", "resources/circle2.png", smoke, true, false, BlendMode::Normal, GpuParticleManager::EffectType::Plane);

            // クルクル回転しながら四方八方に飛び散るキラキラ星片
            GpuParticleManager::EmitterConfig stars{};
            stars.count = 16;
            stars.emitRangeMin = { -0.15f, -0.15f, -0.15f };
            stars.emitRangeMax = {  0.15f,  0.15f,  0.15f };
            stars.lifeTimeMin = 0.36f;
            stars.lifeTimeMax = 0.56f;
            stars.startScaleMin = { 0.16f, 0.16f, 0.16f };
            stars.startScaleMax = { 0.26f, 0.26f, 0.26f };
            stars.endScaleMin = { 0.01f, 0.01f, 0.01f };
            stars.endScaleMax = { 0.03f, 0.03f, 0.03f };
            stars.rotateMin = { 0.0f, 0.0f, -std::numbers::pi_v<float> };
            stars.rotateMax = { 0.0f, 0.0f,  std::numbers::pi_v<float> };
            stars.angularVelocityMin = { 0.0f, 0.0f, -14.0f };
            stars.angularVelocityMax = { 0.0f, 0.0f,  14.0f };
            stars.velocityMin = { -7.5f,  1.0f, -7.5f };
            stars.velocityMax = {  7.5f,  8.5f,  7.5f };
            stars.startColorMin = { 1.6f, 1.35f, 0.40f, 1.0f };
            stars.startColorMax = { 1.9f, 1.75f, 0.85f, 1.0f };
            stars.endColorMin = { 1.0f, 0.45f, 0.10f, 0.0f };
            stars.endColorMax = { 1.0f, 0.65f, 0.20f, 0.0f };
            stars.drag = 4.5f;
            stars.gravity = 6.5f;
            gpuParticle->RegisterEffectGroup("EnemyDefeat", "EnemyDefeat_StarSpin", "resources/white.png", stars, true, false, BlendMode::Add, GpuParticleManager::EffectType::Plane);

            // 放射線状に走る光の筋（速度ストレッチ）
            GpuParticleManager::EmitterConfig scatter{};
            scatter.count = 14;
            scatter.emitRangeMin = { -0.10f, -0.10f, -0.10f };
            scatter.emitRangeMax = {  0.10f,  0.10f,  0.10f };
            scatter.lifeTimeMin = 0.22f;
            scatter.lifeTimeMax = 0.34f;
            scatter.startScaleMin = { 0.14f, 0.20f, 0.14f };
            scatter.startScaleMax = { 0.22f, 0.30f, 0.22f };
            scatter.endScaleMin = { 0.02f, 0.04f, 0.02f };
            scatter.endScaleMax = { 0.04f, 0.06f, 0.04f };
            scatter.velocityMin = { -9.5f, -3.5f, -9.5f };
            scatter.velocityMax = {  9.5f,  9.5f,  9.5f };
            scatter.startColorMin = { 1.5f, 1.15f, 0.40f, 1.0f };
            scatter.startColorMax = { 1.8f, 1.55f, 0.85f, 1.0f };
            scatter.endColorMin = { 1.0f, 0.40f, 0.10f, 0.0f };
            scatter.endColorMax = { 1.0f, 0.60f, 0.20f, 0.0f };
            scatter.drag = 6.5f;
            scatter.gravity = 3.0f;
            gpuParticle->RegisterEffectGroup("EnemyDefeat", "EnemyDefeat_Scatter", "resources/circle2.png", scatter, false, true, BlendMode::Add, GpuParticleManager::EffectType::Plane);
        }

        // 2. プレイヤー近距離攻撃エフェクト ("PlayerMelee")
        {
            // 中心の白い閃光（拳の先に一瞬強く光る芯）
            GpuParticleManager::EmitterConfig meleeFlash{};
            meleeFlash.count = 1;
            meleeFlash.emitRangeMin = { 0.0f, 0.0f, 0.0f };
            meleeFlash.emitRangeMax = { 0.0f, 0.0f, 0.0f };
            meleeFlash.lifeTimeMin = 0.12f;
            meleeFlash.lifeTimeMax = 0.16f;
            meleeFlash.startScaleMin = { 0.85f, 0.85f, 0.85f };
            meleeFlash.startScaleMax = { 1.00f, 1.00f, 1.00f };
            meleeFlash.endScaleMin = { 0.05f, 0.05f, 0.05f };
            meleeFlash.endScaleMax = { 0.08f, 0.08f, 0.08f };
            meleeFlash.velocityMin = { 0.0f, 0.0f, 0.5f };
            meleeFlash.velocityMax = { 0.0f, 0.0f, 1.0f };
            meleeFlash.startColorMin = { 1.5f, 1.7f, 1.8f, 1.0f };
            meleeFlash.startColorMax = { 1.8f, 1.9f, 2.0f, 1.0f };
            meleeFlash.endColorMin = { 0.40f, 0.80f, 1.0f, 0.0f };
            meleeFlash.endColorMax = { 0.60f, 0.90f, 1.0f, 0.0f };
            gpuParticle->RegisterEffectGroup("PlayerMelee", "PlayerMelee_Flash", "resources/circle2.png", meleeFlash, true, false, BlendMode::Add, GpuParticleManager::EffectType::Plane);

            // プレイヤー進行方向に面を向けて前方に進む衝撃波リング（インナー）
            GpuParticleManager::EmitterConfig shockRingInner{};
            shockRingInner.count = 1;
            shockRingInner.emitRangeMin = { 0.0f, 0.0f, 0.10f };
            shockRingInner.emitRangeMax = { 0.0f, 0.0f, 0.18f };
            shockRingInner.lifeTimeMin = 0.20f;
            shockRingInner.lifeTimeMax = 0.25f;
            shockRingInner.startScaleMin = { 0.25f, 0.25f, 0.25f };
            shockRingInner.startScaleMax = { 0.30f, 0.30f, 0.30f };
            shockRingInner.endScaleMin = { 0.95f, 0.95f, 0.95f };
            shockRingInner.endScaleMax = { 1.10f, 1.10f, 1.10f };
            shockRingInner.angularVelocityMin = { 0.0f, 0.0f, 4.0f };
            shockRingInner.angularVelocityMax = { 0.0f, 0.0f, 6.0f };
            shockRingInner.velocityMin = { 0.0f, 0.0f, 4.2f };
            shockRingInner.velocityMax = { 0.0f, 0.0f, 5.2f };
            shockRingInner.startColorMin = { 1.00f, 1.50f, 1.85f, 1.0f };
            shockRingInner.startColorMax = { 1.30f, 1.70f, 2.00f, 1.0f };
            shockRingInner.endColorMin = { 0.25f, 0.65f, 1.0f, 0.0f };
            shockRingInner.endColorMax = { 0.45f, 0.85f, 1.0f, 0.0f };
            gpuParticle->RegisterEffectGroup("PlayerMelee", "PlayerMelee_ShockRing", "resources/Efect.png", shockRingInner, false, false, BlendMode::Add, GpuParticleManager::EffectType::Ring);

            // プレイヤー進行方向に面を向けて大きく広がる外周衝撃波リング（アウター）
            GpuParticleManager::EmitterConfig shockRingOuter{};
            shockRingOuter.count = 1;
            shockRingOuter.emitRangeMin = { 0.0f, 0.0f, 0.0f };
            shockRingOuter.emitRangeMax = { 0.0f, 0.0f, 0.05f };
            shockRingOuter.lifeTimeMin = 0.24f;
            shockRingOuter.lifeTimeMax = 0.30f;
            shockRingOuter.startScaleMin = { 0.35f, 0.35f, 0.35f };
            shockRingOuter.startScaleMax = { 0.42f, 0.42f, 0.42f };
            shockRingOuter.endScaleMin = { 1.35f, 1.35f, 1.35f };
            shockRingOuter.endScaleMax = { 1.55f, 1.55f, 1.55f };
            shockRingOuter.angularVelocityMin = { 0.0f, 0.0f, -5.0f };
            shockRingOuter.angularVelocityMax = { 0.0f, 0.0f, -3.0f };
            shockRingOuter.velocityMin = { 0.0f, 0.0f, 2.0f };
            shockRingOuter.velocityMax = { 0.0f, 0.0f, 2.8f };
            shockRingOuter.startColorMin = { 0.80f, 1.30f, 1.75f, 1.0f };
            shockRingOuter.startColorMax = { 1.10f, 1.50f, 1.95f, 1.0f };
            shockRingOuter.endColorMin = { 0.15f, 0.50f, 1.0f, 0.0f };
            shockRingOuter.endColorMax = { 0.30f, 0.65f, 1.0f, 0.0f };
            gpuParticle->RegisterEffectGroup("PlayerMelee", "PlayerMelee_ShockRingOuter", "resources/Efect.png", shockRingOuter, false, false, BlendMode::Add, GpuParticleManager::EffectType::Ring);

            // シリンダーで表現する、攻撃範囲内（emitPosから約1.2m以内）を飛ぶエネルギー球
            GpuParticleManager::EmitterConfig energySphere{};
            energySphere.count = 1;
            energySphere.emitRangeMin = { 0.0f, 0.0f, 0.0f };
            energySphere.emitRangeMax = { 0.0f, 0.0f, 0.05f };
            energySphere.lifeTimeMin = 0.20f;
            energySphere.lifeTimeMax = 0.22f;
            energySphere.rotateMin = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            energySphere.rotateMax = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            energySphere.startScaleMin = { 0.75f, 0.80f, 0.75f };
            energySphere.startScaleMax = { 0.85f, 0.90f, 0.85f };
            energySphere.endScaleMin = { 0.20f, 0.24f, 0.20f };
            energySphere.endScaleMax = { 0.26f, 0.30f, 0.26f };
            energySphere.velocityMin = { 0.0f, 0.0f, 5.5f };
            energySphere.velocityMax = { 0.0f, 0.0f, 6.5f };
            energySphere.startColorMin = { 1.35f, 1.70f, 2.00f, 1.0f };
            energySphere.startColorMax = { 1.60f, 1.85f, 2.00f, 1.0f };
            energySphere.endColorMin = { 0.30f, 0.75f, 1.00f, 0.0f };
            energySphere.endColorMax = { 0.45f, 0.85f, 1.00f, 0.0f };
            energySphere.uvScrollSpeed = { 0.0f, -2.5f };
            energySphere.drag = 4.0f;
            gpuParticle->RegisterEffectGroup("PlayerMelee", "PlayerMelee_EnergySphere", "resources/circle2.png", energySphere, false, false, BlendMode::Add, GpuParticleManager::EffectType::Cylinder);

            // 飛ぶ球の周囲を包んで攻撃範囲内を突き抜ける疾風シリンダー
            GpuParticleManager::EmitterConfig thrust{};
            thrust.count = 1;
            thrust.emitRangeMin = { 0.0f, 0.0f, 0.0f };
            thrust.emitRangeMax = { 0.0f, 0.0f, 0.05f };
            thrust.lifeTimeMin = 0.18f;
            thrust.lifeTimeMax = 0.22f;
            thrust.rotateMin = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            thrust.rotateMax = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            thrust.startScaleMin = { 0.55f, 0.70f, 0.55f };
            thrust.startScaleMax = { 0.65f, 0.85f, 0.65f };
            thrust.endScaleMin = { 0.25f, 0.35f, 0.25f };
            thrust.endScaleMax = { 0.32f, 0.45f, 0.32f };
            thrust.velocityMin = { 0.0f, 0.0f, 4.0f };
            thrust.velocityMax = { 0.0f, 0.0f, 5.0f };
            thrust.startColorMin = { 0.90f, 1.45f, 1.85f, 1.0f };
            thrust.startColorMax = { 1.20f, 1.65f, 2.00f, 1.0f };
            thrust.endColorMin = { 0.20f, 0.60f, 1.0f, 0.0f };
            thrust.endColorMax = { 0.35f, 0.75f, 1.0f, 0.0f };
            thrust.uvScrollSpeed = { 0.0f, -4.0f };
            thrust.flipUvY = true;
            thrust.drag = 4.5f;
            gpuParticle->RegisterEffectGroup("PlayerMelee", "PlayerMelee_ThrustCylinder", "resources/gradationLine.png", thrust, false, false, BlendMode::Add, GpuParticleManager::EffectType::Cylinder);

            // 攻撃範囲に沿って散る火花（速度ストレッチで細長く伸びる）
            GpuParticleManager::EmitterConfig sparks{};
            sparks.count = 20;
            sparks.emitRangeMin = { -0.15f, -0.15f, 0.0f };
            sparks.emitRangeMax = {  0.15f,  0.15f, 0.15f };
            sparks.lifeTimeMin = 0.20f;
            sparks.lifeTimeMax = 0.28f;
            sparks.startScaleMin = { 0.12f, 0.18f, 0.12f };
            sparks.startScaleMax = { 0.18f, 0.24f, 0.18f };
            sparks.endScaleMin = { 0.02f, 0.04f, 0.02f };
            sparks.endScaleMax = { 0.04f, 0.08f, 0.04f };
            sparks.velocityMin = { -2.4f, -2.0f, 4.5f };
            sparks.velocityMax = {  2.4f,  2.4f, 7.5f };
            sparks.startColorMin = { 1.40f, 1.10f, 0.45f, 1.0f };
            sparks.startColorMax = { 1.80f, 1.70f, 1.20f, 1.0f };
            sparks.endColorMin = { 1.00f, 0.55f, 0.15f, 0.0f };
            sparks.endColorMax = { 1.00f, 0.75f, 0.30f, 0.0f };
            sparks.drag = 7.5f;
            sparks.gravity = 2.0f;
            gpuParticle->RegisterEffectGroup("PlayerMelee", "PlayerMelee_SlashSparks", "resources/circle2.png", sparks, false, true, BlendMode::Add, GpuParticleManager::EffectType::Plane);
        }

        // 3. 敵被弾エフェクト ("EnemyHit": シリンダー)
        {
            // 敵を縦に包んで広がる衝撃シリンダー
            GpuParticleManager::EmitterConfig hitAura{};
            hitAura.count = 1;
            hitAura.emitRangeMin = { 0.0f, -0.45f, 0.0f };
            hitAura.emitRangeMax = { 0.0f, -0.45f, 0.0f };
            hitAura.lifeTimeMin = 0.16f;
            hitAura.lifeTimeMax = 0.20f;
            hitAura.startScaleMin = { 0.70f, 0.90f, 0.70f };
            hitAura.startScaleMax = { 0.80f, 1.00f, 0.80f };
            hitAura.endScaleMin = { 1.45f, 1.50f, 1.45f };
            hitAura.endScaleMax = { 1.65f, 1.70f, 1.65f };
            hitAura.startColorMin = { 1.80f, 0.75f, 0.25f, 1.0f };
            hitAura.startColorMax = { 2.00f, 0.95f, 0.35f, 1.0f };
            hitAura.endColorMin = { 1.00f, 0.15f, 0.05f, 0.0f };
            hitAura.endColorMax = { 1.00f, 0.25f, 0.10f, 0.0f };
            hitAura.uvScrollSpeed = { 0.0f, -5.0f };
            hitAura.flipUvY = true;
            gpuParticle->RegisterEffectGroup("EnemyHit", "EnemyHit_AuraCylinder", "resources/gradationLine.png", hitAura, false, false, BlendMode::Add, GpuParticleManager::EffectType::Cylinder);

            // 被弾方向へ突き抜ける横シリンダー
            GpuParticleManager::EmitterConfig hitImpact{};
            hitImpact.count = 1;
            hitImpact.emitRangeMin = { 0.0f, 0.0f, -0.15f };
            hitImpact.emitRangeMax = { 0.0f, 0.0f,  0.00f };
            hitImpact.lifeTimeMin = 0.12f;
            hitImpact.lifeTimeMax = 0.16f;
            hitImpact.rotateMin = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            hitImpact.rotateMax = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            hitImpact.startScaleMin = { 0.45f, 0.85f, 0.45f };
            hitImpact.startScaleMax = { 0.55f, 0.95f, 0.55f };
            hitImpact.endScaleMin = { 0.95f, 1.35f, 0.95f };
            hitImpact.endScaleMax = { 1.10f, 1.50f, 1.10f };
            hitImpact.velocityMin = { 0.0f, 0.0f, 2.5f };
            hitImpact.velocityMax = { 0.0f, 0.0f, 3.5f };
            hitImpact.startColorMin = { 2.00f, 1.50f, 0.70f, 1.0f };
            hitImpact.startColorMax = { 2.00f, 1.70f, 0.95f, 1.0f };
            hitImpact.endColorMin = { 1.00f, 0.30f, 0.10f, 0.0f };
            hitImpact.endColorMax = { 1.00f, 0.45f, 0.15f, 0.0f };
            hitImpact.uvScrollSpeed = { 0.0f, -4.0f };
            hitImpact.drag = 4.0f;
            gpuParticle->RegisterEffectGroup("EnemyHit", "EnemyHit_ImpactCylinder", "resources/circle2.png", hitImpact, false, false, BlendMode::Add, GpuParticleManager::EffectType::Cylinder);
        }

        // 4. 弾のらせんシリンダーエフェクト ("BulletSpiral")
        {
            GpuParticleManager::EmitterConfig spiralCyl{};
            spiralCyl.count = 1;
            spiralCyl.emitRangeMin = { 0.0f, 0.0f, 0.0f };
            spiralCyl.emitRangeMax = { 0.0f, 0.0f, 0.0f };
            spiralCyl.lifeTimeMin = 0.09f;
            spiralCyl.lifeTimeMax = 0.12f;
            spiralCyl.rotateMin = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            spiralCyl.rotateMax = { std::numbers::pi_v<float> * 0.5f, 0.0f, 0.0f };
            spiralCyl.startScaleMin = { 0.36f, 0.72f, 0.36f };
            spiralCyl.startScaleMax = { 0.42f, 0.80f, 0.42f };
            spiralCyl.endScaleMin = { 0.08f, 0.50f, 0.08f };
            spiralCyl.endScaleMax = { 0.12f, 0.60f, 0.12f };
            spiralCyl.velocityMin = { 0.0f, 0.0f, 2.2f };
            spiralCyl.velocityMax = { 0.0f, 0.0f, 2.8f };
            spiralCyl.startColorMin = { 0.75f, 1.65f, 2.00f, 1.0f };
            spiralCyl.startColorMax = { 1.10f, 1.85f, 2.00f, 1.0f };
            spiralCyl.endColorMin = { 0.15f, 0.55f, 1.00f, 0.0f };
            spiralCyl.endColorMax = { 0.25f, 0.70f, 1.00f, 0.0f };
            spiralCyl.uvScale = { 2.0f, 1.0f };
            spiralCyl.uvTwist = 2.0f;
            spiralCyl.uvScrollSpeed = { 6.0f, -2.0f };
            spiralCyl.useUvWrap = true;
            spiralCyl.drag = 3.0f;
            gpuParticle->RegisterEffectGroup("BulletSpiral", "BulletSpiral_Cylinder", "resources/gradationLine.png", spiralCyl, false, false, BlendMode::Add, GpuParticleManager::EffectType::Cylinder);
        }
    }
    LightManager::GetInstance()->AddDirectionalLight({ 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, 1.0f);










    skyBox = std::make_unique<SkyBox>();
    skyBox->Initialize();
    skyBox->SetCamera(activeCamera_);
    skyBox->SetTextureByFilePath("resources/output_skybox.dds");

    Object3dCommon::GetInstance()->SetDefaultSkyBox(skyBox.get());

    ModelManager::GetInstance()->LoadModel("resources/player/", "playerCursor.obj");
    ModelManager::GetInstance()->LoadModel("resources/player/", "player.obj");

    ModelManager::GetInstance()->LoadModel("resources/AnimatedCube", "AnimatedCube.gltf");
    ModelManager::GetInstance()->LoadModel("resources/simpleSkin", "simpleSkin.gltf");




    // --- ステージマネージャーによるJSONステージデータの読み込み ---
    enemySpawnTriggers_.clear();
    stageManager_ = std::make_unique<StageManager>
        ();
    stageManager_->Load(selectedStagePath_, activeCamera_,
        [this](const LevelObjectData& data) {
            Enemy::EnemyType type = Enemy::EnemyType::Normal;
            if (data.enemyType == "Bound") {
                type = Enemy::EnemyType::Bound;
            } else if (data.enemyType == "Chase") {
                type = Enemy::EnemyType::Chase;
            } else if (data.enemyType == "Hover") {
                type = Enemy::EnemyType::Hover;
            } else if (data.enemyType == "Shield") {
                type = Enemy::EnemyType::Shield;
            }

            float triggerDist = 25.0f;
            auto itDist = data.properties.find("spawn_distance");
            if (itDist != data.properties.end()) {
                try { triggerDist = std::stof(itDist->second); } catch (...) {}
            }

            auto itMode = data.properties.find("spawn_mode");
            if (itMode != data.properties.end() && itMode->second == "IMMEDIATE") {
                // 最初から直接配置
                AddEnemy(data.railPos, type, data.properties);
            } else {
                // カメラ直前スポーン待機リストへ登録
                EnemySpawnTrigger trigger;
                trigger.railPos = data.railPos;
                trigger.enemyType = type;
                trigger.triggerDistance = triggerDist;
                trigger.hasSpawned = false;
                trigger.properties = data.properties;
                enemySpawnTriggers_.push_back(trigger);
            }
        }
    );

    player = std::make_unique<Player>();
    player->Initialize();
    player->SetCamera(activeCamera_);
    player->SetScene(this);

    player->SetPosition(stageManager_->GetPlayerSpawnPosition());

    playerHPUI_ = std::make_unique<PlayerHPUI>();
    playerHPUI_->Initialize(player.get());

    scoreUI_ = std::make_unique<ScoreUI>();
    scoreUI_->Initialize(player.get());

    pauseMenu_ = std::make_unique<PauseMenu>();
    pauseMenu_->Initialize(this);

    cameraController = std::make_unique<CameraController>();
    cameraController->Initialize(cameraMap_["Main"].get());
    cameraController->SetTarget(player.get());

    // ステージレールおよびカメラ設定の同期
    cameraController->SetStageRail(stageManager_->GetStageRail());
    cameraController->SetAutoOffsetParams(
        stageManager_->GetStageCameraDistance(),
        stageManager_->GetStageCameraHeight()
    );
    cameraController->SetDrawDistance(stageManager_->GetStageCameraDrawDistance());

    // カメラレール追従と自動オフセットの切り替え
    if (stageManager_->GetStageCameraMode() == "CAMERA_RAIL" && stageManager_->GetCameraRail()) {
        cameraController->SetCameraMode(CameraController::CameraMode::RailCamera);
        cameraController->SetRailPath(stageManager_->GetCameraRail());
    } else {
        cameraController->SetCameraMode(CameraController::CameraMode::AutoOffset);
        if (stageManager_->GetCameraRail()) {
            cameraController->SetRailPath(stageManager_->GetCameraRail());
        }
    }


    debugCameraC = std::make_unique<CameraController>();
    debugCameraC->Initialize(cameraMap_["Debug"].get());
    debugCameraC->SetTarget(player.get());

    RailPath* currentStageRail = GetStageRaill();
    if (currentStageRail) {
        player->SetRail(currentStageRail);
        player->SetRailPosition(stageManager_->GetPlayerSpawnRailPosition());
    }

    // カメラ初期化直後にプレイヤーを追尾・注視するように更新
    cameraController->Update();

    goal_ = std::make_unique<GoalObject>();
    goal_->Initialize();
    goal_->SetCamera(activeCamera_);
    if (currentStageRail) {
        goal_->SetRail(currentStageRail);
        goal_->SetRailPosition(stageManager_->GetGoalRailPosition());
    }


    // 操作ガイドUIは下部非表示
    gaidUI_ = nullptr;

    DamagePopupManager::GetInstance()->Initialize();

    ChangePhase(std::make_unique<StartPhase>());

}
void GameScene::Finalize() {
    if (currentPhase_) {
        currentPhase_->Finalize(this);
        currentPhase_.reset();
    }

    if (Audio::GetInstance()->IsPlaying(BGMHandle_))
    {
        Audio::GetInstance()->StopAudio(BGMHandle_);
    }
    bannerSprite_.reset();
    pauseMenu_.reset();
}

void GameScene::Update() {
    // 1. 通常ポーズメニューの更新
    if (pauseMenu_) {
        pauseMenu_->Update();
        if (pauseMenu_->IsPaused()) {
            isGameTimeUpdatedThisFrame_ = false;
            return; // ポーズ中はゲーム内の時間を完全停止
        }
    }

    // 2. デバッグ用：時間停止・コマ送り機能
#ifdef USE_IMGUI
    Input* input = Input::GetInstance();
    if (input->TriggerKeyDown(DIK_F1)) {
        isDebugFrozen_ = !isDebugFrozen_;
    }
    if (input->TriggerKeyDown(DIK_F2)) {
        isStepFrame_ = true;
    }

    ImGui::Begin("Debug Time Control");
    if (ImGui::Button(isDebugFrozen_ ? "Resume Time (F1)" : "Freeze Time (F1)")) {
        isDebugFrozen_ = !isDebugFrozen_;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step 1 Frame (F2)")) {
        isStepFrame_ = true;
    }
    ImGui::Text(isDebugFrozen_ ? "Status: [TIME FROZEN]" : "Status: [RUNNING]");
    ImGui::End();
#endif

    // デバッグカメラ・メインカメラの更新（時止め中もデバッグカメラで周囲を観察可能）
    debugCameraC->Update();
    activeCamera_->Update();

    // デバッグ時間停止中で、かつ1コマ送りトリガーもない場合はゲーム世界更新をスキップ
    bool allowGameWorldUpdate = !isDebugFrozen_ || isStepFrame_;
    isStepFrame_ = false;
    isGameTimeUpdatedThisFrame_ = allowGameWorldUpdate;

    if (!allowGameWorldUpdate) {
        return;
    }

    // 統合物理システム(Physics)にステージの三角ポリゴンリストを登録
    Physics::SetTriangles(&GetTriangle());

    CheckPhaseTransition();

    UpdateHitStop();
    if (currentPhase_)
    {
        currentPhase_->Update(this);
    }

    // カメラ直前エネミースポーンの判定更新
    UpdateEnemySpawners();
    // 死んだProjectileを削除
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
            [](const std::unique_ptr<Projectile>& p) { return p->IsDead(); }),
        projectiles_.end()
    );



    // 死亡演出が完了した敵を削除
    enemies_.erase(
        std::remove_if(enemies_.begin(), enemies_.end(),
            [](const std::unique_ptr<Enemy>& e) { return e->IsDeathFinished(); }),
        enemies_.end()
    );


    skyBox->SetTranslate(activeCamera_->GetTranslate());
    skyBox->Update();

   // object3d->Update();
    if (gaidUI_) {
        gaidUI_->Update();
    }
    UpdateBannerSprite();

#ifdef USE_IMGUI
    ImGui::Begin("Debug");
    PrimitiveDrawer::GetInstance()->DrawImGui();
    ImGui::Separator();

    ImGui::Text("Sprite");



    ImGui::SliderFloat3("Start", &(position_.x), 0.1f, 1000.0f);
    // Vector3 Rotate = camera->GetRotate();
    ImGui::DragFloat4("Rotate", &(rotation_.x));
    // camera->SetRotate(Rotate);


    RailPath* currentStageRail = GetStageRaill();
    if (currentStageRail) {
        Vector3 point1_ = currentStageRail->GetPointPos(1);
        Vector3 point2_ = currentStageRail->GetPointPos(2);

        bool changed = false;
        if (ImGui::SliderFloat3("Point1", &(point1_.x), -10.0f, 10.0f)) {
            currentStageRail->SetPointPos(1, point1_);
            changed = true;
        }
        if (ImGui::SliderFloat3("Point2", &(point2_.x), -10.0f, 1000.0f)) {
            currentStageRail->SetPointPos(2, point2_);
            changed = true;
        }
        if (changed) {
            currentStageRail->Update();
        }
    }

    // ステージリロードボタン（JSON再読み込み）
    if (ImGui::Button("Reload Test Stage (test_stage.json)")) {
        enemies_.clear();
        enemySpawnTriggers_.clear();
        stageManager_->Load("resources/Stagemap/test_stage.json", activeCamera_,
            [this](const LevelObjectData& data) {
                Enemy::EnemyType type = Enemy::EnemyType::Normal;
                if (data.enemyType == "Bound") {
                    type = Enemy::EnemyType::Bound;
                } else if (data.enemyType == "Chase") {
                    type = Enemy::EnemyType::Chase;
                } else if (data.enemyType == "Hover") {
                    type = Enemy::EnemyType::Hover;
                } else if (data.enemyType == "Shield") {
                    type = Enemy::EnemyType::Shield;
                }
                AddEnemy(data.railPos, type, data.properties);
            }
        );
        RailPath* sRail = GetStageRaill();
        if (sRail && player) {
            player->SetRail(sRail);
            player->SetRailPosition(stageManager_->GetPlayerSpawnRailPosition());
        }
        if (sRail && goal_) {
            goal_->SetRail(sRail);
            goal_->SetRailPosition(stageManager_->GetGoalRailPosition());
        }
        if (stageManager_->GetCameraRail() && cameraController) {
            cameraController->SetRailPath(stageManager_->GetCameraRail());
        }
        if (cameraController) {
            cameraController->SetAutoOffsetParams(
                stageManager_->GetStageCameraDistance(),
                stageManager_->GetStageCameraHeight()
            );
        }
    }

    if (ImGui::Button("Reload Stage (stage1.json)")) {
        enemies_.clear();
        enemySpawnTriggers_.clear();
        stageManager_->Load("resources/Stagemap/stage1.json", activeCamera_,
            [this](const LevelObjectData& data) {
                Enemy::EnemyType type = Enemy::EnemyType::Normal;
                if (data.enemyType == "Bound") {
                    type = Enemy::EnemyType::Bound;
                } else if (data.enemyType == "Chase") {
                    type = Enemy::EnemyType::Chase;
                } else if (data.enemyType == "Hover") {
                    type = Enemy::EnemyType::Hover;
                } else if (data.enemyType == "Shield") {
                    type = Enemy::EnemyType::Shield;
                }
                AddEnemy(data.railPos, type, data.properties);
            }
        );
        RailPath* sRail = GetStageRaill();
        if (sRail && player) {
            player->SetRail(sRail);
            player->SetRailPosition(stageManager_->GetPlayerSpawnRailPosition());
        }
        if (sRail && goal_) {
            goal_->SetRail(sRail);
            goal_->SetRailPosition(stageManager_->GetGoalRailPosition());
        }
        if (stageManager_->GetCameraRail() && cameraController) {
            cameraController->SetRailPath(stageManager_->GetCameraRail());
        }
        if (cameraController) {
            cameraController->SetAutoOffsetParams(
                stageManager_->GetStageCameraDistance(),
                stageManager_->GetStageCameraHeight()
            );
        }
    }

    //カメラを切り替えるボタン
    if (ImGui::Button("Switch Camera")) {
        isDebugCamera_ = !isDebugCamera_;
        if (isDebugCamera_) {
            ChangeActiveCamera(cameraMap_["Debug"].get());
            PrimitiveDrawer::GetInstance()->SetCamera(cameraMap_["Debug"].get());
        } else {
            ChangeActiveCamera(cameraMap_["Main"].get());
        }
    }

    // 通常状態へリセットボタン（テスト用）
    if (ImGui::Button("Reset to Normal State")) {
        player->ChangeState(PlayerStateFactory::CreateState(PlayerFormType::Normal));
    }

    // 追跡エネミー（ChaseEnemy）をプレイヤー近傍に即座にスポーンさせるテストボタン
    if (ImGui::Button("Spawn Chase Enemy near Player (+12m)")) {
        if (player) {
            float playerProgress = player->GetRailProgress();
            // レール進行度を約 0.08（約12m先）進めた位置にスポーン
            float spawnProgress = (std::min)(playerProgress + 0.08f, 0.95f);
            AddEnemy({ spawnProgress, player->GetWorldPosition().y }, Enemy::EnemyType::Chase);
        }
    }

    // テスト用：ステートが保持しているアクション情報を表示
    ImGui::Separator();
    ImGui::Text("--- Action Debug ---");

    auto currentState = player->GetState();
    if (currentState) {
        ImGui::Text("State Name: %s", currentState->GetName());

        auto moveAction = currentState->GetMoveAction();
        auto jumpAction = currentState->GetJumpAction();
        auto attackAction = currentState->GetAttackAction_();
        auto shootAction = currentState->GetShootAction();

        ImGui::Text("MoveAction: %s", moveAction ? "Available" : "Null");
        ImGui::Text("JumpAction: %s", jumpAction ? "Available" : "Null");
        ImGui::Text("AttackAction: %s", attackAction ? "Available" : "Null");
        ImGui::Text("ShootAction: %s", shootAction ? "Available" : "Null");

        // テスト用：直接アクション実行ボタン
        if (ImGui::Button("Test: Execute Move Action")) {
            if (moveAction) {
                moveAction->Execute(player.get());
            }
        }

        if (ImGui::Button("Test: Execute Shoot Action")) {
            if (shootAction) {
                shootAction->Execute(player.get());
            }
        }
    }

    ImGui::End();

    if (currentStageRail) {
        ImGui::Begin("Rail Debug");
        static float r = 20.0f;
        static float hScale = 0.5522f;

        // ── 補間モード一括切替 ──
        ImGui::Text("Point Count: %zu", currentStageRail->GetPointCount());
        ImGui::Text("Interpolation Mode Switch:");
        if (ImGui::Button("All Linear (Straight)")) {
            currentStageRail->SetAllInterpolationType(RailPath::InterpolationType::Linear);
            currentStageRail->Update();
        }
        ImGui::SameLine();
        if (ImGui::Button("All Bezier (Smooth)")) {
            currentStageRail->SetAllInterpolationType(RailPath::InterpolationType::Bezier);
            currentStageRail->Update();
        }
        ImGui::SameLine();
        if (ImGui::Button("All CatmullRom")) {
            currentStageRail->SetAllInterpolationType(RailPath::InterpolationType::CatmullRom);
            currentStageRail->Update();
        }

        ImGui::Separator();
        ImGui::Text("Circle Preset Test:");
        if (ImGui::SliderFloat("Radius", &r, 5.0f, 50.0f) || ImGui::SliderFloat("Handle Scale", &hScale, 0.1f, 1.0f)) {
            // 値が変わったらレールを再構築
            currentStageRail->Initialize(); // points_をクリア
            float h = r * hScale;
            currentStageRail->AddBezierPoint({ 0, 0,  r }, { -h, 0, 0 }, { h, 0, 0 });
            currentStageRail->AddBezierPoint({ r, 0, 0 }, { 0, 0,  h }, { 0, 0, -h });
            currentStageRail->AddBezierPoint({ 0, 0, -r }, { h, 0, 0 }, { -h, 0, 0 });
            currentStageRail->AddBezierPoint({ -r, 0, 0 }, { 0, 0, -h }, { 0, 0,  h });
            currentStageRail->Update();
        }
        ImGui::End();
    }


    if (IsHitStopActive())
    {
        ImGui::Begin("Hit Stop Debug");

        // HitStopの残り時間を表示
        ImGui::Text("Hit Stop Remaining Time: %.2f seconds", stopTimer_);
        // HitStopの残り時間をゲージで表示
        ImGui::ProgressBar(stopTimer_ / 0.1f, ImVec2(0, 0), "Hit Stop Progress");

        ImGui::End();




    }




#endif // USE_IMGUI

    if (stageManager_) {
        stageManager_->Update(player ? player->GetWorldPosition() : Vector3{ 0.0f, 0.0f, 0.0f });
    }

    triangles_.clear();
    if (stageManager_) {
        const auto& stageTris = stageManager_->GetAllTriangles();
        triangles_.insert(triangles_.end(), stageTris.begin(), stageTris.end());
    }

    // 四角いオブジェクトの更新
    if (boxObject_) {
        //プレハブクラスを作るとき更新はここを参考にする
        boxObject_->SetCamera(activeCamera_);
        boxObject_->Update();
        auto boxTris = boxObject_->GetWorldTriangles(); // ワールド変換済み三角形を取得する想定
        triangles_.insert(triangles_.end(), boxTris.begin(), boxTris.end());
    }
    if (TestGround_)
    {
        TestGround_->Update();
        auto testGroundTris = TestGround_->GetWorldTriangles(); // ワールド変換済み三角形を取得する想定
        triangles_.insert(triangles_.end(), testGroundTris.begin(), testGroundTris.end());
    }
    playerHPUI_->Update();
    scoreUI_->Update();
    DamagePopupManager::GetInstance()->Update();
    DamagePopupManager::GetInstance()->DrawImGui();

}
void GameScene::Draw() {

    skyBox->Draw();

    if (stageManager_) {
        stageManager_->Draw();
#ifdef _DEBUG
        stageManager_->DebugDraw();
#endif
    }
#ifdef _DEBUG
    if (stageRail) stageRail->DebugDraw();
#endif
    if (!isDefeated_) {
        player->Draw();
    }

    if (goal_)
    {
        goal_->Draw();
    }

    // 全てのProjectileを描画
    for (auto& projectile : projectiles_) {
        if (projectile) {
            projectile->Draw();
        }
    }
    // 全ての敵を描画
    for (auto& enemy : enemies_) {
        enemy->Draw();
    }

    ///////スプライトの描画
    //object3d->Draw();

    // 箱オブジェクトの描画
    if (boxObject_) {
        boxObject_->Draw();
    }
    if (TestGround_)
    {
        TestGround_->Draw();
    }

#ifdef _DEBUG
    // --- レイキャストのデバッグ描画 ---
    // 衝突した三角形があれば赤色で強調描画する
    if (player && player->IsRayHit()) {
        hitTriangle_ = player->GetRayHitTriangle();
        PrimitiveDrawer::GetInstance()->DrawTriangle(
            hitTriangle_.vertices[0],
            hitTriangle_.vertices[1],
            hitTriangle_.vertices[2],
            { 1.0f, 0.0f, 0.0f, 0.5f }, // 赤色の半透明
            FillMode::kSolid
        );
        // 輪郭線も描画
        PrimitiveDrawer::GetInstance()->DrawLine(hitTriangle_.vertices[0], hitTriangle_.vertices[1], { 1.0f, 1.0f, 1.0f, 1.0f });
        PrimitiveDrawer::GetInstance()->DrawLine(hitTriangle_.vertices[1], hitTriangle_.vertices[2], { 1.0f, 1.0f, 1.0f, 1.0f });
        PrimitiveDrawer::GetInstance()->DrawLine(hitTriangle_.vertices[2], hitTriangle_.vertices[0], { 1.0f, 1.0f, 1.0f, 1.0f });
    }
#endif

    ParticleManager::GetInstance()->Draw();
    GpuParticleManager::GetInstance()->Draw();


    playerHPUI_->Draw();
    scoreUI_->Draw();
    DamagePopupManager::GetInstance()->Draw();

    if (gaidUI_) {
        gaidUI_->Draw();
    }

    if (currentPhase_)
    {
        currentPhase_->Draw(this);
    }
    DrawBannerSprite();

    if (pauseMenu_) {
        pauseMenu_->Draw();
    }
}
void GameScene::CheckPhaseTransition()
{
    if (goal_)
    {
        if (goal_->IsCleared() && !isCleared_) {
            isCleared_ = true;
            ChangePhase(std::make_unique<ClearPhase>()); // 次のフェーズに遷移
        }
    }
    CheckPlayerFall();
    //プレイヤーが死亡した場合のフェーズ遷移もここでチェックすることができます。
    if (player)
    {
        if (player->IsDead() && !isDefeated_)
        {
            isDefeated_ = true;
            ChangePhase(std::make_unique<defeatPhase>()); // 次のフェーズに遷移
        }
    }
}

void GameScene::CheckPlayerFall()
{

    if (player)
    {
        Vector3 playerPos = player->GetWorldPosition();
        if (playerPos.y < fallLimit_ && !isDefeated_)
        {
            isDefeated_ = true;
            // プレイヤーが落下限界を下回った場合の処理
            player->Die(); // プレイヤーを死亡状態にする
            ChangePhase(std::make_unique<defeatPhase>()); // 次のフェーズに遷移
        }
    }

}
GameScene::GameScene() = default;

GameScene::~GameScene() = default;

Enemy* GameScene::AddEnemy(Vector2 pos, Enemy::EnemyType enemyType, const std::unordered_map<std::string, std::string>& properties)
{
    RailPath* rail = GetStageRaill();
    if (!rail) return nullptr;

    std::unique_ptr<Enemy> newEnemy = EnemyFactory::Create(enemyType);
    if (!newEnemy) return nullptr;

    // 各派生クラスにパラメータ設定をポリモーフィックに委任
    newEnemy->ApplyProperties(properties);

    newEnemy->Initialize();

    // 共通の設定
    newEnemy->SetCamera(cameraMap_["Main"].get());
    newEnemy->SetRail(rail);
    newEnemy->SetScene(this); // SetRailPosition 内で scene_ のレイキャストを使うため先に設定
    newEnemy->SetRailPosition(pos);

    Enemy* enemyPtr = newEnemy.get();
    // ベクターに追加
    enemies_.push_back(std::move(newEnemy));
    return enemyPtr;
}

void GameScene::UpdateEnemySpawners()
{
    if (!player) return;
    const RailMover* pMover = player->GetRailMover();
    if (!pMover) return;

    RailPath* rail = GetStageRaill();
    if (!rail) return;

    float playerProgress = pMover->GetProgress();
    Vector3 playerPos = player->GetTransform().translate;

    for (auto& trigger : enemySpawnTriggers_) {
        if (trigger.hasSpawned) continue;

        // レール上のエネミー座標を取得
        Vector3 spawnWorldPos = rail->GetPosition(trigger.railPos.x);
        Vector3 diff = { spawnWorldPos.x - playerPos.x, spawnWorldPos.y - playerPos.y, spawnWorldPos.z - playerPos.z };
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        float triggerDistSq = trigger.triggerDistance * trigger.triggerDistance;

        // プレイヤーがエネミーの感知距離（カメラ直前）に入った瞬間に実体化
        if (distSq <= triggerDistSq && trigger.railPos.x >= playerProgress - 0.5f) {
            AddEnemy(trigger.railPos, trigger.enemyType, trigger.properties);
            trigger.hasSpawned = true; // 一度出現したら機能停止！
        }
    }
}
// GameScene.cpp

void GameScene::AddProjectile(const Projectile::ProjectileSpawnParam& param, Projectile::ProjectileOwner owner) {
    RailPath* rail = GetStageRaill();
    if (rail) {
        std::unique_ptr<Projectile> newProjectile = std::make_unique<Projectile>();

        // --- 修正箇所 ---
        // 以前はここで direction.x しか見ていませんでしたが、
        // param 自体を Initialize に渡すことで y 方向（高度）の速度も反映させます。

        newProjectile->SetCamera(cameraMap_["Main"].get());
        newProjectile->SetScene(this);

        // Projectile側のInitializeにparamを丸ごと渡す
        newProjectile->Initialize(rail, param, owner);

        projectiles_.push_back(std::move(newProjectile));
    }
}

void GameScene::AddTriangles(std::vector<Triangle> triangles)
{
    triangles_.insert(triangles_.end(), triangles.begin(), triangles.end());
}

void GameScene::RequestCameraShake(float duration, float power)
{
    if (cameraController) {
        cameraController->RequestShake(duration, power);
    }
}

const std::vector<Triangle>& GameScene::GetTriangle() const
{
    if (stageManager_) {
        return stageManager_->GetAllTriangles();
    }
    return triangles_;
}

void GameScene::CheckClear()
{
    // 2. シーン遷移の実行
    if (isCleared_) {


        // 次のシーン（例: TitleScene や ResultScene）へ遷移（仮）
        GetSceneManager()->ChangeScene("TitleScene");
        return; // 遷移が決まったら以降の更新は不要
    }
}

void GameScene::UpdateHitStop()
{
    if (stopTimer_ > 0) {
        stopTimer_ -= DXCommon::kDeltaTime;
    }
}

RailPath* GameScene::GetStageRaill()
{
    if (stageManager_ && stageManager_->GetStageRail()) {
        return stageManager_->GetStageRail();
    }
    return stageRail.get();
}

void GameScene::SetBannerSprite(std::unique_ptr<Sprite> sprite, float startX, float endX, float startY, float totalDuration, float currentTimer)
{
    bannerSprite_ = std::move(sprite);
    bannerStartX_ = startX;
    bannerEndX_ = endX;
    bannerStartY_ = startY;
    bannerDuration_ = totalDuration;
    bannerTimer_ = currentTimer;
}

void GameScene::UpdateBannerSprite()
{
    if (bannerSprite_) {
        bannerTimer_ += 1.0f / 60.0f;
        if (bannerTimer_ >= bannerDuration_) {
            bannerSprite_.reset();
            return;
        }
        float t = std::clamp(bannerTimer_ / bannerDuration_, 0.0f, 1.0f);
        float currentX = bannerStartX_ + (bannerEndX_ - bannerStartX_) * t;
        bannerSprite_->SetPosition({ currentX, bannerStartY_ });
        bannerSprite_->Update();
    }
}

void GameScene::DrawBannerSprite()
{
    if (bannerSprite_) {
        bannerSprite_->Draw();
    }
}

bool GameScene::IsGameTimeStopped() const {
    if (pauseMenu_ && pauseMenu_->IsPaused()) {
        return true;
    }
    if (!isGameTimeUpdatedThisFrame_) {
        return true;
    }
    return false;
}
