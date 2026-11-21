#pragma once
#include "Vector4.h"
#include "Vector2.h"
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include "Transform.h"
#include "PSOManager.h"
#include <initializer_list> 
#include "RenderTypes.h"
#include "IInspectable.h"



namespace Anchor {
    static constexpr Vector2 TopLeft = { 0.0f, 0.0f };// 左上
    static constexpr Vector2 TopMin = { 0.5f, 0.0f };// 上中央
    static constexpr Vector2 TopRight = { 1.0f, 0.0f };// 右上
    static constexpr Vector2 CenterLeft = { 0.0f, 0.5f };// 左中央
    static constexpr Vector2 Center = { 0.5f, 0.5f };// 中央
    static constexpr Vector2 CenterRight = { 1.0f, 0.5f };//右中央
    static constexpr Vector2 BottomLeft = { 0.0f, 1.0f };// 左下
    static constexpr Vector2 BottomMin = { 0.5f, 1.0f };// 下中央
    static constexpr Vector2 BottomRight = { 1.0f, 1.0f };// 右下
}

/**
 * @brief 2Dスプライトの描画・制御を行うクラス
 * @details 描画位置、サイズ、アンカーポイント、UVの切り出し、反転（フリップ）などの基本機能を備えています。
 */
class Sprite : public IEditorInspectable
{
public:
    virtual ~Sprite() override;

    // ─── IEditorInspectable ──────────────────────────────────────────
    std::string GetInspectorName() const override;
    std::string GetInspectorCategory() const override { return "UI / Sprite"; }
    bool* GetInspectorVisiblePtr() override { return &isVisible_; }
    void OnInspectorGUI() override;
    /// @brief 頂点データ構造体
    struct VertexData {
        Vector4 position; //!< 3D座標（w要素含む）
        Vector2 texcord;  //!< テクスチャUV座標
        Vector3 normal;   //!< 法線ベクトル
    };

    #include "../base/RenderTypes.h"

    // Material と TransformationMatrix は共通ヘッダー RenderTypes.h で定義されます

    /**
     * @brief スプライトの初期化
     * @param textureFilePath 読み込むテクスチャのファイルパス
     */
    void Initialize(std::string textureFilePath);

    /**
     * @brief 毎フレームの更新処理
     * @details 頂点座標の計算（アンカーポイントやフリップの適用）および各種行列の計算を行います。
     */
    void Update();

    /**
     * @brief 描画コマンドの積み込み
     */
    void Draw();

    // ==========================================
    // ゲッター / セッター (アクセサ)
    // ==========================================

    /// @brief 表示位置（スクリーン座標系）を取得
    const Vector2& GetPosition() const {
        return position_;
    }
    /// @brief 表示位置（スクリーン座標系）を設定
    void SetPosition(const Vector2& position) {
        position_ = position;
    }

    /// @brief 回転角（ラジアン）を取得
    float GetRotation() const {
        return rotation_;
    }
    /// @brief 回転角（ラジアン）を設定
    void SetRotation(const float rotation) {
        rotation_ = rotation;
    }

    /// @brief スプライトのカラー（RGBA）を取得
    Vector4& GetColor() const {
        return materialData_->color;
    }
    /// @brief スプライトのカラー（RGBA）を設定
    void SetColor(const Vector4& color) {
        materialData_->color = color;
    }

    /// @brief 定数バッファに書き込まれている現在のUV変換行列を取得
    Matrix4x4& GetUV() const {
        return materialData_->uvTransform;
    }
    /// @brief UV変換行列を直接設定
    void SetUV(Matrix4x4& uvTransform) {
        materialData_->uvTransform = uvTransform;
    }

    /// @brief 描画サイズ（ピクセル）を取得
    const Vector2& GetSize() const {
        return size_;
    }
    /// @brief 描画サイズ（ピクセル）を設定
    void SetSize(const Vector2& Size) {
        this->size_ = Size;
    }

