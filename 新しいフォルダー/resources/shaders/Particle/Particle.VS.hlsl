#include "Particle.hlsli"
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

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    
    output.position = mul(input.position, gParticle[instanceId].WVP);
    output.texCoord = mul(float4(input.texCoord, 0.0f, 1.0f), gParticle[instanceId].uvTransform).xy;
    output.color = gParticle[instanceId].color;

    return output;
}