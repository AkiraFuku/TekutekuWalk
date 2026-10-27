#pragma once
#include <set>
#include <vector>
#include <algorithm>
#include <iterator>
#include <string>

/**
 * @brief エネミー接触管理 & ロックオン候補判定クラス (std::set 課題)
 * 
 * 【スライド準拠内容】
 * 1. std::set による重複防止判定
 * 2. 整列順を活かした共通集合の抽出 (std::set_intersection)
 */
class EnemyContactManager {
public:
    EnemyContactManager() {
        LoadSlideSample();
    }

    // --- ロックオン共通集合の抽出 (std::set_intersection) ---
    void AddVisibleEnemy(int characterNo) { visibleCharacters_.insert(characterNo); }
    void RemoveVisibleEnemy(int characterNo) { visibleCharacters_.erase(characterNo); }
    void ClearVisibleEnemies() { visibleCharacters_.clear(); }

    void AddInRangeEnemy(int characterNo) { inRangeCharacters_.insert(characterNo); }
    void RemoveInRangeEnemy(int characterNo) { inRangeCharacters_.erase(characterNo); }
    void ClearInRangeEnemies() { inRangeCharacters_.clear(); }

    const std::set<int>& GetVisibleCharacters() const { return visibleCharacters_; }
    const std::set<int>& GetInRangeCharacters() const { return inRangeCharacters_; }

    /// <summary>
    /// 視界内 (visibleCharacters) ∩ 射程内 (inRangeCharacters) からロックオン候補を抽出
    /// </summary>
    std::vector<int> ComputeLockOnTargets() const {
        std::vector<int> lockOnTargets;
        // 双方とも std::set (ソート済み) なので、直接アルゴリズムへ渡せる
        std::set_intersection(
            visibleCharacters_.begin(), visibleCharacters_.end(),
            inRangeCharacters_.begin(), inRangeCharacters_.end(),
            std::inserter(lockOnTargets, lockOnTargets.end())
        );
        return lockOnTargets;
    }

    /// <summary>
    /// スライドのサンプルデータ（視界: 101, 205, 310 / 射程: 205, 310, 420）をセット
    /// </summary>
    void LoadSlideSample() {
        visibleCharacters_ = { 101, 205, 310 };
        inRangeCharacters_ = { 205, 310, 420 };
    }

private:
    std::set<int> visibleCharacters_;  // 視界内の敵 (整列済み)
    std::set<int> inRangeCharacters_;  // 射程内の敵 (整列済み)
};
