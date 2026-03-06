cmake_minimum_required(VERSION 3.18)

project(camera_dalsa LANGUAGES CXX C)
set(PROJECT_CL_LINE_LIB_NAME ${PROJECT_NAME}_cl_line_lib)
set(PROJECT_CL_LINE_NAME ${PROJECT_NAME}_cl_line)

MESSAGE(STATUS "Build Dalsa Usb camera.")

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
    include_directories("C:/Program Files/Teledyne DALSA/Sapera/Classes")
    include_directories("C:/Program Files/Teledyne DALSA/Sapera/Include")
    link_directories("C:/Program Files/Teledyne DALSA/Sapera/Lib/Win64")
    link_directories("C:/Program Files/micro-i/sc/boost-1.83.0/lib")
    add_definitions(-DBOOST_ALL_NO_LIB)
endif(WIN32)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/ros_msg/rosidl_generator_cpp)

# 生成dalsa_cl_line库
add_library(${PROJECT_CL_LINE_LIB_NAME} SHARED
    CCDalsaDevice.h
    CCDalsaDevice.cpp
    CCDalsaCamera.h
    CCDalsaCamera.cpp
    CCDalsaCameraRosNode.h
    CCDalsaCameraRosNode.cpp
    CCDalsaMainProcess.h
    CCDalsaMainProcess.cpp
    CCDalsaCameraImp.h
    CCDalsaCameraImp.cpp
)
ament_target_dependencies(${PROJECT_CL_LINE_LIB_NAME} rclcpp std_msgs)
target_link_libraries(${PROJECT_CL_LINE_LIB_NAME}
    ros_msg__rosidl_typesupport_cpp
    cc_message
    utility
    ros_msg_shm
    cc_log_client
    cc_toolkit
    SapClassBasic
    corapi
    cc_data_id_validator
)
target_compile_definitions(${PROJECT_CL_LINE_LIB_NAME} PRIVATE DRIVER_LIBRARY)
target_compile_definitions(${PROJECT_CL_LINE_LIB_NAME} PRIVATE DALSA_CL_LINE)

ament_package()

