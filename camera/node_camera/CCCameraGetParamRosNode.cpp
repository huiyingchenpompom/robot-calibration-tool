/**
 * ros节点，和相机驱动通过ros交互
 *
 */
#include "CCCameraGetParamRosNode.h"
#include "LogClientCommon.h"

using namespace driver;
using namespace rclcpp;
using namespace ros_msg::msg;

CCCameraGetParamRosNode::CCCameraGetParamRosNode(const std::string &nodeName, int transmitType)
    : CCMessageNode(nodeName, transmitType)
{

}

void CCCameraGetParamRosNode::createPublishGetParam(const std::string &topic, uint32_t queueSize)
{
    m_publisherGetParam = this->create_publisher<CCCameraParam>(topic, queueSize);
}

void CCCameraGetParamRosNode::publishGetParam(const CCCameraParam::SharedPtr param)
{
    CCDebug("publishGetParam enter.");
    m_publisherGetParam->publish(param);
    CCDebug("publishGetParam end.");
}
