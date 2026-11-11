#include "../CopyImage/FullScreen.hlsli"

float rand2dTo1d(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123);
}
struct Material
{
    float time; // UV変換行列
};
ConstantBuffer<Material> gMaterial : register(b0);



Texture2D<float4> gTexture : register(t0);
SamplerState gSampler: register(s0);
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
PixelShaderOutput main(
VertexShaderOutput input) {
PixelShaderOutput output;
float random = rand2dTo1d(input.texcoord * gMaterial.time); 
    
    float4 randomColor = float4(random, random, random, 1.0f);

    output.color = gTexture.Sample(gSampler, input.texcoord) * randomColor;
    
 
return output;
}

