/**
 * usb相机驱动主程序
 *
 */
#ifndef CCMAINPROCESS_H
#define CCMAINPROCESS_H

#include <memory>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>

#include "utility/core/CCThreadList.hpp"
#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_modbus_data.hpp"

class CCSimulatorCameraRosNode;
class CCSimulatorCameraCameraIO;
class CCSimulatorCameraUdpServer;

namespace utility
{
class CCMessageNode;
class CCEventCondVar;
}

using namespace ros_msg::msg;

class CCSimulatorCameraMainProcess
{
public:
    CCSimulatorCameraMainProcess(const std::string &, const std::string &, const std::string &, int, int, int transmitType, const std::string &imagePath);
    ~CCSimulatorCameraMainProcess();

    void run();
    std::shared_ptr<utility::CCMessageNode> rosNode();

protected:
    void onSubscribeGetImage(const CCGetImage::SharedPtr);
    void onSubscribeSetParam(const CCCameraParam::SharedPtr);
    void onGrabImage(const CCGetImage::SharedPtr);
    void doHardGetImage(const std::string &text);
    void doSoftGetImage();

private:
    const std::string m_objId;
    int m_cameraId;
    int32_t m_triggerType{0};
    std::map<uint32_t, int32_t> m_cameraImageIndex;

    utility::CCThreadList<CCGetImage::SharedPtr> m_getImageList;
//    CCGetImage::SharedPtr m_getImage{nullptr};
    CCCameraParam::SharedPtr m_param;
//    CCImage::SharedPtr m_image;
    cv::Mat m_frame;

    std::shared_ptr<CCSimulatorCameraRosNode> m_node;
    std::shared_ptr<CCSimulatorCameraCameraIO> m_camera;

    boost::asio::io_context m_context;
    std::shared_ptr<CCSimulatorCameraUdpServer> m_server;
    std::thread m_thread;   // udp server run thread
    std::thread m_threadSoftGetImage; // 软触发取图

    std::unique_ptr<utility::CCEventCondVar> m_eventCv;
};

#endif // CCMAINPROCESS_H
