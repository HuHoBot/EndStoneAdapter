#pragma once
#include "WebSocketClient.h"
#include "endstone/logger.h"
#include "nlohmann/json.hpp"
#include "tools.h"
#include <unordered_map>

using cyanray::WebSocketClient;
using endstone::Logger;
using json = nlohmann::json;

enum class ServerRecvEvent;
enum class ServerSendEvent;

struct EnumConverter {
    static std::string ToString(ServerSendEvent e);
    static std::string ToString(ServerRecvEvent e);
    static ServerRecvEvent FromString(const std::string& str);
};

class BotClient{
private:
    WebSocketClient client;
    string serverUrl = "ws://127.0.0.1:8888";
    Logger* logger;
    std::unordered_map<std::string, std::string> bindMap;
    bool shouldReconnect;

    json buildMsg(ServerSendEvent event_type,json body,string packId);
    void shakeHand();

    //Event Handler
    void handler_sendConfig(string packId,json &body);
    void handler_chat(string packId,json &body);
    void handler_add(string packId,json &body);
    void handler_delete_(string packId,json &body);
    void handler_cmd(string packId,json &body);
    void handler_queryList(string packId,json &body);
    void handler_queryOnline(string packId,json &body);
    void handler_shutdown(string packId,json &body);
    void handler_run(string packId,json &body,bool isAdmin);
    void handler_heart(string packId,json &body);
    void handler_bindRequest(string packId,json &body);
    void handler_shaked(string packId,json &body);
public:
    BotClient(Logger* logger);
    void connect();
    void onTextMsg(string& msg);
    void onError(string& errorMsg);
    void onLost(int code);
    void sendMessage(
            ServerSendEvent event_type,
            json& body,
            string packId=tools::generate_pack_id()
    );
    void bindConfirm(string code);
};