#pragma once
#include "RailPath.h"
#include "Vector3.h"
#include <algorithm>
class RailMover {
public:
    enum class MoveDirection {
        Forward = 1, // 終点に向かっている (+)
        Backward = -1 // 始点に向かっている (-)
    };

    bool isRailSet() const {
        return path_ != nullptr;
    }
    void SetPath(const RailPath* path) {
        path_ = path;
    }

    // 外部から進捗のポインタを渡せるようにする
    void BindProgress(float* externalProgress) {
        pProgress_ = externalProgress;
    }

    //
    void Advance(float speed);

    float GetProgress() const {
        return pProgress_ ? *pProgress_ : localProgress_;
    }

    // 現在の座標を取得
    Vector3 GetCurrentPosition() const {
        if (!path_) return { 0,0,0 };
        return path_->GetPosition(GetProgress()); // localProgress_ ではなく GetProgress() を使う
    }

    // 現在の進行方向（向き）を取得
    Vector3 GetCurrentDirection() const {
        return path_ ? path_->GetDirection(GetProgress()) : Vector3{ 0,0,1 };
    }

    void SetProgress(float t) {
        if (pProgress_) *pProgress_ = t;
        else localProgress_ = t;
    }

    const RailPath* GetRailPath() const {

        return path_;
    }

    // 他の RailMover と進行度（割合）を同期させる関数
    void SyncWith(const RailMover* otherMover);

    // 現在の走行距離（メートル）を取得
    float GetCurrentDistance() const;
    MoveDirection GetMoveDirection() const {
        return currentDir_;
    }
    void SetMoveDirection(MoveDirection dir) {
        currentDir_ = dir;
    }
private:


    // メンバ変数に追加
    MoveDirection currentDir_ = MoveDirection::Forward;

    const RailPath* path_ = nullptr;// 進捗の管理方法を柔軟にするため、ローカルと共有の両方を用意
    float localProgress_ = 0.0f;// 個別の進捗。これが共通でないなら、こっちを使う
    float* pProgress_ = nullptr; // これが共通なら、全員同期する
};
