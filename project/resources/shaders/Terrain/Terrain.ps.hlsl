#include "Terrain.hlsli"

struct Material {
    float4 color;
    float  uvTile;
    float3 cameraPos;
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
    float roadMask = smoothstep(0.25f, 0.68f, saturate(input.roadAttr));
    float4 baseCol = lerp(grassCol, roadCol, roadMask);

    // 道の境界にわずかな縁取り（自然な土の陰影）
    float border = saturate(roadMask * (1.0f - roadMask) * 4.0f);
    baseCol.rgb *= lerp(1.0f, 0.86f, border * 0.65f);

    // 半球環境光 + 緩やかなディレクショナルライト（明るくポップな陰影）
    float3 N = normalize(input.normal);
    float3 lightDir = normalize(float3(0.35f, 0.85f, -0.40f));
    float halfLambert = dot(N, lightDir) * 0.25f + 0.75f;
    float3 hemiAmbient = lerp(float3(0.45f, 0.72f, 0.30f), float3(0.75f, 0.94f, 1.00f), N.y * 0.5f + 0.5f) * 0.65f;
    float3 lightFactor = hemiAmbient + float3(0.55f, 0.55f, 0.50f) * halfLambert;

    float3 lit = baseCol.rgb * lightFactor * gMaterial.color.rgb;
    float lum = dot(lit, float3(0.299f, 0.587f, 0.114f));
    lit = saturate(lerp(float3(lum, lum, lum), lit, 1.18f));

    // カメラからの実距離フォグ + 遠景延長ウェイトで地平線・遠景を空の色へ馴染ませる
    float dist = distance(gMaterial.cameraPos, input.worldPosition);
    float distFog = smoothstep(42.0f, 125.0f, dist);
    float farMask = smoothstep(30.0f, 75.0f, dist);
    float extFog  = smoothstep(0.35f, 1.0f, saturate(input.extendWeight)) * 0.45f * farMask;
    float fogFactor = saturate(max(distFog * 0.85f, extFog));
    float3 horizonFogColor = float3(0.58f, 0.86f, 0.85f);
    lit = lerp(lit, horizonFogColor, fogFactor);

    output.color = float4(lit, baseCol.a * gMaterial.color.a);
    return output;
}
