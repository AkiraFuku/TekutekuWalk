/**
 * @file WeaponContainerMain.cpp
 * @brief std::map を使用した武器コンテナ課題（スタンドアロン実行用プログラム）
 * 
 * ゲームエンジンやDirectX等に依存せず、標準C++コンパイラ（g++, clang++, MSVC）で
 * 単体ビルドおよび実行が可能です。
 * 
 * 【課題要件と実装内容】
 * 1. 武器を登録（std::mapへの挿入）
 * 2. 弾数を増やす（イテレータによる参照と更新）
 * 3. 所持しているか調べる（std::map::find による探索）
 * 4. 所持状態を一覧表示する（範囲forおよびstd::pair走査）
 * 5. 指定武器の使用（弾数消費）
 * 6. 指定武器の放棄（std::map::erase による要素削除）
 */

#include <iostream>
#include <map>
#include <string>

// 武器IDの定義
enum class WeaponId {
    Handgun = 100,
    SubmachineGun = 300,
    RocketLauncher = 900,
};

// 武器IDから武器名を取得するヘルパー関数
std::string GetWeaponName(WeaponId id) {
    switch (id) {
    case WeaponId::Handgun:        return "Handgun (ハンドガン)";
    case WeaponId::SubmachineGun:  return "SubmachineGun (サブマシンガン)";
    case WeaponId::RocketLauncher: return "RocketLauncher (ロケットランチャー)";
    default:                       return "Unknown Weapon";
    }
}

/**
 * @brief 所持武器一覧表示
 * std::pair<const WeaponId, int> を用いて全要素を走査
 */
void PrintWeaponList(const std::map<WeaponId, int>& container) {
    std::cout << "--- 現在の所持一覧 ---\n";
    if (container.empty()) {
        std::cout << "  (所持している武器はありません)\n";
    }
    for (const std::pair<const WeaponId, int>& entry : container) {
        const WeaponId weaponId = entry.first;
        const int bulletCount = entry.second;
        std::cout << "  [武器ID: " << static_cast<int>(weaponId) << "] "
                  << GetWeaponName(weaponId) << " -> 残弾数: "
                  << bulletCount << "発\n";
    }
    std::cout << "----------------------\n\n";
}

/**
 * @brief 武器の所持探索 (find)
 */
void CheckWeaponOwned(const std::map<WeaponId, int>& container, WeaponId selectedWeapon) {
    // 探索擬似コード準拠
    std::map<WeaponId, int>::const_iterator itWeapon = container.find(selectedWeapon);
    if (itWeapon == container.end()) {
        std::cout << "【探索】" << GetWeaponName(selectedWeapon) << " は未所持です (Weapon not owned)\n";
    } else {
        std::cout << "【探索】" << GetWeaponName(selectedWeapon) << " を所持しています (残弾: "
                  << itWeapon->second << "発)\n";
    }
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " std::map を使用した武器コンテナ課題 実行デモ\n";
    std::cout << "========================================================\n\n";

    // 武器コンテナ: キー=武器ID, 値=弾数
    std::map<WeaponId, int> weaponBulletCounts;

    // ---------------------------------------------------------
    // 1. 武器を登録（取得）
    // ---------------------------------------------------------
    std::cout << "[Step 1] 武器の登録（Handgun と RocketLauncher を取得）\n";
    weaponBulletCounts[WeaponId::Handgun] = 15;
    weaponBulletCounts[WeaponId::RocketLauncher] = 3;
    PrintWeaponList(weaponBulletCounts);

    // ---------------------------------------------------------
    // 2. 所持しているか調べる
    // ---------------------------------------------------------
    std::cout << "[Step 2] 武器の所持チェック（Handgun と SubmachineGun を探索）\n";
    CheckWeaponOwned(weaponBulletCounts, WeaponId::Handgun);
    CheckWeaponOwned(weaponBulletCounts, WeaponId::SubmachineGun);
    std::cout << '\n';

    // ---------------------------------------------------------
    // 3. 弾数を増やす
    // ---------------------------------------------------------
    std::cout << "[Step 3] 弾数を増やす（Handgunの弾薬を +30発）\n";
    auto itHandgun = weaponBulletCounts.find(WeaponId::Handgun);
    if (itHandgun != weaponBulletCounts.end()) {
        itHandgun->second += 30;
        std::cout << "  -> Handgun の弾薬を補充しました。現在の弾数: " << itHandgun->second << "発\n";
    }
    PrintWeaponList(weaponBulletCounts);

    // ---------------------------------------------------------
    // 4. 武器の使用（弾数を消費）
    // ---------------------------------------------------------
    std::cout << "[Step 4] 武器の使用（RocketLauncher を発射）\n";
    auto itRocket = weaponBulletCounts.find(WeaponId::RocketLauncher);
    if (itRocket != weaponBulletCounts.end()) {
        if (itRocket->second > 0) {
            itRocket->second -= 1;
            std::cout << "  -> ドカーン！ RocketLauncher を1発発射しました。(残弾: " << itRocket->second << "発)\n";
        } else {
            std::cout << "  -> 弾切れのため発射できません！\n";
        }
    }
    PrintWeaponList(weaponBulletCounts);

    // ---------------------------------------------------------
    // 5. 新たな武器を取得（SubmachineGun を追加登録）
    // ---------------------------------------------------------
    std::cout << "[Step 5] 新たな武器の登録（SubmachineGun 60発 を取得）\n";
    weaponBulletCounts[WeaponId::SubmachineGun] = 60;
    PrintWeaponList(weaponBulletCounts);

    // ---------------------------------------------------------
    // 6. 武器の放棄（インベントリから削除: erase）
    // ---------------------------------------------------------
    std::cout << "[Step 6] 武器の放棄（RocketLauncher をインベントリから削除）\n";
    auto itDrop = weaponBulletCounts.find(WeaponId::RocketLauncher);
    if (itDrop != weaponBulletCounts.end()) {
        weaponBulletCounts.erase(itDrop);
        std::cout << "  -> RocketLauncher を放棄しました。\n";
    }
    PrintWeaponList(weaponBulletCounts);

    std::cout << "========================================================\n";
    std::cout << " 一連の武器コンテナ処理デモが正常に完了しました。\n";
    std::cout << "========================================================\n";

    return 0;
}
