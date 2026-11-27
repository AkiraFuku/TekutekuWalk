#include "LightManager.h"
#include "DXCommon.h"
#include <cassert>
#include <algorithm>
#include <d3d12.h>
#include <cmath>
#include <wrl.h>
#include <functional>

void LightManager::Finalize() {
    if (indirectArgumentsBuffer_ && indirectData_) {
        indirectArgumentsBuffer_->Unmap(0, nullptr);
        indirectData_ = nullptr;
    }
    Singleton<LightManager>::Finalize();
}

void LightManager::Initialize() {
    // 初期最大容量
    maxDirectionalLights_ = 16;
    maxPointLights_ = 16;
    maxSpotLights_ = 16;
    nextLightID_ = 0;

    // StructuredBuffer の初期確保
    CreateStructuredBuffer(sizeof(DirectionalLight) * maxDirectionalLights_, dirLightBuff_);
    CreateStructuredBuffer(sizeof(PointLight) * maxPointLights_, pointLightBuff_);
    CreateStructuredBuffer(sizeof(SpotLight) * maxSpotLights_, spotLightBuff_);

    // 間接描画引数 / カウントバッファ作成 & マップ
    CreateConstBuffer(sizeof(IndirectArguments), indirectArgumentsBuffer_);
    indirectArgumentsBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indirectData_));
    if (indirectData_) {
        indirectData_->numDirectionalLights = 0;
        indirectData_->numPointLights = 0;
        indirectData_->numSpotLights = 0;
        indirectData_->padding = 0;
    }
}

void LightManager::ClearLights() {
    directionalLights_.clear();
    dirLightSlotToId_.clear();

    pointLights_.clear();
    pointLightSlotToId_.clear();

    spotLights_.clear();
    spotLightSlotToId_.clear();
}

// ==========================================
// 動的バッファ再割り当て (2倍拡張 & 既存データコピー)
// ==========================================

void LightManager::ReallocateDirectionalLights(size_t newCapacity) {
    size_t oldSize = sizeof(DirectionalLight) * maxDirectionalLights_;
    maxDirectionalLights_ = newCapacity;
    CreateStructuredBuffer(sizeof(DirectionalLight) * maxDirectionalLights_, dirLightBuff_, dirLightBuff_.Get(), oldSize);

    // バッファが変わったため全ライトを Dirty にマーク
    for (auto& pair : directionalLights_) {
        pair.second.isDirty = true;
    }
}

void LightManager::ReallocatePointLights(size_t newCapacity) {
    size_t oldSize = sizeof(PointLight) * maxPointLights_;
    maxPointLights_ = newCapacity;
    CreateStructuredBuffer(sizeof(PointLight) * maxPointLights_, pointLightBuff_, pointLightBuff_.Get(), oldSize);

    for (auto& pair : pointLights_) {
        pair.second.isDirty = true;
    }
}

void LightManager::ReallocateSpotLights(size_t newCapacity) {
    size_t oldSize = sizeof(SpotLight) * maxSpotLights_;
    maxSpotLights_ = newCapacity;
    CreateStructuredBuffer(sizeof(SpotLight) * maxSpotLights_, spotLightBuff_, spotLightBuff_.Get(), oldSize);

    for (auto& pair : spotLights_) {
        pair.second.isDirty = true;
    }
}

// ==========================================
// ID ベース API 実装
// ==========================================

int LightManager::AddDirectionalLight(const Vector4& color, const Vector3& direction, float intensity) {
    if (directionalLights_.size() >= maxDirectionalLights_) {
        ReallocateDirectionalLights(maxDirectionalLights_ * 2);
    }

    int id = nextLightID_++;
    int slot = static_cast<int>(dirLightSlotToId_.size());
    dirLightSlotToId_.push_back(id);

    DirectionalLightData data;
    data.color = color;
    data.direction = direction;
    data.intensity = intensity;
    data.isDirty = true;
    data.gpuSlot = slot;

    directionalLights_[id] = data;
    return id;
}

int LightManager::AddPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay) {
    if (pointLights_.size() >= maxPointLights_) {
        ReallocatePointLights(maxPointLights_ * 2);
    }

    int id = nextLightID_++;
    int slot = static_cast<int>(pointLightSlotToId_.size());
    pointLightSlotToId_.push_back(id);

    PointLightData data;
    data.color = color;
    data.position = position;
    data.intensity = intensity;
    data.radius = radius;
    data.decay = decay;
    data.padding[0] = 0.0f;
    data.padding[1] = 0.0f;
    data.isDirty = true;
    data.gpuSlot = slot;

    pointLights_[id] = data;
    return id;
}

