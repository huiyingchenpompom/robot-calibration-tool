cmake_minimum_required(VERSION 3.15)
project(camera_addon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Node.js 头文件
execute_process(
    COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
# 去掉引号
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

execute_process(
    COMMAND node -p "require('node-api-headers').include_dir"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    OUTPUT_VARIABLE NODE_API_HEADERS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\"" "" NODE_API_HEADERS_DIR ${NODE_API_HEADERS_DIR})

# 源文件
set(SOURCES
    addon_main.cpp
    camera_manager.cpp
    basler_camera.cpp
    hik_camera.cpp
    daheng_camera.cpp
)

# 构建共享库
add_library(camera_addon SHARED ${SOURCES})

# 设置输出为 .node 文件
set_target_properties(camera_addon PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    OUTPUT_NAME "camera_addon"
)

# 包含目录
target_include_directories(camera_addon PRIVATE
    ${NODE_ADDON_API_DIR}
    ${NODE_API_HEADERS_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 编译定义
target_compile_definitions(camera_addon PRIVATE
    NAPI_VERSION=8
    NODE_ADDON_API_DISABLE_DEPRECATED
)

# 可选 SDK 检测
# Basler Pylon SDK
find_path(PYLON_INCLUDE_DIR pylon/PylonIncludes.h
    PATHS
    "C:/Program Files/Basler/pylon 7/Development/include"
    "/opt/pylon/include"
    NO_DEFAULT_PATH
)
if(PYLON_INCLUDE_DIR)
    message(STATUS "Basler Pylon SDK 已找到: ${PYLON_INCLUDE_DIR}")
    target_include_directories(camera_addon PRIVATE ${PYLON_INCLUDE_DIR})
    target_compile_definitions(camera_addon PRIVATE HAVE_PYLON_SDK)
    find_library(PYLON_LIB PylonBase_v9 GCBase_gcc_v3_1_Basler_pylon
        PATHS
        "C:/Program Files/Basler/pylon 7/Development/lib/x64"
        "/opt/pylon/lib"
        NO_DEFAULT_PATH
    )
    if(PYLON_LIB)
        target_link_libraries(camera_addon PRIVATE ${PYLON_LIB})
    endif()
else()
    message(STATUS "Basler Pylon SDK 未找到，Basler 相机功能不可用")
endif()

# 海康威视 MVS SDK
find_path(HIK_INCLUDE_DIR MvCameraControl.h
    PATHS
    "C:/Program Files (x86)/MVS/Development/Includes"
    "/opt/MVS/include"
    NO_DEFAULT_PATH
)
if(HIK_INCLUDE_DIR)
    message(STATUS "HIK MVS SDK 已找到: ${HIK_INCLUDE_DIR}")
    target_include_directories(camera_addon PRIVATE ${HIK_INCLUDE_DIR})
    target_compile_definitions(camera_addon PRIVATE HAVE_HIK_SDK)
    find_library(HIK_LIB MvCameraControl
        PATHS
        "C:/Program Files (x86)/MVS/Development/Libraries/win64"
        "/opt/MVS/lib/64"
        NO_DEFAULT_PATH
    )
    if(HIK_LIB)
        target_link_libraries(camera_addon PRIVATE ${HIK_LIB})
    endif()
else()
    message(STATUS "HIK MVS SDK 未找到，海康相机功能不可用")
endif()

# Windows 特定设置
if(WIN32)
    target_compile_definitions(camera_addon PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    # /utf-8: 告知 MSVC 源文件和执行字符集均为 UTF-8，解决中文字符在 GBK 代码页下
    # 导致的 C4819 警告和 C2001 "常量中有换行符" 编译错误
    target_compile_options(camera_addon PRIVATE /utf-8)

    # 链接 node.lib，提供 napi_* 导出符号。
    # node.lib 与 node.exe 位于同一目录（Node.js 官方安装目录）。
    # 如果 node.lib 不存在，说明 Node.js 未安装完整开发文件，
    # 可从 https://nodejs.org/dist/v{version}/node.lib 下载并放置到 node.exe 同目录。
    execute_process(
        COMMAND node -p "process.execPath"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
        OUTPUT_VARIABLE NODE_EXECUTABLE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE NODE_EXECPATH_RESULT
    )
    if(NOT NODE_EXECPATH_RESULT EQUAL 0 OR NOT NODE_EXECUTABLE)
        message(FATAL_ERROR "无法执行 node 获取 process.execPath，请确认 Node.js 已安装并在 PATH 中。")
    endif()
    execute_process(
        COMMAND node -p "process.versions.node"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
        OUTPUT_VARIABLE NODE_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE NODE_VERSION_RESULT
    )
    if(NOT NODE_VERSION_RESULT EQUAL 0 OR NOT NODE_VERSION)
        set(NODE_VERSION "unknown")
    endif()
    get_filename_component(NODE_ROOT_DIR "${NODE_EXECUTABLE}" DIRECTORY)
    set(NODE_LIB_PATH "${NODE_ROOT_DIR}/node.lib")
    set(NODE_LIB_FALLBACK_PATH "${CMAKE_SOURCE_DIR}/../node.lib")
    if(EXISTS "${NODE_LIB_PATH}")
        message(STATUS "node.lib 已找到: ${NODE_LIB_PATH}")
        target_link_libraries(camera_addon PRIVATE "${NODE_LIB_PATH}")
    elseif(EXISTS "${NODE_LIB_FALLBACK_PATH}")
        message(STATUS "node.lib 已找到（项目根目录）: ${NODE_LIB_FALLBACK_PATH}")
        target_link_libraries(camera_addon PRIVATE "${NODE_LIB_FALLBACK_PATH}")
    else()
        message(FATAL_ERROR
            "未找到 node.lib\n"
            "最简单的解决方法：在项目根目录运行\n"
            "  npm run download:nodelib\n"
            "该命令会自动下载 node.lib 并无需管理员权限。\n\n"
            "或手动从以下地址下载：\n"
            "  https://nodejs.org/dist/v${NODE_VERSION}/node.lib\n"
            "并将 node.lib 放置到以下任意位置：\n"
            "  1) node.exe 同目录: ${NODE_ROOT_DIR}\n"
            "  2) 项目根目录:      ${CMAKE_SOURCE_DIR}/..")
    endif()

    set_target_properties(camera_addon PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
    )
endif()
