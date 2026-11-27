#pragma once

#include <d3d12.h>     // 先にDirectXの定義を入れる
#include <wrl/client.h>
#include <memory>
#include "DXCommon.h"
#include "OffScreen.h"
#include <functional>
#include "Singleton.h"
// アニメーション付きエフェクトのリクエスト構造体
struct EffectRequest {
    PostEffectFlag flag;
    float duration;      // 全体の演出時間（秒）
    float timer = 0.0f;  // 経過時間
    
    // イージング等のパラメータ補間用コールバックやラムダ
    std::function<void(float progress)> updateCallback;
};


class EffectManager : public Singleton<EffectManager>
{

public:
    friend class Singleton<EffectManager>;
    friend struct std::default_delete<EffectManager>;
    void TriggerEffect(PostEffectFlag flag, float duration, std::function<void(float progress)> onUpdate = nullptr);
    void Update();

    /// <summary>
    /// ディゾルブの閾値(threshold)を変化させる標準ファンクショナル
    /// </summary>
    /// <param name="startThreshold">開始時の閾値 (例: 0.0f)</param>
    /// <param name="endThreshold">終了時の閾値 (例: 1.0f)</param>
    static std::function<void(float)> CreateDissolveFunctional(float startThreshold = 0.0f, float endThreshold = 1.0f);

    /// <summary>
    /// ラジアルブラーの強度(blurWidth)を直線的に変化させるファンクショナル
    /// </summary>
    /// <param name="startWidth">開始時のブラー幅 (例: 0.03f)</param>
    /// <param name="endWidth">終了時のブラー幅 (例: 0.0f)</param>
    static std::function<void(float)> CreateRadialBlurFunctional(float startWidth = 0.03f, float endWidth = 0.0f);

    /// <summary>
    /// 衝撃・ヒット用：ブラーが一瞬強くなってから徐々に収まる(山型)ファンクショナル
    /// </summary>
    /// <param name="maxBlurWidth">最大のブラー幅</param>
    static std::function<void(float)> CreateRadialBlurImpulseFunctional(float maxBlurWidth = 0.04f);

protected:
    EffectManager() = default;
    ~EffectManager() override = default;

private:
    float DeltaTime=DXCommon::kDeltaTime;
    std::vector<EffectRequest> activeRequests_;

};

