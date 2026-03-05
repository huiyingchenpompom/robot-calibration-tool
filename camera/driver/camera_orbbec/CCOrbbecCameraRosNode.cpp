/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#include "CCOrbbecCameraRosNode.h"
#include "LogClientCommon.h"

#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "ros_msg/msg/cc_memory_type.hpp"
#include "ros_msg/msg/cc_msg_reply.hpp"

CCOrbbecCameraRosNode::CCOrbbecCameraRosNode(const std::string &nodeName, int transmitType)
    : CCMessageNode(nodeName, transmitType)
{

}


void CCOrbbecCameraRosNode::createPublishImage(const std::string &topic, uint32_t queueSize)
{
    CCInfo(u8"enter %s", __FUNCTION__);
    m_publisherImage = this->create_publisher<CCImage>(topic, queueSize);
    CCInfo(u8"leave %s", __FUNCTION__);
}

void CCOrbbecCameraRosNode::publishImage(const CCImage::SharedPtr image)
{
    CCInfo(u8"enter %s", __FUNCTION__);
    if (image->memory_type == CCMemoryType::MEM_SHARE) {
        const std::string &topic = m_publisherImage->get_topic_name();
        const auto &nodeNames = this->getSubscriptionsNameByTopic(topic);
        utility::CCRosMsgShm rosMsgShm;
        rosMsgShm.addSubCount(image->shm_index, nodeNames);
    }
    m_publisherImage->publish(image);
    CCInfo(u8"leave %s", __FUNCTION__);
}

void CCOrbbecCameraRosNode::createPublishParam(const std::string &topic, uint32_t queueSize)
{
    CCInfo("Create camera param topic: %s", topic.c_str());
    m_publisherParam = this->create_publisher<CCCameraParam>(topic, queueSize);
}

void CCOrbbecCameraRosNode::publishParam(const ros_msg::msg::CCCameraParam::SharedPtr param)
{
    CCInfo(u8"enter %s", __FUNCTION__);
    m_publisherParam->publish(param);
    CCInfo(u8"leave %s", __FUNCTION__);
}

void CCOrbbecCameraRosNode::createPublishCameraSwitchReply(const std::string &topic, uint32_t queueSize)
{
    CCInfo("enter %s, Create camera switch topic: %s", __FUNCTION__, topic.c_str());
//    m_publisherCameraSwitch = this->create_publisher<CCDriverSwitch>(topic, queueSize);
    m_publisherCameraSwitchReply = this->create_publisher<CCMsgReply>(topic, queueSize);
    CCInfo(u8"leave %s", __FUNCTION__);
}

void CCOrbbecCameraRosNode::publishCameraSwitchReply(const ros_msg::msg::CCMsgReply::SharedPtr data)
{
    CCInfo(u8"enter %s", __FUNCTION__);
    m_publisherCameraSwitchReply->publish(data);
    CCInfo(u8"leave %s", __FUNCTION__);
}
