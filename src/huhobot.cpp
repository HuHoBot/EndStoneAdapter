#include "huhobot.h"
#include "ConfigManager.h"
#include "endstone/command/console_command_sender.h"
#include "endstone/command/command_sender_wrapper.h"
#include "endstone/message.h"
#include "endstone/lang/language.h"


using endstone::Player;
using endstone::CommandSenderWrapper;

const string HuHoBot::version = HUHOBOT_VERSION;
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

    //注册事件
    registerEvent(&HuHoBot::onPlayerChat, *this);

    //检测是否已经生成hashKey
    ConfigManager config = ConfigManager();
    if(config.GetHashKey() == ""){
        string serverId = config.GetServerId();
        getLogger().warning("服务器尚未在机器人进行绑定，请在群内输入\"/绑定 " + serverId + "\"");
    }

    client->connect();
}

void HuHoBot::onPlayerChat(endstone::PlayerChatEvent &event){
    string msg = event.getMessage();
    string playerName = event.getPlayer().getName();
    ConfigManager config = ConfigManager::Get();

    //getLogger().info("玩家 " + playerName + " 在游戏中发送了消息: " + msg);

    string format = config.GetChatFormatFromGame();
    string prefix = config.GetPostPrefix();
    bool isPostChat = config.GetPostChat();


    if(msg.rfind(prefix, 0) == 0 && isPostChat){
        std::string formatted = format;
        size_t msg_start_pos = prefix.length();

        formatted = std::regex_replace(formatted, std::regex("\\{name\\}"), playerName);
        formatted = std::regex_replace(formatted, std::regex("\\{msg\\}"), msg.substr(msg_start_pos));

        client->sendChat(formatted);
    }
}

bool HuHoBot::onCommand(endstone::CommandSender &sender, const endstone::Command &command,
               const std::vector<std::string> &args)
{
    if (command.getName() == "huhobot") {
        if(args.size() == 1){
            if(args.at(0) == "reload"){
                try {
                    ConfigManager::Get().Reload();
                    sender.sendMessage("配置重载成功");
                } catch (const std::exception& e) {
                    sender.sendErrorMessage("配置重载失败: " + std::string(e.what()));
                    getLogger().error("配置重载失败: {}", e.what());
                }
            }

            else if(args.at(0) == "reconnect"){
                sender.sendMessage("正在重新连接");
                client->reconnect();
            }
            else if(args.at(0) == "disconnect"){
                sender.sendMessage("已断开连接");
                //client->shutdown(false);
                client->shutdown(true);
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

std::string HuHoBot::getMessageContent(const endstone::Message &msg)
{
    if (auto *str = std::get_if<std::string>(&msg)) {
        return *str;
    }
    else if (auto *translatable = std::get_if<endstone::Translatable>(&msg)) {
        return getServer().getLanguage().translate(*translatable);
    }
    return "";
}

std::pair<string, bool> HuHoBot::runCommand(const std::string &cmd) {
    std::string msgRet;

    // 正确构造 CommandSenderWrapper
    CommandSenderWrapper sender_wrapper(
            getServer().getCommandSender(),  // 原始发送者

            // 正确捕获 Message 类型参数
            [&msgRet, this](const endstone::Message &msg) {
                msgRet += "\n" + getMessageContent(msg);
            },

            [&msgRet, this](const endstone::Message &msg) {
                msgRet += "\n[ERROR] " + getMessageContent(msg);
            }
    );


    // 关键修改：必须使用 wrapper 发送命令
    bool success = getServer().dispatchCommand(sender_wrapper, cmd);

    // 处理无输出情况
    if (msgRet.empty()) {
        msgRet = "无返回值";
    } else {
        // 移除首行多余换行
        msgRet.erase(0, 1);
    }

    return {msgRet,success};
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
                //client->shutdown(true);
                client->reconnect();
            }, 6*60*60*20);
}

std::shared_ptr<endstone::Task> HuHoBot::setHeartTask() {
    return this->getServer().getScheduler().runTaskTimer(
            *this,
            [&]() {
                client->sendHeart();
            }, 0, 5*20);
}

void HuHoBot::broadcastMsg(const string& msg){
    getServer().getScheduler().runTask(*this,[this,msg]() {
        broadcast(msg);
    });
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