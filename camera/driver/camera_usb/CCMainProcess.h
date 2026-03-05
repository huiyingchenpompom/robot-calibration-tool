/**
 * usb相机驱动主程序
 *
 */
#ifndef CCMAINPROCESS_H
#define CCMAINPROCESS_H

#include <memory>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>

#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_image.hpp"

class CCUsbCameraRosNode;
class CCCameraIO;

using namespace ros_msg::msg;
using namespace boost::interprocess;

class CCMainProcess
{
public:
    CCMainProcess(const std::string &, const std::string &, const std::string &, int);

    void run();
    std::shared_ptr<rclcpp::Node> rosNode();

protected:
    void onSubscribeGetImage(const CCGetImage::SharedPtr);
    void onSubscribeSetParam(const CCCameraParam::SharedPtr);

private:
    const std::string m_objId;
    CCImage::SharedPtr m_image;
    cv::Mat m_frame;

    shared_memory_object m_sharedMemory;
    mapped_region m_mappedRegion;

    std::shared_ptr<CCUsbCameraRosNode> m_node;
    std::shared_ptr<CCCameraIO> m_camera;
};

#endif // MAINPROCESS_H
