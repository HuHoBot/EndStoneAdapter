#pragma once
#include "ConfigManager.h"
#include "HuhobotClient.h"
#include "endstone/event/player/player_chat_event.h"
#include "endstone/scheduler/scheduler.h"
#include "endstone/scheduler/task.h"
#include <endstone/plugin/plugin.h>
#include <regex>
#include <string>


#define HUHOBOT_VERSION "0.0.7"

using endstone::Player;
using std::string;


class HuHoBot : public endstone::Plugin {
private:
    static const string        version;
    std::shared_ptr<BotClient> client;
    static HuHoBot*            instance_;
    std::string                getMessageContent(const endstone::Message& msg);

public:
    HuHoBot();
    static string           getVersion();
    void                    broadcast(const string& msg);
    std::pair<string, bool> runCommand(const string& cmd);
    std::vector<Player*>    getOnlinePlayers();
    void                    onLoad() override;
    void                    onEnable() override;
    void                    onDisable() override;
    void                    onPlayerChat(endstone::PlayerChatEvent& event);
    bool
    onCommand(endstone::CommandSender& sender, const endstone::Command& command, const std::vector<std::string>& args)
        override;
    std::shared_ptr<endstone::Task> setReconnectTask();
    std::shared_ptr<endstone::Task> setAutoDisConnectTask();
    std::shared_ptr<endstone::Task> setHeartTask();
    void                            broadcastMsg(const string& msg);
    static HuHoBot&                 getInstance();
    BotClient&                      getBotClient();
};
