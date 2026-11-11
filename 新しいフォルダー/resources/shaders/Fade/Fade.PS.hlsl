cbuffer Material : register(b0)
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    // スプライトのマテリアルカラー（RGBA）をそのまま出力
    return color;
}