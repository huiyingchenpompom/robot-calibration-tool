/**
 * ros节点，和检测流程中的ros节点通过ros交互
 *
 */
#include "CCUsbCameraRosNode.h"

CCUsbCameraRosNode::CCUsbCameraRosNode(const std::string &nodeName)
    : Node(nodeName, rclcpp::NodeOptions().use_intra_process_comms(true))
{

}

void CCUsbCameraRosNode::createPublishImage(const std::string &topic, uint32_t queueSize)
{
    m_publisherImage = this->create_publisher<CCImage>(topic, queueSize);
}

void CCUsbCameraRosNode::publishImage(const CCImage::SharedPtr image)
{
    m_publisherImage->publish(*image);
}

void CCUsbCameraRosNode::createPublishParam(const std::string &topic, uint32_t queueSize)
{
    m_publisherParam = this->create_publisher<CCCameraParam>(topic, queueSize);
}

void CCUsbCameraRosNode::publishParam(const ros_msg::msg::CCCameraParam::SharedPtr param)
{
    m_publisherParam->publish(*param);
}
