#include "Object3d.hlsli"

struct InstanceData
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 color;
};

// インスタンスデータ用 StructuredBuffer (Vertex Shader SRV t5)
StructuredBuffer<InstanceData> gInstanceData : register(t5);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    
    // インスタンスごとの WVP 行列で頂点変換
    output.position = mul(input.position, gInstanceData[instanceId].WVP);
    output.texCoord = input.texCoord;
    output.normal = normalize(mul(input.normal, (float3x3)gInstanceData[instanceId].WorldInverseTranspose));
    output.worldPosition = mul(input.position, gInstanceData[instanceId].World).xyz;
    
    return output;
}
