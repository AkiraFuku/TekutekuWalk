#include "Sprite.h"
#include "SpriteCommon.h"
#include "MathFunction.h"
#include "TextureManager.h"
#include "DXCommon.h"
#include "Transform.h"
#include "EditorManager.h"
#include <filesystem>
#ifdef USE_IMGUI
#include "imgui.h"
#endif

Sprite::~Sprite() {
    EditorManager::SafeUnregister(this);
}

std::string Sprite::GetInspectorName() const {
    if (textureFilePath_.empty()) return "Sprite";
    return std::filesystem::path(textureFilePath_).filename().string();
}

void Sprite::OnInspectorGUI() {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("Sprite (2D)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat2("Position", &position_.x, 1.0f);
        ImGui::DragFloat2("Size", &size_.x, 1.0f, 0.0f, 4096.0f);
        ImGui::DragFloat("Rotation", &rotation_, 0.01f);
        ImGui::DragFloat2("Anchor", &anchorPoint_.x, 0.05f, 0.0f, 1.0f);
        if (materialData_) {
            ImGui::ColorEdit4("Color", &materialData_->color.x);
        }
        ImGui::Checkbox("Flip X", &isFlipX_);
        ImGui::Checkbox("Flip Y", &isFlipY_);
        ImGui::TextDisabled("Texture: %s", textureFilePath_.c_str());
    }
#endif
}

void Sprite::Initialize(std::string textureFilePath) {
    // UVトランスフォームの初期化（等倍、回転なし、ズレなし）
    uvTransform_.scale = { 1.0f, 1.0f };
    uvTransform_.rotate = 0.0f;
    uvTransform_.offset = { 0.0f, 0.0f };

    // 1. 頂点・インデックスバッファの作成とビューの設定 (4頂点 / 6インデックス)
    vertexRecourse_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * 4);
    indexResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(uint32_t) * 6);

    vertexBufferView_.BufferLocation = vertexRecourse_.Get()->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    indexBufferView_.BufferLocation = indexResource_.Get()->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT; // 32ビット整数インデックス

    // CPU側から書き込めるように常時マッピング
    vertexRecourse_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    indexResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

    // 2. マテリアルバッファの作成と初期設定
    materialResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(MaterialCB));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // デフォルトは白
    materialData_->enableLighting = false;                 // スプライトなのでライティングは原則OFF

    Matrix4x4 uvMatrix = MakeUVTransformMatrix(uvTransform_);
    materialData_->uvTransform = uvMatrix;

    // 3. 座標変換行列バッファの作成と初期設定
    transformationMatrixResourse_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResourse_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
    transformationMatrixData_->WVP = Makeidentity4x4();
    transformationMatrixData_->World = Makeidentity4x4();


    RegisterTexture(textureFilePath);
    EditorManager::GetInstance()->RegisterObject(this);
}

void Sprite::Update() {
    // アンカーポイントを考慮した基準となるローカル座標の計算
    float Left = 0.0f - anchorPoint_.x;
    float right = 1.0f - anchorPoint_.x;
    float top = 0.0f - anchorPoint_.y;
    float bottom = 1.0f - anchorPoint_.y;

    // 左右・上下反転の適用
    if (isFlipX_) {
        Left *= -1.0f;
        right *= -1.0f;
    }
    if (isFlipY_) {
        top *= -1.0f;
        bottom *= -1.0f;
    }

    // テクスチャのメタデータからUV座標（0.0〜1.0）を算出
    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath_);
    float tex_left = textureLeftTop.x / metadata.width;
    float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
    float tex_top = textureLeftTop.y / metadata.height;
    float tex_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;

    // 頂点データの更新 (左下・左上・右下・右上)
    // 左下
    vertexData_[0].position = { Left, bottom, 0.0f, 1.0f };
    vertexData_[0].texcord = { tex_left, tex_bottom };
    vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };
    // 左上
    vertexData_[1].position = { Left, top, 0.0f, 1.0f };
    vertexData_[1].texcord = { tex_left, tex_top };
    vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };
    // 右下
    vertexData_[2].position = { right, bottom, 0.0f, 1.0f };
    vertexData_[2].texcord = { tex_right, tex_bottom };
    vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };
    // 右上
    vertexData_[3].position = { right, top, 0.0f, 1.0f };
    vertexData_[3].texcord = { tex_right, tex_top };
    vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

    // 三角形2枚分のインデックス設定（クワッド構成）
    indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
    indexData_[3] = 2; indexData_[4] = 1; indexData_[5] = 3;

    // 各種行列（ワールド、ビュー、平行投影プロジェクション）の合成
    EulerTransform transform{ {size_.x, size_.y, 1.0f}, {0.0f, 0.0f, rotation_}, {position_.x, position_.y, 0.0f} };

    Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
    Matrix4x4 viewMatrix = Makeidentity4x4(); // スプライト用のカメラは恒等行列
    Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 0.0f, 100.0f);

    // 行列を乗算して定数バッファへ転送 (World -> View -> Projection)
    Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
    transformationMatrixData_->WVP = worldViewProjectionMatrix;
    transformationMatrixData_->World = worldMatrix;

    // UV変換行列を再計算してマテリアルバッファに反映
    Matrix4x4 uvMatrix = MakeUVTransformMatrix(uvTransform_);
    materialData_->uvTransform = uvMatrix;
}

