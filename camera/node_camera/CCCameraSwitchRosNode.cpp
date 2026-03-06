#include "CCCameraSwitchRosNode.h"

using namespace rclcpp;
using namespace ros_msg::msg;

using namespace driver;

CCCameraSwitchRosNode::CCCameraSwitchRosNode(const std::string &nodeName, int transmitType)
    : utility::CCMessageNode(nodeName, transmitType)
{

}

void CCCameraSwitchRosNode::createPublishCameraSwitch(const std::string &topic, uint32_t queueSize)
{
    CCInfo(u8"enter %s, topic : %s", __FUNCTION__, topic.c_str());
    m_publishSwitch = create_publisher<CCDriverSwitch>(topic, queueSize);
}

void CCCameraSwitchRosNode::publishCameraSwitch(const CCDriverSwitch::SharedPtr data)
{
    CCInfo(u8"enter %s， instruction = %d （1-on, 2-off）", __FUNCTION__, data->instruction);
    m_publishSwitch->publish(data);
    CCInfo(u8"leave %s", __FUNCTION__);
}
