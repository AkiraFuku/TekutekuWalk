#include "Terrain.hlsli"

struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL0;
    float  roadAttr : TEXCOORD1;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    output.position      = mul(input.position, gTransformationMatrix.WVP);
    output.texCoord      = input.texCoord;
    output.normal        = normalize(mul(input.normal, (float3x3)gTransformationMatrix.WorldInverseTranspose));
    output.roadAttr      = input.roadAttr;
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;

    return output;
}
