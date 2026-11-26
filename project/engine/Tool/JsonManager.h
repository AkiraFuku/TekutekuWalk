#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "externals/Json/json.hpp"
#include "Singleton.h"

/// <summary>
/// JSONファイルの読み込み・保存・キャッシュ管理を行うシングルトンクラス
/// </summary>
class JsonManager : public Singleton<JsonManager>
{
public:
    using json = nlohmann::json;

    friend class Singleton<JsonManager>;
    friend struct std::default_delete<JsonManager>;

    void Initialize();
    void Finalize();

    /// <summary>
    /// JSONファイルを読み込んでキー名でキャッシュする
    /// </summary>
    /// <param name="key">管理用の一意識別名（例: "PlayerParam", "Stage1" など）</param>
    /// <param name="filePath">読み込むJSONファイルのパス（例: "resources/data/stage1.json"）</param>
    /// <returns>読み込み・パース成功ならtrue</returns>
    bool Load(const std::string& key, const std::string& filePath);

    /// <summary>
    /// キャッシュせずにファイルから直接読み込んでjsonオブジェクトを取得する
    /// </summary>
    /// <param name="filePath">読み込むJSONファイルのパス</param>
    /// <param name="outSuccess">成功可否の出力フラグ（省略可）</param>
    /// <returns>読み込んだjsonオブジェクト（失敗時は空のjson）</returns>
    json LoadDirect(const std::string& filePath, bool* outSuccess = nullptr);

    /// <summary>
    /// キャッシュされたJSONデータを取得（読み取り専用）
    /// </summary>
    /// <param name="key">管理キー</param>
    /// <returns>JSON参照（存在しない場合は空のjson参照を返す）</returns>
    const json& Get(const std::string& key) const;

    /// <summary>
    /// キャッシュされたJSONデータを取得（編集可能）
    /// </summary>
    /// <param name="key">管理キー</param>
    /// <returns>JSON参照</returns>
    json& Get(const std::string& key);

    /// <summary>
    /// 指定のキーのJSONが読み込み済みか確認
    /// </summary>
    bool Contains(const std::string& key) const;

    /// <summary>
    /// JSONデータをファイルへ保存（書き出し）する
    /// </summary>
    /// <param name="filePath">保存先ファイルパス</param>
    /// <param name="data">保存するJSONデータ</param>
    /// <param name="indent">インデント幅（デフォルト4、-1で改行なし圧縮）</param>
    /// <returns>保存成功ならtrue</returns>
    bool Save(const std::string& filePath, const json& data, int indent = 4);

    /// <summary>
    /// キャッシュされている指定キーのJSONデータをファイルへ保存する
    /// </summary>
    bool SaveCached(const std::string& key, const std::string& filePath, int indent = 4);

    /// <summary>
    /// 特定のキーのキャッシュを破棄
    /// </summary>
    void Clear(const std::string& key);

    /// <summary>
    /// すべてのキャッシュを破棄
    /// </summary>
    void ClearAll();

protected:
    JsonManager() = default;
    ~JsonManager() override = default;

private:
    // キャッシュされたJSONデータの連想配列
    std::unordered_map<std::string, json> jsonCache_;

    // 存在しないキーが参照された際の空JSON返却用ダミー
    static const json emptyJson_;
};
