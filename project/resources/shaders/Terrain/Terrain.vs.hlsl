#include "Terrain.hlsli"

struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float    horizonExtendDist;
    float    horizonDropY;
    float    sizeX;
    float    sizeY;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

struct VertexShaderInput {
    float4 position   : POSITION0;
    float2 texCoord   : TEXCOORD0;
    float3 normal     : NORMAL0;
    float  roadAttr   : TEXCOORD1;
    float3 extendInfo : TEXCOORD2;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    float4 localPos = input.position;
    float2 uv       = input.texCoord;
    float3 normal   = input.normal;
    float  w        = saturate(input.extendInfo.z);

    if (w > 0.0f)
    {
        float2 dir = input.extendInfo.xy;
        float lenSq = dot(dir, dir);
        if (lenSq > 1e-6f)
        {
            dir *= rsqrt(lenSq);
        }

        // 外周頂点を地平線方向へ押し出す
        float2 offsetXZ = dir * (gTransformationMatrix.horizonExtendDist * w);
        localPos.xz += offsetXZ;
        localPos.y  += gTransformationMatrix.horizonDropY * w;

        // 押し出し後の位置から UV を一貫して計算し、斜め方向の引き伸ばしを防ぐ
        uv.x = (localPos.x + gTransformationMatrix.sizeX * 0.5f) / max(1.0f, gTransformationMatrix.sizeX);
        uv.y = (localPos.z + gTransformationMatrix.sizeY * 0.5f) / max(1.0f, gTransformationMatrix.sizeY);

        // 遠景の草原は法線を上向きに揃えて暗い影を防ぐ
        normal = normalize(lerp(normal, float3(0.0f, 1.0f, 0.0f), w));
    }

    output.position      = mul(localPos, gTransformationMatrix.WVP);
    output.texCoord      = uv;
    output.normal        = normalize(mul(normal, (float3x3)gTransformationMatrix.WorldInverseTranspose));
    output.roadAttr      = input.roadAttr;
    output.worldPosition = mul(localPos, gTransformationMatrix.World).xyz;
    output.extendWeight  = w;

    return output;
}
