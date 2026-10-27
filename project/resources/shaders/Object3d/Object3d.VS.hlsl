#include "Object3d.hlsli"
struct TransformationMatrix{
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
struct VertexShaderInput{ 

    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    float4 worldPos = mul(input.position, gTransformationMatrix.World);
    
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
        output.position = mul(input.position, gTransformationMatrix.WVP);
    }

    output.texCoord = input.texCoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = worldPos.xyz;
    
    return output;
}