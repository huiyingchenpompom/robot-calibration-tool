cmake_minimum_required(VERSION 3.18)

project(camera_usb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugin/driver)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugin/exe)

SET(SUB_TYPE "\"${CMAKE_CURRENT_SOURCE_DIR}\"")
SET(SUB_TYPE_NAME "u8\"USB相机\"")
SET(DEVICE_NODE "\"${PROJECT_NAME}\"")
configure_file(config.h.in config.h)

if(WIN32)
    include_directories("C:/Program Files/micro-i/sc/json/include")
    include_directories("C:/Program Files/micro-i/sc/boost-1.83.0/include/boost-1_83")
    link_directories("C:/Program Files/micro-i/sc/boost-1.83.0/lib")
    add_definitions(-DBOOST_ALL_NO_LIB)
endif(WIN32)
find_package(OpenCV REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/ros_msg/rosidl_generator_cpp)

link_directories(${OpenCV_LIBRARIES_DIRS})

SET(PROJECT_LIB_NAME ${PROJECT_NAME}_lib)
add_library(${PROJECT_LIB_NAME} SHARED
    CCDeviceType.h
    CCDeviceType.cpp
    CCDeviceList.h
    CCDeviceList.cpp

    CCUsbCameraRosNode.h
    CCUsbCameraRosNode.cpp
    CCMainProcess.h
    CCMainProcess.cpp
    CCCameraIO.h
    CCCameraIO.cpp
)
ament_target_dependencies(${PROJECT_LIB_NAME} rclcpp std_msgs)
target_link_libraries(${PROJECT_LIB_NAME}
    ros_msg__rosidl_typesupport_cpp
    utility_core
    cc_log_client
    opencv_core
    opencv_imgcodecs
    opencv_videoio
    )

add_executable(${PROJECT_NAME}
    main.cpp
    CCUsbCameraRosNode.h
    CCUsbCameraRosNode.cpp
    CCMainProcess.h
    CCMainProcess.cpp
    CCCameraIO.h
    CCCameraIO.cpp
)
ament_target_dependencies(${PROJECT_NAME} rclcpp std_msgs)
target_link_libraries(${PROJECT_NAME}
    ros_msg__rosidl_typesupport_cpp

    utility_core
    cc_log_client
    opencv_core
    opencv_imgcodecs
    opencv_videoio
    )

ament_package()