void Sprite::Draw() {
    if (!isVisible_) {
        return;
    }
    // 共通の描画前準備処理を呼び出し
    SpriteCommon::GetInstance()->SpriteCommonDraw();

    // 各種ブレンド・フィルモードに対応したPSO（パイプラインステート）の取得
    auto psoSet = PSOManager::GetInstance()->GetPso(psoName_, blendMode_, fillMode_);

    // パイプラインステートの設定
    DXCommon::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());

    // 各種バッファビューの割り当て
    DXCommon::GetInstance()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
    DXCommon::GetInstance()->GetCommandList()->IASetIndexBuffer(&indexBufferView_);

    // 【ルートパラメータのバインド順の備忘録】
    // RootRegister[0]: マテリアル用 constantBuffer
    // RootRegister[1]: 座標変換行列用 constantBuffer
    // RootRegister[2]: テクスチャ用 DescriptorTable (SRV)

    // マテリアルバッファの設定 (ルートパラメータIndex: 0)
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // 座標変換行列バッファの設定 (ルートパラメータIndex: 1)
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourse_->GetGPUVirtualAddress());

    // テクスチャSRV（ディスクリプタテーブル）の設定 (ルートパラメータIndex: 2)
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_));

    // インデックスを使用したプリミティブの描画（計6インデックス、1インスタンス）
    DXCommon::GetInstance()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::SetTextureByFilePath(const std::string& textureFilePath) {
    textureFilePath_ = textureFilePath; // パスを更新
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
}

size_t Sprite::RegisterTexture(const std::string& textureFilePath)
{

    // すでに同じファイルパスが登録されているかチェック
    for (size_t i = 0; i < registeredTextures_.size(); ++i) {
        if (registeredTextures_[i].filePath == textureFilePath) {
            return i; // 登録済みならそのインデックスを返して終了
        }
    }

    // TextureManagerを介してGPU側のテクスチャインデックスを取得（未ロードならロードされる）
    uint32_t managerIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

    // 情報を格納
    registeredTextures_.push_back({ textureFilePath, managerIndex });

    // 最初に登録されたテクスチャをデフォルトとして適用
    if (registeredTextures_.size() == 1) {
        textureFilePath_ = textureFilePath;
        textureIndex_ = managerIndex;
        AdjustTextureSize();
    }

    // 登録された番号（配列のインデックス）を返す
    return registeredTextures_.size() - 1;
}

void Sprite::RegisterTextures(const std::vector<std::string>& filePaths)
{
    for (const auto& filePath : filePaths) {
        RegisterTexture(filePath);
    }
}
void Sprite::RegisterTextures(std::initializer_list<std::string> filePaths)
{
    for (const auto& filePath : filePaths) {
        RegisterTexture(filePath);
    }
}

