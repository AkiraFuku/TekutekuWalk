#include "Terrain.hlsli"

struct Material {
    float4 color;
    float  uvTile;
    float3 padding;
};

ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float4> gGrassTexture : register(t0);
Texture2D<float4> gRoadTexture  : register(t1);
SamplerState      gSampler      : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UVタイリング適用
    float2 uv = input.texCoord * gMaterial.uvTile;

    float4 grassCol = gGrassTexture.Sample(gSampler, uv);
    float4 roadCol  = gRoadTexture.Sample(gSampler, uv);

    // 道属性フラグ (0.0 = 地面/草, 1.0 = 道) に応じてテクスチャを割り振り
    float4 baseCol = lerp(grassCol, roadCol, saturate(input.roadAttr));

    // 簡易ディレクショナルライティング（Half-Lambert）
    float3 lightDir = normalize(float3(0.3f, 1.0f, -0.4f));
    float NdotL = saturate(dot(input.normal, lightDir));
    float lightFactor = 0.55f + 0.45f * NdotL;

    output.color = float4(baseCol.rgb * lightFactor * gMaterial.color.rgb, baseCol.a * gMaterial.color.a);
    return output;
}
