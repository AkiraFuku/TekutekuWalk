#include "Model.h"
#include "TextureManager.h"
#include "DXCommon.h"
#include "MathFunction.h"
#include <cassert>
#include <fstream> 
#include <sstream>
#include <Windows.h>
#include <numbers>
#include <imgui.h>
#include "PrimitiveDrawer.h"
#include "SrvManager.h"

void Model::Initialize(const std::string& directoryPath, const std::string& filename)
{

    name_ = filename;
    modelData_ = LoadModelFile(directoryPath, filename);
    if (modelData_.material.textureFilePath.empty() || modelData_.material.textureFilePath == (directoryPath + "/")) {
        modelData_.material.textureFilePath = "resources/uvChecker.png"; // 確実に存在する画像を指定
        TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);



    }  //頂点リソースの作成
    CreateVertexBuffer();
    //インデックスリソースの作成
    CreateIndexBuffer();
    //マテリアルリソースの作成
    CreateMaterialResource();
    //テクスチャの読み込み
    TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
    //テクスチャインデックスの取得
    modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(
            modelData_.material.textureFilePath);
    // Model.cpp の初期化フロー（イメージ）
    if (!modelData_.skinClusterData.empty()) {
        skeleton_ = CreateSkelton(modelData_.rootNode);
        skinCluster_ = CreateSkinCluster(skeleton_, modelData_, vertexResource_);
        hasSkinning_ = true; // 新しくフラグを追加しておくと便利
    } else {
        hasSkinning_ = false;
    }


}

void Model::Update()
{
    if (hasSkinning_) {
        if (animation_)
        {
            animation_->Update();

            ApplyAnimation(animation_->GetCurrentTime_());
        }

        UpdateSkeleton();
        UpdateSkinCluster();

    }






    Matrix4x4 uvMatrix = MakeUVTransformMatrix(uvTransform_);

    materialData_->uvTransform = uvMatrix;
#ifdef USE_IMGUI
    ImGui::Begin((std::string("Settings: ") + name_).c_str());
    int* pEnableLighting = reinterpret_cast<int*>(&materialData_->enableLighting);
    int* pEnvironment = reinterpret_cast<int*>(&materialData_->environment);
    ImGui::Checkbox("Enable Lighting", (bool*)pEnableLighting);
    ImGui::Checkbox("Environment", (bool*)pEnvironment);
    if (materialData_->enableLighting) {
        ImGui::Text("Diffuse (Base)");
        const char* diffuseItems[] = { "Lambert", "Half-Lambert" };
        ImGui::Combo("Diffuse Type", &materialData_->diffuseType, diffuseItems, IM_ARRAYSIZE(diffuseItems));

        ImGui::Text("Specular (Shininess)");
        const char* specularItems[] = { "None", "Phong", "Blinn-Phong" };
        ImGui::Combo("Specular Type", &materialData_->specularType, specularItems, IM_ARRAYSIZE(specularItems));

        ImGui::DragFloat("Shininess", &materialData_->shininess, 0.1f, 1.0f, 256.0f);
    }
    if (materialData_->environment)
    {
        ImGui::DragFloat("EnvironmentCoefficient", &materialData_->environmentCoefficient, 0.1f, 1.0f, 256.0f);
    }



    ImGui::End();

#endif // USE_IMGUI

}
void Model::Draw(const Matrix4x4& worldMatrix, std::optional<uint32_t> customTextureIndex) {
    //VBVの設定

    if (hasSkinning_)
    {
        // 変形後の出力頂点バッファをセット
        DXCommon::GetInstance()->GetCommandList()->IASetVertexBuffers(0, 1, &skinCluster_.outputVertexBufferView);

    } else
    {
        DXCommon::GetInstance()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);

    }


    //IBVの設定
   // DXCommon::GetInstance()->GetCommandList()->IASetIndexBuffer(&indexBufferView_);
    //マテリアルリソースの設定
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_.Get()->GetGPUVirtualAddress());
    //SRVのディスクリプタテーブルの設定
    uint32_t texIndex = customTextureIndex.value_or(modelData_.material.textureIndex);
    DXCommon::GetInstance()->
        GetCommandList()->
        SetGraphicsRootDescriptorTable(2,
            TextureManager::GetInstance()->GetSrvHandleGPU(texIndex));
    //描画コマンド
 // インデックスバッファが空でなければインデックスドロー、空なら通常ドロー
    if (!modelData_.indices.empty()) {
        // IBVの設定
        DXCommon::GetInstance()->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

        // インデックスドロー
        DXCommon::GetInstance()->GetCommandList()->DrawIndexedInstanced(
            UINT(modelData_.indices.size()), 1, 0, 0, 0);
    } else {
        // インデックスがない場合は頂点バッファをそのまま描画
        DXCommon::GetInstance()->GetCommandList()->DrawInstanced(
            UINT(modelData_.vertices.size()), 1, 0, 0);
    }
    DebugDrawSkeleton(worldMatrix);
}

