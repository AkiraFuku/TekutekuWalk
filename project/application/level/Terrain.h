#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Transform.h"
#include "Camera.h"
#include "LevelData.h"
#include "DrawFunction.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <memory>

class RailPath;

/// <summary>
/// 地形グリッド専用クラス（Terrain）
/// 専用PSO（シェーダー）を持ち、落とし穴のマス目抜き・縁の三角ポリゴン化、
/// および道・地面テクスチャのマルチマッピング描画を行う。
/// </summary>
class Terrain
{
public:
    /// セル属性
    enum class CellType : uint8_t {
        kGround   = 0, // 通常の地面（草）
        kRoad     = 1, // コースの道
        kHole     = 2, // 完全な穴（ポリゴン削除）
        kEdgeTri0 = 3, // 穴の縁（三角形0のみ残す）
        kEdgeTri1 = 4, // 穴の縁（三角形1のみ残す）
    };

    struct VertexData {
        Vector4 position;
        Vector2 texCoord;
        Vector3 normal;
        float   roadAttr;   // 0.0 = 地面(草), 1.0 = 道
        Vector3 extendInfo; // xy: XZ押し出し方向, z: 延長ウェイト (0.0=元の端, 1.0=地平線)
    };

    struct MaterialData {
        Vector4 color;
        float   uvTile;
        Vector3 cameraPos;
    };

    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        float horizonExtendDist;
        float horizonDropY;
        float sizeX;
        float sizeY;
    };

    Terrain() = default;
    ~Terrain() = default;

    void Initialize(const LevelObjectData& data, Camera* camera);
    void Update();
    void Draw();

    // ─── ゲッター / セッター ──────────────────────────────────────────
    const std::vector<Triangle>& GetWorldTriangles() const { return triangles_; }
    void SetCamera(Camera* camera) { camera_ = camera; }

    const Vector3& GetTranslate() const { return transform_.translate; }
    const Vector3& GetRotate()    const { return transform_.rotate; }
    const Vector3& GetScale()     const { return transform_.scale; }

    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }
    void SetRotate(const Vector3& rotate)       { transform_.rotate = rotate; }
    void SetScale(const Vector3& scale)         { transform_.scale = scale; }

    /// <summary>
    /// ワールド座標からその位置のセル属性（GROUND / ROAD / HOLE）を取得
    /// </summary>
    CellType GetSurfaceType(const Vector3& worldPos) const;

    /// <summary>
    /// StageRail の軌道に沿って地形頂点の roadAttr（道テクスチャ領域）を自動設定する
    /// </summary>
    void ApplyRailRoad(const RailPath* rail, float roadHalfWidth = 1.6f);

private:
    void BuildMesh(const LevelObjectData& data);
    void SetupPSO();

private:
    std::string name_;
    EulerTransform transform_{};
    Camera* camera_ = nullptr;

    float sizeX_ = 160.0f;
    float sizeY_ = 240.0f;
    int   divX_  = 64;
    int   divY_  = 80;
    float uvTile_ = 10.0f;

    std::string grassTexturePath_;
    std::string roadTexturePath_;
    uint32_t grassTexIndex_ = 0;
    uint32_t roadTexIndex_  = 0;

    std::vector<CellType> cellTypes_;
    std::vector<VertexData> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<Triangle> triangles_; // 接地・壁判定用ワールド三角形

    // DirectX12 バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    MaterialData* materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
    TransformationMatrix* wvpData_ = nullptr;
};
