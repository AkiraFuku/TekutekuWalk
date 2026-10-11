#pragma once
#include <memory>
#include <vector>
#include <map>
#include <utility>
#include "GameObject.h" // CollisionCategoryが定義されているヘッダー

class Collider;

// 衝突応答タイプ
enum class CollisionResponseType {
    Ignore,   // 判定しない（完全無視）
    Overlap,  // 重なる（検知・イベント通知のみ。コイン、ゴール、攻撃など）
    Block,    // 重ならない（めり込みを押し出して位置補正。障害物、壁など）
};

class CollisionManager {
public:
    static CollisionManager* GetInstance();
    void Finalize();

    /// <summary>
    /// シーン等から渡されたコライダーリスト全体の衝突チェックを実行
    /// </summary>
    /// <param name="colliders">判定対象となるすべてのコライダー</param>
    void CheckAllCollisions(const std::vector<Collider*>& colliders);

    /// <summary>
    /// カテゴリペアの衝突応答タイプを取得（Ignore / Overlap / Block）
    /// </summary>
    CollisionResponseType GetCollisionResponse(CollisionCategory a, CollisionCategory b) const;

    /// <summary>
    /// カテゴリペアの衝突応答タイプを設定
    /// </summary>
    void SetCollisionResponse(CollisionCategory a, CollisionCategory b, CollisionResponseType response);

private:
    CollisionManager();
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

    static std::unique_ptr<CollisionManager> instance;
    friend struct std::default_delete<CollisionManager>;

    // デフォルトの衝突応答マトリクスを初期化
    void InitializeDefaultMatrix();

    // 衝突を検知・通知する基本処理
    void CheckCollision(Collider* a, Collider* b);

    // カテゴリの組み合わせで判定が必要かをチェック
    bool ShouldCheckCollision(CollisionCategory catA, CollisionCategory catB) const;

    // 離れすぎているオブジェクトを弾く距離閾値（二乗値）
    const float kBroadPhaseMaxDistanceSq = 2500.0f;

    int GetCategoryPriority(CollisionCategory category);

    // 衝突マトリクステーブル（順不同ペア）
    std::map<std::pair<int, int>, CollisionResponseType> responseMatrix_;
};