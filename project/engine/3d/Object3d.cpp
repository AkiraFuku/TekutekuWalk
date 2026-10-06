#include "Object3d.h"
#include "Object3dCommon.h"
#include <cassert>
#include <fstream> // 追加: ifstreamの完全な型を利用するため
#include <sstream> // 追加: istringstreamのため
#include "MathFunction.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include <imgui.h>
#include "LightManager.h"

void Object3d::Initialize()
{

    //WVP行列リソースの作成
    CreateWVPResource();
    //平行光源リソースの作成
   // CreateDirectionalLightResource();
    worldTransform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
    camera_ = Object3dCommon::GetInstance()->GetDefaultCamera();
    box_ = Object3dCommon::GetInstance()->GetDefaultSkyBox();
    CreateCameraResource();
}
void Object3d::Update()
{


    if (Object3dCommon::GetInstance()->GetDefaultCamera() != camera_)
    {
        camera_ = Object3dCommon::GetInstance()->GetDefaultCamera();


    }
    if (model_)
    {
        model_->Update();
    }
    //  WVP行列の作成
    Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransform_.scale, worldTransform_.rotate, worldTransform_.translate);
    Matrix4x4 worldViewProjectionMatrix = {};
    //ワールド行列とビュー行列とプロジェクション行列を掛け算
    if (camera_)
    {
        cameraData_->worldPosition = camera_->GetTranslate();
        worldViewProjectionMatrix = Multiply(worldMatrix, camera_->GetViewProtectionMatrix());
    } else {

        Matrix4x4 viewMatrix = Makeidentity4x4();
        Matrix4x4 projectionMatrix = Makeidentity4x4();
        Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);


        worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
    }
    //行列をGPUに転送
    wvpResource_->WVP = worldViewProjectionMatrix;
    wvpResource_->World = worldMatrix;
    wvpResource_->WorldInverseTranspose = Transpose(Inverse(worldMatrix));

    if (camera_ && cameraData_)
    {
        cameraData_->worldPosition = camera_->GetTranslate();
        cameraData_->farClip = camera_->GetFarCrip(); // ★ここを追加
        // ★追加: カメラのワールド行列から前方ベクトル(Z軸)を抽出
        // 一般的な行優先(Row-Major)の4x4行列の場合、3行目(m[2])がZ軸(Forward)です
        const Matrix4x4& mat = camera_->GetWorldMatrix();
        // 正規化されているはずですが、念のため正規化して送ると安全です
        Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
        cameraData_->cameraForward = Normalize(forward);
    }
}

void Object3d::Draw()
{


    if (!model_ && !box_) {
        return;
    }

    // スキニングモデルの場合は、描画パイプライン設定の前にコンピュートスキニングを実行！
    if (model_ && model_->HasSkinning()) {
        model_->SkinningDispatch();
    }

    psoName_ = "Object3d";
    Object3dCommon::GetInstance()->Object3dCommonDraw();
    auto psoSet = PSOManager::GetInstance()->GetPso(psoName_, blendMode_, fillMode_);

    auto commandList = DXCommon::GetInstance()->GetCommandList();
    commandList->SetGraphicsRootSignature(psoSet.rootSignature.Get());
    commandList->SetPipelineState(psoSet.pipelineState.Get());


    //WVP行列リソースの設定
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_.Get()->GetGPUVirtualAddress());
    LightManager::GetInstance()->Draw(3);

    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(7, cameraResource_->GetGPUVirtualAddress());

    if (box_) {

        // SkyBoxクラスにテクスチャのGPUハンドルを取得するメソッドがあると仮定
        // 例: targetBox->GetSrvHandle()
        commandList->SetGraphicsRootDescriptorTable(8, TextureManager::GetInstance()->GetSrvHandleGPU(box_->GetTextureIndex()));
    }

    //light



    if (model_) {
        model_->Draw(GetWorldMatrix(), customTextureIndex_);
    }
}

void Object3d::SetModel(const std::string& filePath)
{
    model_ = ModelManager::GetInstance()->findModel(filePath);
    customTextureIndex_ = std::nullopt; // モデル変更時は個別テクスチャをリセット
}

void Object3d::SetTexture(const std::string& textureFilePath)
{
    TextureManager::GetInstance()->LoadTexture(textureFilePath);
    customTextureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
}



void Object3d::CreateWVPResource()
{
    //座標変換
    transformationMatrixResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_.Get()->
        Map(0, nullptr, reinterpret_cast<void**>(&wvpResource_));
    wvpResource_->WVP = Makeidentity4x4();
    wvpResource_->World = Makeidentity4x4();
    wvpResource_->WorldInverseTranspose = Inverse(wvpResource_->World);



}



void Object3d::CreateCameraResource()
{
    cameraResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource((sizeof(CameraForGPU) + 0xff) & ~0xff);
    cameraResource_.Get()->
        Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
    if (camera_ != nullptr)
    {
        cameraData_->worldPosition = camera_->GetTranslate();
        cameraData_->farClip = camera_->GetFarCrip(); // ★ここを追加

        // ★追加: カメラのワールド行列から前方ベクトル(Z軸)を抽出
        // 一般的な行優先(Row-Major)の4x4行列の場合、3行目(m[2])がZ軸(Forward)です
        const Matrix4x4& mat = camera_->GetWorldMatrix();
        // 正規化されているはずですが、念のため正規化して送ると安全です
        Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
        cameraData_->cameraForward = Normalize(forward);

    } else
    {
        cameraData_->worldPosition = Vector3{ 1.0f,1.0f,1.0f };
        cameraData_->farClip = 1000.0f;
        cameraData_->cameraForward = Vector3{ 0.0f,0.0f,1.0f };

    }

}

std::vector<Triangle> Object3d::GetWorldTriangles() const {
    std::vector<Triangle> worldTriangles;
    if (!model_) {
        return worldTriangles;
    }

    // モデルからローカル空間の三角形リストを取得
    std::vector<Triangle> localTriangles = model_->GetLocalTriangles();
    worldTriangles.reserve(localTriangles.size());

    // 自身のワールド行列を取得
    Matrix4x4 worldMat = GetWorldMatrix();

    // 各三角形の頂点をワールド空間に変換
    for (const auto& localTri : localTriangles) {
        Triangle worldTri;
        worldTri.vertices[0] = vector3Transform(localTri.vertices[0], worldMat);
        worldTri.vertices[1] = vector3Transform(localTri.vertices[1], worldMat);
        worldTri.vertices[2] = vector3Transform(localTri.vertices[2], worldMat);
        worldTriangles.push_back(worldTri);
    }

    return worldTriangles;
}



