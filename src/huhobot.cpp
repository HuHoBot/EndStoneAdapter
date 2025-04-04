#include "huhobot.h"
#include "ConfigManager.h"


void HuHoBot::onLoad(){
    getLogger().info("HuHoBot v"+version+" loaded.");
}

void HuHoBot::onEnable() {
    client = new BotClient(&getLogger());

    //检测是否已经生成hashKey
    ConfigManager& config = ConfigManager::Get();
    if(config.GetHashKey() == ""){
        string serverId = config.GetServerId();
        getLogger().warning("服务器尚未在机器人进行绑定，请在群内输入\"/绑定 " + serverId + "\"");
    }
}

ENDSTONE_PLUGIN("HuHoBot", "0.0.1", HuHoBot)
{
    description = "HuHoBot Endstone Adapter";
    prefix = "HuHoBot";
    authors = {"HuoHuas001"};
}