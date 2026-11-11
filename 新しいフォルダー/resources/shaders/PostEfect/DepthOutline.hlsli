// DepthOutline.hlsli
#ifndef DEPTH_OUTLINE_HLSLI
#define DEPTH_OUTLINE_HLSLI

// フィルタリネル・インデックス定義
static const float kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f }
};

static const float kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f }
};

static const float2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f },  { 0.0f, 0.0f },  { 1.0f, 0.0f } },
    { { -1.0f, 1.0f },  { 0.0f, 1.0f },  { 1.0f, 1.0f } }
};

/**
 * 深度ベースのアウトライン処理を適用する関数
 * @param sceneColor        元のシーンカラー (gTexture.Sample などの結果)
 * @param uv                入力UV座標
 * @param uvStepSize        1ピクセルあたりのUVサイズ (rcp(width), rcp(height))
 * @param depthTex          深度テクスチャ (t1)
 * @param pointSampler      ポイントサンプラー (s1)
 * @param projectionInverse 逆射影行列
 * @return アウトライン適用後のRGBカラー
 */
float3 ApplyDepthOutline(
    float3 sceneColor,
    float2 uv,
    float2 uvStepSize,
    Texture2D<float> depthTex,
    SamplerState pointSampler,
    float4x4 projectionInverse)
{
    float2 difference = float2(0.0f, 0.0f);

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texcoord = uv + kIndex3x3[x][y] * uvStepSize;
            float ndcDepth = depthTex.Sample(pointSampler, texcoord);
            
            // NDC深度からビュー空間のZ座標を復元
            float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverse);
            float viewZ = -viewSpace.z * rcp(viewSpace.w);
            
            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }

    float weight = saturate(length(difference));
    return (1.0f - weight) * sceneColor;
}

#endif // DEPTH_OUTLINE_HLSLI