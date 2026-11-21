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
#include "EditorManager.h"

#include <filesystem>

Object3d::~Object3d()
{
    SetParent(nullptr);
    for (auto* child : children_) {
        if (child) child->parent_ = nullptr;
    }
    children_.clear();
    EditorManager::SafeUnregister(this);
}

void RotatorComponent::OnUpdate(Object3d* owner) {
    if (!owner) return;
    Vector3 rot = owner->GetRotate();
    rot = rot + speed * DXCommon::kDeltaTime;
    owner->SetRotate(rot);
}

void OscillatorComponent::OnUpdate(Object3d* owner) {
    if (!owner) return;
    if (!initialized_) {
        basePos_ = owner->GetTranslate();
        initialized_ = true;
    }
    time_ += DXCommon::kDeltaTime;
    float s = std::sin(time_ * frequency);
    owner->SetTranslate(basePos_ + amplitude * s);
}

void BoxColliderDebugComponent::OnDraw(Object3d* owner) {
    if (!owner) return;
    Vector3 center = owner->GetTranslate();
    AABB box{ center - halfSize, center + halfSize };
    PrimitiveDrawer::GetInstance()->DrawAABB(box, color);
}

bool Object3d::IsDescendantOf(const Object3d* potentialAncestor) const
{
    const Object3d* cur = parent_;
    while (cur) {
        if (cur == potentialAncestor) return true;
        cur = cur->parent_;
    }
    return false;
}

void Object3d::SetParent(Object3d* newParent)
{
    if (newParent == this) return;
    if (newParent && newParent->IsDescendantOf(this)) return; // 循環参照防止

    if (parent_) {
        auto& siblings = parent_->children_;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }
    parent_ = newParent;
    if (parent_) {
        parent_->children_.push_back(this);
    }
}

bool Object3d::IsVisibleEffective() const
{
    if (!isVisible_) return false;
    if (parent_) return parent_->IsVisibleEffective();
    return true;
}

void Object3d::SetShowInHierarchy(bool show)
{
    showInHierarchy_ = show;
    if (!showInHierarchy_) {
        EditorManager::GetInstance()->UnregisterObject(this);
    }
}

void Object3d::OnInspectorGUI()
{
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("Object3d", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (parent_) {
            ImGui::Text("Parent: %s", parent_->GetInspectorName().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Detach")) {
                SetParent(nullptr);
            }
        }
        ImGui::Text("Model: %s", editorName_.c_str());
        Vector4 col = GetModelInstanceColor();
        if (ImGui::ColorEdit4("Instance Color", &col.x)) {
            SetModelColor(col);
        }
        ImGui::DragFloat("Curvature Override", &curvature_, 0.0001f, -1.0f, 0.01f, "%.5f");
        ImGui::Checkbox("Depth Test", &isDepthTestEnable_);
    }

    // 登録済みコンポーネントの Inspector 描画
    for (size_t i = 0; i < components_.size(); ++i) {
        auto* comp = components_[i].get();
        ImGui::PushID(static_cast<int>(i));
        bool open = ImGui::CollapsingHeader(comp->GetComponentName().c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        if (open) {
            ImGui::Checkbox("Enabled", &comp->enabled);
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                components_.erase(components_.begin() + i);
                ImGui::PopID();
                break;
            }
            comp->OnInspectorGUI();
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f))) {
        ImGui::OpenPopup("AddComponentPopup");
    }
    if (ImGui::BeginPopup("AddComponentPopup")) {
        if (ImGui::MenuItem("Auto Rotator")) {
            AddComponent(std::make_unique<RotatorComponent>());
        }
        if (ImGui::MenuItem("Oscillator")) {
            AddComponent(std::make_unique<OscillatorComponent>());
        }
        if (ImGui::MenuItem("Debug Box Collider")) {
            AddComponent(std::make_unique<BoxColliderDebugComponent>());
        }
        ImGui::EndPopup();
    }
