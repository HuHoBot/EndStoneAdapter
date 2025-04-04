cmake_minimum_required(VERSION 3.15)

project(huhobot CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG main
)
FetchContent_MakeAvailable(endstone)

add_library(HuHoBot
        src/HuhobotClient.cpp
        src/tools.cpp
        src/WebSocketClient.cpp
        src/ConfigManager.cpp  # 新增
        src/huhobot.cpp
)

endstone_add_plugin(${PROJECT_NAME} src/huhobot.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)