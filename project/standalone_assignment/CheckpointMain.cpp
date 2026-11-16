/**
 * @file CheckpointMain.cpp
 * @brief STLコンテナ（std::map + std::stack）を使用したステージ内チェックポイント課題（テスト専用Main）
 *
 * ゲームエンジンやDirectX等に依存せず、標準C++コンパイラ（MSVC, g++, clang++）で
 * 単体ビルドおよび実行が可能です。
 *
 * 【題材4：ステージ内チェックポイント 課題要件と実装内容】
 * 1. 未到達地点の到達フラグ更新（isReached: false -> true）
 * 2. 最後に通過した有効チェックポイントの取得（std::stack による通過順保持・リスポーン地点の即時特定）
 * 3. 地点 ID を指定した座標情報の検索（std::map::find による O(log N) 検索とファストトラベル判定）
 */

#include <iostream>
#include <map>
#include <stack>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "../application/UI/CheckpointContainer.h"

/**
 * @brief 最後に通過した有効チェックポイント（リスポーン地点）を表示するヘルパー関数
 */
void PrintCurrentRespawnPoint(const CheckpointContainer& container) {
    CheckpointId lastId = CheckpointId::Entrance;
    const CheckpointData* lastCp = container.GetLastCheckpoint();
    if (lastCp != nullptr && container.GetLastCheckpointId(lastId)) {
        std::cout << "  [現在のリスポーン地点] ID: " << static_cast<int>(lastId)
                  << " | " << lastCp->name
                  << " | 座標: (" << lastCp->position.x << ", "
                  << lastCp->position.y << ", " << lastCp->position.z << ")\n\n";
    } else {
        std::cout << "  [現在のリスポーン地点] 有効なチェックポイントはまだありません（初期スポーン地点）\n\n";
    }
}

/**
 * @brief 地点 ID を指定して座標を検索し、ファストトラベル可否を確認するヘルパー関数
 */
void SearchAndCheckFastTravel(const CheckpointContainer& container, int rawId) {
    CheckpointId targetId = static_cast<CheckpointId>(rawId);
    CheckpointPos pos{};

    if (!container.FindCheckpointPosition(targetId, pos)) {
        std::cout << "  【ID検索】地点ID: " << rawId << " はステージ内に存在しません (find == end)\n";
        return;
    }

    const CheckpointData* data = container.GetCheckpointData(targetId);
    std::cout << "  【ID検索】地点ID: " << rawId << " (" << (data ? data->name : "") << ") の座標を発見 -> ("
              << pos.x << ", " << pos.y << ", " << pos.z << ")\n";

    if (container.IsReached(targetId)) {
        std::cout << "    -> [ファストトラベル成功] 到達済み地点のため、座標 ("
                  << pos.x << ", " << pos.y << ", " << pos.z << ") へ移動しました！\n";
    } else {
        std::cout << "    -> [ファストトラベル不可] まだ未到達のチェックポイントのため移動できません。\n";
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "=================================================================\n";
    std::cout << " STLコンテナ課題（題材4：ステージ内チェックポイント）テストデモ\n";
    std::cout << "=================================================================\n\n";

    CheckpointContainer checkpoints;

    // -----------------------------------------------------------------
    // [Step 1] ステージ内チェックポイントの初期登録
    // -----------------------------------------------------------------
    std::cout << "[Step 1] ステージ内のチェックポイント5箇所を初期登録（すべて未到達状態）\n";
    checkpoints.InitializeDefaultStageCheckpoints();
    checkpoints.PrintAllCheckpoints();

    // -----------------------------------------------------------------
    // [Step 2] チェックポイント未通過状態でのリスポーン地点確認
    // -----------------------------------------------------------------
    std::cout << "[Step 2] まだ1つも通過していない状態で、最後に通過したチェックポイントを取得\n";
    PrintCurrentRespawnPoint(checkpoints);

    // -----------------------------------------------------------------
    // [Step 3] 未到達地点の到達フラグ更新（順番通り ＆ 任意の順序での解放）
    // -----------------------------------------------------------------
    std::cout << "[Step 3] チェックポイントを通過し、未到達地点の到達フラグを更新\n";
    std::cout << "  -> 1. Entrance (ID: 100) を通過\n";
    if (checkpoints.ReachCheckpoint(CheckpointId::Entrance) == 1) {
        std::cout << "     [新規到達] Entrance (ID: 100) の到達フラグを true に更新しました！\n";
    }

    std::cout << "  -> 2. 脇道ルートを通り、Valley (ID: 200) を飛ばして Ruins (ID: 300) を通過\n";
    if (checkpoints.ReachCheckpoint(CheckpointId::Ruins) == 1) {
        std::cout << "     [新規到達] Ancient Ruins (ID: 300) の到達フラグを true に更新しました！\n";
    }
    std::cout << '\n';
    checkpoints.PrintAllCheckpoints();

    // -----------------------------------------------------------------
    // [Step 4] 最後に通過した有効チェックポイントの取得（死亡時リスポーン）
    // -----------------------------------------------------------------
    std::cout << "[Step 4] プレイヤー死亡時のリスポーン地点（最後に通過した有効チェックポイント）を取得\n";
    PrintCurrentRespawnPoint(checkpoints);

    std::cout << "[Step 4.1] 引き返して未到達だった Valley Bridge (ID: 200) を通過\n";
    if (checkpoints.ReachCheckpoint(CheckpointId::Valley) == 1) {
        std::cout << "  -> [新規到達] Valley Bridge (ID: 200) の到達フラグを true に更新しました！\n";
    }
    std::cout << "  -> ID順（300 > 200）ではなく、実際に最後（直近）に通過した ID: 200 がリスポーン地点になります：\n";
    PrintCurrentRespawnPoint(checkpoints);

    // -----------------------------------------------------------------
    // [Step 5] 地点 ID を指定した座標情報の検索 (std::map::find) とファストトラベル
    // -----------------------------------------------------------------
    std::cout << "[Step 5] 地点 ID を指定した座標情報の検索 (std::map::find) とファストトラベル検証\n";
    checkpoints.PrintFastTravelList();

    // 1. 到達済みの地点 ID: 100
    SearchAndCheckFastTravel(checkpoints, 100);
    // 2. 未到達の地点 ID: 400
    SearchAndCheckFastTravel(checkpoints, 400);
    // 3. 存在しない地点 ID: 999
    SearchAndCheckFastTravel(checkpoints, 999);
    std::cout << '\n';

    // -----------------------------------------------------------------
    // [Step 6] 通過履歴の巻き戻し確認 (std::stack::pop)
    // -----------------------------------------------------------------
    std::cout << "[Step 6] std::stack による通過履歴の巻き戻し（1つ前のチェックポイントへ戻す）\n";
    if (checkpoints.UndoLastCheckpoint()) {
        std::cout << "  -> 直近の通過履歴 (ID: 200) を pop しました。1つ前に通過した地点へ戻ります：\n";
        PrintCurrentRespawnPoint(checkpoints);
    }

    std::cout << "=================================================================\n";
    std::cout << " ステージ内チェックポイント課題の全テストが正常に完了しました。\n";
    std::cout << "=================================================================\n";

    return 0;
}
