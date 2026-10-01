#pragma once
#pragma once
#include <memory>
#include <vector>
class Enemy;
class ICommand;
// --- 行動ビヘイビア (Action) ---
// 物理計算や移動ロジックを担当
class IEnemyAction;

// --- 状態ビヘイビア (State) ---
// どの行動を組み合わせて使うか、どの状態へ遷移するかを担当
class IEnemyState {
public:
    virtual ~IEnemyState() = default;
    virtual void Initialize(Enemy* enemy) = 0;
    virtual void Update(Enemy* enemy) = 0;
    virtual void Finalize(Enemy* enemy) = 0;
    virtual const char* GetName() const = 0; // 追加
    virtual bool IsDead() const { return false; }
    virtual bool CanDamagePlayer() const { return false; }
};
class StateEnemyNormal : public IEnemyState {
public:
    void Initialize(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    void Finalize(Enemy* enemy) override;
    const char* GetName() const override { return "Normal"; }
    bool CanDamagePlayer() const override { return true; }
};

// スタン状態（乗っ取り待機状態）
class StateEnemyStan : public IEnemyState {
public:
    void Initialize(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    void Finalize(Enemy* enemy) override;
    const char* GetName() const override { return "Stan"; }
private:
    float timer_ = 0.0f;
    const float kStanDuration = 3.0f; // 3秒で復帰
};

// 死亡状態（爆発エフェクトや削除待ち）
class StateEnemyDead : public IEnemyState {
public:
    void Initialize(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    void Finalize(Enemy* enemy) override;
    const char* GetName() const override { return "Dead"; }
    bool IsDead() const override { return true; }
};