/**
 * @file WeaponContainerMain.cpp
 * @brief std::map と WeaponState 構造体を使用した武器コンテナ課題（スタンドアロン実行用プログラム）
 * 
 * ゲームエンジンやDirectX等に依存せず、標準C++コンパイラ（g++, clang++, MSVC）で
 * 単体ビルドおよび実行が可能です。
 * 
 * 【課題要件と実装内容】
 * 1. struct WeaponState（hasWeapon, bulletCount）の導入
 * 2. 武器未所持の状態でも弾を拾う（弾数のみ先行加算）
 * 3. 武器の取得（hasWeapon = true）と弾の取得の分離
 * 4. 所持中（hasWeapon == true）の武器のみ一覧表示
 * 5. 武器の所持探索（find による hasWeapon および弾数の確認）
 * 6. 武器の使用（未所持または弾切れ時は使用不可）
 * 7. 武器だけ捨てる（hasWeapon = false にするが、弾数は保持）
 * 8. インベントリから完全破棄（std::map::erase）
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

// 武器の状態構造体
struct WeaponState {
    bool hasWeapon = false; // 所持しているか
    int bulletCount = 0;    // 残弾数
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
 * 所持中（hasWeapon == true）の武器のみを表示する
 */
void PrintOwnedWeaponList(const std::map<WeaponId, WeaponState>& container) {
    std::cout << "--- 現在の所持武器一覧 (hasWeapon == true のみ) ---\n";
    bool anyOwned = false;
    for (const auto& entry : container) {
        const WeaponId weaponId = entry.first;
        const WeaponState& state = entry.second;
        if (state.hasWeapon) {
            anyOwned = true;
            std::cout << "  [武器ID: " << static_cast<int>(weaponId) << "] "
                      << GetWeaponName(weaponId) << " -> 残弾数: "
                      << state.bulletCount << "発\n";
        }
    }
    if (!anyOwned) {
        std::cout << "  (所持している武器はありません)\n";
    }
    std::cout << "---------------------------------------------------\n\n";
}

/**
 * @brief 武器の所持探索 (find)
 */
