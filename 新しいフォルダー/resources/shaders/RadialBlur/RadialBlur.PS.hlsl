#include "../CopyImage/FullScreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler: register(s0);

struct BlurParams
{
    float2 center;
    int samples;
    float blurWidth;
};

ConstantBuffer<BlurParams> gBlurParams : register(b0);

//const BlurParams kBlurParams = { float2(0.5f, 0.5f), 16, 0.01f };

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
PixelShaderOutput main(
VertexShaderOutput input) {
    
    float2 direction = input.texcoord - gBlurParams.center;
    float3 color = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < gBlurParams.samples; ++i)
    {
        float2 texcoord = input.texcoord + direction * gBlurParams.blurWidth * float(i);
        color += gTexture.Sample(gSampler, texcoord).rgb;
    }
    color.rgb *= rcp(gBlurParams.samples);
    PixelShaderOutput output;
    output.color.rgb = color;
    output.color.a = 1.0f;
    return output;
}