#include "JsonManager.h"
#include <fstream>
#include <iostream>
#include "Logger.h"

const nlohmann::json JsonManager::emptyJson_ = nlohmann::json::object();

void JsonManager::Initialize()
{
    jsonCache_.clear();
}

void JsonManager::Finalize()
{
    jsonCache_.clear();
    Singleton<JsonManager>::Finalize();
}

bool JsonManager::Load(const std::string& key, const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("[JsonManager] Failed to open file: " + filePath + "\n");
        return false;
    }

    try {
        json data;
        file >> data;
        jsonCache_[key] = std::move(data);
        Logger::Log("[JsonManager] Loaded JSON successfully: [" + key + "] from " + filePath + "\n");
        return true;
    } catch (const json::parse_error& e) {
        Logger::Log("[JsonManager] Parse error in " + filePath + ": " + e.what() + "\n");
        return false;
    }
}

JsonManager::json JsonManager::LoadDirect(const std::string& filePath, bool* outSuccess)
{
    if (outSuccess) {
        *outSuccess = false;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("[JsonManager] Failed to open file: " + filePath + "\n");
        return emptyJson_;
    }

    try {
        json data;
        file >> data;
        if (outSuccess) {
            *outSuccess = true;
        }
        return data;
    } catch (const json::parse_error& e) {
        Logger::Log("[JsonManager] Parse error in " + filePath + ": " + e.what() + "\n");
        return emptyJson_;
    }
}

const JsonManager::json& JsonManager::Get(const std::string& key) const
{
    auto it = jsonCache_.find(key);
    if (it != jsonCache_.end()) {
        return it->second;
    }
    Logger::Log("[JsonManager] Key not found: " + key + "\n");
    return emptyJson_;
}

JsonManager::json& JsonManager::Get(const std::string& key)
{
    auto it = jsonCache_.find(key);
    if (it != jsonCache_.end()) {
        return it->second;
    }
    // 存在しない場合は新規作成して参照を返す
    Logger::Log("[JsonManager] Key not found. Created empty JSON for key: " + key + "\n");
    return jsonCache_[key];
}

bool JsonManager::Contains(const std::string& key) const
{
    return jsonCache_.find(key) != jsonCache_.end();
}

bool JsonManager::Save(const std::string& filePath, const json& data, int indent)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("[JsonManager] Failed to open file for writing: " + filePath + "\n");
        return false;
    }

    try {
        if (indent >= 0) {
            file << data.dump(indent);
        } else {
            file << data.dump();
        }
        Logger::Log("[JsonManager] Saved JSON successfully: " + filePath + "\n");
        return true;
    } catch (const std::exception& e) {
        Logger::Log("[JsonManager] Failed to save JSON: " + std::string(e.what()) + "\n");
        return false;
    }
}

bool JsonManager::SaveCached(const std::string& key, const std::string& filePath, int indent)
{
    auto it = jsonCache_.find(key);
    if (it == jsonCache_.end()) {
        Logger::Log("[JsonManager] Key not found for saving: " + key + "\n");
        return false;
    }

    return Save(filePath, it->second, indent);
}

void JsonManager::Clear(const std::string& key)
{
    jsonCache_.erase(key);
}

void JsonManager::ClearAll()
{
    jsonCache_.clear();
}
