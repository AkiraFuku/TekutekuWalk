#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include <d3d12.h>
#include <wrl/client.h>
#include "Transform.h"
#include "Camera.h"
#include "../base/RenderTypes.h"
#include "IInspectable.h"
class SkyBox : public IEditorInspectable
{

public:
    virtual ~SkyBox() override;

    std::string GetInspectorName() const override { return "SkyBox"; }
    std::string GetInspectorCategory() const override { return "Environment"; }
    Vector3* GetInspectorRotationPtr() override { return &worldTransform_.rotate; }
    Vector3* GetInspectorScalePtr() override { return &worldTransform_.scale; }
    bool* GetInspectorVisiblePtr() override { return &isVisible_; }
    void OnInspectorGUI() override;



    struct VertexData
    {
        Vector4 position;
        Vector3 texcord;


    };

    struct Material
    {
        Vector4  color;
    };
    // TransformationMatrix は engine/base/RenderTypes.h に移動
    void Initialize();
    void Finalize();
    void Update();
    void Draw();


    //トランスフォームセッター
    void SetScale(const Vector3& scale) {
        worldTransform_.scale = scale;
    }
    void SetRotate(const Vector3& rotate) {
        worldTransform_.rotate = rotate;
    }
    void SetTranslate(const Vector3& translate) {
        worldTransform_.translate = translate;
    }
    void SetCamera(Camera* camera) {
        camera_ = camera;
    }


    //トランスフォームゲッター
    const Vector3& GetScale()const {
        return worldTransform_.scale;
    }
    const Vector3& GetRotate()const {
        return worldTransform_.rotate;
    }
    const Vector3& GetTranslate()const {
        return worldTransform_.translate;
    }

    // マテリアル
    const  Material* GetMaterial()const {
        return materialData_;
    }


  
    void SetTextureByFilePath(const std::string& textureFilePath);

    uint32_t GetTextureIndex() const {
        return textureIndex_;
    }
private:
    Camera* camera_ = nullptr;
    //buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexRecourse_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    VertexData* vertexData_ = nullptr;
    uint32_t* indexData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;
    //マテリアル
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    //WVP行列リソース（0: Gameビュー, 1: Sceneビュー）
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResources_[2];
    TransformationMatrix* wvpResources_[2] = { nullptr, nullptr };
    EulerTransform worldTransform_ = {};
    uint32_t textureIndex_ = 0;
    std::string textureFilePath_;
    bool isVisible_ = true;

};

