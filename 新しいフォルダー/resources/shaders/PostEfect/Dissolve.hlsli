// Dissolve.hlsli
#ifndef DISSOLVE_HLSLI
#define DISSOLVE_HLSLI

/**
 * ディゾルブ処理
 * @param sceneColor  現在のRGBカラー
 * @param uv          UV座標
 * @param maskTex     マスクテクスチャ (t2)
 * @param linearSampler サンプラー (s0)
 * @param threshold   ディゾルブ閾値 (0.0 ～ 1.0)
 * @return 境界発光適用後のRGBカラー
 */
float3 ApplyDissolve(
    float3 sceneColor,
    float2 uv,
    Texture2D<float4> maskTex,
    SamplerState linearSampler,
    float threshold)
{
    float mask = maskTex.Sample(linearSampler, uv).r;
    
    // 閾値以下の領域を破棄
    if (mask <= threshold)
    {
        discard;
    }
    
    // 境界線の発光処理 (閾値のすぐ上の領域を判定)
    float edgeWidth = 0.03f;
    float edge = 1.0f - smoothstep(threshold, threshold + edgeWidth, mask);
    
    // エッジカラー（発光色）を追加
    float3 edgeColor = float3(1.0f, 0.4f, 0.3f);
    return sceneColor + edge * edgeColor;
}

#endif // DISSOLVE_HLSLI