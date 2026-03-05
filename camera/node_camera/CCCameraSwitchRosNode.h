/**
* @author wujitao
* @date   2023-11-9
* @brief  此文件为Camera_Base_Info_Node的ROS节点类
*/
#ifndef CCCAMERASWITCHROSNODE_H
#define CCCAMERASWITCHROSNODE_H

#include "ros_msg/msg/cc_driver_switch.hpp"
#include "ros_msg/msg/cc_msg_reply.hpp"
#include "utility/cc_message/CCMessageNode.hpp"

namespace driver {

class CCCameraSwitchRosNode : public utility::CCMessageNode
{
public:
    explicit CCCameraSwitchRosNode(const std::string &nodeName, int transmitType);
    void createPublishCameraSwitch(const std::string &topic, uint32_t queueSize = 1);
    void publishCameraSwitch(const ros_msg::msg::CCDriverSwitch::SharedPtr data);
//    void publishCameraSwitch(const ros_msg::msg::CCMsgReply::SharedPtr data);

    template<typename CallbackT>
    void createSubscribeCameraReply(const std::string &topic, const std::string &key, CallbackT && callback, uint32_t queueSize = 1);

private:
    utility::CCPublisher<ros_msg::msg::CCDriverSwitch>::SharedPtr m_publishSwitch;
    utility::CCSubscription<ros_msg::msg::CCMsgReply>::SharedPtr m_subscribeReply;
};

template<typename CallbackT>
void CCCameraSwitchRosNode::createSubscribeCameraReply(const std::string &topic, const std::string &key, CallbackT && callback, uint32_t queueSize)
{
    CCInfo(u8"enter %s, topic : %s", __FUNCTION__, topic.c_str());
    m_subscribeReply = create_subscription<ros_msg::msg::CCMsgReply>(topic, queueSize, callback, key);
}

}

#endif // CCCAMERASWITCHROSNODE_H
