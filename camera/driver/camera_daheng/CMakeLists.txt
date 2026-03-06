cmake_minimum_required(VERSION 3.18)

project(camera_daheng LANGUAGES CXX)
set(PROJECT_USB_AREA_LIB_NAME ${PROJECT_NAME}_usb_area_lib)
set(PROJECT_GIGE_AREA_LIB_NAME ${PROJECT_NAME}_gige_area_lib)
set(PROJECT_USB_AREA_NAME ${PROJECT_NAME}_usb_area)
set(PROJECT_GIGE_AREA_NAME ${PROJECT_NAME}_gige_area)

MESSAGE(STATUS "Build Daheng Usb camera.")

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
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(BOOST_FILESYSTEM_LIB boost_filesystem-vc142-mt-gd-x64-1_83)
else()
    set(BOOST_FILESYSTEM_LIB boost_filesystem-vc142-mt-x64-1_83)
endif()
endif(WIN32)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/ros_msg/rosidl_generator_cpp)

# 生成daheng_usb_area库
add_library(${PROJECT_USB_AREA_LIB_NAME} SHARED
    CCDahengDevice.h
    CCDahengDevice.cpp
    CCDahengCamera.h
    CCDahengCamera.cpp

    CCDahengCameraRosNode.h
    CCDahengCameraRosNode.cpp
    CCDahengMainProcess.h
    CCDahengMainProcess.cpp
    CCDahengCameraImp.h
    CCDahengCameraImp.cpp
)
target_link_libraries(${PROJECT_USB_AREA_LIB_NAME}
    utility
    cc_message
    ros_msg_shm
    cc_log_client
    daheng_camera_sdk
    cc_data_id_validator
    lens_computar_usb
    ${BOOST_FILESYSTEM_LIB}
)
target_compile_definitions(${PROJECT_USB_AREA_LIB_NAME} PRIVATE DRIVER_LIBRARY)
target_compile_definitions(${PROJECT_USB_AREA_LIB_NAME} PRIVATE DAHENG_USB_AREA)

# 生成daheng_gige_area库
add_library(${PROJECT_GIGE_AREA_LIB_NAME} SHARED
    CCDahengDevice.h
    CCDahengDevice.cpp
    CCDahengCamera.h
    CCDahengCamera.cpp

    CCDahengCameraRosNode.h
    CCDahengCameraRosNode.cpp
    CCDahengMainProcess.h
    CCDahengMainProcess.cpp
    CCDahengCameraImp.h
    CCDahengCameraImp.cpp
)
target_link_libraries(${PROJECT_GIGE_AREA_LIB_NAME}
    utility
    cc_message
    ros_msg_shm
    cc_log_client
    daheng_camera_sdk
    cc_data_id_validator
    lens_computar_usb
    ${BOOST_FILESYSTEM_LIB}
)
target_compile_definitions(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE DRIVER_LIBRARY)
target_compile_definitions(${PROJECT_GIGE_AREA_LIB_NAME} PRIVATE DAHENG_GIGE_AREA)

