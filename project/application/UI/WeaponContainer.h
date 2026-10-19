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
    // 武器を登録（すでに所持している場合は弾数を加算）
    // 戻り値: true=新規取得, false=既存武器への弾数加算
    bool AddWeapon(WeaponId id, int bulletCount) {
        auto it = weaponBulletCounts_.find(id);
        if (it != weaponBulletCounts_.end()) {
            it->second += bulletCount;
            return false;
        }
        weaponBulletCounts_[id] = bulletCount;
        return true;
    }

    // 弾数を増やす
    // 戻り値: true=成功, false=未所持のため失敗
    bool AddBullets(WeaponId id, int count) {
        auto it = weaponBulletCounts_.find(id);
        if (it == weaponBulletCounts_.end()) {
            return false; // 未所持
        }
        it->second += count;
        return true;
    }

    // 所持しているか調べる (std::map::find)
    bool HasWeapon(WeaponId id) const {
        return weaponBulletCounts_.find(id) != weaponBulletCounts_.end();
    }

    // 弾数を取得（未所持なら0）
    int GetBulletCount(WeaponId id) const {
        auto it = weaponBulletCounts_.find(id);
        if (it != weaponBulletCounts_.end()) {
            return it->second;
        }
        return 0;
    }

    // 武器を使用する（弾数消費）
    // 戻り値: 0=弾切れ/未所持で使用不可, 1=使用成功, 2=今回の使用で弾切れ
    int UseWeapon(WeaponId id, int consume = 1) {
        auto it = weaponBulletCounts_.find(id);
        if (it == weaponBulletCounts_.end()) {
            return 0; // 未所持
        }
        if (it->second <= 0) {
            return 0; // 弾切れ
        }
        it->second -= consume;
        if (it->second <= 0) {
            it->second = 0;
            return 2; // 弾切れになった
        }
        return 1; // 使用成功
    }

    // 武器を放棄（インベントリから削除: std::map::erase）
    bool DropWeapon(WeaponId id) {
        auto it = weaponBulletCounts_.find(id);
        if (it == weaponBulletCounts_.end()) {
            return false; // 未所持
        }
        weaponBulletCounts_.erase(it);
        return true;
    }

    // 全所持武器を取得
    const std::map<WeaponId, int>& GetAll() const {
        return weaponBulletCounts_;
    }

    // コンソールへの一覧表示（std::pairによるイテレーション）
    void PrintAll() const {
        std::cout << "===== 所持武器一覧 =====\n";
        if (weaponBulletCounts_.empty()) {
            std::cout << "(所持武器はありません)\n";
            return;
        }
        for (const std::pair<const WeaponId, int>& entry : weaponBulletCounts_) {
            const WeaponId weaponId = entry.first;
            const int bulletCount = entry.second;
            std::cout << "[ID: " << static_cast<int>(weaponId) << "] "
                      << GetWeaponName(weaponId) << " : " << bulletCount << "発\n";
        }
        std::cout << "========================\n";
    }

private:
    std::map<WeaponId, int> weaponBulletCounts_;
};
