#include "PlayerHPUI.h"
#include "Input.h"
#include "Player.h"
#include "PlayerState.h"
#include "Sprite.h"
#include "TextureManager.h"
#include <algorithm>
#include <filesystem>
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

  // 4. 操作・武器アイコンUI（移動・ジャンプ・ダッシュ・近接攻撃・マシンガン）
  wasMoving_ = player_->IsMoving();
  InitializeWeaponSlot(
      moveIcon_, {500.0f, 50.0f}, "resources/UI/MoveIcon.png",
      "resources/UI/BtnKeyAD.png", "resources/UI/BtnPadLStick.png", 1.0f);

  wasAirborne_ = !player_->IsGround();
  wasHoverIconActive_ = player_->IsHoverIconActive();
  InitializeWeaponSlot(
      jumpIcon_, {596.0f, 50.0f}, "resources/UI/JumpIcon.png",
      "resources/UI/BtnKeySpace.png", "resources/UI/BtnPadA.png",
      player_->GetHoverEnergyRatio());
  jumpIcon_.iconTexIndexDefault = 0;
  if (std::filesystem::exists("resources/UI/HoverIcon.png")) {
    jumpIcon_.iconTexIndexAlt =
        jumpIcon_.iconSprite->RegisterTexture("resources/UI/HoverIcon.png");
  } else {
    jumpIcon_.iconTexIndexAlt = jumpIcon_.iconTexIndexDefault;
  }

  wasDashStepping_ = player_->IsDashStepActive();
  InitializeWeaponSlot(
      dashIcon_, {692.0f, 50.0f}, "resources/UI/DashIcon.png",
      "resources/UI/BtnKeyShift.png", "resources/UI/BtnPadRB.png",
      player_->GetDashStepRecoveryRatio());

  wasMeleeAttacking_ = player_->IsAttacking();
  wasDashAttackIcon_ = player_->IsDashing() || player_->IsDashAttackActive();
  InitializeWeaponSlot(
      meleeIcon_, {788.0f, 50.0f}, "resources/UI/MeleeIcon.png",
      "resources/UI/BtnMouseLeft.png", "resources/UI/BtnPadB.png",
      player_->GetMeleeRecoveryRatio());
  meleeIcon_.iconTexIndexDefault = 0;
  if (std::filesystem::exists("resources/UI/DashAttackIcon.png")) {
    meleeIcon_.iconTexIndexAlt =
        meleeIcon_.iconSprite->RegisterTexture("resources/UI/DashAttackIcon.png");
  } else {
    meleeIcon_.iconTexIndexAlt = meleeIcon_.iconTexIndexDefault;
  }

  prevShootEnergyRatio_ = player_->GetShootEnergyRatio();
  wasShootOverheated_ = player_->IsShootOverheated();
  InitializeWeaponSlot(
      shootIcon_, {884.0f, 50.0f}, "resources/UI/MachineGunIcon.png",
      "resources/UI/BtnMouseRight.png", "resources/UI/BtnPadX.png",
      prevShootEnergyRatio_);
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

  // --- 操作・武器アイコンUI更新 ---
  const float dt = player_->GetDeltaTime();

  // 移動
  const bool isMoving = player_->IsMoving();
  const bool movePop = (!wasMoving_ && isMoving);
  wasMoving_ = isMoving;
  UpdateWeaponSlot(moveIcon_, dt, 1.0f, false, movePop, false);

  // ジャンプ / ホバージャンプ（ホバー発動条件を満たすかホバー中は専用アイコンへ切替＆ゲージ消費）
  const bool isAirborne = !player_->IsGround();
  const float hoverRatio =
      std::clamp(player_->GetHoverEnergyRatio(), 0.0f, 1.0f);
  const bool isHoverIcon = player_->IsHoverIconActive();
  const bool hoverIconSwitched = (wasHoverIconActive_ != isHoverIcon);
  if (hoverIconSwitched && jumpIcon_.iconSprite) {
    jumpIcon_.isIconAlt = isHoverIcon;
    jumpIcon_.iconSprite->SetTextureByIndex(
        isHoverIcon ? jumpIcon_.iconTexIndexAlt
                    : jumpIcon_.iconTexIndexDefault);
  }
  wasHoverIconActive_ = isHoverIcon;
  const bool jumpPop = (wasAirborne_ != isAirborne) || hoverIconSwitched;
  wasAirborne_ = isAirborne;
  UpdateWeaponSlot(jumpIcon_, dt, hoverRatio, (hoverRatio <= 0.0f), jumpPop,
                   true);

  // ダッシュ / ステップ（ステップ中は進捗に応じて貯まり、終了で有効化＋ポップ）
  const bool isDashStep = player_->IsDashStepActive();
  const float dashRatio =
      std::clamp(player_->GetDashStepRecoveryRatio(), 0.0f, 1.0f);
  const bool dashPop = (wasDashStepping_ != isDashStep);
  wasDashStepping_ = isDashStep;
  UpdateWeaponSlot(dashIcon_, dt, dashRatio, isDashStep, dashPop, false);

  // 近接攻撃 / ダッシュ攻撃（ダッシュ中はダッシュ攻撃アイコンへ切替）
  const bool isMeleeAttacking = player_->IsAttacking();
  const float meleeRatio =
      std::clamp(player_->GetMeleeRecoveryRatio(), 0.0f, 1.0f);
  const bool isDashAttackIcon =
      player_->IsDashing() || player_->IsDashAttackActive();
  const bool meleeIconSwitched = (wasDashAttackIcon_ != isDashAttackIcon);
  if (meleeIconSwitched && meleeIcon_.iconSprite) {
    meleeIcon_.isIconAlt = isDashAttackIcon;
    meleeIcon_.iconSprite->SetTextureByIndex(
        isDashAttackIcon ? meleeIcon_.iconTexIndexAlt
                         : meleeIcon_.iconTexIndexDefault);
  }
  wasDashAttackIcon_ = isDashAttackIcon;
  const bool meleePop =
      (wasMeleeAttacking_ != isMeleeAttacking) || meleeIconSwitched;
  wasMeleeAttacking_ = isMeleeAttacking;
  UpdateWeaponSlot(meleeIcon_, dt, meleeRatio, isMeleeAttacking, meleePop,
                   false);

  // マシンガン
  const float shootRatio =
      std::clamp(player_->GetShootEnergyRatio(), 0.0f, 1.0f);
  const bool overheated = player_->IsShootOverheated();
  const bool shootPop = (shootRatio < prevShootEnergyRatio_ - 0.001f) ||
                        (wasShootOverheated_ && !overheated);
  prevShootEnergyRatio_ = shootRatio;
  wasShootOverheated_ = overheated;
  UpdateWeaponSlot(shootIcon_, dt, shootRatio, overheated, shootPop, true);
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

  DrawWeaponSlot(moveIcon_, 1.0f);
  DrawWeaponSlot(jumpIcon_, player_->GetHoverEnergyRatio());
  DrawWeaponSlot(dashIcon_, player_->GetDashStepRecoveryRatio());
  DrawWeaponSlot(meleeIcon_, player_->GetMeleeRecoveryRatio());
  DrawWeaponSlot(shootIcon_, player_->GetShootEnergyRatio());
}

