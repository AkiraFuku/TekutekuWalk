// Vignette.hlsli
#ifndef VIGNETTE_HLSLI
#define VIGNETTE_HLSLI

/**
 * ヴィネット処理（画面周辺減光）
 * @param sceneColor  現在のRGBカラー
 * @param uv          UV座標
 * @return 減光適用後のRGBカラー
 */
float3 ApplyVignette(float3 sceneColor, float2 uv)
{
    float2 correct = uv * (1.0f - uv.yx);
    float vignette = correct.x * correct.y * 16.0f;
    vignette = saturate(pow(vignette, 0.8f));
    
    return sceneColor * vignette;
}

#endif // VIGNETTE_HLSLI