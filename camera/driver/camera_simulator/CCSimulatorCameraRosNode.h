/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#ifndef CCSIMULATORCAMERAROSNODE_H
#define CCSIMULATORCAMERAROSNODE_H

#include <map>

#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_modbus_data.hpp"

#include "utility/cc_message/CCMessageNode.hpp"

using namespace rclcpp;
using namespace ros_msg::msg;

class CCSimulatorCameraRosNode : public utility::CCMessageNode
{
public:
    explicit CCSimulatorCameraRosNode(const std::string &, int transmitType);

    void createPublishImage(const std::string &, uint32_t queueSize = 1);
    void publishImage(const ros_msg::msg::CCImage::SharedPtr);

    void createPublishGetImage(const std::string &, uint32_t queueSize = 1);
    void publishGetImage(const ros_msg::msg::CCGetImage::SharedPtr);
    template<typename CallbackT>
    void createSubscribeGetImage(const std::string &topic, CallbackT && callback,
                         uint32_t queueSize = 1);

    void createPublishParam(const std::string &, uint32_t queueSize = 1);
    void publishParam(const ros_msg::msg::CCCameraParam::SharedPtr);
    template<typename CallbackT>
    void createSubscribeSetParam(const std::string &topic, CallbackT && callback,
                              uint32_t queueSize = 1);

private:
    utility::CCPublisher<CCImage>::SharedPtr m_publisherImage;

    utility::CCPublisher<ros_msg::msg::CCGetImage>::SharedPtr m_publisherGetImage;
    utility::CCSubscription<CCGetImage>::SharedPtr m_subscriptionGetImage;

    utility::CCPublisher<CCCameraParam>::SharedPtr m_publisherParam;
    utility::CCSubscription<CCCameraParam>::SharedPtr m_subscriptionSetParam;
};

template<typename CallbackT>
void CCSimulatorCameraRosNode::createSubscribeGetImage(const std::string& topic,
                                     CallbackT && callback,
                                     uint32_t queueSize)
{
    m_subscriptionGetImage = this->create_subscription<CCGetImage>(
        topic, queueSize, callback);
}

template<typename CallbackT>
void CCSimulatorCameraRosNode::createSubscribeSetParam(const std::string &topic, CallbackT && callback,
                             uint32_t queueSize)
{
    m_subscriptionSetParam = this->create_subscription<CCCameraParam>(
        topic, queueSize, callback);
}

#endif // SIMULATORCAMERAROSNODE_H
