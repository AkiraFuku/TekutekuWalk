#pragma once
#include <d3d12.h>     // 先にDirectXの定義を入れる
#include <wrl/client.h>
#include <memory>
#include <vector>
#include "Vector4.h"
#include "Vector2.h"
#include "PSOManager.h"


struct BlurParam
{
    Vector2 center;
    int32_t radius;
    float blurWidth;

};

// --- ビットフラグの定義 ---
enum class PostEffectFlag : uint32_t {
    None             = 0,       // 通常表示
    GrayScale        = 1 << 0,  // 0x01: グレースケール
    Random           = 1 << 1,  // 0x02: ランダムノイズ
    Vignette         = 1 << 2,  // 0x04: ヴィネット
    RadialBlur       = 1 << 3,  // 0x08: ラジアルブラー
    DepthOutline     = 1 << 4,  // 0x10: 深度アウトライン
    LuminanceOutline = 1 << 5,  // 0x20: 輝度アウトライン
    Dissolve         = 1 << 6,  // 0x40: ディゾルブ
    BoxFilter        = 1 << 7   // 0x80: 5x5ボックスフィルタ(平滑化) ← 追加
};// ビット演算子を使いやすくするためのオーバーロード定義
inline PostEffectFlag operator|(PostEffectFlag a, PostEffectFlag b) {
    return static_cast<PostEffectFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline PostEffectFlag operator&(PostEffectFlag a, PostEffectFlag b) {
    return static_cast<PostEffectFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline PostEffectFlag& operator|=(PostEffectFlag& a, PostEffectFlag b) {
    a = a | b;
    return a;
}
inline PostEffectFlag& operator&=(PostEffectFlag& a, PostEffectFlag b) {
    a = a & b;
    return a;
}
inline PostEffectFlag operator~(PostEffectFlag a) {
    return static_cast<PostEffectFlag>(~static_cast<uint32_t>(a));
}

class Camera;
class OffScreen
{
public:

    struct MaskMaterial
    {
        std::string textureFilePath;
        uint32_t maskTextureSrvIndex;
    };

    static OffScreen* GetInstance();

    OffScreen(const OffScreen&) = delete;
    OffScreen& operator=(const OffScreen&) = delete;
    friend struct std::default_delete<OffScreen>;

    void Initialize();
    void Finalize();
    void Update();
    void Draw();
    void DrawImGuiContents();

    void SetCamera(Camera* camera) {
        camera_ = camera;
    }

    void SetMaskMaterial( std::string textureFilePath) ;

    // --- ビットフラグ操作用 API ---
    /// <summary>
    /// ポストエフェクトの有効フラグを一括で設定します（指定したフラグで上書きします）。
    /// </summary>
    /// <param name="flags">有効にしたいエフェクトフラグの組み合わせ（OR(|)結合可能）</param>
    void SetEffectFlags(PostEffectFlag flags) { activeFlags_ = flags; }
    /// <summary>
    /// 指定したポストエフェクトを追加で有効化します。
    /// </summary>
    /// <param name="flag">有効化したいエフェクト（PostEffectFlag::A | PostEffectFlag::B で複数指定も可能）</param>
    void EnableEffect(PostEffectFlag flag) { activeFlags_ |= flag; }
    /// <summary>
    /// 指定したポストエフェクトを追加で有効化します。
    /// </summary>
    /// <param name="flag">有効化したいエフェクト（PostEffectFlag::A | PostEffectFlag::B で複数指定も可能）</param>
    void DisableEffect(PostEffectFlag flag) { activeFlags_ &= ~flag; }
    /// <summary>
    /// 指定したポストエフェクトの有効／無効状態を反転（トグル）します。
    /// </summary>
    /// <param name="flag">状態を反転させたいエフェクトPostEffectFlag::A | PostEffectFlag::B で複数指定も可能）</param>
    void ToggleEffect(PostEffectFlag flag) {
        activeFlags_ = static_cast<PostEffectFlag>(static_cast<uint32_t>(activeFlags_) ^ static_cast<uint32_t>(flag));
    }
    /// <summary>
    /// 指定したポストエフェクトが有効になっているか判定します。
    /// </summary>
    /// <param name="flag">確認したいエフェクト</param>
    /// <returns>有効な場合は true、無効な場合は false</returns>
    bool IsEffectActive(PostEffectFlag flag) const {
        return (static_cast<uint32_t>(activeFlags_) & static_cast<uint32_t>(flag)) != 0;
    }
    /// <summary>
    /// すべてのポストエフェクトを解除（無効化）し、通常描画状態に戻します。
    /// </summary>
    void ClearEffects() { activeFlags_ = PostEffectFlag::None; }


    BlurParam* GetBlurParam() const {
        return blurParamData_;
    }
    void SetBlurParam(const BlurParam& param) {
        if (blurParamData_) {
            *blurParamData_ = param;
        }
    }
    void SetBlurCenter(const Vector2& center) {
        if (blurParamData_) {
            blurParamData_->center = center;
        }
    }
    void SetBlurRadius(int radius) {
        if (blurParamData_) {
            blurParamData_->radius = radius;
        }
    }
    void SetBlurWidth(float width) {
        if (blurParamData_) {
            blurParamData_->blurWidth = width;
        }
    }

    void SetDissolveThreshold(float threshold) {
        if (dissolveParamData_) {
            dissolveParamData_->threshold = threshold;
        }
    }

    float GetDissolveThreshold() const {
        return dissolveParamData_ ? dissolveParamData_->threshold : 0.0f;
    }

private:
    OffScreen() = default;
    ~OffScreen() = default;

    static std::unique_ptr<OffScreen> instance;

    Camera* camera_;

    // 現在有効なポストエフェクトのフラグ (デフォルトは通常表示)
    PostEffectFlag activeFlags_ = PostEffectFlag::None;

    BlurParam* blurParamData_;
    Microsoft::WRL::ComPtr<ID3D12Resource> blurConstantBuffer_;

struct Material
{
    Matrix4x4 projectionInverse;
    float time;
    uint32_t activeFlags; // シェーダーに送るビットフラグ
    float padding[2];     // 16バイトアラインメント調整
};
    Material* materialData_;
    Microsoft::WRL::ComPtr<ID3D12Resource> materialConstantBuffer_;

    MaskMaterial MaskMaterial_;

    struct DissolveParm
    {
        float threshold;// ディゾルブのしきい値
        float padding[3]; // 16バイトアラインメントのためのパディング
    };
    DissolveParm* dissolveParamData_;
    Microsoft::WRL::ComPtr<ID3D12Resource> dissolveConstantBuffer_;

    // 前回のコードでこれらがエラーになっていた場合、ここに含まれる型が
    // <d3d12.h> を読み込むことで解決されます
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
};