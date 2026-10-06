#pragma once
#include <vector>
#include "Vector2.h"
#include "Vector4.h"
#include "Vector3.h"
#include <wrl.h>
#include <d3d12.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "MathFunction.h"

#include "Animation.h"

#include "Transform.h"
#include <optional>
#include <utility>
#include <span>
#include<array>
#include "DrawFunction.h"
class Model
{
public:
    struct VertexData {
        Vector4 position; // 4D position vector
        Vector2 texcord; // 2D texture coordinate vector
        Vector3 normal;
    };
    struct Material {
        Vector4 color;
        int32_t enableLighting = 1;
        int32_t environment = 1;
        int32_t diffuseType;  // 0:Lambert, 1:Half-Lambert
        int32_t specularType; // 0:None, 1:Phong, 2:BlinnPhong
        Matrix4x4 uvTransform; // UV変換行列
        float  shininess;
        float  environmentCoefficient;//反射率

    };
    struct MaterialData {
        std::string textureFilePath;
        uint_fast16_t textureIndex = 0;
    };
    struct Node
    {
        QuaternionTransform transform;
        Matrix4x4 localMatrix = Makeidentity4x4();
        std::string name;
        std::vector <Node>children;
    };
    struct VertexWeightData {
        float weight;
        uint32_t vertexIndex;
    };
    struct JointWeightData {
        Matrix4x4 inverseBindMatrix;
        std::vector<VertexWeightData> vertexWeights;

    };

    struct ModelData {
        std::map<std::string, JointWeightData>skinClusterData; // ジョイント名とそのウェイトデータのマップ
        std::vector<VertexData> vertices; // 頂点データの配列
        std::vector<uint32_t> indices; // インデックスデータの配列
        MaterialData material; // マテリアルデータ
        Node rootNode;
    };

    struct Joint {
        QuaternionTransform transform;
        Matrix4x4 localMatrix;
        Matrix4x4 skeletonSpaceMatrix; // スケルトン行列
        std::string name;
        std::vector<int32_t> children;
        int32_t index;
        std::optional<int32_t> parentIndex;
    };

    struct Skeleton
    {
        int32_t rootIndex;
        std::map<std::string, int32_t> jointMap; // ジョイント名とインデックスのマップ   
        std::vector<Joint> joints; // ジョイントの配列

    };
    static const uint32_t kNumMaxInfluences = 4; // 頂点あたりの最大影響数

    struct  VertexInfluence
    {
        std::array<float, kNumMaxInfluences > weights; // 頂点に影響を与えるジョイントの重み
        std::array<int32_t, kNumMaxInfluences > jointIndices; // 頂点に影響を与えるジョイントのインデックス

    };
    struct WellForGPU
    {
        Matrix4x4 skeletonSpaceMatrix;
        Matrix4x4 skeletonInverseTransposeMatrix;

    };
    struct SkinningInformation {
        uint32_t numVertices;
    };
    struct SkinCluster
    {
        std::vector<Matrix4x4> inverseBindMatrices; // ジョイントの逆バインド行列の配列
        //
        Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource; // ジョイントの影響データを格納するGPUリソース
        D3D12_VERTEX_BUFFER_VIEW influenceBufferView; // ジョイントの影響データのバッファビュー
        std::span<VertexInfluence> mappedInfluences; // 頂点の影響データのスパン
        uint32_t influenceSrvIndex = 0; // インフルエンスのSRVインデックス
        //
        Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
        std::span<WellForGPU> mappedPalette;
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>paletteSrvHandle;
        uint32_t paletteSrvIndex;

        // コンピュートスキニング用
        uint32_t inputVerticesSrvIndex = 0; // 入力頂点バッファのSRV
        Microsoft::WRL::ComPtr<ID3D12Resource> outputVertexResource; // 出力頂点バッファ (UAV)
        uint32_t outputVerticesUavIndex = 0; // 出力頂点のUAVインデックス
        D3D12_VERTEX_BUFFER_VIEW outputVertexBufferView{}; // 出力頂点のVBV
        Microsoft::WRL::ComPtr<ID3D12Resource> skinningInfoResource; // スキニング情報定数バッファ (CBV)
        bool isFirstDispatch = true; // 初回バリア判定用
    };

    enum  DiffuseType
    {
        Lambert,
        HarfLambert
    };
    enum  SpecularType {
        NONE,
        Phong,
        BlinnPhong,
    };



    void Initialize(const std::string& directryPath, const std::string& filename);
    void Update();