void Model::DrawInstanced(uint32_t instanceCount)
{
    if (instanceCount == 0) return;

    auto commandList = DXCommon::GetInstance()->GetCommandList();
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_.Get()->GetGPUVirtualAddress());

    if (!modelData_.indices.empty()) {
        commandList->IASetIndexBuffer(&indexBufferView_);
        commandList->DrawIndexedInstanced(UINT(modelData_.indices.size()), instanceCount, 0, 0, 0);
    } else {
        commandList->DrawInstanced(UINT(modelData_.vertices.size()), instanceCount, 0, 0);
    }
}

void Model::CreateVertexBuffer() {
    //頂点リソースの作成
    vertexResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(VertexData) * modelData_.vertices.size());
    //頂点バッファビューの設定
    vertexBufferView_.BufferLocation =
        vertexResource_.Get()->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    //頂点データの転送
    memcpy(vertexData_, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
}

void Model::CreateIndexBuffer()
{

    indexResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(uint32_t) * modelData_.indices.size());
    indexBufferView_.BufferLocation =
        indexResource_.Get()->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * modelData_.indices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    indexResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    memcpy(indexData_, modelData_.indices.data(), sizeof(uint32_t) * modelData_.indices.size());
}

void Model::CreateMaterialResource() {
    //マテリアルリソースの作成
    materialResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(Material));
    materialResource_->
        Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = Vector4{ 1.0f,1.0f,1.0f,1.0f };
    materialData_->enableLighting = false;
    uvTransform_ = {};
    Matrix4x4 uvMatrix = MakeUVTransformMatrix(uvTransform_);
    materialData_->uvTransform = uvMatrix;
    materialData_->shininess = 50.0f;
    materialData_->specularType = BlinnPhong;
    materialData_->diffuseType = HarfLambert;

}
void Model::ApplyAnimation(Node& node, float time)
{
    const auto& nodeAnimations = animation_->GetAnimationData().nodeAnimations;

    // このノードにアニメーションがあれば計算
    if (nodeAnimations.find(node.name) != nodeAnimations.end()) {
        const auto& anim = nodeAnimations.at(node.name);
        Vector3 t = animation_->CalculateValue(anim.translate.keyFrames, time);
        Quaternion r = animation_->CalculateValue(anim.rotate.keyFrames, time);
        Vector3 s = animation_->CalculateValue(anim.scale.keyFrames, time);
        node.localMatrix = MakeAffineMatrix(s, r, t);
    }

    // 子供のノードにも再帰的に適用
    for (auto& child : node.children) {
        ApplyAnimation(child, time);
    }
}
void Model::ApplyAnimation(float time)
{
    if (!animation_) {
        return;
    }
    const auto& nodeAnims = animation_->GetAnimationData().nodeAnimations;
    if (animation_)
    {
        for (Joint& joint : skeleton_.joints)
        {
            // ジョイント名を確実に取得
            std::string searchName = joint.name;

            // ここでクラッシュする場合、nodeAnims（std::map）自体が壊れている
            auto it = nodeAnims.find(searchName);

            if (it != nodeAnims.end()) {
                const Animation::NodeAnimation& anima = it->second;
                joint.transform.translate = animation_->CalculateValue(anima.translate.keyFrames, time);
                joint.transform.rotate = animation_->CalculateValue(anima.rotate.keyFrames, time);
                joint.transform.scale = animation_->CalculateValue(anima.scale.keyFrames, time);

            }

        }

    }
}
void Model::UpdateSkeleton()
{
    for (Joint& joint : skeleton_.joints)
    {
        joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
        if (joint.parentIndex)
        {
            joint.skeletonSpaceMatrix = Multiply(joint.localMatrix, skeleton_.joints[*joint.parentIndex].skeletonSpaceMatrix);

        } else
        {
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }

    }
}
void Model::UpdateSkinCluster()
{
    for (size_t jointIndex = 0; jointIndex < skeleton_.joints.size(); ++jointIndex)
    {

        assert(jointIndex < skinCluster_.inverseBindMatrices.size());
        skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
            skinCluster_.inverseBindMatrices[jointIndex] * skeleton_.joints[jointIndex].skeletonSpaceMatrix;
        skinCluster_.mappedPalette[jointIndex].skeletonInverseTransposeMatrix = Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));

    }
}
Model::MaterialData  Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
    //1. 変数の宣言
    MaterialData materialData{}; // 修正: 初期化
    std::string line;
    std::ifstream file(directoryPath + "/" + filename);//ファイルパスを結合して開く
    //2. ファイルを開く
    assert(file.is_open());//ファイルが開けたか確認
    //3. ファイルからデータを読み込みマテリアルデータを作成
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;//行の先頭を識別子として取得

        if (identifier == "map_Kd") {
            std::string textureFileName;
            s >> textureFileName;//テクスチャファイル名を読み込み
            //テクスチャのパスを設定
            materialData.textureFilePath = directoryPath + "/" + textureFileName;
        }
    }

    //4. マテリアルデータを返す
    return materialData;
}

