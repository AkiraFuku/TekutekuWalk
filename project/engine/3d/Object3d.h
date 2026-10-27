#pragma once
#include <Vector4.h>
#include <Vector2.h>
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include "Model.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "PSOManager.h"
#include "MathFunction.h"
#include "SkyBox.h"
#include <numbers>
#include <optional>
#include "RenderTypes.h"

class Animation;
class Object3d
{

public:

    // 共通定義 (engine/base/RenderTypes.h) を使用します
    void Initialize();
    void Update();
    void Draw();
    void SetModel(const std::string& filePath);

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

    void SetBlendMode(BlendMode blendMode) {
        blendMode_ = blendMode;
    }
    void SetFillMode(FillMode fillMode) {
        fillMode_ = fillMode;
    }
    void SetAnimations(Animation* animation) {
        if (model_) {
            model_->SetAnimation(animation);
        }
    }
    void SetAnimationTime(float time) {
        if (model_) {
            model_->SetAnimationTime(time);
        }
    }
    void SetPsoName(const std::string& psoName) {
        psoName_ = psoName;
    }
    EulerTransform GetTransform() const {
        return worldTransform_;
    }
    void SetDepthTestEnable(bool enable) {
        isDepthTestEnable_ = enable;
    }
    
    // モデルゲッター
    Model* GetModel() const { return model_.get(); }

    // ワールド行列ゲッター
    Matrix4x4 GetWorldMatrix() const {
        if (wvpResource_) {
            return wvpResource_->World;
        }
        return Makeidentity4x4();
    }

    // ワールド空間の三角形リストを取得
    std::vector<Triangle> GetWorldTriangles() const;


    void SetModelColor(const Vector4& color) {
        if (model_) {
            model_->SetColor(color);
        }
    }
    Vector4 GetModelInstanceColor() const {
        if (model_) {
            return    model_->GetColor();
        }
        return { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルトの白色
    }






    void SetSkyBox(SkyBox* box) {
        box_ = box;
    }
    void SetUVTransform(const UVTransform& uvTransform) {
        if (model_)
        {
            model_->SetUVTransform(uvTransform);
        }
    }
    UVTransform GetUVTransform() const {
        if (model_)
        {
            return  model_->GetUVTransform();
        }
        return {}; // デフォルトのUVTransform
    }
    Vector2 GetUVScale() const {
        if (model_)
        {
            return  model_->GetUVScale();
        }
        return { 1.0f, 1.0f }; // デフォルトのスケール
    }
    float GetUVRotation() const {
         if (model_)
        {
            return  model_->GetUVRotation();
        }
        return 0.0f; // デフォルトの回転
    }
    Vector2 GetUVOffset() const {
       if (model_)
        {
            return  model_->GetUVOffset();
        }
        return { 0.0f, 0.0f }; // デフォルトのオフセット
    }

    // --- 単一軸用 (度数法) ---
void SetRotateXDegree(float degree) {
    worldTransform_.rotate.x =ToRadians(degree);
}
void SetRotateYDegree(float degree) {
    worldTransform_.rotate.y = ToRadians(degree);
}
void SetRotateZDegree(float degree) {
    worldTransform_.rotate.z = ToRadians(degree);
}

// --- Vector3用 (度数法) ---
void SetRotateDegree(const Vector3& rotateDegree) {
    worldTransform_.rotate.x =ToRadians(rotateDegree.x);
    worldTransform_.rotate.y = ToRadians(rotateDegree.y);
    worldTransform_.rotate.z = ToRadians(rotateDegree.z);
}
void SetTexture(const std::string& textureFilePath);
void SetTextureByIndex(uint32_t textureIndex) {
    customTextureIndex_ = textureIndex;
}
void ResetTexture() {
    customTextureIndex_ = std::nullopt;
}

// 空間湾曲率 (負値ならObject3dCommonのGlobalCurvatureを使用)
void SetCurvature(float curvature) { curvature_ = curvature; }
float GetCurvature() const { return curvature_; }

private:

    //float radius_ = 1.0f;
    std::shared_ptr<Model> model_ = nullptr;
    std::optional<uint32_t> customTextureIndex_ = std::nullopt; // 個別オーバーライド用テクスチャ

    //WVP行列リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    TransformationMatrix* wvpResource_ = nullptr;
    void CreateWVPResource();

    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
    CameraForGPU* cameraData_ = nullptr;
    void  CreateCameraResource();

    //トランスフォーム
    EulerTransform worldTransform_ = {};
     EulerTransform localTransform_ = {};   // モデルのローカルトランスフォーム

    //カメラ　
    Camera* camera_ = nullptr;
    //スカイボックス
    SkyBox* box_ = nullptr;

    FillMode fillMode_ = FillMode::kSolid;
    BlendMode blendMode_ = BlendMode::None;

    std::string psoName_ = "Object3d";
    bool isDepthTestEnable_ = true;

    float curvature_ = -1.0f; // 個別湾曲率 (負数ならグローバルを使用)

};

