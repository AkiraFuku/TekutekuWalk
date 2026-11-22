#include "GpuParticle.hlsli"

StructuredBuffer<Particle> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID) {
    VertexShaderOutput output;
    Particle particle = gParticles[instanceId];
    float4x4 worldMatrix = gPerView.billboardMatrix;
    worldMatrix[0] *= particle.scale.x;
    worldMatrix[1] *= particle.scale.y;
    worldMatrix[2] *= particle.scale.z;
    worldMatrix[3].xyz = particle.translate;
    output.position = mul(input.position, mul(worldMatrix, gPerView.viewProjection));
    output.texCoord = input.texCoord;
    output.color = particle.color;
    return output;
}
