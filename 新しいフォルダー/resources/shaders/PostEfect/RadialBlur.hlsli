// RadialBlur.hlsli
#ifndef RADIAL_BLUR_HLSLI
#define RADIAL_BLUR_HLSLI

/**
 * ラジアルブラー処理
 */
float3 ApplyRadialBlur(
    Texture2D<float4> colorTex,
    SamplerState linearSampler,
    float2 uv,
    float2 center,
    int samples,
    float blurWidth)
{
    float2 direction = uv - center;
    float3 accumulatedColor = float3(0.0f, 0.0f, 0.0f);
    
    int validSamples = max(samples, 1);

    for (int i = 0; i < validSamples; ++i)
    {
        float2 texcoord = uv + direction * blurWidth * float(i);
        accumulatedColor += colorTex.Sample(linearSampler, texcoord).rgb;
    }

    return accumulatedColor * rcp((float) validSamples);
}

#endif // RADIAL_BLUR_HLSLI