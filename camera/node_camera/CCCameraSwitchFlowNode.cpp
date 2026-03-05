#include "CCCameraSwitchFlowNode.h"
#include "CCCameraSwitchRosNode.h"
#include <nlohmann/json.hpp>
#include "utility/core/device/CCDeviceManage.h"
#include "LogClientCommon.h"
#include "utility/core/CCRosName.hpp"
#include "utility/core/event/CCEventCondVar.h"
#include "node/camera/CCCameraTopicName.hpp"

using json = nlohmann::json;
using std::placeholders::_1;
using namespace utility;
using namespace driver;
using namespace ros_msg::msg;

const utility::CCFlowNodeType driver::CCCameraSwitchFlowNode::ms_type = {"camera_switch", u8"相机控制切换"};

CCCameraSwitchFlowNode::CCCameraSwitchFlowNode(const std::string &id, const std::string &name)
    : utility::CCFlowNode(id, name)
{
    m_transmitType = utility::driverTransmitType();
    m_driverSwitch = std::make_shared<CCDriverSwitch>();
    m_driverSwitch->header.set__node_id(id);
}

CCCameraSwitchFlowNode::~CCCameraSwitchFlowNode()
{

}

void CCCameraSwitchFlowNode::run()
{
    CCInfo(u8"xxx publish camera switch data start...");
    auto rosNode = std::dynamic_pointer_cast<CCCameraSwitchRosNode>(m_rosNode);
    rosNode->publishCameraSwitch(m_driverSwitch);

    m_eventCv->sleep();
    CCInfo(u8"xxx publish camera switch data end...");
}

utility::CCFlowNodeType CCCameraSwitchFlowNode::type()
{
    CCInfo(u8"get cameraSwitchFlowNode type : %s - %s", ms_type.type.c_str(), ms_type.typeName.c_str());
    return ms_type;
}

int CCCameraSwitchFlowNode::createOutput(const ordered_json &, const ordered_json &)
{
    return RET_SUCCESS;
}

int CCCameraSwitchFlowNode::parseJsonValue(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam)
{
    CCInfo(u8"enter %s", __FUNCTION__);

    utility::CCDeviceType devType;
    utility::CCDeviceObject devObject;
    try{
        std::string deviceId = jsonDevice.at("id");
        CCDeviceManage::instance().getDeviceObj(deviceId, devType, devObject);
        if (devType.mainType.empty()) {
            CCError(u8"%s设备id: %s不存在", u8"请检查节点设备配置", m_nodeId.c_str(), deviceId.c_str());
            return RET_FAILED;
        }
        m_driverSwitch->header.set__device_id(deviceId);

        // instruction
        m_driverSwitch->instruction = jsonNodeParam.at("instruction");
    } catch (ordered_json::exception &e) {
        CCError(u8"流程节点%s参数解析失败: %s", "请联系SC开发人员检查ST上的节点参数字段是否与SC一致", this->id().data(), e.what());
        return RET_FAILED;
    }

    std::string nodeName = utility::ccRosNodeName(ms_type.type, m_nodeId);
//    nodeName = "node" + nodeName + "_" + this->id() + "_dataflow";
    auto rosNode = std::make_shared<CCCameraSwitchRosNode>(nodeName, m_transmitType);
    rosNode->setTopicTransmitType(m_topicTransmitType);
    m_rosNode = rosNode;

    // create publish
    std::string topicName = driver::ccRosTopicNameCameraSwitch(devType, devObject.deviceObjId);
    rosNode->createPublishCameraSwitch(topicName, 1024);

    // create subscribe of reply
    topicName = driver::ccRosTopicNameCameraSwitchReply(devType, devObject.deviceObjId);
    rosNode->createSubscribeCameraReply(topicName, m_nodeId, std::bind(&CCCameraSwitchFlowNode::onSubscribeCameraReply, this, _1), 1024);


    CCInfo(u8"leave %s", __FUNCTION__);

    return RET_SUCCESS;
}

void CCCameraSwitchFlowNode::onSubscribeCameraReply(const CCMsgReply::SharedPtr reply)
{
    CCInfo(u8"enter %s", __FUNCTION__);
    if(reply->header.node_id == m_nodeId){
        m_eventCv->wakeup(true);
    }
    CCInfo(u8"leave %s", __FUNCTION__);
}
