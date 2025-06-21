#include "HuhobotClient.h"
#include "BotCustomCommand.h"
#include "ConfigManager.h"
#include "endstone/scheduler/scheduler.h"
#include "huhobot.h"
#include "websocketpp/close.hpp"
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>


using endstone::Logger;

// 添加枚举到字符串的转换工具
string EnumConverter::ToString(ServerSendEvent e) {
    static const std::unordered_map<ServerSendEvent, std::string> map{
        {ServerSendEvent::sendMsg,     "sendMsg"    },
        {ServerSendEvent::heart,       "heart"      },
        {ServerSendEvent::chat,        "chat"       },
        {ServerSendEvent::success,     "success"    },
        {ServerSendEvent::error,       "error"      },
        {ServerSendEvent::shakeHand,   "shakeHand"  },
        {ServerSendEvent::queryWl,     "queryWl"    },
        {ServerSendEvent::queryOnline, "queryOnline"},
        {ServerSendEvent::bindConfirm, "bindConfirm"},
        {ServerSendEvent::unknown,     "unknown"    }
    };
    return map.at(e);
};

string EnumConverter::ToString(ServerRecvEvent e) {
    static const std::unordered_map<ServerRecvEvent, std::string> map{
        {ServerRecvEvent::sendConfig,  "sendConfig" },
        {ServerRecvEvent::shaked,      "shaked"     },
        {ServerRecvEvent::chat,        "chat"       },
        {ServerRecvEvent::add,         "add"        },
        {ServerRecvEvent::delete_,     "delete"     },
        {ServerRecvEvent::cmd,         "cmd"        },
        {ServerRecvEvent::queryList,   "queryList"  },
        {ServerRecvEvent::queryOnline, "queryOnline"},
        {ServerRecvEvent::shutdown,    "shutdown"   },
        {ServerRecvEvent::run,         "run"        },
        {ServerRecvEvent::runAdmin,    "runAdmin"   },
        {ServerRecvEvent::heart,       "heart"      },
        {ServerRecvEvent::bindRequest, "bindRequest"}
    };
    return map.at(e);
};

ServerRecvEvent EnumConverter::FromString(const std::string& str) {
    static const std::unordered_map<std::string, ServerRecvEvent> reverse_map = {
        {"sendConfig",  ServerRecvEvent::sendConfig },
        {"shaked",      ServerRecvEvent::shaked     },
        {"chat",        ServerRecvEvent::chat       },
        {"add",         ServerRecvEvent::add        },
        {"delete",      ServerRecvEvent::delete_    }, // 注意字符串"delete"对应delete_
        {"cmd",         ServerRecvEvent::cmd        },
        {"queryList",   ServerRecvEvent::queryList  },
        {"queryOnline", ServerRecvEvent::queryOnline},
        {"shutdown",    ServerRecvEvent::shutdown   },
        {"run",         ServerRecvEvent::run        },
        {"runAdmin",    ServerRecvEvent::runAdmin   },
        {"heart",       ServerRecvEvent::heart      },
        {"bindRequest", ServerRecvEvent::bindRequest}
    };

    try {
        return reverse_map.at(str);
    } catch (const std::out_of_range&) {
        return ServerRecvEvent::unknown;
    }
}


BotClient::BotClient(endstone::Logger* logger) {
    this->logger = logger;
    // 初始化client对象
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);

    // 初始化ASIO
    client.init_asio();

    // 设置TLS处理器
    client.set_tls_init_handler([](websocketpp::connection_hdl) {
        auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);
        try {
            ctx->set_options(
                asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 | asio::ssl::context::no_sslv3
                | asio::ssl::context::single_dh_use
            );
        } catch (std::exception& e) {
            // 处理TLS配置错误
        }
        return ctx;
    });
}

