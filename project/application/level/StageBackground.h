#pragma once
#include "RailPath.h"
#include "Object3d.h"
#include "Camera.h"
#include "Vector3.h"
#include "Vector4.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/// <summary>
/// 星のカービィ64風の背景ポリゴンメッシュを
/// StageRail からランタイム自動生成・描画するクラス。
/// </summary>
class StageBackground
{
public:
    /// <summary>背景メッシュの表現スタイル</summary>
    enum class Style {
        MountainBand,     ///< レール奥側に沿った起伏のある丘・帯状ポリゴン
        PanoramaWall,     ///< レール奥側に沿ったカーブ状パノラマ背景壁
        PropLine,         ///< レール奥側に沿って簡素な立体が連続するメッシュ
        HorizonMountain,  ///< ステージ外周（カメラXZ追従）の滑らかな遠景山並みリング
        HorizonSky,       ///< ステージ外周（カメラXZ追従）の青空パノラマ帯
        HorizonGround,    ///< ステージ外周（カメラXZ追従）の遠景地面リング（隙間防止）
    };

    /// <summary>1レイヤー分の背景生成パラメータ</summary>
    struct LayerConfig {
        Style       style        = Style::HorizonMountain;
        float       distance     = 50.0f;  ///< レール奥側オフセット距離 または リング半径
        float       bottomY      = -10.0f; ///< 下端の基準Y座標
        float       baseHeight   = 15.0f;  ///< 基本の高さ
        float       amplitude    = 3.0f;   ///< 起伏の振幅
        float       frequency    = 0.14f;  ///< 起伏の空間周波数
        float       phase        = 0.0f;   ///< 起伏の位相オフセット
        float       skirtDepth   = 4.0f;   ///< MountainBand下端の手前側張り出し量（斜面を作る）
        float       uvScaleU     = 20.0f;  ///< UV繰り返しスケール
        float       propInterval = 6.0f;   ///< PropLineの配置間隔（m）
        float       propWidth    = 2.8f;   ///< PropLineの標準幅
        float       propDepth    = 2.6f;   ///< PropLineの標準奥行き
        uint32_t    seed         = 42;     ///< PropLine用乱数シード
        std::string texturePath  = "resources/white.png";
        Vector4     color        = { 0.40f, 0.72f, 0.42f, 1.0f };
    };

    /// <summary>背景全体の構成設定</summary>
    struct Config {
        bool  enabled      = true;
        bool  flipSide     = false;  ///< カメラ側反転に合わせて奥側も反転するか
        float stepDistance = 2.0f;   ///< レールサンプリング間隔（m）
        float marginStart  = 14.0f;  ///< 始点側の延長距離（m）
        float marginEnd    = 14.0f;  ///< 終端側の延長距離（m）
        std::vector<LayerConfig> layers;
    };

    StageBackground() = default;
    ~StageBackground() = default;

    /// <summary>デフォルトのカービィ64風4レイヤー設定を返す</summary>
    static Config CreateDefaultConfig();

    /// <summary>JSONのpropertiesマップからConfigをパースする（未指定項目はデフォルト値）</summary>
    static Config ParseConfigFromProperties(const std::unordered_map<std::string, std::string>& props);

    /// <summary>レールと設定から背景メッシュを生成して初期化する</summary>
    void Initialize(const RailPath* rail, Camera* camera, const Config& config);

    /// <summary>毎フレーム更新</summary>
    void Update();

    /// <summary>全レイヤー描画（遠景のため距離カリングなし）</summary>
    void Draw();

    void SetCamera(Camera* camera);
    bool IsEnabled() const { return config_.enabled && !layerObjects_.empty(); }

private:
    struct RailSample {
        Vector3 pos;
        Vector3 fwd;
        Vector3 farDir;   ///< カメラから見て奥側の単位水平ベクトル
        float   dist = 0.0f;
    };

    /// <summary>レールを等間隔サンプリングし、平滑化した奥側ベクトルを算出する</summary>
    static std::vector<RailSample> SampleRail(
        const RailPath* rail,
        float stepDistance,
        float marginStart,
        float marginEnd,
        bool flipSide);

    static void BuildMountainBand(
        const std::vector<RailSample>& samples,
        const LayerConfig& layer,
        bool isLoop,
        std::vector<Model::VertexData>& outVertices,
        std::vector<uint32_t>& outIndices);

    static void BuildPanoramaWall(
        const std::vector<RailSample>& samples,
        const LayerConfig& layer,
        bool isLoop,
        std::vector<Model::VertexData>& outVertices,
        std::vector<uint32_t>& outIndices);

    static void BuildPropLine(
        const std::vector<RailSample>& samples,
        const LayerConfig& layer,
        std::vector<Model::VertexData>& outVertices,
        std::vector<uint32_t>& outIndices);

    static void BuildHorizonMountain(
        const LayerConfig& layer,
        std::vector<Model::VertexData>& outVertices,
        std::vector<uint32_t>& outIndices);

    static void BuildHorizonSky(
        const LayerConfig& layer,
        std::vector<Model::VertexData>& outVertices,
        std::vector<uint32_t>& outIndices);

    static void BuildHorizonGround(
        const LayerConfig& layer,
        std::vector<Model::VertexData>& outVertices,
        std::vector<uint32_t>& outIndices);

private:
    Config config_;
    Camera* camera_ = nullptr;
    std::vector<std::unique_ptr<Object3d>> layerObjects_;
    std::vector<bool> followCameraXZ_;
};
