#include "Particle.hlsli"

struct Material
{
    float4 Color;
    int enableLighting;
    float4x4 uvTransform; // UV変換行列
};
struct DirectionalLight
{
    float4 color; //ライトの色
    float3 direction; //ライトの向き
    float intensity; // 明るさ


};
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texCoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
   
    output.color = gMaterial.Color * textureColor*input.color;
    if (textureColor.a < 0.1f)
    {
        discard; // 透明度が低いピクセルを破棄
    }
    
    return output;
}