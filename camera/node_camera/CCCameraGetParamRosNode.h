/**
 * ros节点，和相机驱动通过ros交互
 *
 */
#ifndef CCCAMERAGETPARAMROSNODE_H
#define CCCAMERAGETPARAMROSNODE_H

#include "ros_msg/msg/cc_camera_param.hpp"
#include "utility/cc_message/CCMessageNode.hpp"

namespace driver
{


class CCCameraGetParamRosNode : public utility::CCMessageNode
{
public:
    explicit CCCameraGetParamRosNode(const std::string &, int transmitType);

    void createPublishGetParam(const std::string &, uint32_t queueSize = 1);
    void publishGetParam(const ros_msg::msg::CCCameraParam::SharedPtr);
    template<typename CallbackT>
    void createSubscribeParam(const std::string &topic, const std::string &key, CallbackT && callback, uint32_t queueSize = 1);

private:
    utility::CCPublisher<ros_msg::msg::CCCameraParam>::SharedPtr m_publisherGetParam;
    utility::CCSubscription<ros_msg::msg::CCCameraParam>::SharedPtr m_subscriptionParam;
};


template<typename CallbackT>
void CCCameraGetParamRosNode::createSubscribeParam(const std::string &topic, const std::string &key, CallbackT && callback, uint32_t queueSize)
{
    m_subscriptionParam = this->create_subscription<ros_msg::msg::CCCameraParam>(topic, queueSize, callback, key);
}

}

#endif // CCCAMERAGETPARAMROSNODE_H