Model::ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename)
{
    //1. 変数の宣言
    ModelData modelData;
    std::string filePath = directoryPath + "/" + filename;

    ////2. ファイルを開く
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filePath.c_str(),
        aiProcess_Triangulate |          // クワッド等を三角形に自動変換
        aiProcess_FlipUVs      |         // UV上下反転（DirectX座標系対応）
        aiProcess_GenSmoothNormals       // 法線がない場合は自動生成
    );
    assert(scene->HasMeshes());
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        // 注: aiProcess_GenSmoothNormals で法線は必ず生成されるが念のためチェック
        bool hasNormals = mesh->HasNormals();
        bool hasUV      = mesh->HasTextureCoords(0);
        modelData.vertices.resize(mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            aiVector3D& position = mesh->mVertices[i];
            VertexData& vertex = modelData.vertices[i];
            vertex.position = { position.x, position.y, position.z, 1.0f };
            // 法線
            if (hasNormals) {
                aiVector3D& normal = mesh->mNormals[i];
                vertex.normal = { normal.x, normal.y, normal.z };
            } else {
                vertex.normal = { 0.0f, 1.0f, 0.0f }; // 法線がない場合は上向きデフォルト
            }
            // UV
            if (hasUV) {
                aiVector3D& texcord = mesh->mTextureCoords[0][i];
                vertex.texcord = { texcord.x, texcord.y };
            } else {
                vertex.texcord = { 0.0f, 0.0f };
            }
        }
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3);
            for (uint32_t element = 0; element < face.mNumIndices; ++element)
            {
                uint32_t vertexIndex = face.mIndices[element];
                modelData.indices.push_back(vertexIndex);
            }

        }
        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            aiBone* bone = mesh->mBones[boneIndex];
            std::string boneName = bone->mName.C_Str();
            JointWeightData& jointWeightData = modelData.skinClusterData[boneName];

            aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
            aiVector3D translate, scale;
            aiQuaternion rotate;
            bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
            Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
                Vector3{ scale.x,scale.y,scale.z },
                Quaternion{ rotate.x,rotate.y,rotate.z,rotate.w },
                Vector3{ translate.x,translate.y,translate.z }
            );
            jointWeightData.inverseBindMatrix = Inverse(bindPoseMatrix);

            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
            {
                jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId });

            }

        }


    }
    for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
    {
        aiMaterial* material = scene->mMaterials[materialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
        {
            aiString textureFilePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
            modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
        }
    }
    modelData.rootNode = ReadNode(scene->mRootNode);

    // 4. モデルデータを返す



    return modelData;

}
Model* Model::CreateSphere(uint32_t subdivision)
{
    Model* model = new Model();
    model->modelData_.material.textureFilePath = "resources/uvChecker.png";
    TextureManager::GetInstance()->LoadTexture(model->modelData_.material.textureFilePath);
    model->modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(model->modelData_.material.textureFilePath);

    const float kLonEvery = 2.0f * std::numbers::pi_v<float> / float(subdivision);
    const float kLatEvery = std::numbers::pi_v<float> / float(subdivision);

    // 1. まず全ての頂点を生成して格納する
    for (uint32_t latIndex = 0; latIndex <= subdivision; ++latIndex) {
        float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
        for (uint32_t lonIndex = 0; lonIndex <= subdivision; ++lonIndex) {
            float lon = lonIndex * kLonEvery;

            VertexData vertex;
            vertex.position.x = std::cos(lat) * std::cos(lon);
            vertex.position.y = std::sin(lat);
            vertex.position.z = std::cos(lat) * std::sin(lon);
            vertex.position.w = 1.0f;

            vertex.normal.x = vertex.position.x;
            vertex.normal.y = vertex.position.y;
            vertex.normal.z = vertex.position.z;

            vertex.texcord.x = float(lonIndex) / float(subdivision);
            vertex.texcord.y = 1.0f - (float(latIndex) / float(subdivision));

            model->modelData_.vertices.push_back(vertex);
        }
    }

    // 2. 頂点同士を繋ぐインデックスを生成する
    for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
            // 現在のトポロジーにおける4点のインデックスを算出
            uint32_t startIdx = latIndex * (subdivision + 1) + lonIndex;
            uint32_t a = startIdx;
            uint32_t b = startIdx + (subdivision + 1);
            uint32_t c = startIdx + 1;
            uint32_t d = startIdx + (subdivision + 1) + 1;

            // 三角形1 (A -> B -> C)
            model->modelData_.indices.push_back(a);
            model->modelData_.indices.push_back(b);
            model->modelData_.indices.push_back(c);

            // 三角形2 (C -> B -> D)
            model->modelData_.indices.push_back(c);
            model->modelData_.indices.push_back(b);
            model->modelData_.indices.push_back(d);
        }
    }

    // 3. GPUバッファの作成（インデックスバッファも呼ぶ！）
    model->CreateVertexBuffer();
    model->CreateIndexBuffer(); // ←これが抜けていたため描画されなかった
    model->CreateMaterialResource();

    return model;
}

