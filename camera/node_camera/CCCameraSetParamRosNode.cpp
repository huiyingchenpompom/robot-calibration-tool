/**
 * ros节点，和相机驱动通过ros交互
 *
 */
#include "CCCameraSetParamRosNode.h"
#include "LogClientCommon.h"

#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "ros_msg/msg/cc_memory_type.hpp"

using namespace driver;
using namespace rclcpp;
using namespace ros_msg::msg;


CCCameraSetParamRosNode::CCCameraSetParamRosNode(const std::string &nodeName, int transmitType)
    : CCMessageNode(nodeName, transmitType)
{

}

void CCCameraSetParamRosNode::createPublishSetParam(const std::string &topic, uint32_t queueSize)
{
    m_publisherSetParam = this->create_publisher<CCCameraParam>(topic, queueSize);
}

void CCCameraSetParamRosNode::publishSetParam(const CCCameraParam::SharedPtr param)
{
    CCDebug("publishSetParam enter.");
    m_publisherSetParam->publish(param);
    CCDebug("publishSetParam exit.");
}
