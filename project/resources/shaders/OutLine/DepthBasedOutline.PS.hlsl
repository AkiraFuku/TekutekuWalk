#include "../CopyImage/FullScreen.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gTextureDepth : register(t1);
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);
struct Material
{
    float4x4 ProjectionInverse; // UV変換行列
};
ConstantBuffer<Material> gMaterial : register(b0);




static const float kPrewittHorizontalKarnel[3][3] =
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
float Luminance(float3 v)
{
    return dot(v, float3(0.2125f, 0.7154f, 0.0721f));
}
static const float2 kIndex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } }
};



float SampleViewZ(float2 texcoord)
{
    float ndcDepth = gTextureDepth.Sample(gSamplerPoint, texcoord);
    float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), gMaterial.ProjectionInverse);
    return abs(-viewSpace.z * rcp(viewSpace.w));
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
PixelShaderOutput main(
VertexShaderOutput input)
{
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(rcp(width), rcp(height));
    
    
    PixelShaderOutput output;
    output.color.rgb = float3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;
    float centerDist = SampleViewZ(input.texcoord);

    // 上下左右の2次微分（ラプラシアン）で平らな斜面の連続的な深度勾配を無視し、輪郭境界のみ検出する
    float dL = SampleViewZ(input.texcoord + float2(-uvStepSize.x, 0.0f));
    float dR = SampleViewZ(input.texcoord + float2( uvStepSize.x, 0.0f));
    float dU = SampleViewZ(input.texcoord + float2(0.0f, -uvStepSize.y));
    float dD = SampleViewZ(input.texcoord + float2(0.0f,  uvStepSize.y));

    float lapX = abs((dL + dR) - 2.0f * centerDist);
    float lapY = abs((dU + dD) - 2.0f * centerDist);
    float edge = max(lapX, lapY);

    // 距離に応じた正規化としきい値、および遠景フェード（山や地平線・奥の地形端の黒線を防ぐ）
    float distScale = max(0.5f, centerDist * 0.08f);
    float farAtten = saturate(1.0f - (centerDist - 14.0f) / 14.0f);
    float weight = smoothstep(0.35f, 1.0f, edge / distScale) * farAtten;
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    
    return output;
}