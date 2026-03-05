/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#include "CCBaslerCameraRosNode.h"
#include "LogClientCommon.h"

#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "ros_msg/msg/cc_memory_type.hpp"

CCBaslerCameraRosNode::CCBaslerCameraRosNode(const std::string &nodeName, int transmitType)
    : CCMessageNode(nodeName, transmitType)
{

}

void CCBaslerCameraRosNode::createPublishImage(const std::string &topic, uint32_t queueSize)
{
    m_publisherImage = this->create_publisher<CCImage>(topic, queueSize);
}

void CCBaslerCameraRosNode::publishImage(const CCImage::SharedPtr image)
{
    if (image->memory_type == CCMemoryType::MEM_SHARE) {
        const std::string &topic = m_publisherImage->get_topic_name();
        const auto &nodeNames = this->getSubscriptionsNameByTopic(topic, image->header.node_id);
        utility::CCRosMsgShm rosMsgShm;
        rosMsgShm.addSubCount(image->shm_index, nodeNames);
    }
    m_publisherImage->publish2(image, image->header.node_id);
}

void CCBaslerCameraRosNode::createPublishParam(const std::string &topic, uint32_t queueSize)
{
    CCInfo("Create camera param topic: %s", topic.c_str());
    m_publisherParam = this->create_publisher<CCCameraParam>(topic, queueSize);
}

void CCBaslerCameraRosNode::publishParam(const ros_msg::msg::CCCameraParam::SharedPtr param)
{
    m_publisherParam->publish2(param, param->header.node_id);
}