#endif
}

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
    if (showInHierarchy_) {
        EditorManager::GetInstance()->RegisterObject(this);
    }
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
    for (auto& comp : components_) {
        if (comp && comp->enabled) {
            comp->OnUpdate(this);
        }
    }

    //  WVP行列の作成（親子階層を考慮）
    Matrix4x4 localMatrix = MakeAffineMatrix(worldTransform_.scale, worldTransform_.rotate, worldTransform_.translate);
    Matrix4x4 worldMatrix = parent_ ? Multiply(localMatrix, parent_->GetWorldMatrix()) : localMatrix;
    currentWorldMatrix_ = worldMatrix;
    // 湾曲率の決定: 個別設定が負ならグローバル設定を使用
    float activeCurvature = (curvature_ >= 0.0f) ? curvature_ : Object3dCommon::GetInstance()->GetGlobalCurvature();

    auto writePass = [&](int idx, Camera* cam) {
        if (!wvpResources_[idx] || !cameraDatas_[idx]) return;
        Matrix4x4 viewMat = Makeidentity4x4();
        Matrix4x4 projMat = Makeidentity4x4();
        Matrix4x4 wvpMat = Makeidentity4x4();
        if (cam) {
            cameraDatas_[idx]->worldPosition = cam->GetTranslate();
            cameraDatas_[idx]->farClip = cam->GetFarCrip();
            const Matrix4x4& mat = cam->GetWorldMatrix();
            Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
            cameraDatas_[idx]->cameraForward = Normalize(forward);
            viewMat = cam->GetViewMatrix();
            projMat = cam->GetProjectionMatrix();
            wvpMat = Multiply(worldMatrix, cam->GetViewProtectionMatrix());
        } else {
            wvpMat = worldMatrix;
        }
        wvpResources_[idx]->WVP = wvpMat;
        wvpResources_[idx]->World = worldMatrix;
        wvpResources_[idx]->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
        wvpResources_[idx]->View = viewMat;
        wvpResources_[idx]->Projection = projMat;
        wvpResources_[idx]->curvature = activeCurvature;
    };

    // パス0: ゲームカメラ
    writePass(0, camera_);

    // パス1: デバッグ（エディタ）カメラ
    Camera* editorCam = EditorManager::GetInstance()->GetEditorCamera();
    writePass(1, editorCam ? editorCam : camera_);
}

void Object3d::Draw()
{
    if (!IsVisibleEffective()) {
        return;
    }

    for (auto& comp : components_) {
        if (comp && comp->enabled) {
            comp->OnDraw(this);
        }
    }

    if (!model_ && !box_) {
        return;
    }

    if (model_ && model_->HasSkinning())
    {
        psoName_ = isDepthTestEnable_ ? "SkiningObj3d" : "SkiningObj3d_NoDepth";

    } else
    {
        psoName_ = isDepthTestEnable_ ? "Object3d" : "Object3d_NoDepth";
    }
    Object3dCommon::GetInstance()->Object3dCommonDraw();
    auto psoSet = PSOManager::GetInstance()->GetPso(psoName_, blendMode_, fillMode_);

    auto commandList = DXCommon::GetInstance()->GetCommandList();
    commandList->SetGraphicsRootSignature(psoSet.rootSignature.Get());
    commandList->SetPipelineState(psoSet.pipelineState.Get());


    //WVP行列リソースの設定
    int pass = Object3dCommon::GetInstance()->GetRenderPassIndex();
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResources_[pass].Get()->GetGPUVirtualAddress());
    LightManager::GetInstance()->Draw(3);

    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(7, cameraResources_[pass].Get()->GetGPUVirtualAddress());

    if (box_) {

        // SkyBoxクラスにテクスチャのGPUハンドルを取得するメソッドがあると仮定
        // 例: targetBox->GetSrvHandle()
        commandList->SetGraphicsRootDescriptorTable(8, TextureManager::GetInstance()->GetSrvHandleGPU(box_->GetTextureIndex()));
    }

    //light



    if (model_) {
        D3D12_GPU_VIRTUAL_ADDRESS customMatCBV = instanceMaterialResource_
            ? instanceMaterialResource_->GetGPUVirtualAddress()
            : 0;
        model_->Draw(GetWorldMatrix(), customTextureIndex_, customMatCBV);
    }
}

