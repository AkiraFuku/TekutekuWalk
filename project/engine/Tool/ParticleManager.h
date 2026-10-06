#pragma once
#include "Vector4.h"
#include "Vector2.h"
#include<random>
#include<list>
#include "DXCommon.h"
#include <wrl/client.h>
#include "d3d12.h"
#include <cstdint>
#include "Camera.h"
#include "Transform.h"
#include <map>
#include <unordered_map>
#include <functional>
#include <string>
#include <variant>


class ParticleManager
{
public:

    struct MaterialData {
        std::string textureFilePath;
        uint_fast16_t textureIndex = 0;
    };
    struct VertexData {
        Vector4 position; // 4D position vector
        Vector2 record; // 2D texture coordinate vector
        Vector3 normal;
    };
    struct Material
    {
        Vector4 color;
        int32_t enableLighting;
        float padding[3]; // パディングを追加してサイズを揃える
        Matrix4x4 uvTransform; // UV変換行列
    };
    struct Particle
    {
        EulerTransform transform;
        Vector3 velocity;
        Vector4 color;
        float lifeTime;
        float currentTime;
        // 追加：UV変換行列
        UVTransform uvTransform;

    };
    struct ParticleCS
    {
        Vector3 position;
        float scale;
        Vector3 velocity;
        Vector4 color;
        float lifeTime;
        float currentTime;
    };

    struct CylinderData
    {
        Vector3 Offset = { 0.0f,0.0f,0.0f };//オフセット
        int32_t pudding[1]; // パディングを追加してサイズを揃える
        Vector2 TopRadius = { 0.0f,0.0f };//上の半径


        Vector2 BottomRadius = { 0.0f,0.0f };//下の半径

        float height = 1.0f;
        // 上の半径と下の半径を統一するか（上下の形状を同期するかどうか）
        bool isUniformTopBottom = false;

        // 半径をXとYで統一するか（楕円ではなく、正円にするかどうか）
        bool isCircularRadius = true;


    };

    // 空の状態を表すための型
    struct EmptyData {};

    // ★追加: いずれかの構造体を1つだけ持てる「可変型の設定データ」
    using EffectSpecificData = std::variant<EmptyData, CylinderData>;

    using ParticleEmitterFunc = std::function<Particle(const Vector3&, std::mt19937&)>;
    using ParticleUpdateFunc = std::function<void(Particle&, float)>;

    struct ParticleForGPU
    {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Vector4 color;
        Matrix4x4 uvTransform; // 追加：インスタンスごとのUV変換行列
    };

    enum class EffectType
    {
        Plane,
        Ring,
        Cylinder

    };

    struct ParticleGroup {

        // 追加：生成時の振る舞いを定義する関数
        ParticleEmitterFunc initialize;
        // 追加：更新時の振る舞いを定義する関数 (オプション)
        ParticleUpdateFunc update;

        MaterialData materialData;
        std::list<Particle> particles;
        uint32_t instancingSrvIndex;
        Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
        ParticleForGPU* instancingData = nullptr;
        uint32_t kNumInstance;
        std::string name;
        EffectType effectType;
    };

    // =====================================================
    // ParticleGroupSet: 複数の ParticleGroup をまとめる構造体
    // =====================================================
    struct ParticleGroupSet {
        std::string name;
        std::unordered_map<std::string, ParticleGroup> groups; // グループ名 -> ParticleGroup
        bool isActive = true; // false にするとセット全体が更新・描画されない
    };

    struct PrimitiveResource {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        D3D12_VERTEX_BUFFER_VIEW vbv;
        uint32_t vertexCount;
    };



    void Initialize();
    void Update();
    void Draw();

    // ---- セット単位の操作 ----
    // セットを作成する（空のセット）
    void CreateParticleGroupSet(const std::string& setName);

    // セット内にパーティクルグループを追加する
    void CreateParticleGroup(
        const std::string& setName,
        const std::string& groupName,
        const std::string& textureFilepath,
        EffectType type = EffectType::Plane,
        ParticleEmitterFunc initialize = nullptr,
        ParticleUpdateFunc update = nullptr
    );

    // セット全体を解放する
    void ReleaseParticleGroupSet(const std::string& setName);

    // 全セットを解放する
    void ReleaseAllParticleGroupSets();

    // セットのisActiveを設定する
    void SetParticleGroupSetActive(const std::string& setName, bool active);

    static ParticleManager* GetInstance();

    // setName のセット内の groupName グループにパーティクルを発生させる
    void Emit(const std::string& setName, const std::string& groupName, const Vector3& position, uint32_t count);

    void Finalize();
    void SetCamera(Camera* camera) {
        camera_ = camera;
    }

    // 後方互換のため残す（全セット削除）
    void ReleaseParticleGroup() {
        ReleaseAllParticleGroupSets();
    }

    // 最上位コンテナ（外部から参照したい場合に公開）
    std::unordered_map<std::string, ParticleGroupSet> particleGroupSets;

    friend struct std::default_delete<ParticleManager>;
    static std::unique_ptr<ParticleManager> instance;

    std::vector<ParticleManager::VertexData> PrimitiveVertexPlane();
    std::vector<ParticleManager::VertexData> PrimitiveVertexRing();
    std::vector<ParticleManager::VertexData> PrimitiveVertexCylinder();
    Particle MakeParticle(std::mt19937& randomEngine, const Vector3& translate);

private:
    ParticleManager() = default;
    ~ParticleManager() = default;
    ParticleManager(ParticleManager&) = delete;
    ParticleManager& operator=(ParticleManager&) = delete;

    static uint32_t kMaxNumInstance;

    // 実際にGPUリソースを生成する内部ヘルパー
    void CreateParticleGroupInternal(ParticleGroupSet& set,
        const std::string& groupName,
        const std::string& textureFilepath,
        EffectType type,
        ParticleEmitterFunc initialize,
        ParticleUpdateFunc update);

    std::random_device seedGen_;
    std::mt19937 randomEngine_;
    HRESULT hr_ = 0;


    //頂点リソース
    std::map<EffectType, PrimitiveResource> primitiveResources_;
    VertexData* vertexData_ = nullptr;
    //  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
      //マテリアル
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;
    void CreateVertexBuffer(EffectType type);
    void CreateMaterialBuffer();
    //  void CreatePSO();




    Camera* camera_ = nullptr;
};

