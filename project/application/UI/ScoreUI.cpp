#include "ScoreUI.h"
#include "NumberTex.h"
#include "Player.h"
#include "Sprite.h"
#include "WinApp.h"

void ScoreUI::Initialize(Player *player) {
  player_ = player;

  // 1. カービィ64風 左下スコア枠（濃い茶色の木枠・プレート）
  ScoreSprite_ = std::make_unique<Sprite>();
  ScoreSprite_->Initialize("resources/white.png");
  ScoreSprite_->SetColor({0.22f, 0.13f, 0.06f, 0.95f});
  ScoreSprite_->SetAnchorPoint({Anchor::TopLeft});

  position_ = {kDefaultPositionX, kDefaultPositionY};
  ScoreSprite_->SetPosition(position_);
  ScoreSprite_->SetSize(
      {ScoreSpriteWidth_ * scale_, kDefaultScoreSpriteHeight * scale_});

  // 2. 数字用スプライトの雛形を作成
  baseCounterSprite_ = std::make_unique<Sprite>();
  baseCounterSprite_->RegisterTextures(numberTextureRegistration());
  baseCounterSprite_->SetSize({counterWidth_ * scale_, counterHeight_ * scale_});
  baseCounterSprite_->SetAnchorPoint({Anchor::TopLeft});

  // 3. 2桁分のスプライトを生成
  CounterSprite_.clear();
  for (int i = 0; i < kMaxDigits; i++) {
    auto counterSprite = baseCounterSprite_->Clone();

    // 桁位置に応じてX座標を左から右へ並べる（10の位、1の位）
    float digitPosX =
        position_.x + counterOffset_.x + (i * counterSpacing_ * scale_);
    float digitPosY = position_.y + counterOffset_.y;

    counterSprite->SetPosition({digitPosX, digitPosY});
    CounterSprite_.push_back(std::move(counterSprite));
  }
  currentScore = 0; // 初期スコアを0に設定
}

void ScoreUI::Update() {
  if (!player_)
    return;

  // 1. スコアを指定値（100）でリセットする（0〜99の範囲に丸める）
  currentScore = currentScore % kScoreResetBoundary;
  if (currentScore < 0) {
    currentScore = 0;
  }

  // 2. 各桁の数値を10進数基準で分解
  int32_t tenDigit = currentScore / kRadixValue; // 10の位
  int32_t oneDigit = currentScore % kRadixValue; // 1の位

  // 各スプライトに対応する数字テクスチャインデックスを適用
  CounterSprite_[kTensPlace]->SetTextureByIndex(static_cast<size_t>(tenDigit));
  CounterSprite_[kOnesPlace]->SetTextureByIndex(static_cast<size_t>(oneDigit));

  // 3. 行列等の更新処理
  ScoreSprite_->Update();
  for (auto &counterSprite : CounterSprite_) {
    counterSprite->Update();
  }
}

void ScoreUI::Draw() {
  if (!this)
    return;

  ScoreSprite_->Draw();

  for (auto &counterSprite : CounterSprite_) {
    counterSprite->Draw();
  }
}
