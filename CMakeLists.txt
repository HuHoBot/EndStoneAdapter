cmake_minimum_required(VERSION 3.15)

project(huhobot CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 接收用户传入参数，设置默认值
set(HUHOBOT_SERVER_URL "ws://127.0.0.1:8888" CACHE STRING "WebSocket server URL")

# 生成配置文件
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

include(FetchContent)
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG main
)
FetchContent_MakeAvailable(endstone)

file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "src/**/*.cpp"
)
endstone_add_plugin(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)