#include "HuhobotClient.h"
#include "ConfigManager.h"
#include "huhobot.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>

using endstone::Logger;

enum class ServerSendEvent {
    sendMsg,
    heart,
    success,
    error,
    shakeHand,
    queryWl,
    queryOnline,
    bindConfirm,
    unknown
};

enum class ServerRecvEvent {
    sendConfig,
    shaked,
    chat,
    add,
    delete_,
    cmd,
    queryList,
    queryOnline,
    shutdown,
    run,
    runAdmin,
    heart,
    bindRequest,
    unknown
};

// 添加枚举到字符串的转换工具
string EnumConverter::ToString(ServerSendEvent e) {
    static const std::unordered_map<ServerSendEvent, std::string> map{
        {ServerSendEvent::sendMsg, "sendMsg"},
        {ServerSendEvent::heart, "heart"},
        {ServerSendEvent::success, "success"},
        {ServerSendEvent::error, "error"},
        {ServerSendEvent::shakeHand, "shakeHand"},
        {ServerSendEvent::queryWl, "queryWl"},
        {ServerSendEvent::queryOnline, "queryOnline"},
        {ServerSendEvent::bindConfirm, "bindConfirm"},
        {ServerSendEvent::unknown, "unknown"}
    };
    return map.at(e);
};

string EnumConverter::ToString(ServerRecvEvent e) {
    static const std::unordered_map<ServerRecvEvent, std::string> map{
        {ServerRecvEvent::sendConfig, "sendConfig"},
        {ServerRecvEvent::shaked, "shaked"},
        {ServerRecvEvent::chat, "chat"},
        {ServerRecvEvent::add, "add"},
        {ServerRecvEvent::delete_, "delete"},
        {ServerRecvEvent::cmd, "cmd"},
        {ServerRecvEvent::queryList, "queryList"},
        {ServerRecvEvent::queryOnline, "queryOnline"},
        {ServerRecvEvent::shutdown, "shutdown"},
        {ServerRecvEvent::run, "run"},
        {ServerRecvEvent::runAdmin, "runAdmin"},
        {ServerRecvEvent::heart, "heart"},
        {ServerRecvEvent::bindRequest, "bindRequest"}
    };
    return map.at(e);
};

static ServerRecvEvent FromString(const std::string& str) {
    static const std::unordered_map<std::string, ServerRecvEvent> reverse_map = {
            {"sendConfig", ServerRecvEvent::sendConfig},
            {"shaked",     ServerRecvEvent::shaked},
            {"chat",       ServerRecvEvent::chat},
            {"add",        ServerRecvEvent::add},
            {"delete",     ServerRecvEvent::delete_},  // 注意字符串"delete"对应delete_
            {"cmd",        ServerRecvEvent::cmd},
            {"queryList",  ServerRecvEvent::queryList},
            {"queryOnline",ServerRecvEvent::queryOnline},
            {"shutdown",   ServerRecvEvent::shutdown},
            {"run",        ServerRecvEvent::run},
            {"runAdmin",   ServerRecvEvent::runAdmin},
            {"heart",      ServerRecvEvent::heart},
            {"bindRequest",ServerRecvEvent::bindRequest}
    };

    try {
        return reverse_map.at(str);
    } catch (const std::out_of_range&) {
        return ServerRecvEvent::unknown;
    }
}


BotClient::BotClient(endstone::Logger *logger) {
    this->logger = logger;
}

void BotClient::connect(){
    logger->info("正在连接服务器...");
    client.Connect(serverUrl);
    client.OnTextReceived([this](cyanray::WebSocketClient& ws, std::string msg) { // 使用成员函数捕获
        this->onTextMsg(msg); // 将消息转发给成员函数
    });
    client.OnError([this](cyanray::WebSocketClient& ws, std::string msg) {

    });
    client.OnLostConnection([this](cyanray::WebSocketClient& ws, int code) {

    });
    logger->info("连接服务器成功.");
    logger->info("正在握手...");
    shakeHand();
}