void BotClient::connect() {

    logger->info("正在连接服务器...");
    try {
        // 设置事件处理器
        std::weak_ptr<BotClient> weak_this = shared_from_this();

        client.set_open_handler([weak_this](auto hdl) {
            if (auto self = weak_this.lock()) {
                self->on_open(&self->client, hdl);
            }
        });

        client.set_open_handler([weak_this](websocketpp::connection_hdl hdl) {
            if (auto ptr = weak_this.lock()) {
                ptr->on_open(&ptr->client, hdl);
            }
        });

        client.set_message_handler([weak_this](websocketpp::connection_hdl hdl, message_ptr msg) {
            if (auto ptr = weak_this.lock()) {
                ptr->on_message(&ptr->client, hdl, msg);
            }
        });

        client.set_close_handler([weak_this](websocketpp::connection_hdl hdl) {
            if (auto ptr = weak_this.lock()) {
                ptr->on_close(&ptr->client, hdl);
            }
        });

        client.set_fail_handler([weak_this](websocketpp::connection_hdl hdl) {
            if (auto ptr = weak_this.lock()) {
                ptr->on_fail(&ptr->client, hdl);
            }
        });

        websocketpp::lib::error_code ec;
        auto                         con = client.get_connection(serverUrl, ec);
        if (ec) {
            logger->error("创建连接失败: {}", ec.message());
            return;
        }

        client.connect(con);

        // 在独立线程中运行IO服务
        io_thread = std::thread([this]() {
            try {
                client.run();
            } catch (const std::exception& e) {
                logger->critical("IO服务错误: {}", e.what());
            }
        });

    } catch (const std::exception& e) {
        logger->critical("连接异常: {}", e.what());
    } catch (...) {
        logger->critical("连接异常, 未知的错误");
    }
}

// 连接成功回调
void BotClient::on_open(ws_client* c, websocketpp::connection_hdl hdl) {
    connection_hdl_ = hdl;
    logger->info("服务端连接成功!");
    shakeHand(); 
    if (reconnectTask) {
        reconnectTask->cancel();
        reconnectTask = nullptr;
    }
}

// 消息处理
void BotClient::on_message(ws_client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::string payload = msg->get_payload();
    onTextMsg(payload);
}

// 连接关闭
void BotClient::on_close(ws_client* c, websocketpp::connection_hdl hdl) {
    logger->error("连接关闭");
    if (!reconnectTask && shouldReconnect) {
        reconnectTask = HuHoBot::getInstance().setReconnectTask();
    }
}

// 连接失败
void BotClient::on_fail(ws_client* c, websocketpp::connection_hdl hdl) {
    logger->error("连接失败");
    if (!reconnectTask && shouldReconnect) {
        reconnectTask = HuHoBot::getInstance().setReconnectTask();
    }
}


void BotClient::Reconnect() {
    logger->info("正在重连服务器...");
    try {
        ShutdownClient();
        connect();
    } catch (...) {
        logger->error("重连失败,未知的错误.");
    }
}

void BotClient::task_reconnect() {
    if (shouldReconnect && reconnectCount < maxReconnectCount) {
        reconnectCount++;
        logger->info(" 正在尝试重新连接,这是第({}/{})次连接", reconnectCount, maxReconnectCount);
        Reconnect();
    }
    if (reconnectCount >= maxReconnectCount) {
        logger->error("重连尝试已达到最大次数，将不再尝试重新连接.");
        reconnectTask->cancel();
        reconnectTask = nullptr;
    }
}

void BotClient::onTextMsg(string& msg) {
    // logger->info("收到服务器消息：" + msg);
    json            msgJson    = json::parse(msg);
    json            header     = msgJson["header"];
    string          type_str   = header["type"];
    ServerRecvEvent event_type = EnumConverter::FromString(type_str);
    string          packId     = header["id"];
    json            body       = msgJson["body"];

    switch (event_type) {
    case ServerRecvEvent::sendConfig:
        handler_sendConfig(packId, body);
        break;
    case ServerRecvEvent::chat:
        handler_chat(packId, body);
        break;
    case ServerRecvEvent::add:
        handler_add(packId, body);
        break;
    case ServerRecvEvent::delete_:
        handler_delete_(packId, body);
        break;
    case ServerRecvEvent::cmd:
        handler_cmd(packId, body);
        break;
    case ServerRecvEvent::queryList:
        handler_queryList(packId, body);
        break;
    case ServerRecvEvent::queryOnline:
        handler_queryOnline(packId, body);
        break;
    case ServerRecvEvent::shutdown:
        handler_shutdown(packId, body);
        break;
    case ServerRecvEvent::run:
        handler_run(packId, body, false);
        break;
    case ServerRecvEvent::runAdmin:
        handler_run(packId, body, true);
        break;
    case ServerRecvEvent::heart:
        handler_heart(packId, body);
        break;
    case ServerRecvEvent::bindRequest:
        handler_bindRequest(packId, body);
        break;
    case ServerRecvEvent::shaked:
        handler_shaked(packId, body);
        break;
    default:
        logger->error("未知事件类型:{}", type_str);
    }
}

