// BotCustomCommand.cpp
#include "BotCustomCommand.h"

BotCustomCommand::~BotCustomCommand() = default;

bool BotCustomCommand::isCancelled() const { return isCancelled_; }

void BotCustomCommand::setCancelled(bool cancel) { isCancelled_ = cancel; }

const string& BotCustomCommand::getCommand() const { return command_; }

const vector<string>& BotCustomCommand::getParam() const { return param_; }

bool BotCustomCommand::isRunByAdmin() const { return runByAdmin_; }

const json& BotCustomCommand::getData() const { return data_; }

void BotCustomCommand::responseString(const string& msg) {
    json body;
    body["msg"] = msg;                                                                          // 构建 JSON 对象
    HuHoBot::getInstance().getBotClient().sendMessage(ServerSendEvent::success, body, packId_); // 调用 sendMessage
}

void BotCustomCommand::responseJson(const json& msg) {
    // 使用 BotClient 对象发送响应
    json body;
    body["msg"] = msg;                                                                          // 构建 JSON 对象
    HuHoBot::getInstance().getBotClient().sendMessage(ServerSendEvent::success, body, packId_); // 直接传递 JSON 对象
}
