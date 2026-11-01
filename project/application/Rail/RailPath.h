#pragma once
#include "MathFunction.h"
#include <vector>
#include "Vector3.h"

class RailPath {
public:
    enum class InterpolationType {
        Linear,     // 直線
        CatmullRom, // キャットムル・ロム
        Bezier      // 三次ベジェ
    };

    struct RailPoint {
        Vector3 position;
        InterpolationType type = InterpolationType::Linear;

        // ベジェ用：この点から「出ていく」制御点と、次の点へ「入る」制御点
        // Catmull-Romの場合は自動計算されるため使いません
        Vector3 controlOut;
        Vector3 controlIn;
    };

    void Initialize() {
        points_.clear();
        distanceTable_.clear();
        totalLength_ = 0.0f;
        isDirty_ = true;
    }
    void AddPoint(const Vector3& pos) {
        points_.push_back({ pos,  InterpolationType::Linear, {0,0,0}, {0,0,0} });
        isDirty_ = true;
    }
    void AddPointCR(const Vector3& pos) {
        // control点は使わないのでゼロ初期化
        points_.push_back({ pos,  InterpolationType::CatmullRom, {0,0,0}, {0,0,0} });
        isDirty_ = true;
    }


    Vector3 GetPointPos(size_t index) const {

        if (index >= points_.size()) return { 0,0,0 };
        return points_[index].position;
    }

    Vector3 GetControlPointOut(size_t index) const {
        if (index >= points_.size()) return { 0,0,0 };
        return points_[index].controlOut;
    }
    Vector3 GetControlPointIn(size_t index) const {
        if (index >= points_.size()) return { 0,0,0 };
        return points_[index].controlIn;
    }

    void SetPointPos(size_t index, const Vector3& pos)
    {
        if (index < points_.size()) {
            Vector3 delta = pos - points_[index].position;
            if (delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f) {
                points_[index].position = pos;
                points_[index].controlIn = points_[index].controlIn + delta;
                points_[index].controlOut = points_[index].controlOut + delta;
                isDirty_ = true;
            }
        }
    }

    void SetControlPointOut(size_t index, const Vector3& control) {
        if (index < points_.size()) {
            points_[index].controlOut = control;
            isDirty_ = true;
        }
    }
    void SetControlPointIn(size_t index, const Vector3& control) {
        if (index < points_.size()) {
            points_[index].controlIn = control;
            isDirty_ = true;
        }
    }
    void Update();

    // ベジェ/直線/CatmullRom等の補間タイプを指定して点を追加
    void AddBezierPoint(const Vector3& pos, const Vector3& offsetIn, const Vector3& offsetOut, InterpolationType type = InterpolationType::Bezier) {
        points_.push_back({
            pos,
            type,
            pos + offsetOut, // 出ていくハンドル
            pos + offsetIn   // 入ってくるハンドル
            });
        isDirty_ = true;
    }
    void SetBezierHandles(size_t index, const Vector3& offsetIn, const Vector3& offsetOut);

    void SetPointType(size_t index, InterpolationType type) {
        if (index < points_.size()) {
            points_[index].type = type;
            isDirty_ = true;
        }
    }
    InterpolationType GetPointType(size_t index) const {
        if (index < points_.size()) {
            return points_[index].type;
        }
        return InterpolationType::Linear;
    }
    size_t GetPointCount() const { return points_.size(); }

    void SetAllInterpolationType(InterpolationType type) {
        for (auto& pt : points_) {
            pt.type = type;
        }
        isDirty_ = true;
    }

    Vector3 GetPosition(float globalT) const {
        if (points_.size() < 2) return points_.empty() ? Vector3{ 0,0,0 } : points_[0].position;

        float maxT = GetMaxT();
        // ループ対応：Tを 0.0 ～ maxT の範囲に収める
        if (isLoop_) {
            globalT = fmodf(globalT, maxT);
            if (globalT < 0) globalT += maxT;
        } else {
            if (globalT <= 0.0f) return points_.front().position;
            if (globalT >= maxT) return points_.back().position;
        }

        size_t index1 = static_cast<size_t>(globalT);
        if (!isLoop_ && index1 >= points_.size() - 1) {
            index1 = points_.size() - 2;
        } else if (isLoop_ && index1 >= points_.size()) {
            index1 = points_.size() - 1;
        }
        size_t index2 = (index1 + 1) % points_.size(); // ループして0に戻る

        float t = globalT - static_cast<float>(index1);

        const auto& p1 = points_[index1];
        const auto& p2 = points_[index2];

        switch (p1.type) {
        case InterpolationType::Linear:
            return Lerp(p1.position, p2.position, t);

        case InterpolationType::Bezier:
            // p1のOutハンドルと、p2のInハンドルで補間
            return Bezier(p1.position, p1.controlOut, p2.controlIn, p2.position, t);

        case InterpolationType::CatmullRom: {
            // ループを考慮した隣接4点の取得
            size_t i0 = (index1 > 0) ? index1 - 1 : (isLoop_ ? points_.size() - 1 : index1);
            size_t i3 = (index2 + 1) % points_.size();
            if (!isLoop_ && index2 == points_.size() - 1) i3 = index2; // 終端クランプ

            return CatmullRom(points_[i0].position, p1.position, p2.position, points_[i3].position, t);
        }
        }
        return p1.position;
    }
    // 次の点への方向ベクトルを取得（回転制御用）
    Vector3 GetDirection(float globalT) const;
    float GetMaxT() const {
        if (points_.empty()) return 0.0f;
        // ループなら点と同じ数（例: 4点あれば T=4.0 で一周）、非ループなら点数-1
        return isLoop_ ? static_cast<float>(points_.size()) : static_cast<float>(points_.size() - 1);
    }
    float GetTFromDistance(float distance) const;
    float GetTotalLength() const {
        return totalLength_;
    }
    void BuildDistanceTable();
    float GetDistanceFromT(float t) const;
    void DebugDraw();
    void SetLoop(bool loop) {
        if (isLoop_ != loop) {
            isLoop_ = loop;
            isDirty_ = true;
        }
    }
    bool IsLoop()const{return isLoop_;}
private:
    bool isLoop_ = false; // ループフラグ
    bool isDirty_ = true; // 距離テーブル再計算フラグ（初期化時または点変更時のみ再計算）
    // RailPath.h に追加
    struct DistanceMap {
        float distance; // 始点からの累積距離
        float t;        // その時の globalT
    };
    std::vector<DistanceMap> distanceTable_;
    float totalLength_ = 0.0f;
    std::vector<RailPoint> points_;
};