json BotClient::buildMsg(ServerSendEvent event_type, json body, string packId) {
    json header = {
        {"type", EnumConverter::ToString(event_type)},
        {"id",   packId                             }
    };
    return json{
        {"header", header},
        {"body",   body  }
    };
}

void BotClient::sendMessage(ServerSendEvent event_type, json& body, string packId) {
    json        msg     = buildMsg(event_type, body, packId);
    std::string msg_str = msg.dump();

    websocketpp::lib::error_code ec;
    auto                         con = client.get_con_from_hdl(connection_hdl_, ec);
    if (!ec && con) {
        // 正确调用 send 方法
        con->send(msg.dump(), websocketpp::frame::opcode::text);
        if (ec) {
            logger->error("消息发送失败: {}", ec.message());
        }
    } else {
        logger->error("获取连接失败: {}", ec.message());
    }
}

void BotClient::shakeHand() {
    ConfigManager& config   = ConfigManager::Get();
    string         serverId = config.GetServerId();
    string         hashKey  = config.GetHashKey();
    string         name     = config.GetServerName();

    json body = {
        {"serverId", serverId             },
        {"hashKey",  hashKey              },
        {"name",     name                 },
        {"version",  HuHoBot::getVersion()},
        {"platform", "endstone"           }
    };
    sendMessage(ServerSendEvent::shakeHand, body);
}

void BotClient::bindConfirm(std::string code) {
    string packId = bindMap.at(code);
    json   emptyJson;
    sendMessage(ServerSendEvent::bindConfirm, emptyJson, packId);
}

void BotClient::sendHeart() {
    json emptyJson;
    sendMessage(ServerSendEvent::heart, emptyJson);
}

void BotClient::sendChat(string msg) {
    ConfigManager& config   = ConfigManager::Get();
    string         serverId = config.GetServerId();

    json body = {
        {"serverId", serverId},
        {"msg",      msg     }
    };

    sendMessage(ServerSendEvent::chat, body);
}

void BotClient::ShutdownClient(bool _shouldReconnect, string reason) {
    shouldReconnect = _shouldReconnect;
    try {
        websocketpp::lib::error_code ec;
        auto                         con = client.get_con_from_hdl(connection_hdl_, ec);

        if (!ec && con) {
            con->close(websocketpp::close::status::going_away, reason);
        }
    } catch (...) {
        logger->error("关闭连接时发生未知错误");
    }
}

void BotClient::DestoryClient() {
    try {
        ShutdownClient(false);
        if (reconnectTask) {
            reconnectTask->cancel();
            reconnectTask = nullptr;
        }

        if (heartTask) {
            heartTask->cancel();
            heartTask = nullptr;
        }

        if (autoDisConnectTask) {
            autoDisConnectTask->cancel();
            autoDisConnectTask = nullptr;
        }

        if (!client.stopped()) {
            client.stop();
        }

        //等待IO线程结束
        if (io_thread.joinable()) {
            io_thread.join();
        }

    } catch (const std::exception& e) {
        logger->error("关闭连接时出错: {}", e.what());
    } catch (...) {
        logger->error("关闭连接时发生未知错误");
    }
}

void BotClient::shakedProcess() {
    // 设置自动断连
    if (autoDisConnectTask != nullptr) {
        autoDisConnectTask->cancel();
    }
    autoDisConnectTask = HuHoBot::getInstance().setAutoDisConnectTask();
    // 设置自动发送心跳包
    if (heartTask != nullptr) {
        heartTask->cancel();
    }
    heartTask = HuHoBot::getInstance().setHeartTask();
}

