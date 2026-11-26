/**
 * @file GameMathMain.cpp
 * @brief 関数テンプレート課題（GameMath.h）の単体テスト用 Main
 *
 * ゲームエンジンやDirectX等に依存せず、標準C++コンパイラで単体ビルド・実行が可能です。
 */

#include <cstdint>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "../engine/math/GameMath.h"

struct Vertex {
    float x, y, z;
    uint32_t color;
};

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "=== 演習確認①：基本3関数の動作確認 ===\n";
    // Clamp（型推論と明示指定）
    std::cout << Clamp(120, 0, 100) << "\n";
    std::cout << Clamp<float>(15.0f, 0, 12) << "\n";

    // Swap（参照渡し）
    int32_t a = 10, b = 20;
    Swap(a, b);
    std::cout << a << ", " << b << "\n";

    // CalcArrayBytes（明示指定）
    std::cout << CalcArrayBytes<Vertex>(10) << " bytes\n\n";

    std::cout << "=== 演習確認②：自力課題の動作確認 ===\n";
    // Min / Max の確認（型推論）
    std::cout << Min(10, 25) << "\n";
    std::cout << Max(10.5f, 3.2f) << "\n";

    // IsInRange の確認
    std::cout << std::boolalpha
              << IsInRange(50, 0, 100) << "\n"
              << IsInRange(150, 0, 100) << "\n";

    return 0;
}
