// ConfigManager.cpp
#include "ConfigManager.h"
#include <fstream>
#include "tools.h" // 包含generate_pack_id()

ConfigManager& ConfigManager::Get() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    Load("plugins/HuHoBot/config.json");
}

void ConfigManager::Load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    path_ = path;

    try {
        std::ifstream fin(path);
        if (fin.good()) {
            data_ = json::parse(fin);
        }
    } catch (...) {
        // 文件不存在或解析失败
    }

    InitDefaults();
    Save();
}

void ConfigManager::Save() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream fout(path_);
    fout << data_.dump(4);
}

void ConfigManager::InitDefaults() {
    if (!data_.contains("version")) {
        data_["version"] = version_;
    }

    if (!data_.contains("serverId") || data_["serverId"].empty()) {
        data_["serverId"] = tools::generate_pack_id();
    }

    if (!data_.contains("hashKey") || data_["hashKey"].empty()) {
        data_["hashKey"] = "";
    }

    if (!data_.contains("chatFormatGroup")) {
        data_["chatFormatGroup"] = "群:<{nick}> {msg}";
    }

    if (!data_.contains("motdUrl")) {
        data_["motdUrl"] = "play.easecation.net:19132";
    }

    if (!data_.contains("serverName")) {
        data_["serverName"] = "AllayMC";
    }

    if (!data_.contains("customCommand")) {
        data_["customCommand"] = std::vector<CustomCommand>{
                {"加白名", "whitelist add &1", 0},
                {"管理加白名", "whitelist add &1", 1}
        };
    }
}

// Getter实现
int ConfigManager::GetVersion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["version"];
}

std::string ConfigManager::GetServerId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["serverId"];
}

std::string ConfigManager::GetHashKey() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["hashKey"];
}

std::string ConfigManager::GetChatFormatGroup() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["chatFormatGroup"];
}

std::string ConfigManager::GetMotdUrl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["motdUrl"];
}

std::string ConfigManager::GetServerName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["serverName"];
}

std::vector<CustomCommand> ConfigManager::GetCustomCommands() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_["customCommand"];
}

// Setter实现
void ConfigManager::SetServerId(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_["serverId"] = id;
}

void ConfigManager::SetHashKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_["hashKey"] = key;
}