//////////////////////////////////// Event Handler /////////////////////////////////////
void BotClient::handler_shaked(string packId, json& body) {
    int    code    = body["code"];
    string msg     = body["msg"];
    reconnectCount = 0;
    switch (code) {
    case 1:
        logger->info("与服务端握手成功.");
        shouldReconnect = true;
        shakedProcess();
        break;
    case 2:
        logger->info("握手完成!附加消息:{}", msg);
        shouldReconnect = true;
        shakedProcess();
        break;
    case 3:
        logger->error("握手失败，客户端密钥错误.");
        shouldReconnect = false;
        break;
    case 6:
        logger->info("与服务端握手成功，服务端等待绑定...");
        shouldReconnect = true;
        shakedProcess();
        break;
    default:
        logger->error("握手失败，原因{}", msg);
        shouldReconnect = false;
    }
}

void BotClient::handler_sendConfig(string packId, json& body) {
    ConfigManager& config  = ConfigManager::Get();
    string         HashKey = body["hashKey"];
    config.SetHashKey(HashKey);
    config.Save();
    shakeHand();
    // Reconnect();
}

void BotClient::handler_chat(string packId, json& body) {
    string         nick     = body["nick"];
    string         msg      = body["msg"];
    ConfigManager& config   = ConfigManager::Get();
    string         format   = config.GetChatFormatFromGroup();
    bool           postChat = config.GetPostChat();

    if (!postChat) return;

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

    HuHoBot::getInstance().broadcastMsg(format);
}

void BotClient::handler_add(string packId, json& body) {
    string XboxId = body["xboxid"];

    string cmd               = "allowlist add \"" + XboxId + "\"";
    auto [output, isSuccess] = HuHoBot::getInstance().runCommand(cmd);
    if (isSuccess) {
        json rBody = {
            {"msg", output}
        };
        sendMessage(ServerSendEvent::success, rBody, packId);
    } else {
        json errorBody = {
            {"msg", output}
        };
        sendMessage(ServerSendEvent::error, errorBody, packId);
    }
}

void BotClient::handler_delete_(string packId, json& body) {
    string XboxId            = body["xboxid"];
    string cmd               = "allowlist remove \"" + XboxId + "\"";
    auto [output, isSuccess] = HuHoBot::getInstance().runCommand(cmd);

    if (isSuccess) {
        json rBody = {
            {"msg", output}
        };
        sendMessage(ServerSendEvent::success, rBody, packId);
    } else {
        json errorBody = {
            {"msg", output}
        };
        sendMessage(ServerSendEvent::error, errorBody, packId);
    }
}

void BotClient::handler_cmd(string packId, json& body) {
    string cmd               = body["cmd"];
    auto [output, isSuccess] = HuHoBot::getInstance().runCommand(cmd);

    if (isSuccess) {
        json rBody = {
            {"msg", output}
        };
        sendMessage(ServerSendEvent::success, rBody, packId);
    } else {
        json errorBody = {
            {"msg", output}
        };
        sendMessage(ServerSendEvent::error, errorBody, packId);
    }
}

void BotClient::handler_queryOnline(string packId, json& body) {
    try {
        std::vector<Player*> onlinePlayers = HuHoBot::getInstance().getOnlinePlayers();

        // 构建玩家列表字符串
        std::ostringstream oss;
        for (Player* player : onlinePlayers) {
            oss << player->getName() << "\n";
        }
        oss << "共" << onlinePlayers.size() << "人在线";

        // 构建嵌套JSON结构
        json list;
        list["msg"]        = oss.str();
        list["url"]        = ConfigManager::Get().GetMotdUrl(); // 从配置获取URL
        list["serverType"] = "bedrock";

        json rBody;
        rBody["list"] = list;

        // 发送消息
        sendMessage(ServerSendEvent::queryOnline, rBody, packId);

    } catch (const std::exception& e) {
        logger->error("处理在线查询失败: {}", e.what());
    }
}

