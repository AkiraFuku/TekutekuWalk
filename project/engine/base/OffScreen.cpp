#include "OffScreen.h"
#include "DXCommon.h"
#include "Logger.h"
#include "SrvManager.h"
#include <cassert>
#include "Camera.h"
#include "mathfunction.h"
#include "TextureManager.h"
#include "SceneManager.h"
#include "Scene.h"
#ifdef USE_IMGUI
#include "imgui.h"
#endif // USE_IMGUI
// インスタンス定義
std::unique_ptr<OffScreen> OffScreen::instance = nullptr;

OffScreen* OffScreen::GetInstance() {
    if (instance == nullptr) {
        struct Helper : public OffScreen {
            Helper() : OffScreen() {}
        };
        instance = std::make_unique<Helper>();
    }
    return instance.get();
}

void OffScreen::Initialize() {


    MaskMaterial_.textureFilePath = "resources/Mask/noise0.png";
    TextureManager::GetInstance()->LoadTexture(MaskMaterial_.textureFilePath);
    MaskMaterial_.maskTextureSrvIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(MaskMaterial_.textureFilePath);



    // =========================================================
// ポストエフェクト統括用 PSO ("PostEffect") の登録
// =========================================================
    PsoConfig psoConfig = {};
    psoConfig.shaderPaths = {
        { ShaderType::VS, L"resources/shaders/CopyImage/FullScreen.vs.hlsl", "main", L"vs_6_0" },
        { ShaderType::PS, L"resources/shaders/CopyImage/PostEfect.PS.hlsl", "main", L"ps_6_0" }
    };

    psoConfig.rootSignatureGenerator = []() {
        // 静的サンプラー s0 (リニアフィルタ) と s1 (ポイントフィルタ)
        auto sampler0 = PSOManager::GetInstance()->StaticSamplers(); // s0

        auto sampler1 = sampler0;
        sampler1.ShaderRegister = 1;                        // s1
        sampler1.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;   // 深度用ポイントフィルタ

        return RootSignatureBuilder()
            // [Param 0] Color Texture (t0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)

            // [Param 1] Depth Texture (t1)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL)

            // [Param 2] Mask Texture (t2)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, D3D12_SHADER_VISIBILITY_PIXEL)

            // [Param 3] Main Material CBV (b0: projectionInverse, time, activeFlags)
            .AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL)

            // [Param 4] Blur Param CBV (b1: center, radius, blurWidth)
            .AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL)

            // [Param 5] Dissolve Param CBV (b2: threshold)
            .AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL)

            // スタティックサンプラー (s0, s1)
            .AddStaticSampler(sampler0)
            .AddStaticSampler(sampler1)

            .Build(DXCommon::GetInstance()->GetDevice().Get());
        };

    psoConfig.inputLayoutGenerator = []() {
        InputLayout inputLayout = {};
        inputLayout.inputLayout = D3D12_INPUT_LAYOUT_DESC{ nullptr, 0 };
        return inputLayout;
        };

    psoConfig.depthEnable = false;

    // "PostEffect" という名前でPSOを登録
    PSOManager::GetInstance()->RegisterPsoGenerator("PostEffect", psoConfig);

    // マテリアル定数バッファの生成とマップ
    materialConstantBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(Material));
    materialConstantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));


    // Initializeの末尾などで生成（materialConstantBuffer_ と同様の処理）
    blurConstantBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(BlurParam));
    blurConstantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&blurParamData_));

    // 初期値をセットしておく
    blurParamData_->center = Vector2(0.5f, 0.5f);
    blurParamData_->radius = 5; // HLSL側の int samples に対応
    blurParamData_->blurWidth = 0.01f;

    // ディゾルブ定数バッファの生成とマップ
    dissolveConstantBuffer_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(DissolveParm));
    dissolveConstantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&dissolveParamData_));


    dissolveParamData_->threshold = 0.5f; // 初期値
}
void OffScreen::Update()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Post Effect Debug Editor"))
    {
        // ----------------------------------------------------
        // 1. エフェクト ON / OFF フラグ切り替え
        // ----------------------------------------------------
        if (ImGui::CollapsingHeader("Effect Toggles", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // フラグ操作用ヘルパー関数
            auto FlagCheckbox = [this](const char* label, PostEffectFlag flag) {
                bool active = IsEffectActive(flag);
                if (ImGui::Checkbox(label, &active))
                {
                    if (active) {
                        EnableEffect(flag);
                    } else {
                        DisableEffect(flag);
                    }
                }
                };

            FlagCheckbox("Gray Scale", PostEffectFlag::GrayScale);
            FlagCheckbox("Random Noise", PostEffectFlag::Random);
            FlagCheckbox("Vignette", PostEffectFlag::Vignette);
            FlagCheckbox("Radial Blur", PostEffectFlag::RadialBlur);
            FlagCheckbox("Depth Outline", PostEffectFlag::DepthOutline);
            FlagCheckbox("Luminance Outline", PostEffectFlag::LuminanceOutline);
            FlagCheckbox("Dissolve", PostEffectFlag::Dissolve);
            FlagCheckbox("Box Filter (5x5)", PostEffectFlag::BoxFilter);

            ImGui::Spacing();
            if (ImGui::Button("Clear All Effects"))
            {
                ClearEffects();
            }
        }

        // ----------------------------------------------------
        // 2. ラジアルブラー用パラメータ調整
        // ----------------------------------------------------
        if (blurParamData_ && IsEffectActive(PostEffectFlag::RadialBlur))
        {
            if (ImGui::CollapsingHeader("Radial Blur Parameters", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat2("Center UV", &blurParamData_->center.x, 0.005f, 0.0f, 1.0f);
                ImGui::SliderInt("Samples (Radius)", &blurParamData_->radius, 1, 64);
                ImGui::SliderFloat("Blur Width", &blurParamData_->blurWidth, 0.0f, 0.05f, "%.4f");
            }
        }

        // ----------------------------------------------------
        // 3. ディゾルブ用パラメータ調整
        // ----------------------------------------------------
        if (dissolveParamData_ && IsEffectActive(PostEffectFlag::Dissolve))
        {
            if (ImGui::CollapsingHeader("Dissolve Parameters", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat("Threshold", &dissolveParamData_->threshold, 0.0f, 1.0f);
            }
        }

        // ----------------------------------------------------
        // 4. マテリアル・タイム情報表示
        // ----------------------------------------------------
        if (materialData_)
        {
            if (ImGui::CollapsingHeader("System Info"))
            {
                ImGui::Text("Shader Time: %.2f sec", materialData_->time);
                if (ImGui::Button("Reset Time"))
                {
                    materialData_->time = 0.0f;
                }
            }
        }
    }
    ImGui::End();
#endif // USE_IMGUI



}
void OffScreen::Draw()
{
    auto commandList = DXCommon::GetInstance()->GetCommandList();
    auto psoManager = PSOManager::GetInstance();
    auto srvManager = SrvManager::GetInstance();

    if (!camera_)
    {
        Logger::Log("Camera is not set for OffScreen.");
        return;
    }

    // 1. 定数バッファの更新
    if (materialData_)
    {
        Scene* curScene = SceneManager::GetInstance()->GetCurrentScene();
        if (!curScene || !curScene->IsGameTimeStopped()) {
            materialData_->time += DXCommon::GetInstance()->kDeltaTime;
        }
        materialData_->projectionInverse = Inverse(camera_->GetProjectionMatrix());
        materialData_->activeFlags = static_cast<uint32_t>(activeFlags_); // フラグをセット
    }

    // 2. 統括PSO の取得とバインド
    PsoSet pso = psoManager->GetPso("PostEffect");
    commandList->SetGraphicsRootSignature(pso.rootSignature.Get());
    commandList->SetPipelineState(pso.pipelineState.Get());

    // 3. トポロジ設定
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 4. 各リソースのバインド (Root Parameter 0 ~ 5)

    // [Param 0] メインカラーテクスチャ (t0)
    uint32_t colorSrvIndex = DXCommon::GetInstance()->GetRenderTextureSrvIndex();
    srvManager->SetGraphicsRootDescriptorTable(0, colorSrvIndex);

    // [Param 1] 深度テクスチャ (t1)
    uint32_t depthSrvIndex = DXCommon::GetInstance()->GetDepthTextureSrvIndex();
    srvManager->SetGraphicsRootDescriptorTable(1, depthSrvIndex);

    // [Param 2] マスクノイズテクスチャ (t2)
    srvManager->SetGraphicsRootDescriptorTable(2, MaskMaterial_.maskTextureSrvIndex);

    // [Param 3] メインマテリアル CBV (b0)
    commandList->SetGraphicsRootConstantBufferView(3, materialConstantBuffer_->GetGPUVirtualAddress());

    // [Param 4] ブラーパラメータ CBV (b1)
    commandList->SetGraphicsRootConstantBufferView(4, blurConstantBuffer_->GetGPUVirtualAddress());

    // [Param 5] ディゾルブパラメータ CBV (b2)
    commandList->SetGraphicsRootConstantBufferView(5, dissolveConstantBuffer_->GetGPUVirtualAddress());

    // 5. 描画実行
    commandList->DrawInstanced(3, 1, 0, 0);
}

void OffScreen::SetMaskMaterial(std::string textureFilePath)
{}