void Object3d::SetModel(const std::string& filePath)
{
    modelFilePath_ = filePath;
    model_ = ModelManager::GetInstance()->findModel(filePath);
    customTextureIndex_ = std::nullopt; // モデル変更時は個別テクスチャをリセット
    if (!filePath.empty()) {
        editorName_ = std::filesystem::path(filePath).filename().string();
    }
}

void Object3d::SetTexture(const std::string& textureFilePath)
{
    TextureManager::GetInstance()->LoadTexture(textureFilePath);
    customTextureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
}

void Object3d::SetModelColor(const Vector4& color)
{
    instanceColor_ = color;
    if (!instanceMaterialResource_) {
        size_t size = (sizeof(Model::Material) + 0xff) & ~0xff;
        instanceMaterialResource_ = DXCommon::GetInstance()->CreateBufferResource(size);
        instanceMaterialResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceMaterialData_));
        if (instanceMaterialData_) {
            instanceMaterialData_->color = color;
            instanceMaterialData_->enableLighting = 0;
            instanceMaterialData_->environment = 0;
            instanceMaterialData_->diffuseType = Model::HarfLambert;
            instanceMaterialData_->specularType = Model::BlinnPhong;
            instanceMaterialData_->uvTransform = Makeidentity4x4();
            instanceMaterialData_->shininess = 50.0f;
            instanceMaterialData_->environmentCoefficient = 0.0f;
        }
    }
    if (instanceMaterialData_) {
        instanceMaterialData_->color = color;
    }
}


void Object3d::CreateWVPResource()
{
    size_t size = (sizeof(TransformationMatrix) + 0xff) & ~0xff;
    for (int i = 0; i < 2; ++i) {
        transformationMatrixResources_[i] = DXCommon::GetInstance()->CreateBufferResource(size);
        transformationMatrixResources_[i].Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpResources_[i]));
        wvpResources_[i]->WVP = Makeidentity4x4();
        wvpResources_[i]->World = Makeidentity4x4();
        wvpResources_[i]->WorldInverseTranspose = Inverse(wvpResources_[i]->World);
        wvpResources_[i]->View = Makeidentity4x4();
        wvpResources_[i]->Projection = Makeidentity4x4();
        wvpResources_[i]->curvature = 0.0f;
    }
}



void Object3d::CreateCameraResource()
{
    for (int i = 0; i < 2; ++i) {
        cameraResources_[i] =
            DXCommon::GetInstance()->
            CreateBufferResource((sizeof(CameraForGPU) + 0xff) & ~0xff);
        cameraResources_[i].Get()->
            Map(0, nullptr, reinterpret_cast<void**>(&cameraDatas_[i]));
        if (camera_ != nullptr)
        {
            cameraDatas_[i]->worldPosition = camera_->GetTranslate();
            cameraDatas_[i]->farClip = camera_->GetFarCrip();
            const Matrix4x4& mat = camera_->GetWorldMatrix();
            Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
            cameraDatas_[i]->cameraForward = Normalize(forward);
        } else
        {
            cameraDatas_[i]->worldPosition = Vector3{ 1.0f,1.0f,1.0f };
            cameraDatas_[i]->farClip = 1000.0f;
            cameraDatas_[i]->cameraForward = Vector3{ 0.0f,0.0f,1.0f };
        }
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

Vector3 Object3d::GetSubPartWorldPosition(const std::string& name) const {
    if (!model_) {
        return worldTransform_.translate;
    }
    Vector3 localCenter = model_->GetSubPartLocalCenter(name);
    Matrix4x4 worldMat = MakeAffineMatrix(worldTransform_.scale, worldTransform_.rotate, worldTransform_.translate);
    return vector3Transform(localCenter, worldMat);
}
