#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <memory>

#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "RenderTypes.h"
#include "PbdSolver.h"
#include "PSOManager.h"

/// <summary>
/// 2Dスクリーン座標系に対応したPBDクロスシミュレーションクラス
/// </summary>
class PbdCloth2D {
public:
    struct VertexData {
        Vector4 position; //!< 頂点座標 (w=1.0f)
        Vector2 texcoord; //!< UV座標
        Vector3 normal;   //!< 法線 (Spriteシェーダー互換)
    };

    PbdCloth2D() = default;
    ~PbdCloth2D() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="position">画面上の配置位置（ピクセル座標）</param>
    /// <param name="size">幅と高さ（ピクセル単位）</param>
    /// <param name="gridX">横の格子数 (例: 16)</param>
    /// <param name="gridY">縦の格子数 (例: 16)</param>
    /// <param name="k">バネの硬さ (0.0f〜1.0f)</param>
    /// <param name="pinMode">固定モード (0: 上辺両端のみ, 1: 上辺全体, 2: 四隅固定)</param>
    void Initialize(const Vector2& position, const Vector2& size, int gridX = 16, int gridY = 16,
                    float k = 0.35f, int pinMode = 0);

    /// <summary>
    /// 毎フレーム更新（物理シミュレーションと頂点バッファの同期）
    /// </summary>
    void Update();

    /// <summary>
    /// 描画（スプライトパイプラインを使用して描画）
    /// </summary>
    void Draw();

    /// <summary>
    /// クロスの再初期化（リセット）
    /// </summary>
    void Reset();

    // --- セッター ---
    void SetPosition(const Vector2& pos) { position_ = pos; }
    void SetSize(const Vector2& size) { size_ = size; }
    void SetRotation(float rot) { rotation_ = rot; }
    void SetAnchorPoint(const Vector2& anchor) { anchorPoint_ = anchor; }
    void SetTexture(const std::string& filePath);
    void SetColor(const Vector4& color);
    void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
    void SetPinMode(int pinMode);

    // 物理パラメータ
    void SetWindEnabled(bool enabled) { windEnabled_ = enabled; }
    void SetWindStrength(float strength) { windStrength_ = strength; }
    void SetGravity(const Vector2& g) { baseGravity_ = g; }
    void SetK(float k) { stiffness_ = k; solver_.SetK(k); }
    void SetSolverIterations(int iterations) { solver_.SetSolverIterations(iterations); }

    /// <summary>
    /// 突風や外力を布の質点に加える
    /// </summary>
    void ApplyGust(float forceX);

    // --- ゲッター ---
    const Vector2& GetPosition() const { return position_; }
    Vector2& GetPosition() { return position_; }
    const Vector2& GetSize() const { return size_; }
    Vector2& GetSize() { return size_; }
    float GetRotation() const { return rotation_; }
    float& GetRotation() { return rotation_; }
    bool GetWindEnabled() const { return windEnabled_; }
    bool& GetWindEnabledRef() { return windEnabled_; }
    float GetWindStrength() const { return windStrength_; }
    float& GetWindStrengthRef() { return windStrength_; }
    int GetPinMode() const { return pinMode_; }
    float GetK() const { return stiffness_; }
    float& GetKRef() { return stiffness_; }
    Vector2& GetBaseGravityRef() { return baseGravity_; }

    PbdSolver& GetSolver() { return solver_; }
    const PbdSolver& GetSolver() const { return solver_; }

private:
    void CreateBuffers();
    void CreateConstantBuffers();
    void UpdateVertices();

    PbdSolver solver_;
    int gridX_ = 16;
    int gridY_ = 16;
    int pinMode_ = 0; // 0: 上辺両端のみ, 1: 上辺全体
    float stiffness_ = 0.35f;

    Vector2 position_ = { 640.0f, 200.0f };
    Vector2 size_ = { 600.0f, 250.0f };
    Vector2 anchorPoint_ = { 0.5f, 0.0f }; // 上部中央基準
    float rotation_ = 0.0f;

    // 風・揺れパラメータ
    bool windEnabled_ = true;
    float windStrength_ = 180.0f; // 横風加速度
    float windTimer_ = 0.0f;
    Vector2 baseGravity_ = { 0.0f, 600.0f }; // 下向き重力

    // Direct3D12 リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    VertexData* vertexDataPtr_ = nullptr;
    UINT vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW ibv_{};
    UINT indexCount_ = 0;

    // 定数バッファ (Spriteシェーダー互換)
    // RootRegister[0]: MaterialCB
    // RootRegister[1]: TransformationMatrix
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    MaterialCB* materialData_ = nullptr;
    TransformationMatrix* transformationData_ = nullptr;

    std::string textureFilePath_ = "resources/default/white.png";
    uint32_t textureIndex_ = 0;
    BlendMode blendMode_ = BlendMode::Normal;
};
