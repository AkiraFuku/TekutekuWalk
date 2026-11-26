#include "Fade.h"
#include "WinApp.h"
#include <algorithm> // std::clamp 用

void Fade::Initialize()
{
    // 1. フェード専用PSOの登録
    RegisterFadePso();

    // 2. スプライトの生成と初期化
    // ※ダミーの単色テクスチャや白画像などを指定します
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize("resources/white.png"); // ※環境に合わせて存在する画像パスに指定してください
    sprite_->SetPosition({ 0.0f, 0.0f });
    sprite_->SetSize({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight) });
    sprite_->SetAnchorPoint(Anchor::TopLeft);

    // スプライトにフェード専用PSOとブレンドモードを指定
    sprite_->SetPSOName("Fade");
    sprite_->SetBlendMode(BlendMode::Normal); // アルファ合成 (通常のフェード)

    // 初期状態
    status_ = Status::None;
    alpha_ = 0.0f;
    fading_ = false;

    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha_ });
    sprite_->Update();
}

void Fade::RegisterFadePso()
{
    PsoConfig config{};

    // シェーダーパスの設定
    PsoConfig::ShaderPath vsPath{ ShaderType::VS, L"resources/shaders/Fade/Fade.VS.hlsl", "main", L"vs_6_0" };
    PsoConfig::ShaderPath psPath{ ShaderType::PS, L"resources/shaders/Fade/Fade.PS.hlsl", "main", L"ps_6_0" };
    config.shaderPaths.push_back(vsPath);
    config.shaderPaths.push_back(psPath);

    // ルートシグネチャ (Sprite::Draw のバインド順と一致させる)
    // 0: マテリアル CBV (b0)
    // 1: 変換行列 CBV (b1)
    // 2: テクスチャ SRV (t0)
    config.rootSignatureGenerator = []() {
        return RootSignatureBuilder()
            .AddCBV(0, D3D12_SHADER_VISIBILITY_ALL)
            .AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(PSOManager::GetInstance()->StaticSamplers())
            .Build(DXCommon::GetInstance()->GetDevice().Get());
        };

    // 入力レイアウト (Sprite::VertexData のレイアウトに合わせる)
    config.inputLayoutGenerator = []() {
        InputLayout inputLayout{};
        inputLayout.inputElement = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        inputLayout.inputLayout.pInputElementDescs = inputLayout.inputElement.data();
        inputLayout.inputLayout.NumElements = static_cast<UINT>(inputLayout.inputElement.size());
        return inputLayout;
        };

    // 深度テストは無効（最前面に描画）
    config.depthEnable = false;
    config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    config.cullMode = D3D12_CULL_MODE_NONE;

    // PSOManager に "Fade" として登録
    PSOManager::GetInstance()->RegisterPsoGenerator("Fade", config);
}

void Fade::StartFadeIn(float duration)
{
    if (!IsFading()) {
        alpha_ = 1.0f; // 不透明からスタート
    };

    status_ = Status::FadeIn;
    fadeDuration_ = duration;
    timer_ = 0.0f;
    fading_ = true;

    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha_ });
    sprite_->Update();
}

void Fade::StartFadeOut(float duration)
{
    if (!IsFading()) {
        alpha_ = 0.0f; // 透明からスタート
    };

    status_ = Status::FadeOut;
    fadeDuration_ = duration;
    timer_ = 0.0f;
    fading_ = true;

    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha_ });
    sprite_->Update();
}

void Fade::Update()
{
    if (status_ == Status::None)
    {
        return;
    }

    timer_ += kDeltaTime_;
    float t = std::clamp(timer_ / fadeDuration_, 0.0f, 1.0f);

    if (status_ == Status::FadeIn)
    {
        // 1.0 (画面真っ黒) -> 0.0 (画面が見える)
        alpha_ = Lerp(1.0f, 0.0f, t);
    } else if (status_ == Status::FadeOut)
    {
        // 0.0 (画面が見える) -> 1.0 (画面真っ黒)
        alpha_ = Lerp(0.0f, 1.0f, t);
    }

    if (t >= 1.0f)
    {
        status_ = Status::None;
        fading_ = false;
    }

    // 黒フェード（RGBA = 0, 0, 0, alpha）
    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, alpha_ });
    sprite_->Update();
}

void Fade::Draw()
{
    // 完全透明かつフェード実行中でない場合は描画をスキップ
    if ((status_ == Status::None && alpha_ <= 0.0f) )
    {
        return;
    }

    sprite_->Draw();
}