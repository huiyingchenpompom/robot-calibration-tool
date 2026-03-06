/**
 * ros节点，和相机驱动通过ros交互
 *
 */
#include "CCCameraGetImageRosNode.h"
#include <opencv2/opencv.hpp>
#include "LogClientCommon.h"
#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "ros_msg/msg/cc_memory_type.hpp"

using namespace driver;
using namespace rclcpp;
using namespace ros_msg::msg;

CCCameraGetImageRosNode::CCCameraGetImageRosNode(const std::string &nodeName, int transmitType)
    : utility::CCMessageNode(nodeName, transmitType)
{

}

void CCCameraGetImageRosNode::createPublishGetImage(const std::string &topic, uint32_t queueSize)
{
    m_publisherGetImage = this->create_publisher<CCGetImage>(topic, queueSize);
}

void CCCameraGetImageRosNode::publishGetImage(const CCGetImage::SharedPtr getImage)
{
    CCInfo("publishGetImage enter.");
    m_publisherGetImage->publish(getImage);
    CCInfo("publishGetImage exit.");
}

void CCCameraGetImageRosNode::createPublishImageCount(const std::string &topic, uint32_t queueSize)
{
    m_publisherImageCount = this->create_publisher<CCModbusData>(topic, queueSize);
}

void CCCameraGetImageRosNode::publishImageCount(const ros_msg::msg::CCModbusData::SharedPtr count)
{
    m_publisherImageCount->publish(count);
}

void CCCameraGetImageRosNode::createPublishImage(const std::string &topic, uint32_t queueSize)
{
    m_publisherImage = this->create_publisher<CCImage>(topic, queueSize);
}

void CCCameraGetImageRosNode::publishImage(const CCImage::SharedPtr image)
{
    CCInfo("publishImage enter.");
    if (image->memory_type == ros_msg::msg::CCMemoryType::MEM_SHARE) {
        const std::string &topic = m_publisherImage->get_topic_name();
        std::list<std::string> nodeNames = this->getSubscriptionsNameByTopic(topic);
        utility::CCRosMsgShm rosMsgShm;
        rosMsgShm.addSubCount(image->shm_index, nodeNames);
    }
    m_publisherImage->publish(image);
    CCInfo("publishImage exit.");
}
