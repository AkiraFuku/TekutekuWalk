#include "Primitive.hlsli"
struct VS_INPUT
{
    float4 position : POSITION0;
    float4 color: COLOR ;
};

struct TransformationMatrix
{
    float4x4 WVP;
};

// b0レジスタで受け取る
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);


VS_OUTPUT main(VS_INPUT input) 
{
    VS_OUTPUT output;
    // WVP行列を使って座標変換
    output.pos = mul(input.position, gTransformationMatrix.WVP);
    output.color = input.color;
    return output;
}