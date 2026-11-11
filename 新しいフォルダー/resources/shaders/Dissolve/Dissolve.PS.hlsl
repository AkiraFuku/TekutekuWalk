#include "../CopyImage/FullScreen.hlsli"

Texture2D<float4> gTexture : register(t0);
Texture2D<float> gMaskTexture: register(t1);
SamplerState gSampler : register(s0);


struct DissolveParameter
{
    float threshold;
};
ConstantBuffer<DissolveParameter> gDissolveParam : register(b0);
//: register(b1);
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
PixelShaderOutput main(
VertexShaderOutput input)
{
    float mask = gMaskTexture.Sample(gSampler, input.texcoord);
    if (mask <= gDissolveParam.threshold)
    {
        discard;
    }
    
    float edge = 1.0f - smoothstep(0.5f, 0.53f, mask);
        
    PixelShaderOutput output;
    output.color=gTexture.Sample(gSampler, input.texcoord);
    
    output.color.rgb += edge * float3(1.0f, 0.4f, 0.3f); // 边缘发光效果，可以根据需要调整颜色和强度
    
    
    return output;
}