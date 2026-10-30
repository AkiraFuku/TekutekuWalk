#include "ScoreUI.h"
#include "Player.h"
#include "Sprite.h"
#include "WinApp.h"
#include "NumberTex.h"
void ScoreUI::Initialize(Player* player)
{ 
    player_ = player;
    
    // 1. 背景スプライトの初期化と右上配置
    ScoreSprite_ = std::make_unique<Sprite>();
    ScoreSprite_->Initialize("resources/white.png");
    ScoreSprite_->SetColor({ 0.1f, 0.1f, 0.1f, 0.6f });
    ScoreSprite_->SetAnchorPoint({ Anchor::TopRight });

    // 画面右端からマージンを引いた座標を算出
    float posX = static_cast<float>(WinApp::kClientWidth) - kMarginRight;
    position_ = { posX, kDefaultPositionTop };
    
    ScoreSprite_->SetPosition(position_);
    ScoreSprite_->SetSize({ ScoreSpriteWidth_ * scale_, kDefaultCounterSize * scale_ });

    // 2. 数字用スプライトの雛形を作成
    baseCounterSprite_ = std::make_unique<Sprite>();
    
    // 0〜9の数字画像を配列としてまとめて登録
    baseCounterSprite_->RegisterTextures(numberTextureRegistration());

    baseCounterSprite_->SetSize({ counterSize_ * scale_, counterSize_ * scale_ });
    baseCounterSprite_->SetAnchorPoint({ Anchor::TopRight });

    // 3. 2桁分のスプライトを生成[cite: 5, 6]
    for (int i = 0; i < kMaxDigits; i++)
    {
        auto counterSprite = baseCounterSprite_->Clone();
        
        // 桁位置に応じてX座標を右から左へ並べる
        float digitPosX = position_.x + counterOffset_.x + (i * counterSpacing_ * scale_);
        float digitPosY = position_.y + counterOffset_.y;
        
        counterSprite->SetPosition({ digitPosX, digitPosY });
        CounterSprite_.push_back(std::move(counterSprite));
    }
    currentScore = 0; // 初期スコアを0に設定
}

void ScoreUI::Update()
{
    if (!player_) return;

    // 1. プレイヤーから現在のスコアを取得
    //= player_->GetScore(); 

    // 2. スコアを指定値（100）でリセットする（0〜99の範囲に丸める）
    currentScore = currentScore % kScoreResetBoundary;
    if (currentScore < 0) {
        currentScore = 0;
    }

    // 3. 各桁の数値を10進数基準で分解[cite: 5]
    int32_t tenDigit = currentScore / kRadixValue; // 10の位
    int32_t oneDigit = currentScore % kRadixValue; // 1の位

    // 各スプライトに対応する数字テクスチャインデックスを適用[cite: 5]
    CounterSprite_[kTensPlace]->SetTextureByIndex(static_cast<size_t>(tenDigit));
    CounterSprite_[kOnesPlace]->SetTextureByIndex(static_cast<size_t>(oneDigit));

    // 4. 行列等の更新処理[cite: 3]
    ScoreSprite_->Update();
    for (auto& counterSprite : CounterSprite_)
    {
        counterSprite->Update();
    }
}

void ScoreUI::Draw()
{
    if (!this) return;

    ScoreSprite_->Draw();

    for (auto& counterSprite : CounterSprite_)
    {
        counterSprite->Draw();
    }
}