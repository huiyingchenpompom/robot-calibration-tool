/**
 * ros节点，和相机驱动通过ros交互
 *
 */
#ifndef CCCAMERASETPARAMROSNODE_H
#define CCCAMERASETPARAMROSNODE_H

#include "ros_msg/msg/cc_modbus_data.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "utility/cc_message/CCMessageNode.hpp"

namespace driver
{


class CCCameraSetParamRosNode : public utility::CCMessageNode
{
public:
    explicit CCCameraSetParamRosNode(const std::string &, int transmitType);

    void createPublishSetParam(const std::string &, uint32_t queueSize = 1);
    void publishSetParam(const ros_msg::msg::CCCameraParam::SharedPtr);
    template<typename CallbackT>
    void createSubscribeParam(const std::string &topic, const std::string &key, CallbackT && callback,
                              uint32_t queueSize = 1);
    template<typename CallbackT>
    void createSubscribeImageId(const std::string &topic, CallbackT && callback,
                                uint32_t queueSize = 1024);

private:
    utility::CCPublisher<ros_msg::msg::CCCameraParam>::SharedPtr m_publisherSetParam;
    utility::CCSubscription<ros_msg::msg::CCCameraParam>::SharedPtr m_subscriptionParam;
    utility::CCSubscription<ros_msg::msg::CCModbusData>::SharedPtr m_subscriptionImageId;
};


template<typename CallbackT>
void CCCameraSetParamRosNode::createSubscribeParam(const std::string &topic, const std::string &key, CallbackT && callback,
                                         uint32_t queueSize)
{
    m_subscriptionParam = this->create_subscription<ros_msg::msg::CCCameraParam>(
        topic, queueSize, callback, key);
}

template<typename CallbackT>
void CCCameraSetParamRosNode::createSubscribeImageId(const std::string &topic, CallbackT && callback, uint32_t queueSize)
{
    m_subscriptionImageId = this->create_subscription<ros_msg::msg::CCModbusData>(
        topic, queueSize, callback);
}

}

#endif // CCCAMERASETPARAMROSNODE_H