int LightManager::AddSpotLight(const Vector4& color, const Vector3& position, float intensity, const Vector3& direction, float distance, float decay, float cosAngle, float cosFalloffStart) {
    if (spotLights_.size() >= maxSpotLights_) {
        ReallocateSpotLights(maxSpotLights_ * 2);
    }

    int id = nextLightID_++;
    int slot = static_cast<int>(spotLightSlotToId_.size());
    spotLightSlotToId_.push_back(id);

    SpotLightData data;
    data.color = color;
    data.position = position;
    data.intensity = intensity;
    data.direction = direction;
    data.distance = distance;
    data.decay = decay;
    data.cosAngle = cosAngle;
    data.cosFalloffStart = cosFalloffStart;
    data.padding = 0.0f;
    data.isDirty = true;
    data.gpuSlot = slot;

    spotLights_[id] = data;
    return id;
}

void LightManager::RemoveDirectionalLight(int id) {
    auto it = directionalLights_.find(id);
    if (it == directionalLights_.end()) {
        return;
    }

    int removedSlot = it->second.gpuSlot;
    int lastSlot = static_cast<int>(dirLightSlotToId_.size()) - 1;

    // スロットの Swap & Pop
    if (removedSlot != lastSlot) {
        int movedId = dirLightSlotToId_[lastSlot];
        dirLightSlotToId_[removedSlot] = movedId;
        directionalLights_[movedId].gpuSlot = removedSlot;
        directionalLights_[movedId].isDirty = true; // スロット位置が変わったためGPU転送が必要
    }
    dirLightSlotToId_.pop_back();
    directionalLights_.erase(it);
}

void LightManager::RemovePointLight(int id) {
    auto it = pointLights_.find(id);
    if (it == pointLights_.end()) {
        return;
    }

    int removedSlot = it->second.gpuSlot;
    int lastSlot = static_cast<int>(pointLightSlotToId_.size()) - 1;

    if (removedSlot != lastSlot) {
        int movedId = pointLightSlotToId_[lastSlot];
        pointLightSlotToId_[removedSlot] = movedId;
        pointLights_[movedId].gpuSlot = removedSlot;
        pointLights_[movedId].isDirty = true;
    }
    pointLightSlotToId_.pop_back();
    pointLights_.erase(it);
}

void LightManager::RemoveSpotLight(int id) {
    auto it = spotLights_.find(id);
    if (it == spotLights_.end()) {
        return;
    }

    int removedSlot = it->second.gpuSlot;
    int lastSlot = static_cast<int>(spotLightSlotToId_.size()) - 1;

    if (removedSlot != lastSlot) {
        int movedId = spotLightSlotToId_[lastSlot];
        spotLightSlotToId_[removedSlot] = movedId;
        spotLights_[movedId].gpuSlot = removedSlot;
        spotLights_[movedId].isDirty = true;
    }
    spotLightSlotToId_.pop_back();
    spotLights_.erase(it);
}

void LightManager::SetDirectionalLight(int id, const Vector4& color, const Vector3& direction, float intensity) {
    auto it = directionalLights_.find(id);
    if (it != directionalLights_.end()) {
        it->second.color = color;
        it->second.direction = direction;
        it->second.intensity = intensity;
        it->second.isDirty = true;
    }
}

void LightManager::SetPointLight(int id, const Vector4& color, const Vector3& position, float intensity, float radius, float decay) {
    auto it = pointLights_.find(id);
    if (it != pointLights_.end()) {
        it->second.color = color;
        it->second.position = position;
        it->second.intensity = intensity;
        it->second.radius = radius;
        it->second.decay = decay;
        it->second.isDirty = true;
    }
}

void LightManager::SetSpotLight(int id, const Vector4& color, const Vector3& position, float intensity, const Vector3& direction, float distance, float decay, float cosAngle, float cosFalloffStart) {
    auto it = spotLights_.find(id);
    if (it != spotLights_.end()) {
        it->second.color = color;
        it->second.position = position;
        it->second.intensity = intensity;
        it->second.direction = direction;
        it->second.distance = distance;
        it->second.decay = decay;
        it->second.cosAngle = cosAngle;
        it->second.cosFalloffStart = cosFalloffStart;
        it->second.isDirty = true;
    }
}

