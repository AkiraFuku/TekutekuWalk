#pragma once
#include <iostream>
#include <map>
#include <stack>
#include <string>

///選択した題材：題材４:ステージ内チェックポイント
///使用したコンテナ:std::map, std::stack
///そのコンテナを選択した理由：飛び飛びのIDでも省メモリな std::map（二分探索木）により5箇所中最大3回の比較（𝑂(log⁡𝑁))O(logN)）で任意IDの座標検索やフラグ更新を行い、出入口を末尾（LIFO）に絞った std::stack に軽量なIDのみを積むことで、最終通過地点の取得を常に1回の参照（𝑂(1)）で済ませるため。  
// チェックポイントIDの定義（離れたID値でも std::map なら効率的に管理可能）
enum class CheckpointId {
    Entrance    = 100, // ステージ入口
    Valley      = 200, // 渓谷の吊り橋
    Ruins       = 300, // 古代遺跡前
    MidBossRoom = 400, // 中ボス部屋前
    Summit      = 500, // 山頂（ボス前）
};

// 3次元座標構造体（単体テスト・ゲーム双方で扱えるシンプルな構造体）
struct CheckpointPos {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// チェックポイント情報構造体
struct CheckpointData {
    std::string name;           // 地点名
    CheckpointPos position;     // 座標情報 (x, y, z)
    bool isReached = false;     // 到達フラグ（false: 未到達, true: 到達済み）
};

// チェックポイント名取得ヘルパー（デフォルト名）
inline std::string GetDefaultCheckpointName(CheckpointId id) {
    switch (id) {
    case CheckpointId::Entrance:    return "Entrance (Stage Start)";
    case CheckpointId::Valley:      return "Valley Bridge";
    case CheckpointId::Ruins:       return "Ancient Ruins";
    case CheckpointId::MidBossRoom: return "Mid-Boss Gate";
    case CheckpointId::Summit:      return "Summit Altar";
    default:                        return "Unknown Checkpoint";
    }
}

/**
 * @brief ステージ内チェックポイント管理クラス
 * - std::map<CheckpointId, CheckpointData> : 地点IDからの高速な座標検索・到達フラグ管理 O(log N)
 * - std::stack<CheckpointId>               : 通過順序の保持・最後に通過した有効チェックポイントの即時取得 O(1)
 */
class CheckpointContainer {
public:
    // ステージ内のチェックポイントを初期登録（未到達状態 isReached = false）
    void RegisterCheckpoint(CheckpointId id, const std::string& name, const CheckpointPos& pos) {
        CheckpointData data;
        data.name = name;
        data.position = pos;
        data.isReached = false;
        checkpoints_[id] = data;
    }

    // デフォルトのステージチェックポイント一式を初期登録
    void InitializeDefaultStageCheckpoints() {
        checkpoints_.clear();
        while (!passedHistory_.empty()) {
            passedHistory_.pop();
        }
        RegisterCheckpoint(CheckpointId::Entrance,    "Entrance (Stage Start)", {   0.0f,  0.0f,   0.0f });
        RegisterCheckpoint(CheckpointId::Valley,      "Valley Bridge",          {  45.0f, 12.0f,  80.0f });
        RegisterCheckpoint(CheckpointId::Ruins,       "Ancient Ruins",          { 120.0f, 25.0f, 150.0f });
        RegisterCheckpoint(CheckpointId::MidBossRoom, "Mid-Boss Gate",          { 210.0f, 30.0f, 240.0f });
        RegisterCheckpoint(CheckpointId::Summit,      "Summit Altar",           { 350.0f, 85.0f, 400.0f });
    }

    /**
     * @brief 【要件1】未到達地点の到達フラグ更新 ＆ 最終通過地点の記録
     * @return 0 = 存在しない地点ID, 1 = 未到達地点に新規到達（フラグ更新）, 2 = 到達済み地点を再通過（最終通過地点を更新）
     */
    int ReachCheckpoint(CheckpointId id) {
        auto it = checkpoints_.find(id);
        if (it == checkpoints_.end()) {
            return 0; // 未登録の地点ID
        }

        bool wasUnreached = !it->second.isReached;
        it->second.isReached = true;

        // 直前に通過した地点と同じでなければ通過履歴スタックに積む
        if (passedHistory_.empty() || passedHistory_.top() != id) {
            passedHistory_.push(id);
        }

        return wasUnreached ? 1 : 2;
    }

