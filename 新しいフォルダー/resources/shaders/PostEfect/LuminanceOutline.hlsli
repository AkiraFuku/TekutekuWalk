// LuminanceOutline.hlsli
#ifndef LUMINANCE_OUTLINE_HLSLI
#define LUMINANCE_OUTLINE_HLSLI

static const float kPrewittLumH[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f }
};

static const float kPrewittLumV[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f }
};

static const float2 kIndex3x3Lum[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } }
};

inline float GetLuminanceValue(float3 color)
{
    return dot(color, float3(0.2125f, 0.7154f, 0.0721f));
}

/**
 * 輝度ベースのアウトライン処理
 */
float3 ApplyLuminanceOutline(
    float3 sceneColor,
    Texture2D<float4> colorTex,
    SamplerState linearSampler,
    float2 uv,
    float2 uvStepSize)
{
    float2 difference = float2(0.0f, 0.0f);

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texcoord = uv + kIndex3x3Lum[x][y] * uvStepSize;
            float3 fetchColor = colorTex.Sample(linearSampler, texcoord).rgb;
            float lum = GetLuminanceValue(fetchColor);
            
            difference.x += lum * kPrewittLumH[x][y];
            difference.y += lum * kPrewittLumV[x][y];
        }
    }

    float weight = saturate(length(difference) * 6.0f);
    return (1.0f - weight) * sceneColor;
}

#endif // LUMINANCE_OUTLINE_HLSLI