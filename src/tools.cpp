#include "tools.h"
#include <random>
#include <sstream>
#include <algorithm>

// UUID版本4生成器（符合RFC 4122）
std::string generate_pack_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;

    // 生成16字节（32个十六进制字符）
    for (int i=0; i<32; ++i) {
        if (i == 12) { // 设置版本号4
            ss << 4;
        }
        else if (i == 16) { // 设置变体位
            int r = dis2(gen);
            ss << std::hex << r;
        }
        else {
            int r = dis(gen);
            ss << std::hex << r;
        }

        // 按UUID格式插入分隔符（后续会移除）
        if (i == 7 || i == 11 || i == 15 || i == 19) {
            ss << "-";
        }
    }

    std::string uuid = ss.str();
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());
    return uuid;
}