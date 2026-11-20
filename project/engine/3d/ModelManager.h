#pragma once
#include <map>
#include <Model.h>
#include <memory>
#include "DXCommon.h"
class ModelManager
{
public:
    void Initialize();
    static ModelManager* GetInstance();
    void Finalize();
    //Modelロード
    void LoadModel(const std::string& directoryPath,const std::string& filePath);
    //Model検索
    std::shared_ptr<Model> findModel(const std::string& filePath);
    static std::unique_ptr<ModelManager> instance;
    friend struct std::default_delete<ModelManager>;
    void CreateSphereModel(const std::string& modelName, uint32_t subdivision = 16);
    /// <summary>
    /// 板ポリゴンをテクスチャから生成する
    /// </summary>
    /// <param name="modelName">モデルの名前</param>
    /// <param name="textureFilePath">テクスチャファイルのパス</param>
    void CreatePlaneFromTex(const std::string& modelName, const std::string& textureFilePath);
    void CreateBoxModel(const std::string& modelName);

    /// <summary>
    /// 地形グリッドモデルを頂点データから動的生成して登録する
    /// </summary>
    void CreateTerrainModel(
        const std::string& modelName,
        float sizeX, float sizeY,
        int divisionsX, int divisionsY,
        const std::string& textureFilePath,
        float uvTile = 1.0f);

    /// <summary>
    /// 動的生成したモデルを名前付きで登録・上書きする
    /// </summary>
    void RegisterModel(const std::string& modelName, std::shared_ptr<Model> model);


private:
  

    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(ModelManager&) = delete;
    ModelManager& operator=(ModelManager&) = delete;

    std::map<std::string, std::shared_ptr<Model>> models;
};