void BotClient::handler_queryList(string packId, json& body) {
    try {
        // 1. 读取白名单文件
        std::ifstream fin("allowlist.json");
        json          allowlist = json::parse(fin);

        // 2. 构建白名单集合
        std::set<std::string> whiteList;
        for (auto& entry : allowlist) {
            whiteList.insert(entry["name"].get<std::string>());
        }

        // 3. 准备响应数据
        json               rBody;
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
            std::copy_if(
                whiteList.begin(),
                whiteList.end(),
                std::back_inserter(filterList),
                [&key](const std::string& name) { return name.find(key) != std::string::npos; }
            );

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
            int                                   page = body["page"].get<int>();
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
                oss << "共有" << splitedNameList.size() << "页，当前为第" << page << "页\n请使用/查白名单 {页码}来翻页";
            }

        } else {
            // 默认显示第一页
            std::vector<std::vector<std::string>> splitedNameList;
            auto                                  it = whiteList.begin();
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
            oss << "共有" << splitedNameList.size() << "页，当前为第1页\n请使用/查白名单 {页码}来翻页";
        }

        // 5. 发送响应
        rBody["list"] = oss.str();
        sendMessage(ServerSendEvent::queryWl, rBody, packId);

    } catch (const std::exception& e) {
        logger->error("查询白名单失败: {}", e.what());
    }
}

void BotClient::handler_shutdown(string packId, json& body) {
    string msg = body["msg"];
    logger->error("服务端命令断开连接 原因:" + msg);
    logger->error("此错误具有不可容错性!请检查插件配置文件!");
    logger->warning("正在断开连接...");
    shouldReconnect = false;
    ShutdownClient();
}

void BotClient::handler_run(string packId, json& body, bool isAdmin) {
    try {
        // 解析请求参数
        std::string              key    = body["key"].get<std::string>();
        std::vector<std::string> params = body["runParams"].get<std::vector<std::string>>();

        // 获取命令映射表
        auto                                           commands = ConfigManager::Get().GetCustomCommands();
        std::unordered_map<std::string, CustomCommand> commandMap;
        for (const auto& cmd : commands) {
            commandMap[cmd.key] = cmd;
        }

        // 查找命令
        auto it = commandMap.find(key);
        if (it == commandMap.end()) {
            BotCustomCommand* event = new BotCustomCommand(key, body, packId, isAdmin, this);
            HuHoBot::getInstance().getServer().getScheduler().runTask(HuHoBot::getInstance(), [event]() {
                HuHoBot::getInstance().getServer().getPluginManager().callEvent(*event);
            });
            if (!event->isCancelled()) {
                json errorBody = {
                    {"msg", "未找到对应命令: " + key}
                };
                sendMessage(ServerSendEvent::error, errorBody, packId);
            }
            return;
        }

        const CustomCommand& result = it->second;

        // 参数替换
        std::string command = result.command;
        for (size_t i = 0; i < params.size(); ++i) {
            std::string placeholder = "&" + std::to_string(i + 1);
            size_t      pos         = 0;
            while ((pos = command.find(placeholder, pos)) != std::string::npos) {
                command.replace(pos, placeholder.length(), params[i]);
                pos += params[i].length();
            }
        }

        // 权限检查
        if (result.permission > 0 && !isAdmin) {
            json errorBody = {
                {"msg", "权限不足，若您为管理员，请使用/管理员执行"}
            };
            sendMessage(ServerSendEvent::error, errorBody, packId);
            return;
        }

        auto [output, isSuccess] = HuHoBot::getInstance().runCommand(command);

        // 执行命令
        if (isSuccess) {
            json rBody = {
                {"msg", output}
            };
            sendMessage(ServerSendEvent::success, rBody, packId);
        } else {
            json errorBody = {
                {"msg", output}
            };
            sendMessage(ServerSendEvent::error, errorBody, packId);
        }

    } catch (const json::exception& e) {
        logger->error("命令执行参数解析失败: {}", e.what());
        json errorBody = {
            {"error", "Invalid request format"}
        };
        sendMessage(ServerSendEvent::error, errorBody, packId);
    } catch (const std::exception& e) {
        logger->error("命令执行失败: {}", e.what());
        json errorBody = {
            {"error", e.what()}
        };
        sendMessage(ServerSendEvent::error, errorBody, packId);
    }
}

void BotClient::handler_heart(string packId, json& body) {}

void BotClient::handler_bindRequest(string packId, json& body) {
    string bindCode = body["bindCode"];
    logger->info("收到一个新的绑定请求，如确认绑定，请输入\"/huhobot bind " + bindCode + "\"来进行确认");
    bindMap[bindCode] = packId;
}
