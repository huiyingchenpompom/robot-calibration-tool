cmake_minimum_required(VERSION 3.18)

project(node_camera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugin/node)

if(WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
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

add_library(node_camera_get_image SHARED
    CCCameraGetImageFlowNode.h
    CCCameraGetImageFlowNode.cpp
    CCCameraGetImageFlowNodeInterface.cpp
    CCCameraGetImageRosNode.h
    CCCameraGetImageRosNode.cpp
    CCCameraGetImage.h
)
target_link_libraries(node_camera_get_image
    utility
    cc_message
    ros_msg_shm
    database
    opencv_core
    opencv_imgcodecs
)
target_compile_definitions(node_camera_get_image PRIVATE DRIVER_LIBRARY)

add_library(node_camera_get_image_fast SHARED
    CCCameraGetImageFastFlowNode.h
    CCCameraGetImageFastFlowNode.cpp
    CCCameraGetImageFastFlowNodeInterface.cpp
    CCCameraGetImageRosNode.h
    CCCameraGetImageRosNode.cpp
    CCCameraGetImage.h
)
target_link_libraries(node_camera_get_image_fast
    utility
    cc_message
    ros_msg_shm
    opencv_core
    opencv_imgcodecs
)
target_compile_definitions(node_camera_get_image_fast PRIVATE DRIVER_LIBRARY)

add_library(node_camera_set_param SHARED
    CCCameraSetParamFlowNode.h
    CCCameraSetParamFlowNode.cpp
    CCCameraSetParamFlowNodeInterface.cpp
    CCCameraSetParamRosNode.h
    CCCameraSetParamRosNode.cpp
)
target_link_libraries(node_camera_set_param
    utility
    cc_message
)
target_compile_definitions(node_camera_set_param PRIVATE DRIVER_LIBRARY)

add_library(node_camera_get_param SHARED
    CCCameraGetParamFlowNode.h
    CCCameraGetParamFlowNode.cpp
    CCCameraGetParamFlowNodeInterface.cpp
    CCCameraGetParamRosNode.h
    CCCameraGetParamRosNode.cpp
)
target_link_libraries(node_camera_get_param
    utility
    cc_message
)
target_compile_definitions(node_camera_get_param PRIVATE DRIVER_LIBRARY)


add_library(node_camera_switch SHARED
    CCCameraSwitchFlowNode.h
    CCCameraSwitchFlowNode.cpp
    CCCameraSwitchFlowNodeInterface.cpp
    CCCameraSwitchRosNode.h
    CCCameraSwitchRosNode.cpp
)
target_link_libraries(node_camera_switch
    utility
    cc_message
)
target_compile_definitions(node_camera_switch PRIVATE DRIVER_LIBRARY)

