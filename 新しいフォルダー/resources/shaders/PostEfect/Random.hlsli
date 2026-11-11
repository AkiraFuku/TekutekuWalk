// RandomNoise.hlsli
#ifndef RANDOM_NOISE_HLSLI
#define RANDOM_NOISE_HLSLI

inline float Rand2dTo1d(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453123f);
}

/**
 * ランダムノイズ処理
 */
float3 ApplyRandomNoise(float3 sceneColor, float2 uv, float time)
{
    float noise = Rand2dTo1d(uv * time);
    return sceneColor * noise;
}

#endif // RANDOM_NOISE_HLSLI