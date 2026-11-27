#pragma once
#include "Vector2.h"
#include "Vector4.h"
#include <memory>
#include <string>
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

    /// <summary>
    /// 武器・スキルアイコンUIの共通スロット
    /// </summary>
    struct WeaponIconSlot {
        std::unique_ptr<Sprite> cardBgSprite = nullptr;     // 後ろの暗い下地枠
        std::unique_ptr<Sprite> cardFillSprite = nullptr;   // 後ろの白いゲージ本体（下基準で増減）
        std::unique_ptr<Sprite> iconSprite = nullptr;       // 手前の武器アイコン本体（常にフル表示）
        std::unique_ptr<Sprite> buttonMarkSprite = nullptr; // 上部の発動ボタンマーク

        size_t btnTexIndexKeyMouse = 0;
        size_t btnTexIndexGamepad = 1;
        bool isBtnGamepad = false;

        size_t iconTexIndexDefault = 0;
        size_t iconTexIndexAlt = 0;
        bool isIconAlt = false;

        Vector2 centerOffset = { 0.0f, 50.0f };
        Vector2 btnMarkSizeKeyMouse = { 30.0f, 30.0f };
        Vector2 btnMarkSizeGamepad = { 30.0f, 30.0f };
        Vector2 btnMarkOffset = { 0.0f, -44.0f };
        float scale = 1.0f;
    };

    void InitializeWeaponSlot(
        WeaponIconSlot& slot,
        const Vector2& centerOffset,
        const std::string& iconPath,
        const std::string& keyMouseBtnPath,
        const std::string& gamepadBtnPath,
        float initialRatio);

    void UpdateWeaponSlot(
        WeaponIconSlot& slot,
        float dt,
        float ratio,
        bool isCooldown,
        bool triggerPop,
        bool lowEnergyWarning);

    void DrawWeaponSlot(const WeaponIconSlot& slot, float ratio) const;

    // 操作・武器アイコンUI（移動・ジャンプ・ダッシュ・近接攻撃・マシンガン）
    WeaponIconSlot moveIcon_;
    WeaponIconSlot jumpIcon_;
    WeaponIconSlot dashIcon_;
    WeaponIconSlot meleeIcon_;
    WeaponIconSlot shootIcon_;

    Vector2 weaponIconCardSize_ = { 68.0f, 68.0f };
    Vector2 weaponIconBaseSize_ = { 56.0f, 56.0f };
    Vector2 weaponBtnMarkOffset_ = { 0.0f, -44.0f };
    Vector2 weaponBtnMarkSize_ = { 30.0f, 30.0f };

    bool wasMoving_ = false;            // 前フレームの移動状態
    bool wasAirborne_ = false;          // 前フレームの空中状態
    bool wasHoverIconActive_ = false;   // 前フレームのホバーアイコン状態
    bool wasDashStepping_ = false;      // 前フレームのステップ状態
    bool wasMeleeAttacking_ = false;    // 前フレームの近接攻撃モーション状態
    bool wasDashAttackIcon_ = false;    // 前フレームのダッシュ攻撃アイコン状態
    float prevShootEnergyRatio_ = 1.0f; // 前フレームのマシンガンエネルギー残量
    bool wasShootOverheated_ = false;   // 前フレームのオーバーヒート状態

};

