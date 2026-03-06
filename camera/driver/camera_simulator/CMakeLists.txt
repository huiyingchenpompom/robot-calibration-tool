cmake_minimum_required(VERSION 3.18)

project(camera_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugin/driver)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugin/exe)

SET(SUB_TYPE "\"${CMAKE_CURRENT_SOURCE_DIR}\"")
SET(SUB_TYPE_NAME "u8\"模拟相机\"")
SET(DEVICE_NODE "\"${PROJECT_NAME}\"")
configure_file(config.h.in config.h)

if(WIN32)
    include_directories("C:/Program Files/micro-i/sc/json/include")
    include_directories("C:/Program Files/micro-i/sc/boost-1.83.0/include/boost-1_83")
    link_directories("C:/Program Files/micro-i/sc/boost-1.83.0/lib")
    add_definitions(-DBOOST_ALL_NO_LIB)
endif(WIN32)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/ros_msg/rosidl_generator_cpp)
link_directories(${OpenCV_LIBRARIES_DIRS})

SET(PROJECT_LIB_NAME ${PROJECT_NAME}_lib)
add_library(${PROJECT_LIB_NAME} SHARED
    CCDeviceList.h
    CCDeviceList.cpp
    CCDeviceType.h
    CCDeviceType.cpp

    CCSimulatorCameraRosNode.h
    CCSimulatorCameraRosNode.cpp
    CCSimulatorCameraMainProcess.h
    CCSimulatorCameraMainProcess.cpp
    CCSimulatorCameraCameraIO.h
    CCSimulatorCameraCameraIO.cpp
    CCSimulatorCameraUdpServer.h
    CCSimulatorCameraUdpServer.cpp
)
target_link_libraries(${PROJECT_LIB_NAME}
    utility
    cc_message
    ros_msg_shm

    opencv_core
    opencv_imgcodecs
    cc_data_id_validator
)
target_compile_definitions(${PROJECT_LIB_NAME} PRIVATE DRIVER_LIBRARY)