void CheckWeaponOwned(const std::map<WeaponId, WeaponState>& container, WeaponId selectedWeapon) {
    auto itWeapon = container.find(selectedWeapon);
    if (itWeapon == container.end() || !itWeapon->second.hasWeapon) {
        int bullets = (itWeapon != container.end()) ? itWeapon->second.bulletCount : 0;
        std::cout << "【探索】" << GetWeaponName(selectedWeapon) << " は未所持です (武器なし";
        if (bullets > 0) {
            std::cout << ", ただし弾薬は " << bullets << "発 所持";
        }
        std::cout << ")\n";
    } else {
        std::cout << "【探索】" << GetWeaponName(selectedWeapon) << " を所持しています (残弾: "
                  << itWeapon->second.bulletCount << "発)\n";
    }
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " std::map + WeaponState 武器コンテナ課題 実行デモ\n";
    std::cout << "========================================================\n\n";

    // 武器コンテナ: キー=武器ID, 値=WeaponState構造体
    std::map<WeaponId, WeaponState> weapons;

    // ---------------------------------------------------------
    // 1. 武器未所持の状態で弾だけ拾う
    // ---------------------------------------------------------
    std::cout << "[Step 1] 武器未所持の状態で RocketLauncher の弾薬を 5発 ひろう\n";
    weapons[WeaponId::RocketLauncher].bulletCount += 5;
    std::cout << "  -> 武器本体は持っていませんが、ロケット弾を 5発 確保しました。\n";
    PrintOwnedWeaponList(weapons); // まだ武器を持っていないので一覧には表示されない！

    // ---------------------------------------------------------
    // 2. 所持チェック
    // ---------------------------------------------------------
    std::cout << "[Step 2] RocketLauncher と Handgun の所持状態をチェック (find)\n";
    CheckWeaponOwned(weapons, WeaponId::RocketLauncher);
    CheckWeaponOwned(weapons, WeaponId::Handgun);
    std::cout << '\n';

    // ---------------------------------------------------------
    // 3. 武器を取得する
    // ---------------------------------------------------------
    std::cout << "[Step 3] 武器を取得（Handgun と RocketLauncher 本体を拾う）\n";
    // Handgun を新規取得（初期弾数 15発）
    weapons[WeaponId::Handgun].hasWeapon = true;
    weapons[WeaponId::Handgun].bulletCount += 15;

    // RocketLauncher 本体を取得（弾数は Step 1 で拾った 5発 をそのまま活用！）
    weapons[WeaponId::RocketLauncher].hasWeapon = true;

    PrintOwnedWeaponList(weapons);

    // ---------------------------------------------------------
    // 4. 弾数を増やす（弾をひろう）
    // ---------------------------------------------------------
    std::cout << "[Step 4] Handgun の弾薬を +30発 ひろう\n";
    weapons[WeaponId::Handgun].bulletCount += 30;
    std::cout << "  -> Handgun の残弾数: " << weapons[WeaponId::Handgun].bulletCount << "発\n";
    PrintOwnedWeaponList(weapons);

    // ---------------------------------------------------------
    // 5. 武器の使用（弾数を消費）
    // ---------------------------------------------------------
    std::cout << "[Step 5] 武器の使用（RocketLauncher を発射）\n";
    auto itRocket = weapons.find(WeaponId::RocketLauncher);
    if (itRocket != weapons.end() && itRocket->second.hasWeapon) {
        if (itRocket->second.bulletCount > 0) {
            itRocket->second.bulletCount -= 1;
            std::cout << "  -> ドカーン！ RocketLauncher を1発発射しました。(残弾: "
                      << itRocket->second.bulletCount << "発)\n";
        }
    }
    PrintOwnedWeaponList(weapons);

    // ---------------------------------------------------------
    // 6. 武器だけ捨てる（hasWeapon = false。弾数は残る！）
    // ---------------------------------------------------------
    std::cout << "[Step 6] 武器だけ捨てる（RocketLauncher の本体を捨てる）\n";
    itRocket = weapons.find(WeaponId::RocketLauncher);
    if (itRocket != weapons.end() && itRocket->second.hasWeapon) {
        itRocket->second.hasWeapon = false;
        std::cout << "  -> RocketLauncher の武器本体を捨てました！（残弾 "
                  << itRocket->second.bulletCount << "発 はそのままインベントリに残ります）\n";
    }
    PrintOwnedWeaponList(weapons); // 武器本体を捨てたので所持武器一覧から消える！

    std::cout << "[Step 6.1] 武器本体を捨てた後の所持チェック\n";
    CheckWeaponOwned(weapons, WeaponId::RocketLauncher);
    std::cout << '\n';

    // ---------------------------------------------------------
    // 7. 再度武器本体を拾うと、残っていた弾薬がそのまま使える
    // ---------------------------------------------------------
    std::cout << "[Step 7] 再び RocketLauncher の武器本体を拾う\n";
    weapons[WeaponId::RocketLauncher].hasWeapon = true;
    std::cout << "  -> RocketLauncher を再取得！残っていた弾薬がそのまま使えます。\n";
    PrintOwnedWeaponList(weapons);

    // ---------------------------------------------------------
    // 8. 完全破棄（erase で弾薬ごと完全に消去）
    // ---------------------------------------------------------
    std::cout << "[Step 8] RocketLauncher を完全破棄（erase）\n";
    weapons.erase(WeaponId::RocketLauncher);
    std::cout << "  -> RocketLauncher を完全破棄しました。\n";
    PrintOwnedWeaponList(weapons);

    std::cout << "========================================================\n";
    std::cout << " 一連の WeaponState 課題デモが正常に完了しました。\n";
    std::cout << "========================================================\n";

    return 0;
}