void PlayerHPUI::InitializeWeaponSlot(WeaponIconSlot &slot,
                                      const Vector2 &centerOffset,
                                      const std::string &iconPath,
                                      const std::string &keyMouseBtnPath,
                                      const std::string &gamepadBtnPath,
                                      float initialRatio) {
  slot.centerOffset = centerOffset;
  slot.scale = 1.0f;

  const Vector2 iconCenter = {position_.x + slot.centerOffset.x,
                              position_.y + slot.centerOffset.y};
  const float clampedRatio = std::clamp(initialRatio, 0.0f, 1.0f);

  // 1. 後ろの暗い下地枠
  slot.cardBgSprite = std::make_unique<Sprite>();
  slot.cardBgSprite->Initialize("resources/white.png");
  slot.cardBgSprite->SetColor({0.18f, 0.18f, 0.20f, 0.90f});
  slot.cardBgSprite->SetAnchorPoint(Anchor::Center);
  slot.cardBgSprite->SetPosition(iconCenter);
  slot.cardBgSprite->SetSize(weaponIconCardSize_);

  // 2. 後ろの白いゲージ本体（下基準で高さが ratio に応じて増減）
  slot.cardFillSprite = std::make_unique<Sprite>();
  slot.cardFillSprite->Initialize("resources/white.png");
  slot.cardFillSprite->SetColor({0.96f, 0.96f, 0.96f, 0.95f});
  slot.cardFillSprite->SetAnchorPoint(Anchor::BottomMin);
  slot.cardFillSprite->SetPosition(
      {iconCenter.x, iconCenter.y + weaponIconCardSize_.y * 0.5f});
  slot.cardFillSprite->SetSize(
      {weaponIconCardSize_.x, weaponIconCardSize_.y * clampedRatio});

  // 3. 手前の武器アイコン本体（常にフルサイズで表示）
  slot.iconSprite = std::make_unique<Sprite>();
  slot.iconSprite->Initialize(iconPath);
  slot.iconSprite->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
  slot.iconSprite->SetAnchorPoint(Anchor::Center);
  slot.iconSprite->SetPosition(iconCenter);
  slot.iconSprite->SetSize(weaponIconBaseSize_);

  // 4. 上部の発動ボタンマーク
  slot.buttonMarkSprite = std::make_unique<Sprite>();
  slot.buttonMarkSprite->Initialize(keyMouseBtnPath);
  slot.btnTexIndexKeyMouse = 0;
  slot.btnTexIndexGamepad =
      slot.buttonMarkSprite->RegisterTexture(gamepadBtnPath);
  slot.buttonMarkSprite->SetAnchorPoint(Anchor::Center);
  slot.buttonMarkSprite->SetPosition(
      {iconCenter.x + weaponBtnMarkOffset_.x,
       iconCenter.y + weaponBtnMarkOffset_.y});
  slot.buttonMarkSprite->SetSize(weaponBtnMarkSize_);

  slot.isBtnGamepad = Input::GetInstance()->IsUsingGamepad();
  slot.buttonMarkSprite->SetTextureByIndex(
      slot.isBtnGamepad ? slot.btnTexIndexGamepad : slot.btnTexIndexKeyMouse);
  slot.buttonMarkSprite->SetSize(weaponBtnMarkSize_);
}

