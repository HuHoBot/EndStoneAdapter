// ConfigManager.h
#pragma once
#include "nlohmann/json.hpp"
#include <string>
#include <vector>

using json = nlohmann::json;

struct CustomCommand {
    std::string key;
    std::string command;
    int permission;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CustomCommand, key, command, permission)
};


class ConfigManager {
public:
    static ConfigManager& Get();

    // 配置访问接口
    int GetVersion() const;
    std::string GetServerId() const;
    std::string GetHashKey() const;
    std::string GetChatFormatFromGame() const;
    std::string GetChatFormatFromGroup() const;
    bool GetPostChat() const;
    std::string GetPostPrefix() const;
    std::string GetMotdUrl() const;
    std::string GetServerName() const;
    std::vector<CustomCommand> GetCustomCommands() const;

    // 配置设置接口
    void SetServerId(const std::string& id);
    void SetHashKey(const std::string& key);

    void Load(const std::string& path);
    void Reload();
    void Save();

    ConfigManager();

private:
    void InitDefaults();

    json data_;
    std::string path_;
    int version_ = 2;
};


