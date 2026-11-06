#include "RailPath.h"
#include "PrimitiveDrawer.h"
#include <algorithm>
#include <cmath>

void RailPath::Update()
{
    if (isDirty_) {
        BuildDistanceTable();
        isDirty_ = false;
    }
}

void RailPath::SetBezierHandles(size_t index, const Vector3& offsetIn, const Vector3& offsetOut) {
    if (index < points_.size()) {
        points_[index].type = InterpolationType::Bezier;
        points_[index].controlIn = points_[index].position + offsetIn;
        points_[index].controlOut = points_[index].position + offsetOut;
        isDirty_ = true;
    }
}

// 次の点への方向ベクトルを取得（回転制御用）
Vector3 RailPath::GetDirection(float globalT) const {
    if (points_.size() < 2) {
        return { 0.0f, 0.0f, 1.0f };
    }

    const float delta = 0.01f; // 微小な変化量
    const float maxT = GetMaxT();
    float clampedT = std::clamp(globalT, 0.0f, maxT);

    Vector3 diff = { 0.0f, 0.0f, 0.0f };
    if (clampedT >= maxT - delta) {
        Vector3 p0 = GetPosition((std::max)(0.0f, clampedT - delta));
        Vector3 p1 = GetPosition(clampedT);
        diff = p1 - p0;
    } else {
        Vector3 p1 = GetPosition(clampedT);
        Vector3 p2 = GetPosition((std::min)(clampedT + delta, maxT));
        diff = p2 - p1;
    }

    // ベジェ制御点がアンカーと同一座標にある場合（始点・終端で速度が0になるケース）のフォールバック
    if (Length(diff) < 1e-5f) {
        const float wideDelta = 0.05f;
        float t0 = (std::max)(0.0f, clampedT - wideDelta);
        float t1 = (std::min)(maxT, clampedT + wideDelta);
        diff = GetPosition(t1) - GetPosition(t0);
    }

    if (Length(diff) < 1e-5f) {
        size_t idx = (std::min)(static_cast<size_t>(clampedT), points_.size() - 2);
        diff = points_[idx + 1].position - points_[idx].position;
    }

    return (Length(diff) > 1e-5f) ? Normalize(diff) : Vector3{ 0.0f, 0.0f, 1.0f };
}

float RailPath::GetTFromDistance(float distance) const
{
    if (distanceTable_.empty() || totalLength_ <= 0.0f) return 0.0f;
    if (distance <= 0.0f) return 0.0f;
    if (distance >= totalLength_) return GetMaxT();

    // 二分探索で指定された距離に対応する区間を高速に探す
    auto it = std::upper_bound(
        distanceTable_.begin(), distanceTable_.end(), distance,
        [](float val, const DistanceMap& entry) { return val < entry.distance; }
    );
    
    if (it == distanceTable_.begin()) return 0.0f;
    if (it == distanceTable_.end()) return GetMaxT();

    const auto& next = *it;
    const auto& prev = *(it - 1);
    float span = next.distance - prev.distance;
    if (span <= 1e-6f) {
        return prev.t;
    }

    float factor = (distance - prev.distance) / span;
    return prev.t + (next.t - prev.t) * factor;
}

void RailPath::BuildDistanceTable() {
    distanceTable_.clear();
    totalLength_ = 0.0f;

    if (points_.size() < 2) {
        distanceTable_.push_back({ 0.0f, 0.0f });
        isDirty_ = false;
        return;
    }

    const float maxT = GetMaxT();
    const int subdivisionsPerSegment = 20; // 1区間あたり20分割（step = 0.05f 相当）
    const int totalSteps = static_cast<int>(std::round(maxT)) * subdivisionsPerSegment;

    if (totalSteps <= 0) {
        distanceTable_.push_back({ 0.0f, 0.0f });
        isDirty_ = false;
        return;
    }

    distanceTable_.reserve(static_cast<size_t>(totalSteps) + 1);
    Vector3 prevPos = GetPosition(0.0f);
    distanceTable_.push_back({ 0.0f, 0.0f });

    for (int i = 1; i <= totalSteps; ++i) {
        float currentT = (static_cast<float>(i) / static_cast<float>(totalSteps)) * maxT;
        Vector3 currentPos = GetPosition(currentT);
        totalLength_ += Length(currentPos - prevPos);
        distanceTable_.push_back({ totalLength_, currentT });
        prevPos = currentPos;
    }
    isDirty_ = false;
}

