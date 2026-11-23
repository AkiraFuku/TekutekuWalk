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

float SampleOutlineViewZ(
    Texture2D<float> depthTex,
    SamplerState pointSampler,
    float2 texcoord,
    float4x4 projectionInverse)
{
    float ndcDepth = depthTex.Sample(pointSampler, texcoord);
    float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverse);
    return abs(-viewSpace.z * rcp(viewSpace.w));
}

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
    float centerDist = SampleOutlineViewZ(depthTex, pointSampler, uv, projectionInverse);

    // 上下左右の2次微分（ラプラシアン）で斜面の連続的な深度勾配を無視し、輪郭境界のみ検出する
    float dL = SampleOutlineViewZ(depthTex, pointSampler, uv + float2(-uvStepSize.x, 0.0f), projectionInverse);
    float dR = SampleOutlineViewZ(depthTex, pointSampler, uv + float2( uvStepSize.x, 0.0f), projectionInverse);
    float dU = SampleOutlineViewZ(depthTex, pointSampler, uv + float2(0.0f, -uvStepSize.y), projectionInverse);
    float dD = SampleOutlineViewZ(depthTex, pointSampler, uv + float2(0.0f,  uvStepSize.y), projectionInverse);

    float lapX = abs((dL + dR) - 2.0f * centerDist);
    float lapY = abs((dU + dD) - 2.0f * centerDist);
    float edge = max(lapX, lapY);

    // 距離に応じた正規化としきい値、および遠景フェード（山や地平線の黒線・黒帯を防ぐ）
    float distScale = max(0.5f, centerDist * 0.08f);
    float farAtten = saturate(1.0f - (centerDist - 20.0f) / 35.0f);
    float weight = smoothstep(0.35f, 1.0f, edge / distScale) * farAtten;
    return (1.0f - weight) * sceneColor;
}

#endif // DEPTH_OUTLINE_HLSLI