Model* Model::CreatePlaneFromTex(const std::string& textureFilePath)
{
    Model* model = new Model();

    TextureManager::GetInstance()->LoadTexture(textureFilePath);
    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath);

    float w = (static_cast<float>(metadata.width) / 2.0f) / 4.0f;
    float h = (static_cast<float>(metadata.height) / 2.0f) / 4.0f;

    // 4つのユニークな頂点を作成
    Model::VertexData a = { {-w, -h, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 左上
    Model::VertexData b = { { w, -h, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 右上
    Model::VertexData c = { {-w,  h, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 左下
    Model::VertexData d = { { w,  h, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 右下

    model->modelData_.vertices.push_back(a); // 0
    model->modelData_.vertices.push_back(b); // 1
    model->modelData_.vertices.push_back(c); // 2
    model->modelData_.vertices.push_back(d); // 3

    // インデックスの設定
    // 三角形1: A -> B -> C
    model->modelData_.indices.push_back(0);
    model->modelData_.indices.push_back(1);
    model->modelData_.indices.push_back(2);
    // 三角形2: C -> B -> D
    model->modelData_.indices.push_back(2);
    model->modelData_.indices.push_back(1);
    model->modelData_.indices.push_back(3);

    model->modelData_.material.textureFilePath = textureFilePath;
    model->modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

    model->CreateVertexBuffer();
    model->CreateIndexBuffer(); // ←ここを追加
    model->CreateMaterialResource();

    return model;
}
Model::Node Model::ReadNode(aiNode* node)
{
    Node result_;

    aiMatrix4x4 aiLocalMatrix = node->mTransformation;
    aiLocalMatrix.Transpose();


    //for (int i = 0; i < 4; i++) {
    //    for (int j = 0; j < 4; j++) {
    //        // Assimpの行列は [row][col] でアクセス可能
    //        // 自作Matrix構造体の定義に合わせて代入 (例: result.localMatrix.m[i][j])
    //        result_.localMatrix.m[i][j] = aiLocalMatrix[i][j];
    //    }
    //}
    result_.name = node->mName.C_Str();
    result_.children.resize(node->mNumChildren);
    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
    {
        result_.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }

    aiVector3D scale, translate;
    aiQuaternion rotate;
    node->mTransformation.Decompose(scale, rotate, translate);
    result_.transform.scale = { scale.x,scale.y,scale.z };
    result_.transform.rotate = { rotate.x,rotate.y,rotate.z,rotate.w };
    result_.transform.translate = { translate.x,translate.y,translate.z };
    result_.localMatrix = MakeAffineMatrix(result_.transform.scale, result_.transform.rotate, result_.transform.translate);



    return result_;
}

Model::Skeleton Model::CreateSkelton(const Node& rootNode)
{
    Skeleton skeleton;
    skeleton.rootIndex = CreateJoint(rootNode, {}, skeleton.joints);

    for (const Joint& joint : skeleton.joints)
    {
        skeleton.jointMap.emplace(joint.name, joint.index);
    }
    return skeleton;
}

int32_t Model::CreateJoint(const Node& node, std::optional<int32_t> parent, std::vector<Joint>& joints)
{
    Joint joint;
    joint.name = node.name;
    joint.localMatrix = node.localMatrix;
    joint.skeletonSpaceMatrix = Makeidentity4x4();
    joint.transform = node.transform;
    joint.index = static_cast<int32_t>(joints.size());
    joint.parentIndex = parent;
    joints.push_back(joint);

    for (const Node& child : node.children)
    {
        int32_t childIndex = CreateJoint(child, joint.index, joints);
        joints[joint.index].children.push_back(childIndex);
    }

    return joint.index;
}

Model::SkinCluster Model::CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData, const Microsoft::WRL::ComPtr<ID3D12Resource>& vertexResource)
{

    auto dxCommon = DXCommon::GetInstance();
    auto srv = SrvManager::GetInstance();
    SkinCluster skinCluster;
    //パレットリソース
    skinCluster.paletteResource = dxCommon->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
    WellForGPU* mappedPalette = nullptr;
    skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
    skinCluster.mappedPalette = { mappedPalette,skeleton.joints.size() };
    uint32_t index = srv->AllocateSRV();
    skinCluster.paletteSrvIndex = index;
    skinCluster.paletteSrvHandle.first = srv->GetCPUDescriptorHandle(index);
    skinCluster.paletteSrvHandle.second = srv->GetGPUDescriptorHandle(index);

    //パレットSRV
    srv->CreateSRVForMatrixPalette(
        skinCluster.paletteResource.Get(),
        UINT(skeleton.joints.size()),
        sizeof(WellForGPU),
        skinCluster.paletteSrvHandle.first
    );
    //インフルエンスリソース
    skinCluster.influenceResource = dxCommon->CreateBufferResource(sizeof(VertexInfluence) * modelData.vertices.size());
    VertexInfluence* mappedInfluence = nullptr;
    skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
    std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());
    skinCluster.mappedInfluences = { mappedInfluence,modelData.vertices.size() };
    //インフルエンスVBV
    skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
    skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
    skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);
    //ジョイントの逆バインド行列の保存領域
    skinCluster.inverseBindMatrices.resize(skeleton.joints.size());
    std::generate(skinCluster.inverseBindMatrices.begin(), skinCluster.inverseBindMatrices.end(), Makeidentity4x4);
    //モデルデータのスキンクラスターでインフルエンスの中身を埋める

    for (const auto& jointWeight : modelData.skinClusterData)
    {
        auto it = skeleton.jointMap.find(jointWeight.first);
        if (it == skeleton.jointMap.end())
        {
            continue;
        }
        skinCluster.inverseBindMatrices[(*it).second] = jointWeight.second.inverseBindMatrix;
        for (const auto& vertexWeight : jointWeight.second.vertexWeights)
        {
            auto& currentInfluence = skinCluster.mappedInfluences[vertexWeight.vertexIndex];

            for (uint32_t i = 0; i < kNumMaxInfluences; ++i)
            {
                if (currentInfluence.weights[i] == 0.0f)
                {
                    currentInfluence.weights[i] = vertexWeight.weight;
                    currentInfluence.jointIndices[i] = (*it).second;
                    break;
                }
            }

        }

    }

    // ==========================================
    // コンピュートスキニング用リソースの初期化
    // ==========================================
    // 1. インフルエンスのSRV作成 (t2)
    skinCluster.influenceSrvIndex = srv->AllocateSRV();
    srv->CreateSRVForStructuredBuffer(
        skinCluster.influenceSrvIndex,
        skinCluster.influenceResource.Get(),
        UINT(modelData.vertices.size()),
        sizeof(VertexInfluence));

    // 2. 入力頂点バッファのSRV作成 (t1)
    skinCluster.inputVerticesSrvIndex = srv->AllocateSRV();
    srv->CreateSRVForStructuredBuffer(
        skinCluster.inputVerticesSrvIndex,
        vertexResource.Get(),
        UINT(modelData.vertices.size()),
        sizeof(VertexData));

    // 3. 出力頂点バッファ (UAV) 生成 (u0)
    size_t vertexBufferSize = sizeof(VertexData) * modelData.vertices.size();
    uint32_t dummyUav = 0;
    skinCluster.outputVertexResource = dxCommon->CreateUAVBufferResource(vertexBufferSize, dummyUav);
    skinCluster.outputVerticesUavIndex = srv->AllocateSRV();
    srv->CreateUAVForStructuredBuffer(
        skinCluster.outputVerticesUavIndex,
        skinCluster.outputVertexResource.Get(),
        UINT(modelData.vertices.size()),
        sizeof(VertexData));

    // 4. 出力頂点のVBV設定 (描画用)
    skinCluster.outputVertexBufferView.BufferLocation = skinCluster.outputVertexResource->GetGPUVirtualAddress();
    skinCluster.outputVertexBufferView.SizeInBytes = UINT(vertexBufferSize);
    skinCluster.outputVertexBufferView.StrideInBytes = sizeof(VertexData);

    // 5. スキニング情報定数バッファ (CBV b0) 生成 (256バイトアライメント)
    size_t infoBufferSize = (sizeof(SkinningInformation) + 0xff) & ~0xff;
    skinCluster.skinningInfoResource = dxCommon->CreateBufferResource(infoBufferSize);
    SkinningInformation* mappedInfo = nullptr;
    skinCluster.skinningInfoResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfo));
    mappedInfo->numVertices = static_cast<uint32_t>(modelData.vertices.size());
    skinCluster.skinningInfoResource->Unmap(0, nullptr);

    return skinCluster;
}

void Model::SkinningDispatch()
{
    auto commandList = DXCommon::GetInstance()->GetCommandList();
    auto psoSet = PSOManager::GetInstance()->GetComputePso("SkinningCS");

    // 1. リソースバリア: 初回はCOMMON、2回目以降はVERTEX_AND_CONSTANT_BUFFERからUAVへ遷移
    D3D12_RESOURCE_BARRIER preBarrier{};
    preBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    preBarrier.Transition.pResource = skinCluster_.outputVertexResource.Get();
    preBarrier.Transition.StateBefore = skinCluster_.isFirstDispatch ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    preBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    preBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &preBarrier);
    skinCluster_.isFirstDispatch = false;

    // 2. パイプラインステートとルートシグネチャのセット
    commandList->SetComputeRootSignature(psoSet.rootSignature.Get());
    commandList->SetPipelineState(psoSet.pipelineState.Get());

    // 3. ディスクリプタヒープのセット
    ID3D12DescriptorHeap* heaps[] = { SrvManager::GetInstance()->GetDescriptorHeap().Get() };
    commandList->SetDescriptorHeaps(1, heaps);

    // 4. 各パラメータをバインド
    // b0: スキニング情報 (CBV)
    commandList->SetComputeRootConstantBufferView(0, skinCluster_.skinningInfoResource->GetGPUVirtualAddress());
    // t0: 行列パレット (SRV)
    SrvManager::GetInstance()->SetComputeRootDescriptorTable(1, skinCluster_.paletteSrvIndex);
    // t1: 入力頂点 (SRV)
    SrvManager::GetInstance()->SetComputeRootDescriptorTable(2, skinCluster_.inputVerticesSrvIndex);
    // t2: インフルエンス (SRV)
    SrvManager::GetInstance()->SetComputeRootDescriptorTable(3, skinCluster_.influenceSrvIndex);
    // u0: 出力頂点 (UAV)
    SrvManager::GetInstance()->SetComputeRootDescriptorTable(4, skinCluster_.outputVerticesUavIndex);

    // 5. ディスパッチ実行 (スレッドグループサイズ: 1024)
    UINT numVertices = static_cast<UINT>(modelData_.vertices.size());
    commandList->Dispatch((numVertices + 1023) / 1024, 1, 1);

    // 6. リソースバリア: UAV書き込み完了待ち → 描画用頂点バッファ状態へ遷移
    D3D12_RESOURCE_BARRIER postBarrier{};
    postBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    postBarrier.Transition.pResource = skinCluster_.outputVertexResource.Get();
    postBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    postBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    postBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &postBarrier);
}

