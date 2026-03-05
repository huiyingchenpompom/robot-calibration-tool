/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#ifndef CC_ORBBEC_CAMERA_ROSNODE_H
#define CC_ORBBEC_CAMERA_ROSNODE_H

#include <map>

#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_driver_switch.hpp"
#include "ros_msg/msg/cc_msg_reply.hpp"

#include "utility/cc_message/CCMessageNode.hpp"

using namespace rclcpp;
using namespace ros_msg::msg;

class CCOrbbecCameraRosNode : public utility::CCMessageNode
{
public:
    explicit CCOrbbecCameraRosNode(const std::string &, int transmitType);

    void createPublishImage(const std::string &, uint32_t queueSize = 1);
    void publishImage(const CCImage::SharedPtr);
    template<typename CallbackT>
    void     createSubscribeGetImage(const std::string &topic, CallbackT && callback, uint32_t queueSize = 1);

    void     createPublishParam(const std::string &, uint32_t queueSize = 1);
    void     publishParam(const ros_msg::msg::CCCameraParam::SharedPtr);
    template<typename CallbackT>
    void     createSubscribeSetParam( const std::string &topic, CallbackT && callback, uint32_t queueSize = 1);

    void createPublishCameraSwitchReply(const std::string &topic, uint32_t queueSize = 1);
    void publishCameraSwitchReply(const ros_msg::msg::CCMsgReply::SharedPtr data);
    template<typename CallbackT>
    void createSubscribeCameraReply(const std::string &topic, CallbackT && callback, uint32_t queueSize = 1);

private:
    utility::CCPublisher<CCImage>::SharedPtr m_publisherImage;
    utility::CCSubscription<CCGetImage>::SharedPtr    m_subscriptionGetImage;

    utility::CCPublisher<CCCameraParam>::SharedPtr    m_publisherParam;
    utility::CCSubscription<CCCameraParam>::SharedPtr m_subscriptionSetParam;

    utility::CCPublisher<CCMsgReply>::SharedPtr    m_publisherCameraSwitchReply;
    utility::CCSubscription<CCDriverSwitch>::SharedPtr m_subscriptionCameraSwitch;
};

template<typename CallbackT>
void CCOrbbecCameraRosNode::createSubscribeGetImage(  const std::string& topic
                                                     , CallbackT && callback
                                                     , uint32_t queueSize)
{
    m_subscriptionGetImage = this->create_subscription<CCGetImage>(  topic
                                                                   , queueSize
                                                                   , callback);
}

template<typename CallbackT>
void CCOrbbecCameraRosNode::createSubscribeSetParam( const std::string &topic
                                                     , CallbackT && callback
                                                     , uint32_t queueSize)
{
    m_subscriptionSetParam = this->create_subscription<CCCameraParam>(
        topic, queueSize, callback);
}

template<typename CallbackT>
void CCOrbbecCameraRosNode::createSubscribeCameraReply( const std::string &topic
                                                     , CallbackT && callback
                                                     , uint32_t queueSize)
{
    CCInfo(u8"enter %s, topic : %s", __FUNCTION__, topic.c_str());
    m_subscriptionCameraSwitch = this->create_subscription<CCDriverSwitch>(
        topic, queueSize, callback);
}

#endif // CC_ORBBEC_CAMERA_ROSNODE_H
