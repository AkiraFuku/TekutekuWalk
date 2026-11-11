#include "PlayerHPUI.h"
#include "Player.h"
#include "PlayerState.h"
#include "Sprite.h"
#include <string>

void PlayerHPUI::Initialize(Player *player) {
  if (!player) {
    return;
  }
  player_ = player;

  // 1. カービィ64風 下部UIベースバー（木目調の温かみのあるゴールド・オーカー色）
  hpSprite_ = std::make_unique<Sprite>();
  hpSprite_->Initialize("resources/white.png");
  hpSprite_->SetColor({0.78f, 0.58f, 0.26f, 0.95f});
  hpSprite_->SetAnchorPoint({Anchor::TopLeft});
  hpSprite_->SetPosition({position_.x, position_.y});
  hpSprite_->SetSize({hpSpriteWidth_, 90.0f});

  // バー上部の木枠トリムライン（濃い茶色）
  barTrimSprite_ = std::make_unique<Sprite>();
  barTrimSprite_->Initialize("resources/white.png");
  barTrimSprite_->SetColor({0.32f, 0.20f, 0.10f, 1.0f});
  barTrimSprite_->SetAnchorPoint({Anchor::TopLeft});
  barTrimSprite_->SetPosition({position_.x, position_.y});
  barTrimSprite_->SetSize({hpSpriteWidth_, 4.0f});

  // 2. HPカウンター（卵型アイコン）の雛形
  baseCounterSprite_ = std::make_unique<Sprite>();
  baseCounterSprite_->Initialize("resources/HPUI/ActiveHP.png");
  baseCounterSprite_->RegisterTexture("resources/HPUI/DeactiveHP.png");
  baseCounterSprite_->SetSize({counterSize_.x, counterSize_.y});
  baseCounterSprite_->SetAnchorPoint({Anchor::TopLeft});

  // プレイヤーの最大HPを取得してスプライト生成
  int maxHP = player_->GetMaxHitPoints();
  CounterSprite_.clear();

  float counterStartX = position_.x + counterOffset_.x;
  float counterStartY = position_.y + counterOffset_.y;

  for (int i = 0; i < maxHP; i++) {
    auto counterSprite = baseCounterSprite_->Clone();
    counterSprite->SetPosition(
        {counterStartX + i * counterSpacing_, counterStartY});
    CounterSprite_.push_back(std::move(counterSprite));
  }

  // 3. カービィ64風 右側：コピー能力（形態）表示カード枠（白カード）
  stateCardSprite_ = std::make_unique<Sprite>();
  stateCardSprite_->Initialize("resources/white.png");
  stateCardSprite_->SetColor({0.96f, 0.96f, 0.96f, 0.95f});
  stateCardSprite_->SetAnchorPoint({Anchor::TopLeft});
  stateCardSprite_->SetPosition({1145.0f, position_.y + 12.0f});
  stateCardSprite_->SetSize({95.0f, 66.0f});

  // 形態アイコンスプライト
  stateSprite_ = std::make_unique<Sprite>();
  stateSprite_->Initialize("resources/Mark/WarlkMark.png");
  stateSprite_->RegisterTexture("resources/Mark/JumpMark.png");
  stateSprite_->RegisterTexture("resources/Mark/BulletMark.png");
  stateSprite_->SetAnchorPoint({Anchor::TopLeft});
  stateSprite_->SetPosition({1162.0f, position_.y + 15.0f});
  stateSprite_->SetSize({markSize_, markSize_});
}

void PlayerHPUI::Update() {
  if (!player_) {
    return;
  }
  int currentHP = player_->GetHitPoints(); // 現在のHPを取得
  int maxHP = player_->GetMaxHitPoints();  // 最大のHPを取得

  float counterStartX = position_.x + counterOffset_.x;
  float counterStartY = position_.y + counterOffset_.y;

  while (CounterSprite_.size() > static_cast<size_t>(maxHP)) {
    CounterSprite_.pop_back();
  }
  while (CounterSprite_.size() < static_cast<size_t>(maxHP)) {
    auto counterSprite = baseCounterSprite_->Clone();
    counterSprite->SetPosition(
        {counterStartX + CounterSprite_.size() * counterSpacing_,
         counterStartY});
    CounterSprite_.push_back(std::move(counterSprite));
  }

  // 各スプライトのテクスチャを現在HPに応じて切り替える
  for (size_t i = 0; i < CounterSprite_.size(); i++) {
    if (static_cast<int>(i) < currentHP) {
      CounterSprite_[i]->SetTextureByIndex(texIndexFull_);
    } else {
      CounterSprite_[i]->SetTextureByIndex(texIndexEmpty_);
    }
  }

  hpSprite_->Update();
  if (barTrimSprite_) {
    barTrimSprite_->Update();
  }
  if (stateCardSprite_) {
    stateCardSprite_->Update();
  }

  for (auto &counterSprite : CounterSprite_) {
    counterSprite->Update();
  }

  // --- Markアイコンの切り替え ---
  if (stateSprite_) {
    const char *stateName =
        player_->GetState() ? player_->GetState()->GetName() : "Normal";
    std::string name(stateName);

    if (name == "Bound") {
      stateSprite_->SetTextureByIndex(markTexIndexJump_);
    } else if (name == "RideOnTest") {
      stateSprite_->SetTextureByIndex(markTexIndexBullet_);
    } else {
      stateSprite_->SetTextureByIndex(markTexIndexWalk_);
    }

    stateSprite_->Update();
  }
}

void PlayerHPUI::Draw() {
  if (!player_) {
    return;
  }

  hpSprite_->Draw();
  if (barTrimSprite_) {
    barTrimSprite_->Draw();
  }
  if (stateCardSprite_) {
    stateCardSprite_->Draw();
  }

  for (auto &counterSprite : CounterSprite_) {
    counterSprite->Draw();
  }

  if (stateSprite_) {
    stateSprite_->Draw();
  }
}
