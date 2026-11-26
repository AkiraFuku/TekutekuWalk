#pragma once
#include <cstddef>

/// <summary>
/// 値を [minVal, maxVal] の範囲に制限する
/// </summary>
template <typename T>
T Clamp(T value, T minVal, T maxVal) {
    if (value < minVal) {
        return minVal;
    }
    if (value > maxVal) {
        return maxVal;
    }
    return value;
}

/// <summary>
/// 2つの変数の値を入れ替える（参照渡し）
/// </summary>
template <typename T>
void Swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

/// <summary>
/// 要素型 T と要素数 count から合計バイト数を計算する
/// </summary>
template <typename T>
size_t CalcArrayBytes(size_t count) {
    return sizeof(T) * count;
}

/// <summary>
/// 2値の小さい方を返す
/// </summary>
template <typename T>
T Min(T a, T b) {
    return (a < b) ? a : b;
}

/// <summary>
/// 2値の大きい方を返す
/// </summary>
template <typename T>
T Max(T a, T b) {
    return (a > b) ? a : b;
}

/// <summary>
/// 値が [minVal, maxVal] の範囲内（両端含む）にあるか判定する
/// </summary>
template <typename T>
bool IsInRange(T value, T minVal, T maxVal) {
    return (minVal <= value) && (value <= maxVal);
}