    /**
     * @brief 【要件2】最後に通過した有効チェックポイントのIDを取得 O(1)
     */
    bool GetLastCheckpointId(CheckpointId& outId) const {
        if (passedHistory_.empty()) {
            return false;
        }
        outId = passedHistory_.top();
        return true;
    }

    /**
     * @brief 【要件2】最後に通過した有効チェックポイントのデータを取得
     * @return 有効なチェックポイントがあればそのポインタ、未通過なら nullptr
     */
    const CheckpointData* GetLastCheckpoint() const {
        if (passedHistory_.empty()) {
            return nullptr;
        }
        CheckpointId lastId = passedHistory_.top();
        auto it = checkpoints_.find(lastId);
        if (it != checkpoints_.end() && it->second.isReached) {
            return &(it->second);
        }
        return nullptr;
    }

    /**
     * @brief 【要件3】地点 ID を指定した座標情報の検索 (std::map::find)
     * @return 地点が存在すれば true し outPos に座標を格納、存在しなければ false
     */
    bool FindCheckpointPosition(CheckpointId id, CheckpointPos& outPos) const {
        auto it = checkpoints_.find(id);
        if (it == checkpoints_.end()) {
            return false;
        }
        outPos = it->second.position;
        return true;
    }

    // 地点 ID を指定してチェックポイント情報全体を取得（存在しなければ nullptr）
    const CheckpointData* GetCheckpointData(CheckpointId id) const {
        auto it = checkpoints_.find(id);
        if (it != checkpoints_.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    // 指定した地点に到達済み（ファストトラベル可能）か調べる
    bool IsReached(CheckpointId id) const {
        auto it = checkpoints_.find(id);
        if (it != checkpoints_.end()) {
            return it->second.isReached;
        }
        return false;
    }

    // 通過履歴スタック（std::stack）を使って直前のチェックポイントへ1つ巻き戻す
    bool UndoLastCheckpoint() {
        if (passedHistory_.empty()) {
            return false;
        }
        passedHistory_.pop();
        return true;
    }

    // 全チェックポイントを未到達状態にリセット
    void ResetAll() {
        for (auto& pair : checkpoints_) {
            pair.second.isReached = false;
        }
        while (!passedHistory_.empty()) {
            passedHistory_.pop();
        }
    }

    // 通過履歴の件数を取得
    size_t GetHistorySize() const {
        return passedHistory_.size();
    }

    // 全チェックポイントのマップを取得
    const std::map<CheckpointId, CheckpointData>& GetAll() const {
        return checkpoints_;
    }

    // コンソールへの全チェックポイント一覧出力
    void PrintAllCheckpoints() const {
        std::cout << "===== ステージ内チェックポイント一覧 (std::map) =====\n";
        for (const auto& entry : checkpoints_) {
            const int idNum = static_cast<int>(entry.first);
            const CheckpointData& data = entry.second;
            std::cout << "  [ID: " << idNum << "] " << data.name
                      << " | 座標: (" << data.position.x << ", " << data.position.y << ", " << data.position.z << ")"
                      << " | 状態: " << (data.isReached ? "[到達済み / 有効]" : "[未到達]") << "\n";
        }
        std::cout << "=====================================================\n\n";
    }

    // コンソールへのファストトラベル可能（到達済み）リスト出力
    void PrintFastTravelList() const {
        std::cout << "--- ファストトラベル可能地点一覧 (isReached == true) ---\n";
        bool anyReached = false;
        for (const auto& entry : checkpoints_) {
            if (entry.second.isReached) {
                anyReached = true;
                std::cout << "  * [ID: " << static_cast<int>(entry.first) << "] "
                          << entry.second.name << " -> 移動先座標: ("
                          << entry.second.position.x << ", "
                          << entry.second.position.y << ", "
                          << entry.second.position.z << ")\n";
            }
        }
        if (!anyReached) {
            std::cout << "  (解放されているファストトラベル地点はありません)\n";
        }
        std::cout << "--------------------------------------------------------\n\n";
    }

private:
    std::map<CheckpointId, CheckpointData> checkpoints_;
    std::stack<CheckpointId> passedHistory_;
};