void BotClient::onTextMsg(string& msg){
    logger->info("收到服务器消息：" + msg);
    json msgJson = json::parse(msg);
    json header = msgJson["header"];
    string type_str = header["type"];
    ServerRecvEvent event_type = FromString(type_str);
    string packId = header["id"];
    json body = msgJson["body"];

    switch (event_type) {
        case ServerRecvEvent::sendConfig:
            handler_sendConfig(packId,body);
            break;
        case ServerRecvEvent::chat:
            handler_chat(packId,body);
            break;
        case ServerRecvEvent::add:
            handler_add(packId,body);
            break;
        case ServerRecvEvent::delete_:
            handler_delete_(packId,body);
            break;
        case ServerRecvEvent::cmd:
            handler_cmd(packId,body);
            break;
        case ServerRecvEvent::queryList:
            handler_queryList(packId,body);
            break;
        case ServerRecvEvent::queryOnline:
            handler_queryOnline(packId,body);
            break;
        case ServerRecvEvent::shutdown:
            handler_shutdown(packId,body);
            break;
        case ServerRecvEvent::run:
            handler_run(packId,body,false);
            break;
        case ServerRecvEvent::runAdmin:
            handler_run(packId,body,true);
            break;
        case ServerRecvEvent::heart:
            handler_heart(packId,body);
            break;
        case ServerRecvEvent::bindRequest:
            handler_bindRequest(packId,body);
            break;
        case ServerRecvEvent::shaked:
            handler_shaked(packId,body);
            break;
        default:
            logger->error("未知事件类型:{}", type_str);
    }

}

void BotClient::onError(std::string &errorMsg) {

}

void BotClient::onLost(int code) {

}

json BotClient::buildMsg(ServerSendEvent event_type,json body,string packId) {
    json header = {
            {"type", EnumConverter::ToString(event_type)},
            {"id", packId}
    };
    return json{
            {"header", header},
            {"body", body}
    };
}

void BotClient::sendMessage(ServerSendEvent event_type,json& body,string packId) {
    json msg = buildMsg(event_type,body,packId);
    client.SendText(msg.dump());
}

void BotClient::shakeHand() {
    ConfigManager& config = ConfigManager::Get();
    string serverId = config.GetServerId();
    string hashKey = config.GetHashKey();
    string name = config.GetServerName();

    json body = {
            {"serverId", serverId},
            {"hashKey", hashKey},
            {"name", name},
            {"version", HuHoBot::getVersion()},
            {"platform", "endstone"}
    };
    sendMessage(ServerSendEvent::shakeHand,body);
}

void BotClient::bindConfirm(std::string code) {
    string packId = bindMap.at(code);
    json emptyJson;
    sendMessage(ServerSendEvent::bindConfirm,emptyJson,packId);
}

//////////////////////////////////// Event Handler /////////////////////////////////////
void BotClient::handler_shaked(string packId,json &body) {
    int code = body["code"];
    string msg = body["msg"];
    switch (code) {
        case 1:
            logger->info("与服务端握手成功.");
            shouldReconnect = true;
            //shakedProcess();
            break;
        case 2:
            logger->info("握手完成!附加消息:{}", msg);
            shouldReconnect = true;
            //shakedProcess();
            break;
        case 3:
            logger->error("握手失败，客户端密钥错误.");
            shouldReconnect = false;
            break;
        case 6:
            logger->info("与服务端握手成功，服务端等待绑定...");
            shouldReconnect = true;
            //shakedProcess();
            break;
        default:
            logger->error("握手失败，原因{}", msg);
            shouldReconnect = false;
    }
}

void BotClient::handler_sendConfig(string packId,json &body){
    ConfigManager& config = ConfigManager::Get();
    string HashKey = body["hashKey"];
    config.SetHashKey(HashKey);
    config.Save();

    //ToDo: 自动重连
}

void BotClient::handler_chat(string packId,json &body) {
    string nick = body["nick"];
    string msg = body["msg"];
    ConfigManager& config = ConfigManager::Get();
    string format = config.GetChatFormatGroup();

    // 替换{nick}
    size_t pos = format.find("{nick}");
    if (pos != std::string::npos) {
        format.replace(pos, 6, nick);
    }

    // 替换{msg}
    pos = format.find("{msg}");
    if (pos != std::string::npos) {
        format.replace(pos, 5, msg);
    }

    //ToDo: 发送消息
}

void BotClient::handler_add(string packId,json &body) {
    string XboxId = body["xboxid"];

    //ToDo: 执行命令
}

void BotClient::handler_delete_(string packId,json &body) {
    string XboxId = body["xboxid"];

    //ToDo: 执行命令
}

void BotClient::handler_cmd(string packId,json &body) {
    string cmd = body["cmd"];

    //ToDo: 执行命令
}