void Model::DebugDrawSkeleton(const Matrix4x4& worldMatrix)
{
    if (HasSkinning())
    {
        for (const Joint& joint : skeleton_.joints) {
            // 現在のジョイントのローカル座標
            Vector3 localStart = { joint.skeletonSpaceMatrix.m[3][0], joint.skeletonSpaceMatrix.m[3][1], joint.skeletonSpaceMatrix.m[3][2] };
            // ワールド座標に変換
            Vector3 start = vector3Transform(localStart, worldMatrix);

            for (int32_t childIndex : joint.children) {
                const Joint& childJoint = skeleton_.joints[childIndex];
                // 子ジョイントのローカル座標
                Vector3 localEnd = { childJoint.skeletonSpaceMatrix.m[3][0], childJoint.skeletonSpaceMatrix.m[3][1], childJoint.skeletonSpaceMatrix.m[3][2] };
                // ワールド座標に変換
                Vector3 end = vector3Transform(localEnd, worldMatrix);

                PrimitiveDrawer::GetInstance()->DrawLine(start, end, { 1.0f, 0.0f, 0.0f, 1.0f });
            }
        }
    }
}

std::vector<Triangle> Model::GetLocalTriangles() const {
    std::vector<Triangle> triangles;
    const auto& vertices = modelData_.vertices;
    const auto& indices = modelData_.indices;

    // 3点ごとに三角形を構築
    triangles.reserve(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (i + 2 < indices.size()) {
            Triangle tri;
            tri.vertices[0] = vertices[indices[i]].position.ToVector3();
            tri.vertices[1] = vertices[indices[i + 1]].position.ToVector3();
            tri.vertices[2] = vertices[indices[i + 2]].position.ToVector3();
            triangles.push_back(tri);
        }
    }
    return triangles;
}

