#include "../Particle/Particle.hlsli"
struct ParticleForGPU
{

float4x4 WVP;
float4x4 World;
float4 color;
float4x4 uvTransform;
};
StructuredBuffer<ParticleForGPU> gParticle : register(t0);
//
struct VertexShaderInput
{

    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input,uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    
    output.position = mul(input.position, gParticle[instanceId].WVP);
    float2 transformedTexCoord = mul(float4(input.texCoord, 0.0f, 1.0f), gParticle[instanceId].uvTransform).xy;
    transformedTexCoord.y = 1.0f - transformedTexCoord.y; // テクスチャ座標のY軸を反転
    output.texCoord =transformedTexCoord;

    
    output.color = gParticle[instanceId].color;
    return output;
}