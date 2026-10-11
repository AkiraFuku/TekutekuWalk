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
#include "Camera.h"

class PbdCloth {
public:
    struct VertexData {
        Vector4 position; // 頂点座標 (w=1.0f)
        Vector2 texcoord; // UV座標
        Vector3 normal;   // 法線
    };

    PbdCloth() = default;
    ~PbdCloth() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(const Vector3& startPos, const Vector3& endPos, int width, int height,
                    float k = 0.1f, float dt = 1.0f / 60.0f, float kDamping = 0.05f,
                    const Vector3& gravity = { 0.0f, -9.8f, 0.0f });

    /// <summary>
    /// 毎フレーム更新（物理計算 + 頂点/法線バッファ更新）
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    // --- セッター ---
    void SetTexture(const std::string& filePath);
    void SetColor(const Vector4& color);
    void SetEnableLighting(bool enable);
    void SetCamera(Camera* camera) { camera_ = camera; }

    // トランスフォーム制御
    void SetTranslate(const Vector3& translate) { translate_ = translate; }
    void SetRotate(const Vector3& rotate) { rotate_ = rotate; }
    void SetScale(const Vector3& scale) { scale_ = scale; }
    const Vector3& GetTranslate() const { return translate_; }
    const Vector3& GetRotate() const { return rotate_; }
    const Vector3& GetScale() const { return scale_; }

    /// 物理パラメータのセッター
    /// <summary>
    /// スタート地点
    /// </summary>
    void SetStartPos(const Vector3& pos) { solver_.SetStartPos(pos); }
    void SetEndPos(const Vector3& pos) { solver_.SetEndPos(pos); }
    /// <summary>
    /// 幅・高さ (格子数)
    /// </summary>
    void SetK(float k) { solver_.SetK(k); }
    void SetDt(float dt) { solver_.SetDt(dt); }
    void SetMass(float mass) { solver_.SetMass(mass); }
    void SetGravity(const Vector3& g) { solver_.SetGravity(g); }
    void SetFixed(int x, int y, bool isFixed) { solver_.SetFixed(x, y, isFixed); }

    // --- ゲッター ---
    PbdSolver& GetSolver() { return solver_; }
    const PbdSolver& GetSolver() const { return solver_; }

private:
    void CreateBuffers();
    void CreateConstantBuffers();
    void UpdateNormalsAndVertices();

    PbdSolver solver_;

    Camera* camera_ = nullptr;

    Vector3 translate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotate_    = { 0.0f, 0.0f, 0.0f };
    Vector3 scale_     = { 1.0f, 1.0f, 1.0f };

    // 頂点バッファ (動的更新・UPLOAD)
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    VertexData* vertexDataPtr_ = nullptr;
    UINT vertexCount_ = 0;

    // インデックスバッファ (静的・DEFAULT/UPLOAD)
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW ibv_{};
    UINT indexCount_ = 0;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> transformResource_; // slot 0: WVP
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;  // slot 1: Material
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;    // slot 3: Camera

    TransformationMatrix* transformData_ = nullptr;
    MaterialCB*           materialData_  = nullptr;
    CameraForGPU*         cameraData_    = nullptr;

    // テクスチャ
    std::string textureFilePath_ = "resources/default/white.png";
    uint32_t textureIndex_ = 0;
};
