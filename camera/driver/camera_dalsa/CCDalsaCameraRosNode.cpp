/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#include "CCDalsaCameraRosNode.h"
#include "LogClientCommon.h"

#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "ros_msg/msg/cc_memory_type.hpp"

CCDalsaCameraRosNode::CCDalsaCameraRosNode(const std::string &nodeName, int transmitType)
    : CCMessageNode(nodeName, transmitType)
{

}

void CCDalsaCameraRosNode::createPublishImage(const std::string &topic, uint32_t queueSize)
{
    CCInfo("Create image data topic: %s", topic.c_str());
    m_publisherImage = this->create_publisher<CCImage>(topic, queueSize);
}

void CCDalsaCameraRosNode::publishImage(const CCImage::SharedPtr image)
{
    if (image->memory_type == CCMemoryType::MEM_SHARE) {
        const std::string &topic = m_publisherImage->get_topic_name();
        const auto &nodeNames = this->getSubscriptionsNameByTopic(topic);
        utility::CCRosMsgShm rosMsgShm;
        rosMsgShm.addSubCount(image->shm_index, nodeNames);
    }
    m_publisherImage->publish(image);
}

void CCDalsaCameraRosNode::createPublishParam(const std::string &topic, uint32_t queueSize)
{
    CCInfo("Create camera param topic: %s", topic.c_str());
    m_publisherParam = this->create_publisher<CCCameraParam>(topic, queueSize);
}

void CCDalsaCameraRosNode::publishParam(const ros_msg::msg::CCCameraParam::SharedPtr param)
{
    m_publisherParam->publish(param);
}
