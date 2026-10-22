#include "BoundEnemy.h"

#include "TestEnemy.h"
#include "TestRobot.h"
#include "Robot.h"
#include "Player.h"
#include "PlayerState.h"
#include "EnemyState.h"
#include "boundRobot.h"
#include "JsonManager.h"
#include "Logger.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void BoundEnemy::LoadGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("EnemyTypesConfig", filePath)) {
        return;
    }
    const auto& root = jsonMgr->Get("EnemyTypesConfig");
    if (!root.contains("bound")) return;

    const auto& j = root["bound"];
    s_config.texture = j.value("texture", s_config.texture);
    s_config.jumpAcceleration = j.value("jumpAcceleration", s_config.jumpAcceleration);
    s_config.gravityScaleFloating = j.value("gravityScaleFloating", s_config.gravityScaleFloating);
    s_config.gravityScaleNormal = j.value("gravityScaleNormal", s_config.gravityScaleNormal);
    s_config.hoverLimitHeight = j.value("hoverLimitHeight", s_config.hoverLimitHeight);
}

void BoundEnemy::SaveGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    nlohmann::json root;
    if (jsonMgr->Load("EnemyTypesConfig", filePath)) {
        root = jsonMgr->Get("EnemyTypesConfig");
    }

    root["bound"] = {
        {"texture", s_config.texture},
        {"jumpAcceleration", s_config.jumpAcceleration},
        {"gravityScaleFloating", s_config.gravityScaleFloating},
        {"gravityScaleNormal", s_config.gravityScaleNormal},
        {"hoverLimitHeight", s_config.hoverLimitHeight}
    };

    jsonMgr->Save(filePath, root);
}

BoundEnemy::BoundEnemy() {}

BoundEnemy::~BoundEnemy() = default;

void BoundEnemy::Initialize() {
    static bool s_loaded = false;
    if (!s_loaded) {
        LoadGlobalConfig();
        s_loaded = true;
    }

    Enemy::Initialize();

    SetTexture(s_config.texture);
    // バウンドエネミー専用のビヘイビアに上書き！
    ChangeBehavior(std::make_unique<EnemyBehaviorBounce>());

    // 基底クラスのメソッドを使ってロボットを登録するだけ
    SetRobot(std::make_unique<BoundRobot>());
}

void BoundEnemy::Update() {
    Enemy::Update();
    // ロボット自体は更新が必要ないため、スキップ
}

void BoundEnemy::Draw() {
    Enemy::Draw();
    // ロボット自体は描画が必要ないため、スキップ
}

void EnemyBehaviorBounce::Initialize(Enemy* enemy)
{
    currentAction_ = std::make_unique<JumpAction>();
    dynamic_cast<JumpAction*>(currentAction_.get())->SetAcceleration(BoundEnemy::GetGlobalConfig().jumpAcceleration);
}

void EnemyBehaviorBounce::Update(Enemy* enemy)
{
    if (enemy->IsGround()) {
        // 【地上にいるとき】
        // 1. 前進はストップ or 減速
        // 2. 着地タメタイマーを進める
      //  bounceTimer_ += enemy->GetDeltaTime(); // デルタタイムを利用

        // 3. タメが終わったらジャンプ！
   //     if (bounceTimer_ >= kChargeTime) {
        currentAction_->Execute(enemy); // JumpActionを実行
        //    bounceTimer_ = 0.0f;
        // }

         // 地上ではアニメーションを縮める（潰れ演出）などの処理
    } else {
        // 【空中にいるとき】
        float velY = enemy->GetVelocity().y;

        const auto& cfg = BoundEnemy::GetGlobalConfig();
        if (velY > 0.0f) {
            // ① 上昇中：頂点に達するまでフワッとさせる
            if (velY < 5.0f) {
                enemy->SetGravityScale(cfg.gravityScaleFloating);
            } else {
                enemy->SetGravityScale(cfg.gravityScaleNormal);
            }
        } else {
            // ② 下降中（ここが落下のホバリング計算コードです）
            float currentY = enemy->GetWorldPosition().y;
            float groundY = enemy->GetGroundY();
            float targetLandingY = groundY + 0.5f;
            float heightFromGround = currentY - targetLandingY;

            if (heightFromGround > cfg.hoverLimitHeight) {
                enemy->SetGravityScale(cfg.gravityScaleFloating);
            } else {
                enemy->SetGravityScale(cfg.gravityScaleNormal);
            }
        }
    }
    enemy->UpdateGravity();
}

void EnemyBehaviorBounce::Finalize(Enemy* enemy)
{}