    /// @brief アンカーポイント（基準点。左上[0,0]〜右下[1,1]）を取得
    const Vector2& GetAnchorPoint() const {
        return anchorPoint_;
    }
    /// @brief アンカーポイント（基準点。左上[0,0]〜右下[1,1]）を設定
    void SetAnchorPoint(const Vector2& anchorPoint) {
        anchorPoint_ = anchorPoint;
    }

    /// @brief X軸方向の反転フラグを取得
    bool GetIsFlipX() const {
        return isFlipX_;
    }
    /// @brief X軸方向の反転フラグを設定
    void SetIsFlipX(bool isFlipX) {
        isFlipX_ = isFlipX;
    }

    /// @brief Y軸方向の反転フラグを取得
    bool GetIsFlipY() const {
        return isFlipY_;
    }
    /// @brief Y軸方向の反転フラグを設定
    void SetIsFlipY(bool isFlipY) {
        isFlipY_ = isFlipY;
    }

    /// @brief テクスチャの切り出し開始左上座標を取得
    Vector2 GetTextureLeftTop() const {
        return textureLeftTop;
    }
    /// @brief テクスチャの切り出し開始左上座標を設定
    void SetTextureLeftTop(const Vector2& textureLeftTop) {
        this->textureLeftTop = textureLeftTop;
    }

    /// @brief テクスチャの切り出しサイズを取得
    Vector2 GetTextureSize() const {
        return textureSize;
    }
    /// @brief テクスチャの切り出しサイズを設定
    void SetTextureSize(const Vector2& textureSize) {
        this->textureSize = textureSize;
    }

    /// @brief ブレンドモード（Alpha, Addなど）を設定
    void SetBlendMode(BlendMode blendMode) {
        blendMode_ = blendMode;
    }
    /// @brief ブレンドモードを取得
    BlendMode GetBlendMode() const {
        return blendMode_;
    }

    /// @brief フィルモード（Solid, Wireframe）を設定
    void SetFillMode(FillMode fillMode) {
        fillMode_ = fillMode;
    }

    /**
     * @brief 使用するテクスチャを変更
     * @param textureFilePath 新しいテクスチャのファイルパス
     */
    void SetTextureByFilePath(const std::string& textureFilePath);

    /**
     * @brief 切り替え用のテクスチャリスト（配列）にテクスチャを追加登録する
     * @param textureFilePath 追加するテクスチャのパス
     * @return 登録されたインデックス番号
     */
    size_t RegisterTexture(const std::string& textureFilePath);

    // ==========================================
    // 追加: まとめて登録できる配列版の登録関数
    // ==========================================
    
    /**
     * @brief 複数のテクスチャパスをまとめて登録する（vector版）
     * @param filePaths 追加するテクスチャパスの配列
     */
    void RegisterTextures(const std::vector<std::string>& filePaths);

    /**
     * @brief 複数のテクスチャパスをまとめて登録する（初期化子リスト版: {"a.png", "b.png"} と書けるようにするため）
     * @param filePaths 追加するテクスチャパスのリスト
     */
    void RegisterTextures(std::initializer_list<std::string> filePaths);

    /**
     * @brief 登録済みのテクスチャリストから、指定した番号のテクスチャに切り替える
     * @param index 登録されたインデックス
     */
    void SetTextureByIndex(size_t index);

    /**
     * @brief 登録されているテクスチャの総数を取得する
     */
    size_t GetRegisteredTextureCount() const {
        return registeredTextures_.size();
    }


    /// @brief UVトランスフォーム構造体（Scale, Rotate, Offset）を取得
    UVTransform GetUVTransform() const {
        return uvTransform_;
    }
    /// @brief UVトランスフォーム構造体をまとめて設定
    void SetUVTransform(const UVTransform& uvTransform) {
        uvTransform_ = uvTransform;
    }

    /// @brief UVのスケール（反復率）を設定
    void SetUVScale(const Vector2& scale) {
        uvTransform_.scale = scale;
    }
    /// @brief UVのスケールを取得
    Vector2 GetUVScale() const {
        return uvTransform_.scale;
    }

