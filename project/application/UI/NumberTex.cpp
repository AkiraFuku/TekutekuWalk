#include "NumberTex.h"
#include "TextureManager.h"
std::vector<std::string>& numberTextureRegistration() {
    static std::vector<std::string> filePaths;
    if (filePaths.empty()) {
        for (int i = 0; i < 10; i++)
        {
            std::string filePath = "resources/number/" + std::to_string(i) + ".png";
            filePaths.push_back(filePath);

            TextureManager::GetInstance()->LoadTexture(filePath);
        }
    }

    return filePaths;
}