void PlayerHPUI::UpdateWeaponSlot(WeaponIconSlot &slot, float dt, float ratio,
                                  bool isCooldown, bool triggerPop,
                                  bool lowEnergyWarning) {
  if (triggerPop) {
    slot.scale = 1.18f;
  }
  slot.scale += (1.0f - slot.scale) * std::min(1.0f, dt * 12.0f);

  const float clampedRatio = std::clamp(ratio, 0.0f, 1.0f);
  const Vector2 iconCenter = {position_.x + slot.centerOffset.x,
                              position_.y + slot.centerOffset.y};
  const Vector2 scaledCardSize = {weaponIconCardSize_.x * slot.scale,
                                  weaponIconCardSize_.y * slot.scale};
  const Vector2 scaledIconSize = {weaponIconBaseSize_.x * slot.scale,
                                  weaponIconBaseSize_.y * slot.scale};

  const Vector4 cardFillColor =
      isCooldown
          ? Vector4{0.55f, 0.55f, 0.58f, 0.92f}
          : ((lowEnergyWarning && clampedRatio <= 0.30f)
                 ? Vector4{1.0f, 0.80f, 0.20f, 0.95f}
                 : Vector4{0.96f, 0.96f, 0.96f, 0.95f});
  const Vector4 iconColor = isCooldown
                                ? Vector4{0.48f, 0.48f, 0.52f, 0.95f}
                                : Vector4{1.0f, 1.0f, 1.0f, 1.0f};
  const Vector4 markColor = isCooldown
                                ? Vector4{0.50f, 0.50f, 0.53f, 0.90f}
                                : Vector4{1.0f, 1.0f, 1.0f, 1.0f};

  if (slot.cardBgSprite) {
    slot.cardBgSprite->SetPosition(iconCenter);
    slot.cardBgSprite->SetSize(scaledCardSize);
    slot.cardBgSprite->Update();
  }
  if (slot.cardFillSprite) {
    slot.cardFillSprite->SetPosition(
        {iconCenter.x, iconCenter.y + scaledCardSize.y * 0.5f});
    slot.cardFillSprite->SetSize(
        {scaledCardSize.x, scaledCardSize.y * clampedRatio});
    slot.cardFillSprite->SetColor(cardFillColor);
    slot.cardFillSprite->Update();
  }
  if (slot.iconSprite) {
    slot.iconSprite->SetPosition(iconCenter);
    slot.iconSprite->SetSize(scaledIconSize);
    slot.iconSprite->SetColor(iconColor);
    slot.iconSprite->Update();
  }
  if (slot.buttonMarkSprite) {
    const bool usePad = Input::GetInstance()->IsUsingGamepad();
    if (usePad != slot.isBtnGamepad) {
      slot.isBtnGamepad = usePad;
      slot.buttonMarkSprite->SetTextureByIndex(
          slot.isBtnGamepad ? slot.btnTexIndexGamepad
                            : slot.btnTexIndexKeyMouse);
    }
    slot.buttonMarkSprite->SetPosition(
        {iconCenter.x + weaponBtnMarkOffset_.x * slot.scale,
         iconCenter.y + weaponBtnMarkOffset_.y * slot.scale});
    slot.buttonMarkSprite->SetSize(
        {weaponBtnMarkSize_.x * slot.scale, weaponBtnMarkSize_.y * slot.scale});
    slot.buttonMarkSprite->SetColor(markColor);
    slot.buttonMarkSprite->Update();
  }
}

void PlayerHPUI::DrawWeaponSlot(const WeaponIconSlot &slot, float ratio) const {
  if (slot.cardBgSprite) {
    slot.cardBgSprite->Draw();
  }
  if (slot.cardFillSprite && ratio > 0.001f) {
    slot.cardFillSprite->Draw();
  }
  if (slot.iconSprite) {
    slot.iconSprite->Draw();
  }
  if (slot.buttonMarkSprite) {
    slot.buttonMarkSprite->Draw();
  }
}
