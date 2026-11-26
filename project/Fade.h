#pragma once
#include "DXCommon.h"
#include "MathFunction.h"
#include "PSOManager.h"
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>
#include "Sprite.h"
#include "Singleton.h"

class Fade : public Singleton<Fade> {
public:
    enum class Status {
        None,   // フェードしていない
        FadeIn, // フェードイン中 (1 -> 0)
        FadeOut // フェードアウト中 (0 -> 1)
    };

    friend class Singleton<Fade>;
    friend struct std::default_delete<Fade>;

    void Initialize();
    void StartFadeIn(float duration);
    void StartFadeOut(float duration);

    void Update();
    void Draw();

    // フェード中かどうかを取得
    bool IsFading() const { return fading_; }
    // フェード状態を取得
    Status GetStatus() const { return status_; }

protected:
    Fade() = default;
    ~Fade() override = default;

private:
    std::unique_ptr<Sprite> sprite_;

    // フェード用PSOを登録する
    void RegisterFadePso();

    const float kDeltaTime_ = DXCommon::kDeltaTime; // デルタタイム[cite: 2, 3]
    bool fading_ = false;
    Status status_ = Status::None;
    float alpha_ = 0.0f;
    float fadeDuration_ = 1.0f; // フェードにかける時間（秒）[cite: 2, 3]
    float timer_ = 0.0f;        // 経過時間[cite: 2, 3]
};