#include "ModelManager.h"
#include "TextureManager.h"

std::unique_ptr<ModelManager> ModelManager::instance = nullptr;
void ModelManager::Initialize() {
 

}
ModelManager* ModelManager::GetInstance() {
       if (instance == nullptr) {
        // privateコンストラクタを呼び出せるヘルパー構造体
        struct Helper : public ModelManager {
            Helper() : ModelManager() {}
        };
        instance = std::make_unique<Helper>();
    }
    return instance.get();

};
void ModelManager::Finalize() {

    models.clear(); 
    instance.reset();
}

void ModelManager::LoadModel(const std::string& directoryPath,const std::string& filePath)
{
    //読み込み済か確認
    if (models.contains(filePath))return;
    //読み込み.初期化
   std::shared_ptr<Model> model = std::make_shared<Model>();
    model->Initialize(directoryPath, filePath);
    //格納
    models.insert(std::make_pair(filePath, std::move(model)));


}

std::shared_ptr<Model> ModelManager::findModel(const std::string& filePath)
{
    if (models.contains(filePath)) {
        return models.at(filePath);
    }


    LoadModel( "resources",filePath);
    if (models.contains(filePath)) {
        return models.at(filePath);
    }
    return nullptr;
}

void ModelManager::CreateSphereModel(const std::string& modelName, uint32_t subdivision)
{
    // 既に同じ名前で登録されていたら何もしない（あるいは上書き）
    if (models.contains(modelName)) {
        return;
    }

    // 1. Modelクラスの便利関数を使って球体を生成（unique_ptrからshared_ptrへ所有権移行）
    std::shared_ptr<Model> model = Model::CreateSphere(subdivision);

    // 2. マップに登録
    // これで "Sphere" などの名前で検索できるようになります
    models.insert(std::make_pair(modelName, std::move(model)));
}

void ModelManager::CreatePlaneFromTex(const std::string& modelName, const std::string& textureFilePath)
{
    if (models.contains(modelName)) return;

    // 何かしらパスが正しいかどうかを確認する処理が必要

    std::shared_ptr<Model> model = Model::CreatePlaneFromTex(textureFilePath);

    model->SetName(modelName);

    models.insert(std::make_pair(modelName, std::move(model)));

    
}

void ModelManager::CreateBoxModel(const std::string& modelName)
{
    if (models.contains(modelName)) return;

    std::shared_ptr<Model> model = Model::CreateBox();
    model->SetName(modelName);

    models.insert(std::make_pair(modelName, std::move(model)));
}

void ModelManager::CreateTerrainModel(
    const std::string& modelName,
    float sizeX, float sizeY,
    int divisionsX, int divisionsY,
    const std::string& textureFilePath,
    float uvTile)
{
    if (models.contains(modelName)) return;

    std::shared_ptr<Model> model = Model::CreateTerrainGrid(
        sizeX, sizeY, divisionsX, divisionsY, textureFilePath, uvTile);
    model->SetName(modelName);

    models.insert(std::make_pair(modelName, std::move(model)));
}
