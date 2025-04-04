#pragma once
#include "WebSocketClient.h"
#include "endstone/logger.h"
#include "nlohmann/json.hpp"
#include "tools.h"

using cyanray::WebSocketClient;
using endstone::Logger;
using json = nlohmann::json;

enum class ServerRecvEvent;
enum class ServerSendEvent;

struct EnumConverter {
    static std::string ToString(ServerSendEvent e);
    static std::string ToString(ServerRecvEvent e);
};

namespace huhobot {
    class BotClient{
    private:
        WebSocketClient client;
        string serverUrl = "ws://127.0.0.1:8888";
        Logger* logger;
        json buildMsg(ServerSendEvent event_type,json body,string packId);
        void shakeHand();
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
    };

}