#pragma once
#include "Vector2.h"
#include <memory>
#include <vector>
#include <string>

class Player;
class Sprite;

class ScoreUI
{
public:
    void Initialize(Player* player);
    void Update();
    void Draw();

    void SetPlayer(Player* player) { player_ = player; }

    // セッター
    void SetPosition(const Vector2& position) { position_ = position; }
    void SetCounterOffset(const Vector2& offset) { counterOffset_ = offset; }
    void SetCounterSpacing(float spacing) { counterSpacing_ = spacing; }
    void SetCounterSize(float width, float height) { counterWidth_ = width; counterHeight_ = height; }
    void SetCounterSize(float size) { counterWidth_ = size; counterHeight_ = size; }
    void SetScoreSpriteWidth(float width) { ScoreSpriteWidth_ = width; }

    // ゲッター
    const Vector2& GetPosition() const { return position_; }
    const Vector2& GetCounterOffset() const { return counterOffset_; }
    float GetCounterSpacing() const { return counterSpacing_; }
    float GetCounterWidth() const { return counterWidth_; }
    float GetCounterHeight() const { return counterHeight_; }
    float GetCounterSize() const { return counterWidth_; }
    float GetScoreSpriteWidth() const { return ScoreSpriteWidth_; }

    void AddScore(int32_t score=1) {
        currentScore += score;
    }

private:
    // ---- 配置・デザイン用定数 ----
    static constexpr float kDefaultScale = 1.0f;              // UIの拡大率
    static constexpr float kDefaultPositionX = 35.0f;          // 画面左端からの配置位置
    static constexpr float kDefaultPositionY = 642.0f;         // 画面上部からの配置位置
    static constexpr float kDefaultScoreSpriteWidth = 100.0f;  // スコアUIの背景幅
    static constexpr float kDefaultScoreSpriteHeight = 66.0f; // スコアUIの背景高さ
    static constexpr float kDefaultCounterWidth = 38.0f;       // 数字の幅
    static constexpr float kDefaultCounterHeight = 50.0f;      // 数字の高さ
    static constexpr float kDefaultCounterSpacing = 42.0f;     // 数字ごとの間隔

    // ---- スコアロジック用定数 ----
    static constexpr int32_t kMaxDigits = 2;                  // 表示する桁数[cite: 6]
    static constexpr int32_t kScoreResetBoundary = 100;       // リセット対象となる数値（100でリセット）
    static constexpr int32_t kRadixValue = 10;                 // 10進数の基数

    // 各桁のインデックス用列挙型
    enum DigitIndex {
        kTensPlace = 0, // 10の位
        kOnesPlace = 1, // 1の位
    };

    // ---- メンバ変数 ----
    Vector2 position_ = { 0.0f, 0.0f };
    float scale_ = kDefaultScale;

    // プレイヤー
    Player* player_ = nullptr;
    
    // 背景スプライト用
    std::unique_ptr<Sprite> ScoreSprite_ = nullptr;
    std::unique_ptr<Sprite> baseCounterSprite_ = nullptr;
    
    // スコアのカウンタースプライト用
    std::vector<std::unique_ptr<Sprite>> CounterSprite_;
    
    Vector2 counterOffset_ = { 10.0f, 8.0f }; // 左上基準からの数字のオフセット位置
    float counterSpacing_ = kDefaultCounterSpacing;
    float counterWidth_ = kDefaultCounterWidth;
    float counterHeight_ = kDefaultCounterHeight;
    float ScoreSpriteWidth_ = kDefaultScoreSpriteWidth;
    int32_t currentScore = 0;
};