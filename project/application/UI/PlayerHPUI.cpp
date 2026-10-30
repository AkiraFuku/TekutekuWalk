#include "PlayerHPUI.h"
#include "Player.h"
#include "Sprite.h"
#include "PlayerState.h"
#include <string>
void PlayerHPUI::Initialize(Player* player){
    if (!player)
    {
        return;
    }
    //HP UIの背景部分
    player_ = player;
    hpSprite_ = std::make_unique<Sprite>();
    hpSprite_->Initialize("resources/white.png");
    hpSprite_->SetColor({ 0.1f, 0.1f, 0.1f, 0.6f });
    hpSprite_->SetAnchorPoint({ Anchor::TopLeft });
    hpSprite_->SetPosition({ position_.x, position_.y });
    hpSprite_->SetSize({ hpSpriteWidth_ * scale_, 30.0f * scale_ });
    // 
    

     baseCounterSprite_ = std::make_unique<Sprite>();
    // 1. 初期テクスチャ（実HP）で初期化
    baseCounterSprite_->Initialize("resources/HPUI/ActiveHP.png");

    // 2. 空テクスチャを追加登録
    baseCounterSprite_->RegisterTexture("resources//HPUI/DeactiveHP.png");

    // 座標とサイズの設定
    baseCounterSprite_->SetPosition({ position_.x + counterOffset_.x + (counterSpacing_+counterSize_ )* scale_, position_.y + counterOffset_.y });
    baseCounterSprite_->SetSize({ counterSize_ * scale_, counterSize_ * scale_ });
    baseCounterSprite_->SetAnchorPoint({ Anchor::TopLeft });// 中心を基準にする場合


    // プレイヤーの最大HPを取得 (※関数名は実際のPlayerクラスのものに合わせてください)
    int maxHP = player_->GetMaxHitPoints();

    if (maxHP > 0)
    {
        for (int i = 0; i < maxHP; i++)
        {
            // 2. 雛形からコピーを作成
            auto counterSprite = baseCounterSprite_->Clone();
            
            // ステートマーク幅 + 間隔分だけ右にオフセットして配置
            // レイアウト: [MarkIcon] + markOffset_.x(間隔) + [Counter×n]
            float counterStartX = position_.x + counterOffset_.x
                + markSize_ * scale_ + markOffset_.x;
            counterSprite->SetPosition({
                counterStartX + i * (counterSpacing_ + counterSize_) * scale_,
                position_.y + counterOffset_.y });
            CounterSprite_.push_back(std::move(counterSprite));
        }
    }

    // --- Markアイコンスプライトの初期化 ---
    // ステートマークは counterOffset_ の位置（カウンターより左）に配置
    float markX = position_.x + counterOffset_.x;
    float markY = position_.y + counterOffset_.y;

    stateSprite_ = std::make_unique<Sprite>();
    // テクスチャ登録：インデックス0 = WarlkMark（通常時）
    stateSprite_->Initialize("resources/Mark/WarlkMark.png");
    // インデックス1 = JumpMark（バウンドロボット形態）
    stateSprite_->RegisterTexture("resources/Mark/JumpMark.png");
    // インデックス2 = BulletMark（テストロボット形態）
    stateSprite_->RegisterTexture("resources/Mark/BulletMark.png");

    stateSprite_->SetAnchorPoint({ Anchor::TopLeft });
    stateSprite_->SetPosition({ markX, markY });
    stateSprite_->SetSize({ markSize_ * scale_, markSize_ * scale_ });

}

void PlayerHPUI::Update()
{
    if (!player_)
    {
        return;
    }
    int currentHP = player_->GetHitPoints();// 現在のHPを取得
    int maxHP = player_->GetMaxHitPoints();  // 最大のHPを取得
    // もしゲーム中に最大HPが上昇・下降した場合に備え、スプライト数を最大HPに同期させる
    while (CounterSprite_.size() > static_cast<size_t>(maxHP))
    {
        CounterSprite_.pop_back();
    }
    while (CounterSprite_.size() < static_cast<size_t>(maxHP))
    {
        auto counterSprite = baseCounterSprite_->Clone();
        // Mark右端 + 間隔 + i番目の位置
        float counterStartX = position_.x + counterOffset_.x
            + markSize_ * scale_ + markOffset_.x;
        counterSprite->SetPosition({
            counterStartX + CounterSprite_.size() * (counterSpacing_ + counterSize_) * scale_,
            position_.y + counterOffset_.y });
        CounterSprite_.push_back(std::move(counterSprite));
    }

    // 各スプライトのテクスチャを現在HPに応じて切り替える
    for (int i = 0; i < CounterSprite_.size(); i++)
    {
        if (i < currentHP)
        {
            // 現在HP以下のインデックスは「実HP(インデックス0)」
            CounterSprite_[i]->SetTextureByIndex(texIndexFull_);
        } else
        {
            // 現在HPを超えたインデックスは「空HP(インデックス1)」
            CounterSprite_[i]->SetTextureByIndex(texIndexEmpty_);
        }
    }

    hpSprite_->Update(); 

    for (auto& counterSprite : CounterSprite_)
    {
        counterSprite->Update(); 
    }

    // --- Markアイコンの切り替え ---
    if (stateSprite_)
    {
        // プレイヤーの状態名で形態を判定
        const char* stateName = player_->GetState() ? player_->GetState()->GetName() : "Normal";
        std::string name(stateName);

        if (name == "Bound")
        {
            // バウンドロボット形態 → JumpMark
            stateSprite_->SetTextureByIndex(markTexIndexJump_);
        }
        else if (name == "RideOnTest")
        {
            // テストロボット形態 → BulletMark
            stateSprite_->SetTextureByIndex(markTexIndexBullet_);
        }
        else
        {
            // 通常（Normal / Dead など）→ WarlkMark
            stateSprite_->SetTextureByIndex(markTexIndexWalk_);
        }

        stateSprite_->Update();
    }

}

void PlayerHPUI::Draw()
{
    if (!player_)
    {
        return;
    }

    hpSprite_->Draw();

    for (auto& counterSprite : CounterSprite_)
    {
        counterSprite->Draw();
    }

    // --- Markアイコンの描画 ---
    if (stateSprite_)
    {
        stateSprite_->Draw();
    }
}
