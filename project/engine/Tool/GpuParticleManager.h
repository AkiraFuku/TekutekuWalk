#pragma once
#include "DXCommon.h"
#include "Camera.h"
#include "PSOManager.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Vector2.h"
#include "MathFunction.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <cstdint>
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include "Singleton.h"

class GpuParticleManager : public Singleton<GpuParticleManager> {
public:
    static constexpr uint32_t kMaxParticles = 1024;
    static constexpr uint32_t kMaxEmitRequestsPerFrame = 64;

    enum class EffectType {
        Plane,
        Ring,
        Cylinder,
    };

    struct EmitterConfig {
        uint32_t count = 16;
        Vector3 emitRangeMin{ -0.2f, -0.2f, -0.2f };
        Vector3 emitRangeMax{  0.2f,  0.2f,  0.2f };
        float lifeTimeMin = 0.3f;
        float lifeTimeMax = 0.6f;
        Vector3 startScaleMin{ 0.3f, 0.3f, 0.3f };
        Vector3 startScaleMax{ 0.3f, 0.3f, 0.3f };
        Vector3 endScaleMin{ 0.0f, 0.0f, 0.0f };
        Vector3 endScaleMax{ 0.0f, 0.0f, 0.0f };
        Vector3 rotateMin{ 0.0f, 0.0f, 0.0f };
        Vector3 rotateMax{ 0.0f, 0.0f, 0.0f };
        Vector3 velocityMin{ -1.0f, -1.0f, -1.0f };
        Vector3 velocityMax{  1.0f,  1.0f,  1.0f };
        Vector4 startColorMin{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vector4 startColorMax{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vector4 endColorMin{ 1.0f, 1.0f, 1.0f, 0.0f };
        Vector4 endColorMax{ 1.0f, 1.0f, 1.0f, 0.0f };
        Vector3 angularVelocityMin{ 0.0f, 0.0f, 0.0f };
        Vector3 angularVelocityMax{ 0.0f, 0.0f, 0.0f };
        Vector2 uvOffset{ 0.0f, 0.0f };
        Vector2 uvScale{ 1.0f, 1.0f };
        Vector2 uvScrollSpeed{ 0.0f, 0.0f };
        float uvTwist = 0.0f;
        bool flipUvY = false;
        bool useUvWrap = false;
        float drag = 0.0f;
        float gravity = 0.0f;
    };

    struct EmitRequest {
        Vector3 position{};
        Vector3 direction{ 0.0f, 0.0f, 1.0f };
        bool useDirection = false;
    };

    struct ParticleCS {
        Vector3 translate;
        float lifeTime;
        Vector3 scale;
        float currentTime;
        Vector3 velocity;
        float drag;
        Vector3 rotate;
        float gravity;
        Vector3 startScale;
        float pad0;
        Vector3 endScale;
        float pad1;
        Vector4 color;
        Vector4 startColor;
        Vector4 endColor;
        Vector3 angularVelocity;
        float pad2;
        Vector2 uvOffset;
        Vector2 uvScale;
        Vector2 uvScrollSpeed;
        Vector2 pad3;
    };
    static_assert(sizeof(ParticleCS) % 16 == 0, "ParticleCS must be 16-byte aligned");

    struct PerView {
        Matrix4x4 viewProjection;
        Matrix4x4 billboardMatrix;
        uint32_t useBillboard;
        uint32_t useVelocityStretch;
        uint32_t flipUvY;
        float uvTwist;
    };
    static_assert(sizeof(PerView) % 16 == 0, "PerView must be 16-byte aligned");

    struct PerFrame {
        float time;
        float deltaTime;
    };

    struct EmitterConfigGPU {
        Vector3 translate;
        uint32_t count;
        Vector3 emitRangeMin;
        float lifeTimeMin;
        Vector3 emitRangeMax;
        float lifeTimeMax;
        Vector3 startScaleMin;
        float drag;
        Vector3 startScaleMax;
        float gravity;
        Vector3 endScaleMin;
        float pad0;
        Vector3 endScaleMax;
        float pad1;
        Vector3 rotateMin;
        float pad2;
        Vector3 rotateMax;
        float pad3;
        Vector3 velocityMin;
        float pad4;
        Vector3 velocityMax;
        float pad5;
        Vector4 startColorMin;
        Vector4 startColorMax;
        Vector4 endColorMin;
        Vector4 endColorMax;
        Vector3 angularVelocityMin;
        float randomSeed;
        Vector3 angularVelocityMax;
        float pad6;
        Vector2 uvOffset;
        Vector2 uvScale;
        Vector2 uvScrollSpeed;
        Vector2 pad7;
    };
    static_assert(sizeof(EmitterConfigGPU) % 16 == 0, "EmitterConfigGPU must be 16-byte aligned");

    struct VertexData {
        Vector4 position;
        Vector2 texCoord;
        Vector3 normal;
    };

    struct PrimitiveResource {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        uint32_t vertexCount = 0;
    };

    struct GpuParticleGroup {
        std::string name;
        EffectType effectType = EffectType::Plane;
        EmitterConfig config{};
        bool useBillboard = true;
        bool useVelocityStretch = false;
        BlendMode blendMode = BlendMode::Add;
        uint32_t textureSrvIndex = 0;
        bool isActive = true;
        float seedOffset = 0.0f;
        uint32_t seedCounter = 0;

        bool isInitializedGpu = false;
        bool isFreeListBarrierInitialized = false;
        D3D12_RESOURCE_STATES particleResourceState = D3D12_RESOURCE_STATE_COMMON;

        Microsoft::WRL::ComPtr<ID3D12Resource> particleResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource;
        Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource;

        uint32_t particleUavIndex = 0;
        uint32_t particleSrvIndex = 0;
        uint32_t freeListIndexUavIndex = 0;
        uint32_t freeListUavIndex = 0;

        Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource;
        PerView* perViewData = nullptr;

        std::vector<EmitRequest> pendingEmits;
        std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxEmitRequestsPerFrame> emitRequestResources;
        std::array<EmitterConfigGPU*, kMaxEmitRequestsPerFrame> emitRequestData{};
    };

    friend class Singleton<GpuParticleManager>;
    friend struct std::default_delete<GpuParticleManager>;

    void Initialize();
    void Finalize();
    void Update();
    void Draw();

    void SetCamera(Camera* camera) { camera_ = camera; }
    void SetActive(bool active) { isActive_ = active; }
    bool IsActive() const { return isActive_; }

    // グループ・エフェクトセット登録
    void CreateParticleGroup(
        const std::string& groupName,
        const std::string& textureFilepath,
        const EmitterConfig& config,
        bool useBillboard = true,
        bool useVelocityStretch = false,
        BlendMode blendMode = BlendMode::Add,
        EffectType effectType = EffectType::Plane
    );
    void AddGroupToSet(const std::string& setName, const std::string& groupName);
    void RegisterEffectGroup(
        const std::string& setName,
        const std::string& groupName,
        const std::string& textureFilepath,
        const EmitterConfig& config,
        bool useBillboard = true,
        bool useVelocityStretch = false,
        BlendMode blendMode = BlendMode::Add,
        EffectType effectType = EffectType::Plane
    );

    // 発生リクエスト
    void Emit(const std::string& setName, const Vector3& position);
    void Emit(const std::string& setName, const Vector3& position, const Vector3& direction);
    void EmitGroup(const std::string& groupName, const Vector3& position);
    void EmitGroup(const std::string& groupName, const Vector3& position, const Vector3& direction);

    // 後方互換用
    void Emit(const Vector3& position, uint32_t count = 24, float radius = 0.8f);

    // セット・グループの有効制御と解放
    void SetEffectSetActive(const std::string& setName, bool active);
    bool IsEffectSetActive(const std::string& setName) const;
    void SetGroupActive(const std::string& groupName, bool active);
    void ReleaseEffectSet(const std::string& setName);

    // GPU側のバッファを再初期化するフラグを立てる（シーン切り替え時など）
    void ResetParticles();

protected:
    GpuParticleManager() = default;
    ~GpuParticleManager() override = default;

private:
    void RegisterPsos();
    void CreateSharedResources();
    void CreatePrimitiveBuffers();
    void CreatePrimitiveBuffer(EffectType type);
    std::vector<VertexData> PrimitiveVertexPlane();
    std::vector<VertexData> PrimitiveVertexRing();
    std::vector<VertexData> PrimitiveVertexCylinder();
    void InitGroupResources(GpuParticleGroup& group);
    EmitterConfigGPU MakeGpuConfig(GpuParticleGroup& group, const EmitRequest& req) const;

    Camera* camera_ = nullptr;
    bool isActive_ = true;

    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_;
    PerFrame* perFrameData_ = nullptr;

    std::unordered_map<EffectType, PrimitiveResource> primitiveResources_;

    std::unordered_map<std::string, std::unique_ptr<GpuParticleGroup>> groups_;
    std::vector<std::string> groupOrder_;
    std::unordered_map<std::string, std::vector<std::string>> effectSets_;
    std::unordered_map<std::string, bool> effectSetActive_;

    float totalTime_ = 0.0f;
};
