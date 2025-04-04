#pragma once
#include <endstone/plugin/plugin.h>
#include "HuhobotClient.h"
#include "ConfigManager.h"


using huhobot::BotClient;
using std::string;

class HuHoBot : public endstone::Plugin {
private:
    string version = "0.0.1";
    BotClient* client;
public:
    void onLoad() override;
    void onEnable() override;
};

