#pragma once
#include "Vector2.h"
#include <memory>
#include <vector>
class Player;
class Sprite;

enum HPUIState
{
    ACTIVE,// HP UIがアクティブな状態
    INACTIVE,// HP UIが非アクティブな状態
};

/// <summary>
/// プレイヤー形態マークの種類
/// </summary>
enum class StateMarkType
{
    Walk,   // 通常時  : WarlkMark
    Jump,   // バウンドロボット形態 : JumpMark
    Bullet, // テストロボット形態  : BulletMark
};

/// <summary>
/// プレイヤーのHPを表示するUIクラス
/// </summary>
class PlayerHPUI
{
public:
    void Initialize( Player* player);
    void Update();
    void Draw();

    void SetPlayer(Player* player) {
        player_ = player;
    }


    //セッター
    void SetPosition(const Vector2& position) {
        position_ = position;
    }
    
    void SetCounterOffset(const Vector2& offset) {
        counterOffset_ = offset;
    }
    void SetCounterSpacing(float spacing) {
        counterSpacing_ = spacing;
    }
    void SetCounterSize(const Vector2& size) {
        counterSize_ = size;
    }
    void SetCounterSize(float size) {
        counterSize_ = { size, size };
    }
    void SetHPSpriteWidth(float width) {
        hpSpriteWidth_ = width;
    }
    // ゲッター
    const Vector2& GetPosition() const {
        return position_;
    }

    const Vector2& GetCounterOffset() const {
        return counterOffset_;
    }
    float GetCounterSpacing() const {
        return counterSpacing_;
    }
    const Vector2& GetCounterSize() const {
        return counterSize_;
    }
    float GetHPSpriteWidth() const {
        return hpSpriteWidth_;
    }

private:

    Vector2 position_ = { 0.0f, 630.0f }; // 下部バー基準位置
    float scale_ = 1.0f;                  // HP UIのスケール

    //プレイヤー
    Player* player_ = nullptr;
    //HP下部バーの背景スプライト（木目調オーカー）
    std::unique_ptr<Sprite> hpSprite_ = nullptr;
    //下部バー上部の木枠トリムラインスプライト
    std::unique_ptr<Sprite> barTrimSprite_ = nullptr;
    //右側：形態（能力）表示カード背景スプライト
    std::unique_ptr<Sprite> stateCardSprite_ = nullptr;
    //プレイヤーの形態状態の把握用スプライト（Markアイコン表示）
    std::unique_ptr<Sprite> stateSprite_ = nullptr;

    // Markテクスチャのインデックス
    size_t markTexIndexWalk_   = 0; // WarlkMark
    size_t markTexIndexJump_   = 1; // JumpMark
    size_t markTexIndexBullet_ = 2; // BulletMark

    // Markアイコンのサイズ
    float markSize_ = 60.0f;

    //HPのカウンター用スプライト
    std::unique_ptr<Sprite> baseCounterSprite_ = nullptr;
    std::vector<std::unique_ptr<Sprite>> CounterSprite_ ;
    // 追加：テクスチャリスト内のインデックスを保持
    size_t texIndexFull_ = ACTIVE;  // HPが有る時のインデックス
    size_t texIndexEmpty_ = INACTIVE; // HPが空の時のインデックス

    Vector2 counterOffset_ = { 155.0f, 18.0f }; // カウンタースプライトのオフセット位置
    float counterSpacing_ = 48.0f;              // カウンタースプライト間のスペース
    Vector2 counterSize_ = { 40.0f, 54.0f };    // カウンタースプライトのサイズ
    float hpSpriteWidth_ = 1280.0f;             // HP UIの幅

};

