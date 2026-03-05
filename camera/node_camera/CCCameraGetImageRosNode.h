/**
 * ros节点，和相机驱动通过ros交互
 *
 */
#ifndef CCCAMERAGETIMAGEROSNODE_H
#define CCCAMERAGETIMAGEROSNODE_H

#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_modbus_data.hpp"
#include "ros_msg/msg/cc_string.hpp"
#include "ros_msg/msg/cc_driver_switch.hpp"
#include "ros_msg/msg/cc_msg_reply.hpp"
#include "ros_msg/msg/cc_string_array.hpp"

#include "utility/cc_message/CCMessageNode.hpp"

namespace driver
{


class CCCameraGetImageRosNode : public utility::CCMessageNode
{
public:
    explicit CCCameraGetImageRosNode(const std::string &, int transmitType);

    void createPublishGetImage(const std::string &, uint32_t queueSize = 1);
    void publishGetImage(const ros_msg::msg::CCGetImage::SharedPtr);

    void createPublishImageCount(const std::string &, uint32_t queueSize = 1);
    void publishImageCount(const ros_msg::msg::CCModbusData::SharedPtr);

    void createPublishImage(const std::string &, uint32_t queueSize = 1);
    void publishImage(const ros_msg::msg::CCImage::SharedPtr);
    template<typename CallbackT>
    void createSubscribeGetImage(const std::string &topic, const std::string &key, CallbackT && callback,
                         uint32_t queueSize = 1024);

    template<typename CallbackT>
    void createSubscribeCycleId(const std::string &topic, CallbackT && callback,
                               uint32_t queueSize = 1024);
    template<typename CallbackT>
    void createSubscribeImageId(const std::string &topic, CallbackT && callback,
                                uint32_t queueSize = 1024);
    template<typename CallbackT>
    void createSubscribeImageCount(const std::string &topic, CallbackT && callback,
                                   uint32_t queueSize = 1024);

    template<typename CallbackT>
    void createSubscribeProductCode(const std::string &topic, CallbackT && callback,
                                   uint32_t queueSize = 1024);

    template<typename CallbackT>
    void createSubscribeTrajectoryName(const std::string &topic, CallbackT && callback, uint32_t queueSize = 1);
    template<typename CallbackT>
    void createSubscribeParam(const std::string &topic, const std::string &key, CallbackT && callback, uint32_t queueSize = 1);


private:
    utility::CCPublisher<ros_msg::msg::CCImage>::SharedPtr m_publisherImage;
    utility::CCPublisher<ros_msg::msg::CCGetImage>::SharedPtr m_publisherGetImage;
    utility::CCPublisher<ros_msg::msg::CCModbusData>::SharedPtr m_publisherImageCount;

    utility::CCSubscription<ros_msg::msg::CCCameraParam>::SharedPtr m_subscriptionParam;
    utility::CCSubscription<ros_msg::msg::CCImage>::SharedPtr m_subscriptionGetImage;
    utility::CCSubscription<ros_msg::msg::CCModbusData>::SharedPtr m_subscriptionCycleId;
    utility::CCSubscription<ros_msg::msg::CCModbusData>::SharedPtr m_subscriptionImageId;
    utility::CCSubscription<ros_msg::msg::CCModbusData>::SharedPtr m_subscriptionImageCount;
    utility::CCSubscription<ros_msg::msg::CCString>::SharedPtr m_subscriptionProductCode;
    utility::CCSubscription<ros_msg::msg::CCStringArray>::SharedPtr m_subscribeTrajectoryName;

    utility::CCPublisher<ros_msg::msg::CCDriverSwitch>::SharedPtr m_publishCameraSwitch;
    utility::CCSubscription<ros_msg::msg::CCMsgReply>::SharedPtr m_subscribeCameraSwitchReply;
};

template<typename CallbackT>
void CCCameraGetImageRosNode::createSubscribeParam(const std::string &topic, const std::string &key, CallbackT && callback, uint32_t queueSize)
{
    m_subscriptionParam = this->create_subscription<ros_msg::msg::CCCameraParam>(topic, queueSize, callback, key);
}

template<typename CallbackT>
void CCCameraGetImageRosNode::createSubscribeGetImage(const std::string& topic,
                                                      const std::string &key,
                                     CallbackT && callback,
                                     uint32_t queueSize)
{
    m_subscriptionGetImage = this->create_subscription<ros_msg::msg::CCImage>(
        topic, queueSize, callback, key);
}

template<typename CallbackT>
void CCCameraGetImageRosNode::createSubscribeCycleId(const std::string &topic, CallbackT && callback, uint32_t queueSize)
{
    m_subscriptionCycleId = this->create_subscription<ros_msg::msg::CCModbusData>(
        topic, queueSize, callback);
}

template<typename CallbackT>
void CCCameraGetImageRosNode::createSubscribeImageId(const std::string &topic, CallbackT && callback, uint32_t queueSize)
{
    m_subscriptionImageId = this->create_subscription<ros_msg::msg::CCModbusData>(
        topic, queueSize, callback);
}

template<typename CallbackT>
void CCCameraGetImageRosNode::createSubscribeImageCount(const std::string &topic, CallbackT && callback, uint32_t queueSize)
{
    m_subscriptionImageCount = this->create_subscription<ros_msg::msg::CCModbusData>(
        topic, queueSize, callback);
}

template<typename CallbackT>
void CCCameraGetImageRosNode::createSubscribeProductCode(const std::string &topic, CallbackT && callback, uint32_t queueSize)
{
    m_subscriptionProductCode = this->create_subscription<ros_msg::msg::CCString>(
        topic, queueSize, callback);
}

template<typename CallbackT>
void CCCameraGetImageRosNode::createSubscribeTrajectoryName(const std::string &topic, CallbackT && callback, uint32_t queueSize)
{
    m_subscribeTrajectoryName = create_subscription<ros_msg::msg::CCStringArray>(topic, queueSize, callback);
}

}

#endif // CCCAMERAGETIMAGEROSNODE_H
