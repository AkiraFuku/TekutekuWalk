#pragma once
#include <map>
#include <string>
#include <vector>
#include <iostream>

// 武器IDの定義
enum class WeaponId {
    Handgun = 100,
    SubmachineGun = 300,
    RocketLauncher = 900,
};

// 武器の状態構造体
struct WeaponState {
    bool hasWeapon = false; // 所持しているか
    int bulletCount = 0;    // 残弾数
};

// 武器名取得ヘルパー
inline std::string GetWeaponName(WeaponId id) {
    switch (id) {
    case WeaponId::Handgun:        return "Handgun";
    case WeaponId::SubmachineGun:  return "SubmachineGun";
    case WeaponId::RocketLauncher: return "RocketLauncher";
    default:                       return "Unknown Weapon";
    }
}

// 武器コンテナクラス（std::map を用いた所持武器・残弾数管理）
class WeaponContainer {
public:
    // 武器を取得・登録（hasWeapon を true に設定。初期弾数があれば加算）
    // 戻り値: true=新規に武器本体を取得, false=既に武器を持っていた
    bool AddWeapon(WeaponId id, int initialBullets = 0) {
        WeaponState& state = weapons_[id];
        bool wasNotOwned = !state.hasWeapon;
        state.hasWeapon = true;
        state.bulletCount += initialBullets;
        return wasNotOwned;
    }

    // 弾薬を拾う・補充する（※武器を持っていなくても弾だけ拾える）
    void AddBullets(WeaponId id, int count) {
        weapons_[id].bulletCount += count;
    }

    // 武器本体を所持しているか調べる
    bool HasWeapon(WeaponId id) const {
        auto it = weapons_.find(id);
        if (it != weapons_.end()) {
            return it->second.hasWeapon;
        }
        return false;
    }

    // 残弾数を取得（未登録なら0）
    int GetBulletCount(WeaponId id) const {
        auto it = weapons_.find(id);
        if (it != weapons_.end()) {
            return it->second.bulletCount;
        }
        return 0;
    }

    // 状態全体を取得（存在しなければ nullptr）
    const WeaponState* GetWeaponState(WeaponId id) const {
        auto it = weapons_.find(id);
        if (it != weapons_.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    // 武器を使用する（弾数消費）
    // 戻り値: 0=武器未所持または弾切れ, 1=使用成功, 2=今回の使用で弾切れ
    int UseWeapon(WeaponId id, int consume = 1) {
        auto it = weapons_.find(id);
        if (it == weapons_.end() || !it->second.hasWeapon) {
            return 0; // 武器未所持
        }
        if (it->second.bulletCount <= 0) {
            return 0; // 弾切れ
        }
        it->second.bulletCount -= consume;
        if (it->second.bulletCount <= 0) {
            it->second.bulletCount = 0;
            return 2; // 弾切れになった
        }
        return 1; // 使用成功
    }

    // 武器だけを捨てる（hasWeapon = false にするが、弾数は保持）
    bool DropWeaponOnly(WeaponId id) {
        auto it = weapons_.find(id);
        if (it == weapons_.end() || !it->second.hasWeapon) {
            return false; // そもそも武器を持っていない
        }
        it->second.hasWeapon = false;
        return true;
    }

    // 武器と弾薬を両方放棄（コンテナから完全に要素削除: erase）
    bool DropAll(WeaponId id) {
        auto it = weapons_.find(id);
        if (it == weapons_.end()) {
            return false;
        }
        weapons_.erase(it);
        return true;
    }

    // 全要素を取得
    const std::map<WeaponId, WeaponState>& GetAll() const {
        return weapons_;
    }

    // コンソールへの一覧表示（所持中の武器のみ表示）
    void PrintAll() const {
        std::cout << "===== 所持武器一覧 =====\n";
        bool anyOwned = false;
        for (const auto& entry : weapons_) {
            if (entry.second.hasWeapon) {
                anyOwned = true;
                std::cout << "[ID: " << static_cast<int>(entry.first) << "] "
                          << GetWeaponName(entry.first) << " : "
                          << entry.second.bulletCount << "発\n";
            }
        }
        if (!anyOwned) {
            std::cout << "(所持している武器はありません)\n";
        }
        std::cout << "========================\n";
    }

private:
    std::map<WeaponId, WeaponState> weapons_;
};