LightManager::DirectionalLightData* LightManager::GetDirectionalLightByID(int id) {
    auto it = directionalLights_.find(id);
    if (it != directionalLights_.end()) {
        return &it->second;
    }
    return nullptr;
}

LightManager::PointLightData* LightManager::GetPointLightByID(int id) {
    auto it = pointLights_.find(id);
    if (it != pointLights_.end()) {
        return &it->second;
    }
    return nullptr;
}

LightManager::SpotLightData* LightManager::GetSpotLightByID(int id) {
    auto it = spotLights_.find(id);
    if (it != spotLights_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ==========================================
// レガシー互換 API 実装
// ==========================================

LightManager::DirectionalLightData& LightManager::GetDirectionalLight(size_t index) {
    assert(index < dirLightSlotToId_.size() && "DirectionalLight index out of range.");
    int id = dirLightSlotToId_[index];
    return directionalLights_[id];
}

LightManager::PointLightData& LightManager::GetPointLight(size_t index) {
    assert(index < pointLightSlotToId_.size() && "PointLight index out of range.");
    int id = pointLightSlotToId_[index];
    return pointLights_[id];
}

LightManager::SpotLightData& LightManager::GetSpotLight(size_t index) {
    assert(index < spotLightSlotToId_.size() && "SpotLight index out of range.");
    int id = spotLightSlotToId_[index];
    return spotLights_[id];
}

void LightManager::SetDirectionalLight(size_t index, const Vector4& color, const Vector3& direction, float intensity) {
    if (index < dirLightSlotToId_.size()) {
        int id = dirLightSlotToId_[index];
        SetDirectionalLight(id, color, direction, intensity);
    }
}

void LightManager::SetPointLight(size_t index, const Vector4& color, const Vector3& position, float intensity, float radius, float decay) {
    if (index < pointLightSlotToId_.size()) {
        int id = pointLightSlotToId_[index];
        SetPointLight(id, color, position, intensity, radius, decay);
    }
}

void LightManager::SetPointLightPos(size_t index, const Vector3& position) {
    if (index < pointLightSlotToId_.size()) {
        int id = pointLightSlotToId_[index];
        auto it = pointLights_.find(id);
        if (it != pointLights_.end()) {
            it->second.position = position;
            it->second.isDirty = true;
        }
    }
}

void LightManager::SetSpotLight(size_t index, const Vector4& color, const Vector3& position, float intensity, const Vector3& direction, float distance, float decay, float cosAngle, float cosFalloffStart) {
    if (index < spotLightSlotToId_.size()) {
        int id = spotLightSlotToId_[index];
        SetSpotLight(id, color, position, intensity, direction, distance, decay, cosAngle, cosFalloffStart);
    }
}

void LightManager::SetSpotLightDirection(size_t index, const Vector3& direction) {
    if (index < spotLightSlotToId_.size()) {
        int id = spotLightSlotToId_[index];
        auto it = spotLights_.find(id);
        if (it != spotLights_.end()) {
            it->second.direction = direction;
            it->second.isDirty = true;
        }
    }
}

// ==========================================
// 更新と描画バインド
// ==========================================

void LightManager::Update() {
    // 1. IndirectArguments (カウント情報) の更新
    if (indirectData_) {
        indirectData_->numDirectionalLights = static_cast<uint32_t>(directionalLights_.size());
        indirectData_->numPointLights = static_cast<uint32_t>(pointLights_.size());
        indirectData_->numSpotLights = static_cast<uint32_t>(spotLights_.size());
    }

    // 2. Directional Lights: Dirty な要素のみ GPU バッファへ転送
    bool hasDirtyDir = false;
    for (const auto& pair : directionalLights_) {
        if (pair.second.isDirty) {
            hasDirtyDir = true;
            break;
        }
    }
    if (hasDirtyDir && !directionalLights_.empty()) {
        DirectionalLight* dst = nullptr;
        dirLightBuff_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
        if (dst) {
            for (auto& pair : directionalLights_) {
                if (pair.second.isDirty && pair.second.gpuSlot >= 0) {
                    DirectionalLight gpuLight;
                    gpuLight.color = pair.second.color;
                    gpuLight.direction = pair.second.direction;
                    gpuLight.intensity = pair.second.intensity;
                    dst[pair.second.gpuSlot] = gpuLight;
                    pair.second.isDirty = false;
                }
            }
            dirLightBuff_->Unmap(0, nullptr);
        }
    }

    // 3. Point Lights: Dirty な要素のみ GPU バッファへ転送
    bool hasDirtyPoint = false;
    for (const auto& pair : pointLights_) {
        if (pair.second.isDirty) {
            hasDirtyPoint = true;
            break;
        }
    }
    if (hasDirtyPoint && !pointLights_.empty()) {
        PointLight* dst = nullptr;
        pointLightBuff_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
        if (dst) {
            for (auto& pair : pointLights_) {
                if (pair.second.isDirty && pair.second.gpuSlot >= 0) {
                    PointLight gpuLight;
                    gpuLight.color = pair.second.color;
                    gpuLight.position = pair.second.position;
                    gpuLight.intensity = pair.second.intensity;
                    gpuLight.radius = pair.second.radius;
                    gpuLight.decay = pair.second.decay;
                    gpuLight.padding[0] = pair.second.padding[0];
                    gpuLight.padding[1] = pair.second.padding[1];
                    dst[pair.second.gpuSlot] = gpuLight;
                    pair.second.isDirty = false;
                }
            }
            pointLightBuff_->Unmap(0, nullptr);
        }
    }

    // 4. Spot Lights: Dirty な要素のみ GPU バッファへ転送
    bool hasDirtySpot = false;
    for (const auto& pair : spotLights_) {
        if (pair.second.isDirty) {
            hasDirtySpot = true;
            break;
        }
    }
    if (hasDirtySpot && !spotLights_.empty()) {
        SpotLight* dst = nullptr;
        spotLightBuff_->Map(0, nullptr, reinterpret_cast<void**>(&dst));
        if (dst) {
            for (auto& pair : spotLights_) {
                if (pair.second.isDirty && pair.second.gpuSlot >= 0) {
                    SpotLight gpuLight;
                    gpuLight.color = pair.second.color;
                    gpuLight.position = pair.second.position;
                    gpuLight.intensity = pair.second.intensity;
                    gpuLight.direction = pair.second.direction;
                    gpuLight.distance = pair.second.distance;
                    gpuLight.decay = pair.second.decay;
                    gpuLight.cosAngle = pair.second.cosAngle;
                    gpuLight.cosFalloffStart = pair.second.cosFalloffStart;
                    gpuLight.padding = pair.second.padding;
                    dst[pair.second.gpuSlot] = gpuLight;
                    pair.second.isDirty = false;
                }
            }
            spotLightBuff_->Unmap(0, nullptr);
        }
    }
}

void LightManager::Draw(UINT rootParameterIndex) {
    auto* cmdList = DXCommon::GetInstance()->GetCommandList().Get();
    // Dir (Index + 0)
    cmdList->SetGraphicsRootShaderResourceView(rootParameterIndex + 0, dirLightBuff_->GetGPUVirtualAddress());
    // Point (Index + 1)
    cmdList->SetGraphicsRootShaderResourceView(rootParameterIndex + 1, pointLightBuff_->GetGPUVirtualAddress());
    // Spot (Index + 2)
    cmdList->SetGraphicsRootShaderResourceView(rootParameterIndex + 2, spotLightBuff_->GetGPUVirtualAddress());
    // IndirectArguments / Count (Index + 3)
    cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex + 3, indirectArgumentsBuffer_->GetGPUVirtualAddress());
}

// ==========================================
// バッファ作成ヘルパー
// ==========================================

void LightManager::CreateStructuredBuffer(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, ID3D12Resource* oldResource, size_t oldSizeInBytes) {
    Microsoft::WRL::ComPtr<ID3D12Resource> newResource = DXCommon::GetInstance()->CreateBufferResource(sizeInBytes);

    if (oldResource && oldSizeInBytes > 0) {
        void* src = nullptr;
        void* dst = nullptr;
        oldResource->Map(0, nullptr, &src);
        newResource->Map(0, nullptr, &dst);
        if (src && dst) {
            size_t copySize = (sizeInBytes < oldSizeInBytes) ? sizeInBytes : oldSizeInBytes;
            memcpy(dst, src, copySize);
        }
        if (dst) {
            newResource->Unmap(0, nullptr);
        }
        if (src) {
            oldResource->Unmap(0, nullptr);
        }
    }

    resource = newResource;
}

void LightManager::CreateConstBuffer(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& resource) {
    resource = DXCommon::GetInstance()->CreateBufferResource(sizeInBytes);
}
