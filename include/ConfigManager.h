// ConfigManager.h
#pragma once
#include "nlohmann/json.hpp"
#include <string>
#include <vector>
#include <mutex>

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

using json = nlohmann::json;

struct DLL_EXPORT CustomCommand {
    std::string key;
    std::string command;
    int permission;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CustomCommand, key, command, permission)
};

class DLL_EXPORT ConfigManager {
public:
    static ConfigManager& Get();

    // 配置访问接口
    int GetVersion() const;
    std::string GetServerId() const;
    std::string GetHashKey() const;
    std::string GetChatFormatGroup() const;
    std::string GetMotdUrl() const;
    std::string GetServerName() const;
    std::vector<CustomCommand> GetCustomCommands() const;

    // 配置设置接口
    void SetServerId(const std::string& id);
    void SetHashKey(const std::string& key);

    void Load(const std::string& path);
    void Save();

private:
    ConfigManager();
    void InitDefaults();

    json data_;
    std::string path_;
    int version_ = 1;
    mutable std::mutex mutex_;
};
