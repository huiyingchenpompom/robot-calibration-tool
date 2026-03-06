/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#include "CCSimulatorCameraRosNode.h"
#include "LogClientCommon.h"

#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "ros_msg/msg/cc_memory_type.hpp"

CCSimulatorCameraRosNode::CCSimulatorCameraRosNode(const std::string &nodeName, int transmitType)
    : CCMessageNode(nodeName, transmitType)
{

}

void CCSimulatorCameraRosNode::createPublishImage(const std::string &topic, uint32_t queueSize)
{
    CCDebug("createPublishImage enter topic: %s, %d", topic.c_str(), queueSize);
    m_publisherImage = this->create_publisher<CCImage>(topic, queueSize);
    CCDebug("createPublishImage exit");
}

void CCSimulatorCameraRosNode::publishImage(const CCImage::SharedPtr image)
{
    if (image->memory_type == CCMemoryType::MEM_SHARE) {
        const std::string &topic = m_publisherImage->get_topic_name();
        std::list<std::string> nodeNames = this->getSubscriptionsNameByTopic(topic, image->header.node_id);
        utility::CCRosMsgShm rosMsgShm;
        rosMsgShm.addSubCount(image->shm_index, nodeNames);
    }
    m_publisherImage->publish2(image, image->header.node_id);
}

void CCSimulatorCameraRosNode::createPublishGetImage(const std::string &topic, uint32_t queueSize)
{
    m_publisherGetImage = this->create_publisher<CCGetImage>(topic, queueSize);
}

void CCSimulatorCameraRosNode::publishGetImage(const CCGetImage::SharedPtr getImage)
{
    m_publisherGetImage->publish2(getImage, getImage->header.node_id);
}

void CCSimulatorCameraRosNode::createPublishParam(const std::string &topic, uint32_t queueSize)
{
    m_publisherParam = this->create_publisher<CCCameraParam>(topic, queueSize);
}

void CCSimulatorCameraRosNode::publishParam(const ros_msg::msg::CCCameraParam::SharedPtr param)
{
    m_publisherParam->publish2(param, param->header.node_id);
}
