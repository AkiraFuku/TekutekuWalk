#pragma once
#include <queue>
#include <vector>
#include <memory>
#include <string>
#include "Vector2.h"
#include "Vector3.h"
#include "Sprite.h"

class Camera;

/// <summary>
/// スライド準拠：キューに待機させる表示要求
/// </summary>
struct DamagePopupRequest {
    int damage = 0;
    int x = 0;
    int y = 0;
};

/// <summary>
/// 画面上に現在表示されているアクティブなダメージ表示スプライト
/// </summary>
struct ActiveDamagePopup {
    int damage = 0;
    Vector2 position = { 0.0f, 0.0f };
    int remainingFrames = 60; // 表示持続フレーム（60フレーム）
    int maxFrames = 60;
    std::vector<std::unique_ptr<Sprite>> digitSprites;
};

/// <summary>
/// ダメージポップアップ管理クラス（FIFOキュー制御 & ImGuiデバッグ）
/// </summary>
class DamagePopupManager {
public:
    static DamagePopupManager* GetInstance();

    void Initialize();
    void Update();
    void Draw();

    void AddDamagePopup(int damage, int x, int y);
    void AddDamagePopupFromWorld(int damage, const Vector3& worldPos, Camera* camera);
    void DrawImGui();
    void Clear();

    void SetIntervalFrames(int frames) { intervalFrames_ = frames; }
    int GetIntervalFrames() const { return intervalFrames_; }

private:
    DamagePopupManager() = default;
    ~DamagePopupManager() = default;
    DamagePopupManager(const DamagePopupManager&) = delete;
    DamagePopupManager& operator=(const DamagePopupManager&) = delete;

    void Display();

    static constexpr float kDigitSize = 36.0f;    // 数字スプライトの基本サイズ
    static constexpr float kDigitSpacing = 24.0f; // 数字の間隔

    int intervalFrames_ = 15; // デフォルト間隔（15フレーム）
    int waitFrames_ = 0;      // 残り待機フレーム
    std::queue<DamagePopupRequest> damagePopups_;
    std::vector<ActiveDamagePopup> activePopups_;

    std::unique_ptr<Sprite> baseDigitSprite_;
};
