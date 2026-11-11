#include "Sprite.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
};

struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};

// Sprite.cppの SetGraphicsRootConstantBufferView(1, ...) に対応
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

// Sprite.cppの SetGraphicsRootConstantBufferView(0, ...) に対応
// UVTransformを使うために必要
ConstantBuffer<Material> gMaterial : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    // 座標変換
    output.position = mul(input.position, gTransformationMatrix.WVP);

    // UV座標変換 (Sprite.cppで設定したuvTransformを適用)
    // 頂点シェーダーで計算する方が効率的です
    output.texcoord = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;
    
    // 法線変換（スプライトではあまり使いませんが構造維持のため）
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));

    return output;
}