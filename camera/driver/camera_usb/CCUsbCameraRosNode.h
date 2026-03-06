/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#ifndef CCUSBCAMERAROSNODE_H
#define CCUSBCAMERAROSNODE_H

#include <map>
#include <rclcpp/rclcpp.hpp>

#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"

using namespace rclcpp;
using namespace ros_msg::msg;

class CCUsbCameraRosNode : public rclcpp::Node
{
public:
    explicit CCUsbCameraRosNode(const std::string &);

    void createPublishImage(const std::string &, uint32_t queueSize = 1);
    void publishImage(const ros_msg::msg::CCImage::SharedPtr);
    template<typename CallbackT>
    void createSubscribeGetImage(const std::string &topic, CallbackT && callback,
                         uint32_t queueSize = 1);

    void createPublishParam(const std::string &, uint32_t queueSize = 1);
    void publishParam(const ros_msg::msg::CCCameraParam::SharedPtr);
    template<typename CallbackT>
    void createSubscribeSetParam(const std::string &topic, CallbackT && callback,
                              uint32_t queueSize = 1);

private:
    Publisher<CCImage>::SharedPtr m_publisherImage;
    Subscription<CCGetImage>::SharedPtr m_subscriptionGetImage;

    Publisher<CCCameraParam>::SharedPtr m_publisherParam;
    Subscription<CCCameraParam>::SharedPtr m_subscriptionSetParam;
};

template<typename CallbackT>
void CCUsbCameraRosNode::createSubscribeGetImage(const std::string& topic,
                                     CallbackT && callback,
                                     uint32_t queueSize)
{
    m_subscriptionGetImage = this->create_subscription<CCGetImage>(
        topic, queueSize, callback);
}

template<typename CallbackT>
void CCUsbCameraRosNode::createSubscribeSetParam(const std::string &topic, CallbackT && callback,
                             uint32_t queueSize)
{
    m_subscriptionSetParam = this->create_subscription<CCCameraParam>(
        topic, queueSize, callback);
}

#endif // CCUSBCAMERAROSNODE_H
