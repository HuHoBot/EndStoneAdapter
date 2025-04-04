#pragma once
#include <endstone/plugin/plugin.h>
#include "HuhobotClient.h"
#include "ConfigManager.h"

using std::string;
using endstone::Player;


class HuHoBot : public endstone::Plugin {
private:
    static const string version;
    BotClient* client;
public:
    static HuHoBot& Get();
    static string getVersion();
    void broadcast(const string& msg);
    void runCommand(const string& cmd);
    std::vector<Player *> HuHoBot::getOnlinePlayers();
    void onLoad() override;
    void onEnable() override;
    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command,
                   const std::vector<std::string> &args) override;
};