void Sprite::SetTextureByIndex(size_t index)
{
    // 範囲チェック
    if (index >= registeredTextures_.size()) {
        assert(false && "指定されたテクスチャインデックスは登録されていません。");
        return;
    }

    // カレントのテクスチャ情報を更新
    textureFilePath_ = registeredTextures_[index].filePath;
    textureIndex_ = registeredTextures_[index].managerIndex;

    // サイズを新しいテクスチャに合わせる
    //AdjustTextureSize();
    FitUVScaleToSpriteSize();
    uvTransform_.scale = { 1.0f, 1.0f };
    uvTransform_.offset = { 0.0f, 0.0f };
    uvTransform_.rotate = 0.0f;
}

std::unique_ptr<Sprite> Sprite::Clone() const {
    // 1. 新しいインスタンスを生成
    auto newSprite = std::make_unique<Sprite>();

    // 2. バッファ生成とテクスチャの初期化
    // 登録されているテクスチャがあるか安全にチェックする
    if (!this->registeredTextures_.empty()) {
        // 最初のテクスチャで Initialize を呼び出してバッファを構築[cite: 16]
        newSprite->Initialize(this->registeredTextures_[0].filePath);

        // 2枚目以降があれば追加登録[cite: 16]
        for (size_t i = 1; i < this->registeredTextures_.size(); ++i) {
            newSprite->RegisterTexture(this->registeredTextures_[i].filePath);
        }
    } else {
        // 万が一 registeredTextures_ が空の場合の安全策（フォールバック）
        // カレントのテクスチャパスを利用して最低限の初期化を試みる
        newSprite->Initialize(this->textureFilePath_);
    }

    // 3. トランスフォームや各種設定パラメータのコピー
    newSprite->position_ = this->position_;
    newSprite->rotation_ = this->rotation_;
    newSprite->size_ = this->size_;
    newSprite->anchorPoint_ = this->anchorPoint_;
    newSprite->isFlipX_ = this->isFlipX_;
    newSprite->isFlipY_ = this->isFlipY_;
    newSprite->blendMode_ = this->blendMode_;
    newSprite->fillMode_ = this->fillMode_;

    // 4. テクスチャの切り出し位置・サイズ・UVアニメーションのコピー
    newSprite->textureLeftTop = this->textureLeftTop;
    newSprite->textureSize = this->textureSize;
    newSprite->uvTransform_ = this->uvTransform_;

    // 5. 現在選択されているテクスチャ状態の同期
    newSprite->textureFilePath_ = this->textureFilePath_;
    newSprite->textureIndex_ = this->textureIndex_;

    // 6. 色（マテリアル）のコピー
    if (this->materialData_ && newSprite->materialData_) {
        newSprite->materialData_->color = this->materialData_->color;
    }
    newSprite->psoName_ = this->psoName_;

    return newSprite;
}
void Sprite::AdjustTextureSize() {
    // テクスチャのメタデータを取得して、スプライト全体のサイズを画像本来のサイズに合わせる
    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath_);

    textureSize.x = static_cast<float>(metadata.width);
    textureSize.y = static_cast<float>(metadata.height);
    size_ = textureSize;
}

void Sprite::AdjustSpriteSize()
{
    textureSize = size_;
}
void Sprite::FitUVScaleToSpriteSize() {

    // UVトランスフォーム（行列）は余計な変形をしないよう等倍にリセットする
    uvTransform_.scale = { 1.0f, 1.0f };
    uvTransform_.offset = { 0.0f, 0.0f };
    uvTransform_.rotate = 0.0f;
    // 1. テクスチャの元の解像度を取得

    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath_);


    if (metadata.width == 0 || metadata.height == 0 || size_.x == 0.0f || size_.y == 0.0f) {
        return; // ゼロ除算防止
    }
    // 1. 頂点UVを「画像全体(0.0〜1.0)」にするため、テクスチャ本来のサイズを設定
    textureSize.x = static_cast<float>(metadata.width);
    textureSize.y = static_cast<float>(metadata.height);

    // 2. UV行列（シェーダー側）でスプライトのサイズに合わせるため、逆数を設定する
    // ※一般的なUVトランスフォーム行列は、1.0を基準に「大きくするとテクスチャが縮小（タイリング）」します。
    // スプライトサイズにぴったり収めるには、テクスチャサイズに対するスプライトの比率を設定します。
    uvTransform_.scale.x = static_cast<float>(metadata.width) / size_.x;
    uvTransform_.scale.y = static_cast<float>(metadata.height) / size_.y;
}
