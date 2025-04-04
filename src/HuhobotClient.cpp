#include "HuhobotClient.h"

using namespace huhobot;
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
    bindRequest
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
    json msg = buildMsg(ServerSendEvent::sendMsg,body,packId);
    client.SendText(msg.dump());
}
