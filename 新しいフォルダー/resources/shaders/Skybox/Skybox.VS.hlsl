#include "Skybox.hlsli"
struct TransformationMatrix{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);
//
struct VertexShaderInput{ 

    float4 position : POSITION0;
    float3 texCoord : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    output.position = mul(input.position,gTransformationMatrix.WVP).xyww;
    output.texCoord = input.texCoord;
 
    
    return output;
}