Model* Model::CreateBox() {
    Model* model = new Model();
    model->modelData_.material.textureFilePath = "resources/uvChecker.png";
    TextureManager::GetInstance()->LoadTexture(model->modelData_.material.textureFilePath);
    model->modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(model->modelData_.material.textureFilePath);

    // 立方体のサイズは 1x1x1 (半径 0.5f)
    float w = 0.5f;

    // 24頂点のデータ定義 (位置, UV, 法線)
    struct TempVertex {
        Vector3 pos;
        Vector2 uv;
        Vector3 normal;
    };

    std::vector<TempVertex> tempVertices = {
        // 前面 (Z+)
        { {-w, -w,  w}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // 0: 左下
        { { w, -w,  w}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // 1: 右下
        { {-w,  w,  w}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // 2: 左上
        { { w,  w,  w}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // 3: 右上

        // 後面 (Z-)
        { { w, -w, -w}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 4
        { {-w, -w, -w}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }, // 5
        { { w,  w, -w}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 6
        { {-w,  w, -w}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }, // 7

        // 左面 (X-)
        { {-w, -w, -w}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} }, // 8
        { {-w, -w,  w}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} }, // 9
        { {-w,  w, -w}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} }, // 10
        { {-w,  w,  w}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} }, // 11

        // 右面 (X+)
        { { w, -w,  w}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} }, // 12
        { { w, -w, -w}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} }, // 13
        { { w,  w,  w}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} }, // 14
        { { w,  w, -w}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} }, // 15

        // 上面 (Y+)
        { {-w,  w,  w}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} }, // 16
        { { w,  w,  w}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} }, // 17
        { {-w,  w, -w}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} }, // 18
        { { w,  w, -w}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} }, // 19

        // 下面 (Y-)
        { {-w, -w, -w}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} }, // 20
        { { w, -w, -w}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f} }, // 21
        { {-w, -w,  w}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} }, // 22
        { { w, -w,  w}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} }, // 23
    };

    model->modelData_.vertices.resize(tempVertices.size());
    for (size_t i = 0; i < tempVertices.size(); ++i) {
        model->modelData_.vertices[i].position = { tempVertices[i].pos.x, tempVertices[i].pos.y, tempVertices[i].pos.z, 1.0f };
        model->modelData_.vertices[i].texcord = tempVertices[i].uv;
        model->modelData_.vertices[i].normal = tempVertices[i].normal;
    }

    // 【修正】インデックスを時計回り（表面が外側を向く）になるように設定
    for (uint32_t i = 0; i < 6; ++i) {
        uint32_t offset = i * 4;

        // 三角形1 (左下 -> 左上 -> 右下 で組むと時計回りになる)
        model->modelData_.indices.push_back(offset + 0);
        model->modelData_.indices.push_back(offset + 1);
        model->modelData_.indices.push_back(offset + 2);

        // 三角形2 (左上 -> 右下 -> 右上 で組むと時計回りになる)
        model->modelData_.indices.push_back(offset + 2);
        model->modelData_.indices.push_back(offset + 1);
        model->modelData_.indices.push_back(offset + 3);
    }

    model->CreateVertexBuffer();
    model->CreateIndexBuffer();
    model->CreateMaterialResource();

    return model;
}

