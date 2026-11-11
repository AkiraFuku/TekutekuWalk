#include "Skybox.hlsli"
struct Material
{
    float4 Color;
   
};

//struct Camera
//{
//    float3 worldPosition;
//    float farClip;
//    float3 cameraForward; // ★追加
//    float padding;
//};

//ConstantBuffer<Camera> gCamera : register(b2);

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
TextureCube<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);




PixelShaderOutput main(VertexShaderOutput input)
{
   
    PixelShaderOutput output;
    float4 texColor = gTexture.Sample(gSampler, input.texCoord.xyz);
    output.color = texColor * gMaterial.Color; // テクスチャカラーとマテリアルカラーを掛け合わせる
 
   
    
    return output;
}