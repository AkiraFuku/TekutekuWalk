#include "DamagePopupManager.h"
#include "Camera.h"
#include "NumberTex.h"
#include "WinApp.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <iostream>

DamagePopupManager *DamagePopupManager::GetInstance() {
  static DamagePopupManager instance;
  return &instance;
}

void DamagePopupManager::Initialize() {
  Clear();
  intervalFrames_ = 15;
  waitFrames_ = 0;

  baseDigitSprite_ = std::make_unique<Sprite>();
  baseDigitSprite_->RegisterTextures(numberTextureRegistration());
  baseDigitSprite_->SetSize({kDigitSize, kDigitSize});
  baseDigitSprite_->SetAnchorPoint(Anchor::Center);
}

void DamagePopupManager::Clear() {
  while (!damagePopups_.empty()) {
    damagePopups_.pop();
  }
  activePopups_.clear();
  waitFrames_ = 0;
}

void DamagePopupManager::AddDamagePopup(int damage, int x, int y) {
  DamagePopupRequest popup{damage, x, y};
  damagePopups_.push(popup);

  if (damagePopups_.size() == 1 && waitFrames_ <= 0) {
    Display();
    waitFrames_ = intervalFrames_;
  } else {
    std::cout << "[DamagePopup] Request Queued: Damage=" << damage << ", Pos=("
              << x << ", " << y << "), QueueCount=" << damagePopups_.size()
              << std::endl;
  }
}

void DamagePopupManager::AddDamagePopupFromWorld(int damage,
                                                 const Vector3 &worldPos,
                                                 Camera *camera) {
  int screenX = WinApp::kClientWidth / 2;
  int screenY = WinApp::kClientHeight / 2;

  if (camera) {
    const Matrix4x4 &mat = camera->GetViewProtectionMatrix();
    float x = worldPos.x * mat.m[0][0] + worldPos.y * mat.m[1][0] +
              worldPos.z * mat.m[2][0] + mat.m[3][0];
    float y = worldPos.x * mat.m[0][1] + worldPos.y * mat.m[1][1] +
              worldPos.z * mat.m[2][1] + mat.m[3][1];
    float w = worldPos.x * mat.m[0][3] + worldPos.y * mat.m[1][3] +
              worldPos.z * mat.m[2][3] + mat.m[3][3];

    if (std::abs(w) > 0.0001f) {
      float ndcX = x / w;
      float ndcY = y / w;
      screenX = static_cast<int>((ndcX + 1.0f) * 0.5f *
                                 static_cast<float>(WinApp::kClientWidth));
      screenY = static_cast<int>((1.0f - ndcY) * 0.5f *
                                 static_cast<float>(WinApp::kClientHeight));
    }
  }

  AddDamagePopup(damage, screenX, screenY);
}

void DamagePopupManager::Display() {
  if (damagePopups_.empty()) {
    return;
  }

  DamagePopupRequest popup = damagePopups_.front();

  std::cout << "[DamagePopup] Display Executed: Dealt " << popup.damage
            << " damage! Pos=(" << popup.x << ", " << popup.y << ")"
            << std::endl;

  if (baseDigitSprite_) {
    ActiveDamagePopup active;
    active.damage = popup.damage;
    active.position = {static_cast<float>(popup.x),
                       static_cast<float>(popup.y)};
    active.maxFrames = 60;
    active.remainingFrames = 60;

    std::string damageStr = std::to_string((std::max)(0, popup.damage));
    float totalWidth = static_cast<float>(damageStr.length()) * kDigitSpacing;
    float startX =
        active.position.x - (totalWidth * 0.5f) + (kDigitSpacing * 0.5f);

    for (size_t i = 0; i < damageStr.length(); ++i) {
      int digit = damageStr[i] - '0';
      auto sprite = baseDigitSprite_->Clone();
      sprite->SetTextureByIndex(static_cast<size_t>(digit));
      sprite->SetPosition({startX + (static_cast<float>(i) * kDigitSpacing),
                           active.position.y});
      sprite->SetColor({1.0f, 0.45f, 0.1f, 1.0f});
      sprite->Update();
      active.digitSprites.push_back(std::move(sprite));
    }

    activePopups_.push_back(std::move(active));
  }

  damagePopups_.pop();
}