void Model::SetTexture(std::string textureFilePath)
{
    modelData_.material.textureFilePath = textureFilePath;
        TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
    modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData_.material.textureFilePath);

}

Model* Model::CreateTerrainGrid(
    float sizeX, float sizeY,
    int divisionsX, int divisionsY,
    const std::string& textureFilePath,
    float uvTile)
{
    Model* model = new Model();

    // テクスチャ設定
    const std::string& texPath = textureFilePath.empty()
        ? std::string("resources/uvChecker.png")
        : textureFilePath;
    model->modelData_.material.textureFilePath = texPath;
    TextureManager::GetInstance()->LoadTexture(texPath);
    model->modelData_.material.textureIndex =
        TextureManager::GetInstance()->GetTextureIndexByFilePath(texPath);

    // ─── 頂点生成 ────────────────────────────────────────────────────
    // XZ平面上のグリッド（水平な地面）。法線は Y+ 方向（上向き）
    const float halfX = sizeX * 0.5f;
    const float halfZ = sizeY * 0.5f;   // 引数 sizeY を Z方向奥行きとして使用
    const int colCount = divisionsX + 1;  // X方向頂点数
    const int rowCount = divisionsY + 1;  // Z方向頂点数

    model->modelData_.vertices.reserve(static_cast<size_t>(colCount) * rowCount);

    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < colCount; ++col) {
            float t = static_cast<float>(col) / static_cast<float>(divisionsX);
            float s = static_cast<float>(row) / static_cast<float>(divisionsY);

            VertexData v;
            v.position.x = -halfX + t * sizeX;   // X: 左端→右端
            v.position.y = 0.0f;                  // Y: 高さ固定（地面）
            v.position.z = -halfZ + s * sizeY;    // Z: 手前→奥
            v.position.w = 1.0f;
            v.texcord.x  = t * uvTile;
            v.texcord.y  = s * uvTile;            // Z方向にそのままタイリング
            v.normal     = { 0.0f, 1.0f, 0.0f };  // Y+ 上向き法線
            model->modelData_.vertices.push_back(v);
        }
    }

    // ─── インデックス生成 ─────────────────────────────────────────────
    // XZ 平面の各セルを2三角形で構成（Y+ 法線に合わせ反時計回り = 上面が表）
    model->modelData_.indices.reserve(static_cast<size_t>(divisionsX) * divisionsY * 6);

    for (int row = 0; row < divisionsY; ++row) {
        for (int col = 0; col < divisionsX; ++col) {
            uint32_t lb = static_cast<uint32_t>( row      * colCount + col);       // 左手前
            uint32_t rb = static_cast<uint32_t>( row      * colCount + col + 1);   // 右手前
            uint32_t lt = static_cast<uint32_t>((row + 1) * colCount + col);       // 左奥
            uint32_t rt = static_cast<uint32_t>((row + 1) * colCount + col + 1);   // 右奥

            // 三角形1: 左手前 → 左奥 → 右手前（Y+法線・反時計回り）
            model->modelData_.indices.push_back(lb);
            model->modelData_.indices.push_back(lt);
            model->modelData_.indices.push_back(rb);

            // 三角形2: 右手前 → 左奥 → 右奥
            model->modelData_.indices.push_back(rb);
            model->modelData_.indices.push_back(lt);
            model->modelData_.indices.push_back(rt);
        }
    }

    model->CreateVertexBuffer();
    model->CreateIndexBuffer();
    model->CreateMaterialResource();

    return model;
}
