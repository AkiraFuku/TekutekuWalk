#pragma once
#include "DXCommon.h"
#include "Camera.h"
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

class GpuParticleManager {
public:
    static constexpr uint32_t kMaxParticles = 1024;
    static constexpr uint32_t kMaxEmitRequestsPerFrame = 16;

    struct EmitRequest {
        Vector3 translate;
        float radius;
        uint32_t count;
    };

    struct ParticleCS {
        Vector3 translate;
        Vector3 scale;
        float lifeTime;
        Vector3 velocity;
        float currentTime;
        Vector4 color;
    };

    struct PerView {
        Matrix4x4 viewProjection;
        Matrix4x4 billboardMatrix;
    };

    struct PerFrame {
        float time;
        float deltaTime;
    };

    struct EmitterSphere {
        Vector3 translate;
        float radius;
        uint32_t count;
        float frequency;
        float frequencyTime;
        uint32_t emit;
    };

    struct VertexData {
        Vector4 position;
        Vector2 texCoord;
        Vector3 normal;
    };

    static GpuParticleManager* GetInstance();
    friend struct std::default_delete<GpuParticleManager>;

    void Initialize();
    void Finalize();
    void Update();
    void Draw();

    void SetCamera(Camera* camera) { camera_ = camera; }
    void SetActive(bool active) { isActive_ = active; }
    bool IsActive() const { return isActive_; }
    void SetAutoEmit(bool autoEmit) { autoEmit_ = autoEmit; }

    void Emit(const Vector3& position, uint32_t count = 24, float radius = 0.8f);

    void SetEmitterPosition(const Vector3& pos);
    void SetEmitterRadius(float radius);
    void SetEmitterCount(uint32_t count);
    void SetEmitterFrequency(float frequency);
    void SetTexture(const std::string& textureFilepath);

    // GPU側のバッファを再初期化するフラグを立てる（シーン切り替え時など）
    void ResetParticles();

private:
    GpuParticleManager() = default;
    ~GpuParticleManager() = default;
    GpuParticleManager(const GpuParticleManager&) = delete;
    GpuParticleManager& operator=(const GpuParticleManager&) = delete;

    void RegisterPsos();
    void CreateResources();
    void CreateVertexBuffer();

    static std::unique_ptr<GpuParticleManager> instance_;

    Camera* camera_ = nullptr;
    bool isActive_ = true;
    bool autoEmit_ = false;
    bool isInitializedGpu_ = false;
    bool isFreeListBarrierInitialized_ = false;
    D3D12_RESOURCE_STATES particleResourceState_ = D3D12_RESOURCE_STATE_COMMON;

    // GPUバッファ (DEFAULTヒープ)
    Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_;

    // ディスクリプタインデックス
    uint32_t particleUavIndex_ = 0;
    uint32_t particleSrvIndex_ = 0;
    uint32_t freeListIndexUavIndex_ = 0;
    uint32_t freeListUavIndex_ = 0;
    uint32_t textureSrvIndex_ = 0;

    // 定数バッファ (UPLOADヒープ)
    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
    PerView* perViewData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_;
    PerFrame* perFrameData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> emitterResource_;
    EmitterSphere* emitterData_ = nullptr;

    std::vector<EmitRequest> pendingEmits_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxEmitRequestsPerFrame> emitRequestResources_;
    std::array<EmitterSphere*, kMaxEmitRequestsPerFrame> emitRequestData_{};

    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    uint32_t vertexCount_ = 0;

    float totalTime_ = 0.0f;
};
