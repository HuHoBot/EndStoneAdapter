#pragma once

#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#include <functional> // 添加回调支持
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>


typedef websocketpp::client<websocketpp::config::asio_tls_client> ws_client;
typedef websocketpp::config::asio_client::message_type::ptr       message_ptr;

#include "config.h"
#include "endstone/logger.h"
#include "endstone/scheduler/task.h"
#include "nlohmann/json.hpp"
#include "tools.h"
#include <memory>
#include <unordered_map>


// using cyanray::WebSocketClient;
using endstone::Logger;
using json = nlohmann::json;
using std::string;


enum class ServerSendEvent {
    sendMsg,
    heart,
    chat,
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

struct EnumConverter {
    static std::string     ToString(ServerSendEvent e);
    static std::string     ToString(ServerRecvEvent e);
    static ServerRecvEvent FromString(const std::string& str);
};

class BotClient : public std::enable_shared_from_this<BotClient> {
private:
    ws_client                   client;
    std::thread                 io_thread;
    websocketpp::connection_hdl connection_hdl_;

    std::string                                  serverUrl = HUHOBOT_SERVER_URL;
    Logger*                                      logger;
    std::unordered_map<std::string, std::string> bindMap;
    bool                                         shouldReconnect;
    bool                                         waitingReconnect   = false;
    int                                          reconnectCount     = 0;
    int                                          maxReconnectCount  = 5;
    std::shared_ptr<endstone::Task>              reconnectTask      = nullptr;
    std::shared_ptr<endstone::Task>              heartTask          = nullptr;
    std::shared_ptr<endstone::Task>              autoDisConnectTask = nullptr;

    json buildMsg(ServerSendEvent event_type, json body, string packId);
    void shakeHand();
    void shakedProcess();
    bool isConnected();
    void CancelAllTask(bool cancelReconnectTask=true);

    // Event Handler
    void handler_sendConfig(string packId, json& body);
    void handler_chat(string packId, json& body);
    void handler_add(string packId, json& body);
    void handler_delete_(string packId, json& body);
    void handler_cmd(string packId, json& body);
    void handler_queryList(string packId, json& body);
    void handler_queryOnline(string packId, json& body);
    void handler_shutdown(string packId, json& body);
    void handler_run(string packId, json& body, bool isAdmin);
    void handler_heart(string packId, json& body);
    void handler_bindRequest(string packId, json& body);
    void handler_shaked(string packId, json& body);

public:
    BotClient(Logger* logger);
    void connect();

    // 替换为IXWebSocket的回调类型
    void on_open(ws_client* c, websocketpp::connection_hdl hdl);
    void on_message(ws_client* c, websocketpp::connection_hdl hdl, message_ptr msg);
    void on_close(ws_client* c, websocketpp::connection_hdl hdl);
    void on_fail(ws_client* c, websocketpp::connection_hdl hdl);
    void onTextMsg(string& msg);

    void sendMessage(ServerSendEvent event_type, json& body, string packId = tools::generate_pack_id());
    void bindConfirm(string code);
    void Reconnect();
    void sendHeart();
    void sendChat(string msg);

    void task_reconnect();

    void ShutdownClient(bool _shouldReconnect = true, string reason = ""); // 关闭客户端
    void DestoryClient();                                                  // 销毁客户端
};