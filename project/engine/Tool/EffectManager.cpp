#include "EffectManager.h"
#include <iostream>
#include <algorithm>
#include <numbers>
#include "MathFunction.h"

void EffectManager::TriggerEffect(PostEffectFlag flag, float duration, std::function<void(float progress)> onUpdate)
{
    // オフスクリーン側でフラグを有効化
    OffScreen::GetInstance()->EnableEffect(flag);

    // タイマーキューに追加
    activeRequests_.push_back({ flag, duration, 0.0f, onUpdate });
}

void EffectManager::Update()
{

    for (auto it = activeRequests_.begin(); it != activeRequests_.end(); ) {
        it->timer += DeltaTime;
        float progress = std::clamp(it->timer / it->duration, 0.0f, 1.0f);

        // アニメーション更新（イージング適用等）
        if (it->updateCallback) {
            it->updateCallback(progress);
        }

        // 時間切れで自動解除
        if (it->timer >= it->duration) {
            OffScreen::GetInstance()->DisableEffect(it->flag);
            it = activeRequests_.erase(it);
        } else {
            ++it;
        }
    }
}
// =========================================================
// パラメータ変化ファンクショナル (std::function) の生成実装
// =========================================================

// ディゾルブパラメータ変化
std::function<void(float)> EffectManager::CreateDissolveFunctional(float startThreshold, float endThreshold)
{
    return [startThreshold, endThreshold](float progress) {
        // 線形補間（必要に応じて EaseInOut などに変更可能）
        float currentThreshold = Lerp(startThreshold, endThreshold, progress);
        OffScreen::GetInstance()->SetDissolveThreshold(currentThreshold);
        };
}

// ラジアルブラーパラメータ変化（イージング減衰付き）
std::function<void(float)> EffectManager::CreateRadialBlurFunctional(float startWidth, float endWidth)
{
    return [startWidth, endWidth](float progress) {
        // イージング(EaseOutQuad)を適用して自然に減衰させる: 1 - (1 - t)^2
        float easeProgress = Lerp(0.0f, 1.0f, 1.0f - (1.0f - progress) * (1.0f - progress));
        float currentWidth = Lerp(startWidth, endWidth, easeProgress);
        OffScreen::GetInstance()->SetBlurWidth(currentWidth);
        };
}

// ラジアルブラー（衝撃・アタック演出向け：山型変化）
std::function<void(float)> EffectManager::CreateRadialBlurImpulseFunctional(float maxBlurWidth)
{
    return [maxBlurWidth](float progress) {
        // sin( progress * PI ) を使うことで 0.0 -> 1.0 -> 0.0 と推移
        float impulse = std::sin(progress * std::numbers::pi_v<float>);
        OffScreen::GetInstance()->SetBlurWidth(maxBlurWidth * impulse);
        };
}