    void Draw(const Matrix4x4& worldMatrix = Makeidentity4x4(), std::optional<uint32_t> customTextureIndex = std::nullopt);
    void DrawInstanced(uint32_t instanceCount);
    void SkinningDispatch();
    void SetAnimation(Animation* animation) {
        animation_ = animation;
    }
    void SetAnimationTime(float time) {

        if (animation_) {
            animation_->SetCurrentTime(time);
        }

    }

    ModelData GetModelData() {
        return modelData_;
    }
    //マテリアルの読み込み
    static MaterialData LoadMaterialTemplateFile(const std::string& directryPath, const std::string& filename);
    //OBJファイルの読み込み
    static ModelData LoadModelFile(const std::string& directryPath, const std::string& filename);

    static Model* CreateSphere(uint32_t subdivision = 16);

    static Model* CreatePlaneFromTex(const std::string& textureFilePath);

    static Node ReadNode(aiNode* node);
    static Skeleton CreateSkelton(const Node& rootNode);
    static int32_t CreateJoint(const Node& node, std::optional<int32_t> parent, std::vector<Joint>& joints);
    static SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData, const Microsoft::WRL::ComPtr<ID3D12Resource>& vertexResource);
public:
    bool HasSkinning() const {
        return !modelData_.skinClusterData.empty() ;
    }
public: // 外部入出力

    void SetName(const std::string& name) {
        name_ = name;
    }

public: // 外部入出力
    void SetColor(const Vector4& color) {
        materialData_->color = color;
    }
    Vector4 GetColor() const {
        return materialData_->color;
    }
    void SetEnableLighting(bool enable) {
        materialData_->enableLighting = enable ? 1 : 0;
    }
    bool GetEnableLighting() const {
        return materialData_->enableLighting != 0;
    }
    void SetEnvironment(bool enable) {
        materialData_->environment = enable ? 1 : 0;
    }
    bool GetEnvironment() const {
        return materialData_->environment != 0;
    }
    //UV移動
    void SetUVTransform(const UVTransform& uvTransform) {
        uvTransform_ = uvTransform;
    }
    UVTransform GetUVTransform() const {
        return uvTransform_;
    }
    void SetUVScale(const Vector2& scale) {
        uvTransform_.scale = scale;
    }
    Vector2 GetUVScale() const {
        return uvTransform_.scale;
    }
    void SetUVRotation(float rotation) {
        uvTransform_.rotate = rotation;
    }
    float GetUVRotation() const {
        return uvTransform_.rotate;
    }
    void SetUVOffset(const Vector2& offset) {
        uvTransform_.offset = offset;
    }
    Vector2 GetUVOffset() const {
        return uvTransform_.offset;
    }

    const std::vector<VertexData>& GetVertices() const {
        return modelData_.vertices;
    }
    const std::vector<uint32_t>& GetIndices() const {
        return modelData_.indices;
    }
    
    // ローカル空間の三角形リストを取得
    std::vector<Triangle> GetLocalTriangles() const;

    // 四角いモデル（Box）を動的に生成する
    static Model* CreateBox();

    /// <summary>
    /// 地形グリッドを頂点データから動的生成する（モデルファイル不要）
    /// </summary>
    /// <param name="sizeX">X方向のワールドサイズ（中心から±sizeX/2）</param>
    /// <param name="sizeY">Y方向のワールドサイズ（中心から±sizeY/2）</param>
    /// <param name="divisionsX">X方向の分割数</param>
    /// <param name="divisionsY">Y方向の分割数</param>
    /// <param name="textureFilePath">テクスチャファイルパス</param>
    /// <param name="uvTile">UVタイリング回数（1.0でメッシュ全体で0〜1）</param>
    static Model* CreateTerrainGrid(
        float sizeX, float sizeY,
        int divisionsX, int divisionsY,
        const std::string& textureFilePath,
        float uvTile = 1.0f);

    void SetTexture(std::string textureFilePath);

private:

    void DebugDrawSkeleton(const Matrix4x4& worldMatrix);

    ModelData modelData_;

    Animation* animation_ = nullptr;

    std::string name_ = "name";

    //頂点リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    VertexData* vertexData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    void CreateVertexBuffer();
    //インデックスリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    uint32_t* indexData_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;
    void CreateIndexBuffer();
    //マテリアルリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
    void CreateMaterialResource();
    void ApplyAnimation(Node& node, float time);
    void ApplyAnimation(float time);

    Skeleton skeleton_;

    void UpdateSkeleton();

    SkinCluster skinCluster_;

    void UpdateSkinCluster();
    bool hasSkinning_ = false; // 初期値は false

   

    UVTransform uvTransform_ = {};
};