void DamagePopupManager::Update() {
  if (waitFrames_ > 0) {
    --waitFrames_;
  }

  if (!damagePopups_.empty()) {
    if (waitFrames_ <= 0) {
      Display();
      waitFrames_ = intervalFrames_;
    }
  }

  // 表示終了したスプライトをリストから削除（ImGui上からも即座に消去されます）
  for (auto it = activePopups_.begin(); it != activePopups_.end();) {
    --it->remainingFrames;
    if (it->remainingFrames <= 0) {
      it = activePopups_.erase(it);
    } else {
      it->position.y -= 0.6f;
      float alpha = static_cast<float>(it->remainingFrames) /
                    static_cast<float>(it->maxFrames);

      std::string damageStr = std::to_string((std::max)(0, it->damage));
      float totalWidth = static_cast<float>(damageStr.length()) * kDigitSpacing;
      float startX =
          it->position.x - (totalWidth * 0.5f) + (kDigitSpacing * 0.5f);

      for (size_t i = 0; i < it->digitSprites.size(); ++i) {
        it->digitSprites[i]->SetPosition(
            {startX + (static_cast<float>(i) * kDigitSpacing), it->position.y});
        it->digitSprites[i]->SetColor({1.0f, 0.45f, 0.1f, alpha});
        it->digitSprites[i]->Update();
      }
      ++it;
    }
  }
}

void DamagePopupManager::Draw() {
  for (auto &active : activePopups_) {
    for (auto &sprite : active.digitSprites) {
      sprite->Draw();
    }
  }
}

void DamagePopupManager::DrawImGui() {
#ifdef USE_IMGUI
  ImGui::Begin("Damage Popup Debug");

  // --- [1] キュー内の待機中スプライト (Queue) ---
  ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
                     "=== [1] Queue: Waiting Popups ===");
  ImGui::Text("Pending Count: %zu items", damagePopups_.size());
  if (intervalFrames_ > 0) {
    ImGui::Text("Next Display in: %d / %d frames", waitFrames_,
                intervalFrames_);
    ImGui::ProgressBar(1.0f - (static_cast<float>(waitFrames_) /
                               static_cast<float>(intervalFrames_)),
                       ImVec2(0, 0));
  }

  if (damagePopups_.empty()) {
    ImGui::TextDisabled("  (No waiting popups in queue)");
  } else {
    std::queue<DamagePopupRequest> tempQueue = damagePopups_;
    int queueIndex = 1;
    while (!tempQueue.empty()) {
      const auto &req = tempQueue.front();
      ImGui::BulletText("Wait #%d | Damage: %4d | Pos: (X: %4d, Y: %4d)",
                        queueIndex, req.damage, req.x, req.y);
      tempQueue.pop();
      ++queueIndex;
    }
  }

  ImGui::Separator();

  // --- [2] 画面に表示中のスプライト (Active) ---
  // ※表示終了したものはリストから消去されるため、ここに残りません
  ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                     "=== [2] Active: Displaying Popups ===");
  ImGui::Text("Active Count: %zu sprites", activePopups_.size());
  if (activePopups_.empty()) {
    ImGui::TextDisabled("  (No active popups on screen)");
  } else {
    for (size_t i = 0; i < activePopups_.size(); ++i) {
      const auto &active = activePopups_[i];
      ImGui::BulletText(
          "Sprite #%zu | Damage: %4d | Pos: (X: %6.1f, Y: %6.1f) | Life: %2d F",
          i + 1, active.damage, active.position.x, active.position.y,
          active.remainingFrames);
    }
  }

  ImGui::Separator();

  // --- [3] 設定 & テストボタン ---
  ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f),
                     "=== [3] Controls & Settings ===");
  ImGui::SliderInt("Interval Frames", &intervalFrames_, 1, 60);

  if (ImGui::Button("Slide Demo (100 -> 50 -> 25)")) {
    AddDamagePopup(100, 640, 300);
    AddDamagePopup(50, 640, 300);
    AddDamagePopup(25, 640, 300);
  }
  ImGui::SameLine();
  if (ImGui::Button("Multi Enemy Demo")) {
    AddDamagePopup(100, 480, 320);
    AddDamagePopup(50, 800, 320);
    AddDamagePopup(25, 480, 320);
  }

  static int testDamage = 25;
  static int testPos[2] = {640, 360};
  ImGui::SliderInt("Test Damage", &testDamage, 1, 999);
  ImGui::SliderInt2("Test Pos (X, Y)", testPos, 0, 1280);
  if (ImGui::Button("Add Custom Damage")) {
    AddDamagePopup(testDamage, testPos[0], testPos[1]);
  }

  ImGui::End();
#endif
}
