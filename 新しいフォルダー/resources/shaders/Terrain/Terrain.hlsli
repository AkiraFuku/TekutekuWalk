struct VertexShaderOutput {
    float4 position      : SV_POSITION;
    float2 texCoord      : TEXCOORD0;
    float3 normal        : NORMAL0;
    float  roadAttr      : TEXCOORD1;
    float3 worldPosition : POSITION0;
};
