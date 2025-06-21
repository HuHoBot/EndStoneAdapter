// BotCustomCommand.h
#pragma once

#include "HuhobotClient.h"
#include "endstone/event/cancellable.h"
#include "endstone/event/event.h"
#include "huhobot.h"
#include "nlohmann/json.hpp"
#include <string>
#include <vector>


using endstone::Cancellable;
using endstone::Event;
using std::string;
using std::vector;
using json = nlohmann::json; // 别名简化

class BotCustomCommand : public Event {
public:
    explicit BotCustomCommand(
        const string& command,
        const json&   data,
        const string& packId,
        bool          runByAdmin,
        BotClient*    botClient
    )
    : command_(command),
      data_(data),
      packId_(packId),
      runByAdmin_(runByAdmin) {
        // 解析参数
        if (data.contains("runParams") && data["runParams"].is_array()) {
            for (const auto& param : data["runParams"]) {
                param_.push_back(param.get<string>());
            }
        }
    };

    ~BotCustomCommand() override;

    bool isCancelled() const;
    void setCancelled(bool cancel);

    const string&         getCommand() const;
    const vector<string>& getParam() const;
    bool                  isRunByAdmin() const;
    const json&           getData() const;

    void responseString(const string& msg);
    void responseJson(const json& msg);

    // 实现纯虚函数 getEventName
    std::string getEventName() const override { return "BotCustomCommand"; }

private:
    bool           isCancelled_ = false; // 取消状态
    string         command_;             // 命令
    json           data_;                // 数据
    vector<string> param_;               // 参数列表
    string         packId_;              // 包 ID
    bool           runByAdmin_;          // 是否由管理员执行
};
