// Grayscale.hlsli
#ifndef GRAYSCALE_HLSLI
#define GRAYSCALE_HLSLI

// 輝度計算用の定数（Rec.709規格）
static const float3 kGrayscaleWeights = float3(0.2125f, 0.7154f, 0.0721f);

/**
 * グレースケール処理を適用する関数
 * @param color 元のRGBカラー
 * @return グレースケール適用後のRGBカラー
 */
float3 ApplyGrayscale(float3 color)
{
    float luminance = dot(color, kGrayscaleWeights);
    return float3(luminance, luminance, luminance);
}

/**
 * 適用度（強度）を指定可能なグレースケール関数
 * @param color 元のRGBカラー
 * @param factor 適用率 (0.0: 元色 ～ 1.0: 完全グレースケール)
 * @return 適用後のRGBカラー
 */
float3 ApplyGrayscale(float3 color, float factor)
{
    float luminance = dot(color, kGrayscaleWeights);
    float3 gray = float3(luminance, luminance, luminance);
    return lerp(color, gray, factor);
}

#endif // GRAYSCALE_HLSLI