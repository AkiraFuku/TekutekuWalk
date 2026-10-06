struct VertexShaderOutput{
    float4 position : SV_POSITION; // Position in clip space
    float2 texCoord : TEXCOORD0; // Texture coordinates
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
};
struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTranspaseMatrix;
};
struct Skinned
{
    float4 position;
    float3 normal;
};

// ==========================================
// 1. 頂点データ構造体 (InputVertices / OutputVertices 用)
// ==========================================
// C++側の Model::VertexData と一致させます
struct Vertex
{
    float4 position; // 頂点座標 (x, y, z, w)
    float2 texcoord; // テクスチャ座標 (u, v)
    float3 normal; // 法線ベクトル (x, y, z)
};
// ==========================================
// 2. インフルエンスデータ構造体 (Influence 用)
// ==========================================
// C++側の Model::VertexInfluence と一致させます
struct VertexInfluence
{
    float4 weights; // ジョイントごとのウェイト (最大4つ)
    int4 jointIndices; // ジョイントインデックス (最大4つ)
};
// ==========================================
// 3. スキニング情報定数バッファ (SkinningInformation 用)
// ==========================================
// スレッドの範囲外アクセスを防ぐために頂点数を渡します
struct SkinningInformation
{
    uint numVertices; // モデルの総頂点数
};
