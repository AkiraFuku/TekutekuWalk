struct VertexShaderOutput{
    float4 position : SV_POSITION; // Position in clip space
    float2 texCoord : TEXCOORD0; // Texture coordinates
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
};