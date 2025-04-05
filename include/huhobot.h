#pragma once
#include <endstone/plugin/plugin.h>
#include "HuhobotClient.h"
#include "ConfigManager.h"
#include "endstone/scheduler/task.h"
#include "endstone/scheduler/scheduler.h"

using std::string;
using endstone::Player;


class HuHoBot : public endstone::Plugin {
private:
    static const string version;
    BotClient* client;
    static HuHoBot* instance_;
public:
    HuHoBot();
    static string getVersion();
    void broadcast(const string& msg);
    bool runCommand(const string& cmd);
    std::vector<Player *> getOnlinePlayers();
    void onLoad() override;
    void onEnable() override;
    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command,
                   const std::vector<std::string> &args) override;
    std::shared_ptr<endstone::Task> setReconnectTask();
    std::shared_ptr<endstone::Task> setAutoDisConnectTask();
    std::shared_ptr<endstone::Task> setHeartTask();
    static HuHoBot& getInstance();
};

