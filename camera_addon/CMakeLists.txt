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
# ---------------------------------------------------------------
# 本地 SDK 路径覆盖（local_sdk_paths.cmake）
#
# 若 SDK 安装路径不在下方硬编码的默认路径中，可在此文件旁边
# 创建 local_sdk_paths.cmake（参考 local_sdk_paths.cmake.example），
# 设置 PYLON_ROOT / DAHENG_ROOT / HIK_ROOT 变量后，CMake 将
# 优先在这些路径下搜索 SDK。
# 该文件已加入 .gitignore，不会意外提交到版本库。
# ---------------------------------------------------------------
include("${CMAKE_CURRENT_SOURCE_DIR}/local_sdk_paths.cmake" OPTIONAL
        RESULT_VARIABLE _LOCAL_SDK_PATHS_FILE)
if(_LOCAL_SDK_PATHS_FILE)
    message(STATUS "已加载本地 SDK 路径配置: ${_LOCAL_SDK_PATHS_FILE}")
endif()

# Basler Pylon SDK
set(_pylon_extra_paths "")
if(PYLON_ROOT)
    list(APPEND _pylon_extra_paths
        "${PYLON_ROOT}/include"
        "${PYLON_ROOT}")
endif()
find_path(PYLON_INCLUDE_DIR pylon/PylonIncludes.h
    PATHS
    ${_pylon_extra_paths}
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
        "${PYLON_ROOT}/lib/x64"
        "${PYLON_ROOT}/lib"
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

# 大恒相机 Galaxy SDK
# 搜索顺序：标准 Daheng Imaging 安装目录 → micro-i OEM 安装目录 → Linux 默认路径
set(_daheng_extra_paths "")
if(DAHENG_ROOT)
    list(APPEND _daheng_extra_paths
        "${DAHENG_ROOT}/inc"
        "${DAHENG_ROOT}/include"
        "${DAHENG_ROOT}")
endif()
find_path(DAHENG_INCLUDE_DIR GxIAPI.h
    PATHS
    ${_daheng_extra_paths}
    "C:/Program Files/Daheng Imaging/GalaxySDK/APIDll/Win64/include"
    "C:/Program Files/Daheng Imaging/GalaxySDK/APIDll/Win32/include"
    "C:/Program Files/micro-i/sc/camera/inc"
    "C:/Program Files/micro-i/sc/camera/include"
    "C:/Program Files/micro-i/sc/camera"
    "/opt/Galaxy_camera/inc"
    NO_DEFAULT_PATH
)
if(DAHENG_INCLUDE_DIR)
    message(STATUS "Daheng Galaxy SDK 已找到: ${DAHENG_INCLUDE_DIR}")
    target_include_directories(camera_addon PRIVATE ${DAHENG_INCLUDE_DIR})
    target_compile_definitions(camera_addon PRIVATE HAVE_DAHENG_SDK)
    find_library(DAHENG_LIB GxIAPI
        PATHS
        "${DAHENG_ROOT}/lib/x64"
        "${DAHENG_ROOT}/lib"
        "${DAHENG_ROOT}/libs"
        "${DAHENG_ROOT}"
        "C:/Program Files/Daheng Imaging/GalaxySDK/APIDll/Win64"
        "C:/Program Files/micro-i/sc/camera/lib"
        "C:/Program Files/micro-i/sc/camera/lib/x64"
        "C:/Program Files/micro-i/sc/camera/libs"
        "/opt/Galaxy_camera/lib"
        NO_DEFAULT_PATH
    )
    if(DAHENG_LIB)
        target_link_libraries(camera_addon PRIVATE ${DAHENG_LIB})
    endif()
else()
    message(STATUS "Daheng Galaxy SDK 未找到，大恒相机功能不可用")
endif()

# 海康威视 MVS SDK
set(_hik_extra_paths "")
if(HIK_ROOT)
    list(APPEND _hik_extra_paths
        "${HIK_ROOT}/Includes"
        "${HIK_ROOT}/include"
        "${HIK_ROOT}")
endif()
find_path(HIK_INCLUDE_DIR MvCameraControl.h
    PATHS
    ${_hik_extra_paths}
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
        "${HIK_ROOT}/Libraries/win64"
        "${HIK_ROOT}/lib/x64"
        "${HIK_ROOT}/lib"
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

    # ---------------------------------------------------------------
    # 定位 node.lib（提供 napi_* 导出符号的 Windows 导入库）
    #
    # 搜索顺序：
    #   1. node.exe 所在目录（Node.js 官方安装包的标准位置）
    #   2. 项目根目录（npm run download:nodelib 的下载位置）
    #   3. 从 node-api-headers 提供的 .def 文件自动生成
    #      ── 只需 npm install，无需下载、无需管理员权限 ──
    # ---------------------------------------------------------------
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

    # --- 候选路径列表 ---
    set(FOUND_NODE_LIB "")
    foreach(CANDIDATE IN ITEMS
            "${NODE_ROOT_DIR}/node.lib"
            "${CMAKE_SOURCE_DIR}/../node.lib")
        if(EXISTS "${CANDIDATE}")
            set(FOUND_NODE_LIB "${CANDIDATE}")
            break()
        endif()
    endforeach()

    if(FOUND_NODE_LIB)
        message(STATUS "node.lib 已找到: ${FOUND_NODE_LIB}")
        target_link_libraries(camera_addon PRIVATE "${FOUND_NODE_LIB}")
    else()
        # --- 候选路径均未找到，从 node-api-headers .def 文件自动生成 ---
        message(STATUS "node.lib 未找到，正在从 node-api-headers .def 文件自动生成…")

        execute_process(
            COMMAND node -p "require('node-api-headers').def_paths.node_api_def"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
            OUTPUT_VARIABLE NODE_API_DEF_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE DEF_QUERY_RESULT
        )
        # 去掉引号并统一为正斜杠
        string(REPLACE "\"" "" NODE_API_DEF_PATH "${NODE_API_DEF_PATH}")
        string(REPLACE "\\" "/" NODE_API_DEF_PATH "${NODE_API_DEF_PATH}")

        # 找到与 cl.exe 同目录的 lib.exe（MSVC 导入库生成工具）
        get_filename_component(MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_program(LIB_EXECUTABLE NAMES lib HINTS "${MSVC_BIN_DIR}" DOC "MSVC lib.exe")
        if(NOT LIB_EXECUTABLE AND CMAKE_AR)
            set(LIB_EXECUTABLE "${CMAKE_AR}")
        endif()

        if(NOT DEF_QUERY_RESULT EQUAL 0 OR NOT EXISTS "${NODE_API_DEF_PATH}")
            message(FATAL_ERROR
                "未找到 node.lib，且无法从 node-api-headers 获取 .def 文件路径（node-api-headers 可能未安装）。\n"
                "请先运行: npm install\n"
                "若仍失败，运行: npm run download:nodelib\n"
                "或从 https://nodejs.org/dist/v${NODE_VERSION}/node.lib 手动下载\n"
                "并将文件放到项目根目录或 node.exe 所在目录: ${NODE_ROOT_DIR}")
        endif()
        if(NOT LIB_EXECUTABLE)
            message(FATAL_ERROR
                "未找到 node.lib，且找不到 lib.exe（无法自动生成）。\n"
                "请在 x64 Native Tools Command Prompt 中运行，确保 MSVC 工具在 PATH 中。\n"
                "或运行: npm run download:nodelib")
        endif()

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(LIB_MACHINE "X64")
        else()
            set(LIB_MACHINE "X86")
        endif()

        set(GENERATED_NODE_LIB "${CMAKE_BINARY_DIR}/node.lib")
        execute_process(
            COMMAND "${LIB_EXECUTABLE}"
                    "/def:${NODE_API_DEF_PATH}"
                    "/machine:${LIB_MACHINE}"
                    "/out:${GENERATED_NODE_LIB}"
            RESULT_VARIABLE LIB_GEN_RESULT
            OUTPUT_VARIABLE LIB_GEN_OUTPUT
            ERROR_VARIABLE  LIB_GEN_ERROR
        )

        if(LIB_GEN_RESULT EQUAL 0 AND EXISTS "${GENERATED_NODE_LIB}")
            message(STATUS "node.lib 已自动生成: ${GENERATED_NODE_LIB}")
            target_link_libraries(camera_addon PRIVATE "${GENERATED_NODE_LIB}")
        else()
            message(WARNING "lib.exe 生成 node.lib 失败:\n${LIB_GEN_OUTPUT}\n${LIB_GEN_ERROR}")
            message(FATAL_ERROR
                "无法自动生成 node.lib。\n"
                "请在项目根目录运行: npm run download:nodelib\n"
                "或从 https://nodejs.org/dist/v${NODE_VERSION}/node.lib 手动下载\n"
                "并将文件放到项目根目录或 node.exe 所在目录: ${NODE_ROOT_DIR}")
        endif()
    endif()

    set_target_properties(camera_addon PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
    )
endif()