void BotClient::handler_queryOnline(string packId,json &body) {
    //ToDo: 获取玩家列表

}

void BotClient::handler_queryList(string packId,json &body) {
    try {
        // 1. 读取白名单文件
        std::ifstream fin("allowlist.json");
        json allowlist = json::parse(fin);

        // 2. 构建白名单集合
        std::set<std::string> whiteList;
        for (auto& entry : allowlist) {
            whiteList.insert(entry["name"].get<std::string>());
        }

        // 3. 准备响应数据
        json rBody;
        std::ostringstream oss;

        // 4. 处理不同查询条件
        if (body.contains("key")) {
            std::string key = body["key"].get<std::string>();

            if (key.length() < 2) {
                oss << "查询白名单关键词:" << key << "结果如下:\n"
                    << "请使用两个字母及以上的关键词进行查询!";
                rBody["list"] = oss.str();
                sendMessage(ServerSendEvent::queryWl, rBody);
                return;
            }

            // 过滤包含关键词的名单
            std::vector<std::string> filterList;
            std::copy_if(whiteList.begin(), whiteList.end(),
                         std::back_inserter(filterList),
                         [&key](const std::string& name) {
                             return name.find(key) != std::string::npos;
                         });

            oss << "查询白名单关键词:" << key << "结果如下:\n";
            if (filterList.empty()) {
                oss << "无结果\n";
            } else {
                for (auto& name : filterList) {
                    oss << name << "\n";
                }
                oss << "共有" << filterList.size() << "个结果";
            }

        } else if (body.contains("page")) {
            // 分页处理
            int page = body["page"].get<int>();
            std::vector<std::vector<std::string>> splitedNameList;

            // 分块处理（每页10条）
            auto it = whiteList.begin();
            while (it != whiteList.end()) {
                std::vector<std::string> chunk;
                for (int i = 0; i < 10 && it != whiteList.end(); ++i, ++it) {
                    chunk.push_back(*it);
                }
                splitedNameList.push_back(chunk);
            }

            oss << "服内白名单如下:\n";
            if (page - 1 >= splitedNameList.size() || page < 1) {
                oss << "没有该页码\n"
                    << "共有" << splitedNameList.size() << "页\n请使用/查白名单 {页码}来翻页";
            } else {
                auto& currentList = splitedNameList[page - 1];
                for (auto& name : currentList) {
                    oss << name << "\n";
                }
                oss << "共有" << splitedNameList.size()
                    << "页，当前为第" << page << "页\n请使用/查白名单 {页码}来翻页";
            }

        } else {
            // 默认显示第一页
            std::vector<std::vector<std::string>> splitedNameList;
            auto it = whiteList.begin();
            while (it != whiteList.end()) {
                std::vector<std::string> chunk;
                for (int i = 0; i < 10 && it != whiteList.end(); ++i, ++it) {
                    chunk.push_back(*it);
                }
                splitedNameList.push_back(chunk);
            }

            oss << "服内白名单如下:\n";
            if (!splitedNameList.empty()) {
                for (auto& name : splitedNameList[0]) {
                    oss << name << "\n";
                }
            }
            oss << "共有" << splitedNameList.size()
                << "页，当前为第1页\n请使用/查白名单 {页码}来翻页";
        }

        // 5. 发送响应
        rBody["list"] = oss.str();
        sendMessage(ServerSendEvent::queryWl, rBody);

    } catch (const std::exception& e) {
        logger->error("查询白名单失败: {}", e.what());
    }
}

void BotClient::handler_shutdown(string packId,json &body) {
    logger->error("服务端命令断开连接 原因:"+body["msg"]);
    logger->error("此错误具有不可容错性!请检查插件配置文件!");
    logger->warning("正在断开连接...");
    shouldReconnect = false;
    //HuHoBot.getClientManager().setShouldReconnect(false);
    //HuHoBot.getClientManager().shutdownClient();
    client.Shutdown();
}

void BotClient::handler_run(string packId,json &body, bool isAdmin) {
    //ToDo: 执行命令

}

void BotClient::handler_heart(string packId,json &body) {}

void BotClient::handler_bindRequest(string packId,json &body) {
    string bindCode = body["bindCode"];
    logger->info("收到一个新的绑定请求，如确认绑定，请输入\"/huhobot bind "+bindCode+"\"来进行确认");
    bindMap[bindCode] = packId;
}

