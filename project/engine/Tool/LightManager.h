#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include "Vector4.h"
#include "Vector3.h"
#include "RenderTypes.h"
#include "Singleton.h"

class LightManager : public Singleton<LightManager>
{
public:
    friend class Singleton<LightManager>;
    friend struct std::default_delete<LightManager>;

    // 初期化・更新・描画・終了処理
    void Initialize();
    void Update();
    void Draw(UINT rootParameterIndex);
    void Finalize();

    // Dirty flag 付きライトデータ構造体
    struct DirectionalLightData {
        Vector4 color;
        Vector3 direction;
        float intensity;
        bool isDirty = true;  // 変更フラグ
        int gpuSlot = -1;     // GPU StructuredBuffer 上のスロットインデックス
    };

    struct PointLightData {
        Vector4 color;        // ライトの色
        Vector3 position;     // ライトの位置
        float intensity;      // 明るさ
        float radius;         // 影響半径
        float decay;          // 減衰率
        float padding[2];
        bool isDirty = true;  // 変更フラグ
        int gpuSlot = -1;     // GPU StructuredBuffer 上のスロットインデックス
    };

    struct SpotLightData {
        Vector4 color;          // ライトの色
        Vector3 position;       // ライトの位置
        float intensity;        // 明るさ
        Vector3 direction;      // ライトの向き
        float distance;         // 照射距離
        float decay;            // 減衰率
        float cosAngle;         // スポット角度
        float cosFalloffStart;  // フォールオフ開始角度
        float padding;
        bool isDirty = true;    // 変更フラグ
        int gpuSlot = -1;       // GPU StructuredBuffer 上のスロットインデックス
    };

    // ==========================================
    // ID ベース API (Task 2.5 推奨 API)
    // ==========================================

    // ライト追加 (返り値: 発行された一意のライトID)
    int AddDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);
    int AddPointLight(const Vector4& color, const Vector3& position, float intensity, float radius, float decay);
    int AddSpotLight(const Vector4& color, const Vector3& position, float intensity, const Vector3& direction, float distance, float decay, float cosAngle, float cosFalloffStart);

    // ライト削除
    void RemoveDirectionalLight(int id);
    void RemovePointLight(int id);
    void RemoveSpotLight(int id);

    // ライト設定 / 更新 (Dirty flag を自動セット)
    void SetDirectionalLight(int id, const Vector4& color, const Vector3& direction, float intensity);
    void SetPointLight(int id, const Vector4& color, const Vector3& position, float intensity, float radius, float decay);
    void SetSpotLight(int id, const Vector4& color, const Vector3& position, float intensity, const Vector3& direction, float distance, float decay, float cosAngle, float cosFalloffStart);

    // ライト取得 (ID指定)
    DirectionalLightData* GetDirectionalLightByID(int id);
    PointLightData* GetPointLightByID(int id);
    SpotLightData* GetSpotLightByID(int id);

    // ==========================================
    // カウント取得
    // ==========================================
    size_t GetDirectionalLightCount() const { return directionalLights_.size(); }
    size_t GetPointLightCount() const { return pointLights_.size(); }
    size_t GetSpotLightCount() const { return spotLights_.size(); }

    // ==========================================
    // レガシー互換 API (インデックス指定 / 全クリア)
    // ==========================================
    void ClearLights();
    DirectionalLightData& GetDirectionalLight(size_t index);
    PointLightData& GetPointLight(size_t index);
    SpotLightData& GetSpotLight(size_t index);
    void SetDirectionalLight(size_t index, const Vector4& color, const Vector3& direction, float intensity);
    void SetPointLight(size_t index, const Vector4& color, const Vector3& position, float intensity, float radius, float decay);
    void SetPointLightPos(size_t index, const Vector3& position);
    void SetSpotLight(size_t index, const Vector4& color, const Vector3& position, float intensity, const Vector3& direction, float distance, float decay, float cosAngle, float cosFalloffStart);
    void SetSpotLightDirection(size_t index, const Vector3& direction);

protected:
    LightManager() = default;
    ~LightManager() override = default;

private:
    // バッファ生成ヘルパー (既存リソースからのコピー付き再割り当て対応)
    void CreateStructuredBuffer(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, ID3D12Resource* oldResource = nullptr, size_t oldSizeInBytes = 0);
    void CreateConstBuffer(size_t sizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& resource);

    // 動的バッファ再割り当て
    void ReallocateDirectionalLights(size_t newCapacity);
    void ReallocatePointLights(size_t newCapacity);
    void ReallocateSpotLights(size_t newCapacity);

private:
    // ライト ID 採番用カウンタ
    int nextLightID_ = 0;

    // ID ベースデータ保持用コンテナ
    std::unordered_map<int, DirectionalLightData> directionalLights_;
    std::unordered_map<int, PointLightData> pointLights_;
    std::unordered_map<int, SpotLightData> spotLights_;

    // GPU スロット (0..N-1) と ライト ID の対応付け配列
    std::vector<int> dirLightSlotToId_;
    std::vector<int> pointLightSlotToId_;
    std::vector<int> spotLightSlotToId_;

    // 動的バッファ最大許容容量 (超えたら2倍に拡張)
    size_t maxDirectionalLights_ = 16;
    size_t maxPointLights_ = 16;
    size_t maxSpotLights_ = 16;

    // GPU リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> dirLightBuff_;
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightBuff_;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightBuff_;

    // DrawIndirect / カウントバッファ (IndirectArguments 互換)
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgumentsBuffer_;
    IndirectArguments* indirectData_ = nullptr;
};
