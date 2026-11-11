#include "Object3d.hlsli"
struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4x4 View;
    float4x4 Projection;
    float curvature;
    float3 padding;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
//
struct VertexShaderInput
{ 

    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 weight : WEIGHT0;
    int4 index : INDEX0;
};

struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTranspaseMatrix;
};
StructuredBuffer<Well> gMatrixPalette : register(t0);

struct Skinned
{
    float4 position;
    float3 normal;
};
Skinned Skinning(VertexShaderInput input)
{
    Skinned skinned;
    //位置の変換
    skinned.position = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
    skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
    skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
    skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinned.position.w = 1.0f;
    //法線の変換
    skinned.normal = mul(input.normal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTranspaseMatrix) * input.weight.x;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTranspaseMatrix) * input.weight.y;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTranspaseMatrix) * input.weight.z;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTranspaseMatrix) * input.weight.w;
    skinned.normal = normalize(skinned.normal);
    return skinned;
}


VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    Skinned skinned = Skinning(input);
    
    float4 worldPos = mul(skinned.position, gTransformationMatrix.World);

    // 空間湾曲（Curved World）: カメラ奥方向（Z）への距離に応じて高さを落とす
    if (gTransformationMatrix.curvature > 0.0f)
    {
        float4 viewPos = mul(worldPos, gTransformationMatrix.View);
        float depth = max(0.0f, viewPos.z);
        viewPos.y -= gTransformationMatrix.curvature * (depth * depth);
        output.position = mul(viewPos, gTransformationMatrix.Projection);
    }
    else
    {
        output.position = mul(skinned.position, gTransformationMatrix.WVP);
    }

    output.worldPosition = worldPos.xyz;
    output.texCoord = input.texCoord;
    output.normal = normalize(mul(skinned.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    
    return output;
}