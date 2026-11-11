#include "Sprite.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};

// Sprite.cppの SetGraphicsRootConstantBufferView(0, ...) に対応
ConstantBuffer<Material> gMaterial : register(b0);

// Sprite.cppの SetGraphicsRootDescriptorTable(2, ...) に対応
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    // テクスチャの色を取得
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // マテリアルの色とテクスチャの色を乗算（色変更やアルファ値の適用）
    float4 outputColor = gMaterial.color * textureColor;

    // 透明度0の場合は描画しない（Discard）
    if (outputColor.a == 0.0f)
    {
        discard;
    }

    return outputColor;
}