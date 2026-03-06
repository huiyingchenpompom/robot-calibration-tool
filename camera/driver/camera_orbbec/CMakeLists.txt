cmake_minimum_required(VERSION 3.18)

project(camera_orbbec LANGUAGES CXX C)
set(PROJECT_GIGE_AREA_LIB_NAME ${PROJECT_NAME}_lib)

MESSAGE(STATUS "Build orbbec gige camera.")

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
    include_directories("C:/Program Files/micro-i/sc/opencv-3.4.6/include")
    link_directories("C:/Program Files/micro-i/sc/boost-1.83.0/lib")
    add_definitions(-DBOOST_ALL_NO_LIB)
endif(WIN32)
find_package(OpenCV REQUIRED)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/ros_msg/rosidl_generator_cpp)

# 生成orbbec_gige_area库
add_library(${PROJECT_GIGE_AREA_LIB_NAME} SHARED
	CCOrbbecDevice.h
	CCOrbbecDevice.cpp
	CCOrbbecCamera.h
	CCOrbbecCamera.cpp

	CCOrbbecCameraRosNode.h
	CCOrbbecCameraRosNode.cpp
	CCOrbbecMainProcess.h
	CCOrbbecMainProcess.cpp
	CCOrbbecCameraImp.h
	CCOrbbecCameraImp.cpp
)

include_directories(${OpenCV_INCLUDE_DIRS})

target_include_directories(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE "C:/Program Files/micro-i/sc/camera/orbbec_camera/include")
target_link_directories(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE "C:/Program Files/micro-i/sc/camera/orbbec_camera/lib/win_x64")

link_directories(${OpenCV_LIBRARIES_DIRS})

target_link_libraries(${PROJECT_GIGE_AREA_LIB_NAME}
    utility
    cc_message
    ros_msg_shm
    cc_log_client
    orbbec_camera_sdk
    cc_data_id_validator
    opencv_core
    opencv_imgcodecs
#    OrbbecSDK
)

target_compile_definitions(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE DRIVER_LIBRARY)
#target_compile_definitions(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE ORBBEC_GIGE_AREA)

