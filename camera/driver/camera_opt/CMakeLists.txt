cmake_minimum_required(VERSION 3.18)

project(camera_opt LANGUAGES CXX C)
set(PROJECT_USB_AREA_NAME ${PROJECT_NAME}_usb_area)
set(PROJECT_USB_AREA_LIB_NAME ${PROJECT_NAME}_usb_area_lib)
set(PROJECT_GIGE_AREA_NAME ${PROJECT_NAME}_gige_area)
set(PROJECT_GIGE_AREA_LIB_NAME ${PROJECT_NAME}_gige_area_lib)
set(PROJECT_GIGE_LINE_NAME ${PROJECT_NAME}_gige_line)
set(PROJECT_GIGE_LINE_LIB_NAME ${PROJECT_NAME}_gige_line_lib)

MESSAGE(STATUS "Build Opt Usb camera.")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugin/driver)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugin/exe)

if(WIN32)
    include_directories("C:/Program Files/micro-i/sc/json/include")
    include_directories("C:/Program Files/micro-i/sc/boost-1.83.0/include/boost-1_83")
    link_directories("C:/Program Files/micro-i/sc/boost-1.83.0/lib")
    add_definitions(-DBOOST_ALL_NO_LIB)
endif(WIN32)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/ros_msg/rosidl_generator_cpp)

# 生成opt_usb_area库
add_library(${PROJECT_USB_AREA_LIB_NAME} SHARED
    CCOptDevice.h
    CCOptDevice.cpp
    CCOptCamera.h
    CCOptCamera.cpp

    CCOptCameraRosNode.h
    CCOptCameraRosNode.cpp
    CCOptMainProcess.h
    CCOptMainProcess.cpp
    CCOptCameraImp.h
    CCOptCameraImp.cpp
)
target_link_libraries(${PROJECT_USB_AREA_LIB_NAME}
    utility
    cc_message
    ros_msg_shm
    cc_log_client
    opt_camera_sdk
    cc_data_id_validator
)
target_compile_definitions(${PROJECT_USB_AREA_LIB_NAME} PRIVATE DRIVER_LIBRARY)
target_compile_definitions(${PROJECT_USB_AREA_LIB_NAME} PRIVATE OPT_USB_AREA)

# 生成opt_gige_area库
add_library(${PROJECT_GIGE_AREA_LIB_NAME} SHARED
    CCOptDevice.h
    CCOptDevice.cpp
    CCOptCamera.h
    CCOptCamera.cpp

    CCOptCameraRosNode.h
    CCOptCameraRosNode.cpp
    CCOptMainProcess.h
    CCOptMainProcess.cpp
    CCOptCameraImp.h
    CCOptCameraImp.cpp
)
target_link_libraries(${PROJECT_GIGE_AREA_LIB_NAME}
    utility
    cc_message
    ros_msg_shm
    cc_log_client
    opt_camera_sdk
    cc_data_id_validator
)
target_compile_definitions(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE DRIVER_LIBRARY)
target_compile_definitions(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE OPT_GIGE_AREA)

# 生成opt_gige_line库
add_library(${PROJECT_GIGE_LINE_LIB_NAME} SHARED
    CCOptDevice.h
    CCOptDevice.cpp
    CCOptCamera.h
    CCOptCamera.cpp

    CCOptCameraRosNode.h
    CCOptCameraRosNode.cpp
    CCOptMainProcess.h
    CCOptMainProcess.cpp
    CCOptCameraImp.h
    CCOptCameraImp.cpp
)
target_link_libraries(${PROJECT_GIGE_LINE_LIB_NAME}
    utility
    cc_message
    ros_msg_shm
    cc_log_client
    opt_camera_sdk
    cc_data_id_validator
)
target_compile_definitions(${PROJECT_GIGE_LINE_LIB_NAME} PRIVATE DRIVER_LIBRARY)
target_compile_definitions(${PROJECT_GIGE_LINE_LIB_NAME} PRIVATE OPT_GIGE_LINE)
