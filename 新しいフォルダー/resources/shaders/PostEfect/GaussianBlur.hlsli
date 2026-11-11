// GaussianBlur.hlsli
#ifndef GAUSSIAN_BLUR_HLSLI
#define GAUSSIAN_BLUR_HLSLI

static const float kGaussianPI = 3.14159265f;

static const float2 kGaussianIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } }
};

inline float CalculateGaussWeight(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * kGaussianPI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

/**
 * 3x3 ガウシアンブラー処理
 */
float3 ApplyGaussianBlur(
    Texture2D<float4> colorTex,
    SamplerState linearSampler,
    float2 uv,
    float2 uvStepSize,
    float sigma = 2.0f)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float weight = CalculateGaussWeight(kGaussianIndex3x3[x][y].x, kGaussianIndex3x3[x][y].y, sigma);
            float2 texcoord = uv + kGaussianIndex3x3[x][y] * uvStepSize;
            float3 fetchColor = colorTex.Sample(linearSampler, texcoord).rgb;
            
            result += fetchColor * weight;
            totalWeight += weight;
        }
    }

    return result * rcp(totalWeight);
}

#endif // GAUSSIAN_BLUR_HLSLI