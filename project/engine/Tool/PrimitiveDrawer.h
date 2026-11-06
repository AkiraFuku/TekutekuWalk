#pragma once
#include <vector>
#include <wrl/client.h>
#include <d3d12.h>
#include "Vector4.h"
#include "Vector3.h"
#include <memory>

#include "PSOManager.h"

#include <map>

#include"DrawFunction.h"
class Camera;

class PrimitiveDrawer {
public:

    struct VertexData {
        Vector4 position;
        Vector4 color;
    };
    // プリミティブの種類
    enum class PrimtiveType {
        kLine,
        kTriangle,
        kPoint,
        kSpher,
        kGrid,
        kAABB,
        kSegment,
        Plane,
        kCount // 種類の総数
    };
    struct WVPMatrix {
        Matrix4x4 WVP;
    };
    struct DrawCommand {
        //uint32_t vertexStart; // 全体バッファ内の開始インデックス
        //uint32_t vertexCount; // 描画する頂点数

        uint32_t indexStart; // インデックスバッファ内の開始位置
        uint32_t indexCount; // 描画するインデックス数
        uint32_t baseVertex; // 頂点バッファのオフセット

        D3D_PRIMITIVE_TOPOLOGY topology;
        Toporogy psoTopology; // PSOManager用のトポロジ指定
        FillMode fillMode;
        BlendMode blendMode;
    };

    static PrimitiveDrawer* GetInstance();
    friend struct std::default_delete<PrimitiveDrawer>;
    void Initialize();
    void Finalize();
    void SetCamera(Camera* camera) {
        camera_ = camera;
    }
    void Draw();


    void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);
    void DrawTriangle(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector4& color, FillMode fillMode = FillMode::kSolid, BlendMode blendMode = BlendMode::Normal);

    void DrawSphere(const Sphere& sphere, const Vector4& color);
    void DrawGrid(); // 引数は内部のカメラ行列を使うため不要に
    void DrawAABB(const AABB& aabb, const Vector4& color);
    void DrawSegment(const Segment& segment, const Vector4& color);
    void DrawPlane(const Plane& plane, const Vector4& color);

    // 全体ON/OFF
    bool IsEnable() const { return isEnabled_; }
    void SetEnable(bool enable) { isEnabled_ = enable; }
    void ToggleEnable() { isEnabled_ = !isEnabled_; }

    // 種類別ON/OFF
    bool IsTypeEnable(PrimtiveType type) const {
        return isTypeEnabled_[static_cast<size_t>(type)];
    }
    void SetTypeEnable(PrimtiveType type, bool enable) {
        isTypeEnabled_[static_cast<size_t>(type)] = enable;
    }

    // ImGuiデバッグ用コントロール描画
    void DrawImGui();

private:
    PrimitiveDrawer() = default;
    ~PrimitiveDrawer() = default;
    static std::unique_ptr<PrimitiveDrawer> instance_;

   // 単一の大きな頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_;
    VertexData* vertexDataPtr_ = nullptr;

    // インデックスバッファ（追加）
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW ibv_{};
    uint32_t* indexDataPtr_ = nullptr;
    
    // CPU側の頂点一時保存と描画コマンド
    std::vector<VertexData> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<DrawCommand> commands_;

    static const uint32_t kMaxVertices = 1000000; // 十分なサイズを確保
    static const uint32_t kMaxIndices = 3000000; // インデックス用の最大数

    void AddPSO();


private:
  
    Microsoft::WRL::ComPtr<ID3D12Resource> WVPResource_;
    WVPMatrix* wvpData_ = nullptr;
    void WVPResourceCreate();

    Camera* camera_ = nullptr;

    bool isEnabled_ = true;
    bool isTypeEnabled_[static_cast<size_t>(PrimtiveType::kCount)] = {
        true, // kLine
        true, // kTriangle
        true, // kPoint
        true, // kSpher
        true, // kGrid
        true, // kAABB
        true, // kSegment
        true  // Plane
    };




};