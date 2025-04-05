#include "huhobot.h"
#include "ConfigManager.h"
#include "endstone/command/console_command_sender.h"


using endstone::Player;

const string HuHoBot::version = "0.0.1";
HuHoBot* HuHoBot::instance_ = nullptr;

HuHoBot::HuHoBot() {
    instance_ = this; // 在构造函数中保存实例
}

// 添加单例访问方法
HuHoBot& HuHoBot::getInstance() {
    return *instance_;
}
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
                sender.sendMessage("正在重新连接");
                client->reconnect();
            }
            else if(args.at(0) == "disconnect"){
                sender.sendMessage("已断开连接");
                client->shutdown(false);
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
    return false;
}

void HuHoBot::broadcast(const std::string &msg) {
    this->getServer().broadcastMessage(msg);
}

bool HuHoBot::runCommand(const std::string &cmd) {
    return this->getServer().dispatchCommand(
            this->getServer().getCommandSender(),
            cmd);
}

std::vector<Player *> HuHoBot::getOnlinePlayers() {
    return this->getServer().getOnlinePlayers();
}

std::shared_ptr<endstone::Task> HuHoBot::setReconnectTask() {
    return this->getServer().getScheduler().runTaskTimer(
            *this,
            [&]() {
                client->task_reconnect();
                }, 0, 5*20);
}

std::shared_ptr<endstone::Task> HuHoBot::setAutoDisConnectTask() {
    return this->getServer().getScheduler().runTaskLater(
            *this,
            [&]() {
                getLogger().info("连接超时，已自动重连");
                client->shutdown(true);
            }, 6*60*60*20);
}

std::shared_ptr<endstone::Task> HuHoBot::setHeartTask() {
    return this->getServer().getScheduler().runTaskTimer(
            *this,
            [&]() {
                client->sendHeart();
            }, 0, 5*20);
}


ENDSTONE_PLUGIN("huhobot", HuHoBot::getVersion(), HuHoBot)
{
    description = "HuHoBot Endstone Adapter";
    prefix = "HuHoBot";
    authors = {"HuoHuas001"};

    command("huhobot") //
            .description("HuHoBot's control command.")
            .usages("/huhobot (bind)<bindAction: bindAction> <bindCode: message>")
            .usages("/huhobot (reload|reconnect|disconnect|help)<action: botAction>")
            .permissions("huhobot.command.huhobot");

    permission("huhobot.command.huhobot")
            .description("Allow users to use the /huhobot command.")
            .default_(endstone::PermissionDefault::Operator);
}