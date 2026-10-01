#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Vector3.h"
#include "Vector2.h"
#include "RailPath.h"

/// <summary>
/// レベルエディター（Blenderアドオン）からエクスポートされたJSONを
/// ゲームエンジン側で表現するためのデータ構造定義ヘッダ
/// </summary>

// ─── 座標変換マクロ ───────────────────────────────────────────────────
// Blender : 右手系 Z-up  (X, Y, Z)
// DirectX : 左手系 Y-up  (X, Z, Y) ※ Z/Yを入れ替え
// rotationはオイラー角（度）で出力されるため、ラジアン変換も行う
// ─────────────────────────────────────────────────────────────────────

// ─── オブジェクト種別 ─────────────────────────────────────────────────
enum class LevelObjectType {
    kProp,         ///< 装飾・背景モデル（当たり判定なし）
    kTerrain,      ///< 地形メッシュ（ポリゴン接地/壁判定を持つ）
    kBlock,        ///< 接地/壁判定付きプレハブブロック（StageBlock）
    kEnemy,        ///< 敵スポーン位置・設定
    kPlayerSpawn,  ///< プレイヤー初期位置（Emptyオブジェクト）
    kGoal,         ///< ゴール位置・オブジェクト
    kCoin,         ///< 収集アイテム（コイン）
    kTrigger,      ///< イベントトリガー（侵入検知）
    kPbdRope,      ///< PBDロープ物理ギミック
    kPbdCloth,     ///< PBD布物理ギミック
    kRailStage,    ///< ステージ進行用レール（ベジェ曲線）
    kRailCamera,   ///< カメラ用レール（ベジェ曲線）
    kCamera,       ///< カメラ設定オブジェクト
    kUnknown,      ///< 未知のタイプ（スキップ）
};

// ─── コライダー設定 ───────────────────────────────────────────────────
struct LevelColliderData {
    enum class Shape { Box, Sphere, None };
    Shape  shape  = Shape::None;
    Vector3 center = { 0.0f, 0.0f, 0.0f }; ///< ローカル中心オフセット
    Vector3 size   = { 1.0f, 1.0f, 1.0f }; ///< Box: 半辺長 3軸 / Sphere: x=半径
};

// ─── トランスフォーム ─────────────────────────────────────────────────
struct LevelTransform {
    Vector3 translation = { 0.0f, 0.0f, 0.0f }; ///< ワールド位置（DX座標系）
    Vector3 rotation    = { 0.0f, 0.0f, 0.0f }; ///< オイラー角（ラジアン, DX座標系）
    Vector3 scale       = { 1.0f, 1.0f, 1.0f }; ///< スケール
};

// ─── レールポイント ───────────────────────────────────────────────────
struct LevelRailPoint {
    Vector3 position;     ///< 制御点座標
    Vector3 handleIn;     ///< 入りハンドル（絶対座標）
    Vector3 handleOut;    ///< 出しハンドル（絶対座標）
    RailPath::InterpolationType type = RailPath::InterpolationType::Bezier; ///< 補間方式 (Linear / Bezier / CatmullRom)
};

// ─── PBD設定 ─────────────────────────────────────────────────────────
struct LevelPbdData {
    Vector3 startPos    = { 0.0f, 0.0f, 0.0f };  ///< 始点（DX座標系）
    Vector3 endPos      = { 0.0f, 0.0f, 0.0f };  ///< 終点（DX座標系）
    int     numPoints   = 10;                     ///< ロープ: 分割数
    int     widthPoints = 8;                      ///< 布: 横分割数
    int     heightPoints= 8;                      ///< 布: 縦分割数
    float   stiffness   = 0.2f;                   ///< バネ剛性 k
    float   damping     = 0.05f;                  ///< 減衰係数
    float   gravityY    = -9.8f;                  ///< 重力Y成分
    bool    fixStart    = true;                   ///< 始点を固定するか
    bool    fixEnd      = false;                  ///< 終点を固定するか
    std::string texturePath = "";                 ///< 布テクスチャ（Clothのみ）
};

// ─── トリガー設定 ─────────────────────────────────────────────────────
struct LevelTriggerData {
    enum class FireMode { Once, Continuous, EnterExit };
    FireMode    fireMode  = FireMode::Once;   ///< 発火モード
    std::string eventName = "";              ///< 発火するイベント名
    /// カスタムパラメータ（イベントに渡す任意の値）
    std::unordered_map<std::string, std::string> params;
};

// ─── 個別オブジェクトデータ ───────────────────────────────────────────
struct LevelObjectData {
    std::string     name       = "";            ///< オブジェクト名（Blender上の名前）
    LevelObjectType type       = LevelObjectType::kUnknown;
    LevelTransform  transform;
    std::string     modelName   = "";            ///< モデルファイル名（例: "box.obj"）
    std::string     modelDir    = "resources";  ///< モデルフォルダパス
    std::string     texturePath = "";            ///< テクスチャパス（例: "resources/grass.png"）
    bool            disabled    = false;         ///< 無効化フラグ（trueならスキップ）
    Vector2         railPos     = { 0.0f, 0.0f }; ///< レール配置時の2D座標 (t, offset)

    // コライダー（Prop以外でoptional）
    LevelColliderData collider;

    // トリガー専用（kTrigger のみ使用）
    LevelTriggerData trigger;

    // PBD専用（kPbdRope / kPbdCloth のみ使用）
    LevelPbdData pbd;

    // レール専用（kRailStage / kRailCamera のみ使用）
    std::vector<LevelRailPoint> railPoints;
    bool railLoop = false;

    // 敵専用
    std::string enemyType = "Normal";   ///< 敵種別文字列（"Normal","Bound","Test"等）

    // カスタムプロパティ（拡張用汎用マップ）
    std::unordered_map<std::string, std::string> properties;

    // 地形等の動的頂点位置データ（DX座標系）
    std::vector<Vector3> vertexPositions;

    // 地形等のポリゴン三角形インデックスデータ
    std::vector<uint32_t> indices;

    // 子オブジェクト（Blenderの親子関係）
    std::vector<LevelObjectData> children;
};

// ─── レベル全体データ ─────────────────────────────────────────────────
struct LevelData {
    std::string                 sceneName;  ///< シーン名
    std::vector<LevelObjectData> objects;   ///< トップレベルオブジェクト一覧
};
