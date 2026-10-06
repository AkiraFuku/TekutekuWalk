#include "../Object3d/Object3d.hlsli"
// --- 入力リソース ---
StructuredBuffer<Well> gMatrixPalette : register(t0);
StructuredBuffer<Vertex> gInputVertices : register(t1);
StructuredBuffer<VertexInfluence> gInfluences : register(t2);
// --- 出力リソース ---
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);
// --- 定数バッファ ---
ConstantBuffer<SkinningInformation> gSkinningInformation : register(b0);


Skinned Skinning(Vertex input, VertexInfluence influence)
{
    Skinned skinned;
    //位置の変換
    skinned.position = mul(input.position, gMatrixPalette[influence.jointIndices.x].skeletonSpaceMatrix) * influence.weights.x;
    skinned.position += mul(input.position, gMatrixPalette[influence.jointIndices.y].skeletonSpaceMatrix) * influence.weights.y;
    skinned.position += mul(input.position, gMatrixPalette[influence.jointIndices.z].skeletonSpaceMatrix) * influence.weights.z;
    skinned.position += mul(input.position, gMatrixPalette[influence.jointIndices.w].skeletonSpaceMatrix) * influence.weights.w;
    skinned.position.w = 1.0f;
    //法線の変換
    skinned.normal = mul(input.normal, (float3x3) gMatrixPalette[influence.jointIndices.x].skeletonSpaceInverseTranspaseMatrix) * influence.weights.x;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[influence.jointIndices.y].skeletonSpaceInverseTranspaseMatrix) * influence.weights.y;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[influence.jointIndices.z].skeletonSpaceInverseTranspaseMatrix) * influence.weights.z;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[influence.jointIndices.w].skeletonSpaceInverseTranspaseMatrix) * influence.weights.w;
    skinned.normal = normalize(skinned.normal);
    return skinned;
}

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint VertexIndex = DTid.x;
    if (VertexIndex < gSkinningInformation.numVertices)
    {
        Vertex input = gInputVertices[VertexIndex];
        VertexInfluence influence = gInfluences[VertexIndex];
        
        Vertex output;
        output.texcoord = input.texcoord;
        Skinned skinned = Skinning(input, influence);
        output.position = skinned.position;
        output.normal = skinned.normal;
        gOutputVertices[VertexIndex] = output;
    }
    
}