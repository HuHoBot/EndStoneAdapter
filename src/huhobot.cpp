#include "huhobot.h"
#include "ConfigManager.h"

using endstone::Player;

const string HuHoBot::version = "0.0.1";
string HuHoBot::getVersion(){
    return version;
}

void HuHoBot::onLoad(){
    getLogger().info("HuHoBot v"+version+" loaded.");
}

void HuHoBot::onEnable() {
    client = new BotClient(&getLogger());

    //检测是否已经生成hashKey
    ConfigManager config = ConfigManager();
    if(config.GetHashKey() == ""){
        string serverId = config.GetServerId();
        getLogger().warning("服务器尚未在机器人进行绑定，请在群内输入\"/绑定 " + serverId + "\"");
    }

    client->connect();
}

bool HuHoBot::onCommand(endstone::CommandSender &sender, const endstone::Command &command,
               const std::vector<std::string> &args)
{
    if (command.getName() == "huhobot") {
        if(args.size() == 1){
            if(args.at(0) == "reload"){
                ConfigManager config = ConfigManager::Get();
                sender.sendMessage("重载配置文件成功");
            }

            else if(args.at(0) == "reconnect"){
                //Todo: 重新连接
            }
            else if(args.at(0) == "disconnect"){
                //Todo: 断开连接
                sender.sendMessage("已断开连接");
            }

            else if(args.at(0) == "help"){
                sender.sendMessage("HuHoBot 帮助列表:");
                sender.sendMessage("- /huhobot reload: 重载配置文件");
                sender.sendMessage("- /huhobot reconnect: 重新连接");
                sender.sendMessage("- /huhobot disconnect: 断开服务器连接");
                sender.sendMessage("- /huhobot bind <bindCode:str>: 绑定服务器");
                sender.sendMessage("- /huhobot help: 显示帮助列表");
            }

        }

        else if(args.size() == 2){
            if(args.at(0) == "bind"){
                client->bindConfirm(args.at(1));
            }
            else{
                sender.sendErrorMessage("参数有误. 请使用/huhobot help查看帮助列表");
            }
        }

        return true;
    }

}

void HuHoBot::broadcast(const std::string &msg) {
    this->getServer().broadcastMessage(msg);
}

void HuHoBot::runCommand(const std::string &cmd) {
    this->getServer().dispatchCommand(
            this->getServer().getCommandSender(),
            cmd);
}

std::vector<Player *> HuHoBot::getOnlinePlayers() {
    return this->getServer().getOnlinePlayers();
}

ENDSTONE_PLUGIN("huhobot", HuHoBot::getVersion(), HuHoBot)
{
    description = "HuHoBot Endstone Adapter";
    prefix = "HuHoBot";
    authors = {"HuoHuas001"};

    command("huhobot") //
            .description("HuHoBot's control command.")
            .usages("/huhobot (bind)<bindAction: bindAction> <bindCode: str>")
            .usages("/huhobot (reload|reconnect|disconnect|help)<action: botAction>")
            .permissions("huhobot.command.huhobot");

    permission("huhobot.command.huhobot")
            .description("Allow users to use the /huhobot command.")
            .default_(endstone::PermissionDefault::Operator);
}