    /// @brief UVの回転角を設定
    void SetUVRotate(float rotate) {
        uvTransform_.rotate = rotate;
    }
    /// @brief UVの回転角を取得
    float GetUVRotate() const {
        return uvTransform_.rotate;
    }

    /// @brief UVのオフセット（シフト移動）を設定
    void SetUVOffset(const Vector2& offset) {
        uvTransform_.offset = offset;
    }
    /// @brief UVのオフセットを取得
    Vector2 GetUVOffset() const {
        return uvTransform_.offset;
    }

    /**
     * @brief 自身と同じ設定を持つ新しいスプライトを生成（複製）する
     * @return 複製されたスプライトのユニークポインタ
     */
    std::unique_ptr<Sprite> Clone() const;

    /**
     * @brief スプライトのサイズに合わせてUVのスケール（反復率）を自動調整する
     */
    void FitUVScaleToSpriteSize();
    /// @brief PSO名を設定する
    void SetPSOName(const std::string& psoName) { psoName_ = psoName; }
    /// @brief PSO名を取得する
    const std::string& GetPSOName() const { return psoName_; }
private:
    /**
     * @brief テクスチャの元サイズに合わせてスプライトサイズを自動調整する
     */
    void AdjustTextureSize();
    /**
     * @brief スプライトのサイズに合わせてテクスチャの切り出しサイズを自動調整する
     */
    void AdjustSpriteSize();

private:
    std::string psoName_ = "Sprite"; //!< 使用するPSO名 (デフォルトは "Sprite")
    BlendMode blendMode_ = BlendMode::Normal; //!< ブレンドモード（デフォルトは通常アルファ）
    FillMode fillMode_ = FillMode::kSolid;    //!< フィルモード（デフォルトは塗りつぶし）

    UVTransform uvTransform_; //!< UVの変形パラメータ

    Vector2 position_ = { 0.0f, 0.0f }; //!< 描画中心（またはアンカー）のスクリーン座標
    float rotation_ = 0.0f;             //!< 回転角（ラジアン）
    Vector2 size_ = { 10.0f, 10.0f };   //!< 描画サイズ（ピクセル）

    /**
     * @brief アンカーポイント
     * @details 原点を頂点のどこに置くか。(0.0, 0.0)で左上、(0.5, 0.5)で中央、(1.0, 1.0)で右下になります。
     */
    Vector2 anchorPoint_ = { 0.0f, 0.0f };

    bool isFlipX_ = false; //!< 左右反転フラグ
    bool isFlipY_ = false; //!< 上下反転フラグ
    bool isVisible_ = true; //!< 表示フラグ

    Vector2 textureLeftTop = { 0.0f, 0.0f }; //!< テクスチャ切り出しの左上座標（ピクセル単位）
    Vector2 textureSize{ 100.0f, 100.0f };   //!< テクスチャ切り出しサイズ（ピクセル単位）

    // Direct3D12 関連のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexRecourse_; //!< 頂点バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;  //!< インデックスバッファリソース
    VertexData* vertexData_ = nullptr;                      //!< 頂点バッファのマッピングポインタ
    uint32_t* indexData_ = nullptr;                         //!< インデックスバッファのマッピングポインタ
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;             //!< 頂点バッファビュー
    D3D12_INDEX_BUFFER_VIEW indexBufferView_;               //!< インデックスバッファビュー

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_; //!< マテリアル用定数バッファリソース
    MaterialCB* materialData_ = nullptr;                        //!< マテリアルのマッピングポインタ

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourse_; //!< 行列用定数バッファリソース
    TransformationMatrix* transformationMatrixData_ = nullptr;           //!< 行列のマッピングポインタ

    uint32_t textureIndex_ = 0;        //!< テクスチャマネージャーが管理する固有ID
    std::string textureFilePath_;      //!< 読み込んでいるテクスチャのパス
     //!< 読み込んでいるテクスチャのパス


    // ==========================================
    // 追加: 登録用テクスチャ情報構造体とリスト
    // ==========================================
    struct RegisteredTextureInfo {
        std::string filePath;
        uint32_t managerIndex;
    };
    std::vector<RegisteredTextureInfo> registeredTextures_;

};