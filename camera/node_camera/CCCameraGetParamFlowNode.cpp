#include "CCCameraGetParamFlowNode.h"
#include <nlohmann/json.hpp>

#include "CCCameraGetParamRosNode.h"
#include "utility/core/device/CCDeviceManage.h"
#include "node/camera/CCCameraTopicName.hpp"
#include "LogClientCommon.h"

using std::placeholders::_1;
using namespace driver;
using namespace utility;
using namespace ros_msg::msg;
using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

CCFlowNodeType CCCameraGetParamFlowNode::ms_type = CCFlowNodeType{"camera_get_param", "获取相机参数"};

CCCameraGetParamFlowNode::CCCameraGetParamFlowNode(const std::string &id, const std::string &name)
    : CCFlowNode(id, name)
{
    m_transmitType = utility::driverTransmitType();
}

CCCameraGetParamFlowNode::~CCCameraGetParamFlowNode()
{
}

int CCCameraGetParamFlowNode::start()
{
    return RET_SUCCESS;
}

void CCCameraGetParamFlowNode::stop()
{
}

void CCCameraGetParamFlowNode::run()
{
    CCDebug("publish get camera data start...");
    auto rosNode = std::dynamic_pointer_cast<CCCameraGetParamRosNode>(m_rosNode);
    rosNode->publishGetParam(m_param);
}

int CCCameraGetParamFlowNode::parseJsonValue(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam)
{
    utility::CCDeviceType devType;
    utility::CCDeviceObject devObject;
    try {
        std::string deviceId = jsonDevice.at("id");
        CCDeviceManage::instance().getDeviceObj(deviceId, devType, devObject);
        if (devType.mainType.empty()) {
            CCError(u8"节点%s设备[%s]不存在", u8"请确定节点设备配置是否正确", m_nodeId.c_str(), deviceId.data());
            return RET_FAILED;
        }

        m_param->params.clear();
        m_param->values.clear();
        for (const auto &objItem : jsonNodeParam.items()) {
            auto jsonObjItem = objItem.value();
            m_param->params.push_back(jsonObjItem.at("index"));
            m_param->values.push_back(jsonObjItem.at("value"));
        }
    } catch (ordered_json::exception &e) {
        CCError("流程节点%s参数解析失败: %s", u8"请检查对应节点参数设置", this->id().data(), e.what());
        return RET_FAILED;
    }

    std::string nodeName = utility::ccRosNodeName(ms_type.type, m_nodeId);
//    nodeName = "node_" + nodeName + "_get_param";
    auto rosNode = std::make_shared<CCCameraGetParamRosNode>(nodeName, m_transmitType);
    rosNode->setTopicTransmitType(m_topicTransmitType);
    m_rosNode = rosNode;

    std::string topicName = driver::ccRosTopicNameGetParam(devType, devObject.deviceObjId);
    rosNode->createPublishGetParam(topicName, 10);
    topicName = driver::ccRosTopicNameGetParamData(devType, devObject.deviceObjId);
    rosNode->createSubscribeParam(topicName, m_nodeId, std::bind(&CCCameraGetParamFlowNode::onSubscribeParam, this, _1), 1024);

    return RET_SUCCESS;
}

CCFlowNodeType CCCameraGetParamFlowNode::type()
{
    return ms_type;
}

void CCCameraGetParamFlowNode::onSubscribeParam(const CCCameraParam::SharedPtr)
{
    CCDebug("subscribe data end....");
}