void RailPath::DebugDraw()
{

    if (points_.empty()) return;

    float maxT = GetMaxT();
    float step = 0.1f; // 分割精度

    // --- 1. レール本体の描画 ---
    for (float t = 0; t < maxT; t += step) {
        float nextT = min(t + step, maxT);
        
        // 区間の開始インデックスから補間タイプを特定（色を変えると分かりやすい）
        size_t index = static_cast<size_t>(t);
        Vector4 color = { 1, 1, 1, 1 }; // デフォルト：白

        if (points_[index].type == InterpolationType::Linear)     color = { 1, 1, 0, 1 }; // 黄
        if (points_[index].type == InterpolationType::CatmullRom) color = { 1, 0, 0, 1 }; // 赤
        if (points_[index].type == InterpolationType::Bezier)     color = { 0, 1, 0, 1 }; // 緑

        PrimitiveDrawer::GetInstance()->DrawLine(GetPosition(t), GetPosition(nextT), color);
    }

    // --- 2. 制御点（アンカー）とハンドルの描画 ---
    for (size_t i = 0; i < points_.size(); ++i) {
        const auto& p = points_[i];
        const auto& pnext = (i + 1 < points_.size()) ? points_[i + 1] : p; // 次の点（存在しない場合は同じ点）

        // アンカーポイント（実際に通る点）を立方体や球で描画
        // ※PrimitivaeDrawerにDrawBoxなどがある想定
        PrimitiveDrawer::GetInstance()->DrawSphere({ p.position, 0.5f }, { 1, 1, 1, 1 }); // アンカーポイントを白い球で描画

        // ベジェハンドルの描画
        if (p.type == InterpolationType::Bezier) {
            // 出ていくハンドル (controlOut)
            PrimitiveDrawer::GetInstance()->DrawLine(p.position, p.controlOut, { 0, 0.8f, 1, 1 });
            PrimitiveDrawer::GetInstance()->DrawLine(p.controlOut, pnext.position, { 0, 0.8f, 1, 1 });

            PrimitiveDrawer::GetInstance()->DrawSphere({ p.controlOut, 0.25f ,{ 0.0f,0.0f,0.0f,1.0f }}, { 0, 1.0f, 1, 1 }); // controlOutをシアンの球で描画
            // 入ってくるハンドル (controlIn)
            // ※ 次の点がある場合、次の点の controlIn と繋がっている
            if (i > 0) {
                 PrimitiveDrawer::GetInstance()->DrawLine(p.position, p.controlIn, { 0, 0.5f, 1, 1 });
                 PrimitiveDrawer::GetInstance()->DrawLine(p.controlIn, pnext.position, { 0, 0.5f, 1, 1 });

                 PrimitiveDrawer::GetInstance()->DrawSphere({ p.controlIn, 0.25f,{ 0.0f,0.0f,0.0f,1.0f } }, { 0, 1.0f, 1, 1 }); // controlInを薄いシアンの球
            }
        }
    }



}

float RailPath::GetDistanceFromT(float t) const {
    if (distanceTable_.empty() || totalLength_ <= 0.0f) return 0.0f;
    if (t <= 0.0f) return 0.0f;
    if (t >= GetMaxT()) return totalLength_;

    auto it = std::upper_bound(
        distanceTable_.begin(), distanceTable_.end(), t,
        [](float val, const DistanceMap& entry) { return val < entry.t; }
    );

    if (it == distanceTable_.begin()) return 0.0f;
    if (it == distanceTable_.end()) return totalLength_;

    const auto& next = *it;
    const auto& prev = *(it - 1);
    float span = next.t - prev.t;
    if (span <= 1e-6f) {
        return prev.distance;
    }

    float factor = (t - prev.t) / span;
    return prev.distance + (next.distance - prev.distance) * factor;
}
