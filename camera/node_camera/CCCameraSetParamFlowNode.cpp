/**
 * 流程设置相机参数节点
 *
 */
#include "CCCameraSetParamFlowNode.h"

#include <nlohmann/json.hpp>

#include "CCCameraSetParamRosNode.h"
#include "utility/utility/CCTime.hpp"
#include "utility/core/CCUtilityDefine.h"
#include "utility/core/device/CCDeviceManage.h"
#include "utility/core/event/CCEventCondVar.h"
#include "node/camera/CCCameraTopicName.hpp"
#include "LogClientCommon.h"

using std::placeholders::_1;
using namespace driver;
using namespace utility;
using namespace ros_msg::msg;
using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

CCFlowNodeType CCCameraSetParamFlowNode::ms_type = CCFlowNodeType{"camera_set_param", "设置相机参数"};

CCCameraSetParamFlowNode::CCCameraSetParamFlowNode(const std::string &id, const std::string &name)
    : CCFlowNode(id, name)
{
    m_transmitType = utility::driverTransmitType();
}

CCCameraSetParamFlowNode::~CCCameraSetParamFlowNode()
{

}

int CCCameraSetParamFlowNode::start()
{
    return RET_SUCCESS;
}

void CCCameraSetParamFlowNode::stop()
{
    m_eventCv->wakeup(true);
}

void CCCameraSetParamFlowNode::run()
{
    CCDebug("publish set camera(%s) data start...", m_nodeId.c_str());
    // 判断是否收到所有输入
    int32_t index = m_eventCv->wait();
    if (index > 0) {
        CCError(u8"%s未接收所有输入数据,index:%d", u8"请检查流程是否正确", m_nodeId.c_str(), index);
    }
    CCDebug("%s run publish set params input: %d", m_nodeId.c_str(), m_imageId);

    auto rosNode = std::dynamic_pointer_cast<CCCameraSetParamRosNode>(m_rosNode);
    CCCameraParam setParams;
    if (m_param.count(m_imageId) == 0) {
        setParams.params.emplace_back(CCCameraParam::PARAM_EXPOSURE);
        setParams.values.emplace_back(-1);
        setParams.params.emplace_back(CCCameraParam::PARAM_GAMMA);
        setParams.values.emplace_back(-1);
        setParams.params.emplace_back(CCCameraParam::PARAM_GAIN);
        setParams.values.emplace_back(-1);
        CCError(u8"%s未找到相机图号%d的相关节点参数", u8"1.请检查图号是否正确 2.若图号不正确请检查流程中图号输入 3.若图号正确请检查节点参数是否配置", m_nodeId.c_str(), m_imageId);
    } else {
        setParams = m_param[m_imageId];
    }

    CCCameraParam::SharedPtr param = std::make_shared<CCCameraParam>(setParams);
    param->header.set__time(utility::ccNow());
    param->header.set__node_id(m_nodeId);
    rosNode->publishSetParam(param);
    m_eventCv->sleep();
    CCDebug("publish set camera(%s) data exit...", m_nodeId.c_str());
}

int CCCameraSetParamFlowNode::parseJsonValue(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam)
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

        if (jsonNodeParam.contains("camera_param")) {
            const auto &jsonParam = jsonNodeParam.at("camera_param");
            for (const auto &item : jsonParam) {
                if (item.empty()) {
                    continue;
                }

                int64_t imageId = item.at("camera_image_id");
                CCCameraParam param;
                if (item.contains("exposure")) {
                    param.params.push_back(CCCameraParam::PARAM_EXPOSURE);
                    param.values.push_back(item.at("exposure"));
                }
                if (item.contains("gamma")) {
                    param.params.push_back(CCCameraParam::PARAM_GAMMA);
                    param.values.push_back(item.at("gamma"));
                }
                if (item.contains("gain")) {
                    param.params.push_back(CCCameraParam::PARAM_GAIN);
                    param.values.push_back(item.at("gain"));
                }
                if (item.contains("preamp_gain")) {
                    param.params.push_back(CCCameraParam::PARAM_PREAMP_GAIN);
                    param.values.push_back(item.at("preamp_gain"));
                }
                if (item.contains("line_rate")) {
                    param.params.push_back(CCCameraParam::PARAM_LINERATE);
                    param.values.push_back(item.at("line_rate"));
                }
                if (item.contains("offset_x")) {
                    param.params.push_back(CCCameraParam::PARAM_OFFSETX);
                    param.values.push_back(item.at("offset_x"));
                }
                if (item.contains("offset_y")) {
                    param.params.push_back(CCCameraParam::PARAM_OFFSETY);
                    param.values.push_back(item.at("offset_y"));
                }
                if (item.contains("width")) {
                    param.params.push_back(CCCameraParam::PARAM_WIDTH);
                    param.values.push_back(item.at("width"));
                }
                if (item.contains("height")) {
                    param.params.push_back(CCCameraParam::PARAM_HEIGHT);
                    param.values.push_back(item.at("height"));
                }
                if (item.contains("focal_length_step") && !item.at("focal_length_step").is_null()) {
                    param.params.push_back(CCCameraParam::PARAM_FOCUS);
                    param.values.push_back(item.at("focal_length_step"));
                }
                if (item.contains("aperture_step") && !item.at("aperture_step").is_null()) {
                    param.params.push_back(CCCameraParam::PARAM_IRIS);
                    param.values.push_back(item.at("aperture_step"));
                }
                m_param[imageId] = param;
            }
        } else {
            CCCameraParam param;
            param.params.push_back(CCCameraParam::PARAM_EXPOSURE);
            param.values.push_back(jsonNodeParam.at("exposure"));
            param.params.push_back(CCCameraParam::PARAM_GAMMA);
            param.values.push_back(jsonNodeParam.at("gamma"));
            param.params.push_back(CCCameraParam::PARAM_GAIN);
            param.values.push_back(jsonNodeParam.at("gain"));
            if (jsonNodeParam.contains("preamp_gain")) {
                param.params.push_back(CCCameraParam::PARAM_PREAMP_GAIN);
                param.values.push_back(jsonNodeParam.at("preamp_gain"));
            }
            if (jsonNodeParam.contains("line_rate")) {
                param.params.push_back(CCCameraParam::PARAM_LINERATE);
                param.values.push_back(jsonNodeParam.at("line_rate"));
            }
            if (jsonNodeParam.contains("focal_length_step") && !jsonNodeParam.at("focal_length_step").is_null()) {
                param.params.push_back(CCCameraParam::PARAM_FOCUS);
                param.values.push_back(jsonNodeParam.at("focal_length_step"));
            }
            if (jsonNodeParam.contains("aperture_step") && !jsonNodeParam.at("aperture_step").is_null()) {
                param.params.push_back(CCCameraParam::PARAM_IRIS);
                param.values.push_back(jsonNodeParam.at("aperture_step"));
            }
            m_param[0] = param;
        }

        //  附加参数
        if (jsonNodeParam.contains("camera_param_feature")) {
            const auto &jsonCameraParams = jsonNodeParam.at("camera_param_feature");
            for (const auto &jsonParamItem : jsonCameraParams) {
                if (jsonParamItem.empty()) {
                    continue;
                }
                // parse
                int64_t imageId = jsonParamItem.at("camera_image_id_feature");
                const std::string featureName = jsonParamItem.at("feature_name");
                if (featureName.empty()) {
                    CCError(u8"%s参数解析失败: feature_name为空", u8"请检查对应节点参数配置", m_nodeId.c_str());
                    return RET_FAILED;
                }
                uint8_t valueType = CCCameraParamItem::TYPE_INT;
                const std::string valueTypeStr = jsonParamItem.at("value_type");
                if (valueTypeStr == "int") {
                    valueType = CCCameraParamItem::TYPE_INT;
                } else if (valueTypeStr == "float") {
                    valueType = CCCameraParamItem::TYPE_FLOAT;
                } else if (valueTypeStr == "enum") {
                    valueType = CCCameraParamItem::TYPE_ENUM;
                } else if (valueTypeStr == "bool") {
                    valueType = CCCameraParamItem::TYPE_BOOL;
                } else if (valueTypeStr == "string") {
                    valueType = CCCameraParamItem::TYPE_STRING;
                } else {
                    CCError(u8"%s参数解析失败: value_type不支持类型%s", u8"目前仅支持int/float/enum/bool/string，请检查对应节点参数配置", m_nodeId.c_str(), valueTypeStr.c_str());
                    return RET_FAILED;
                }

                // assign
                CCCameraParamItem item;
                item.set__feature_name(featureName);
                item.set__value_type(valueType);
                if (jsonParamItem.contains("value_int")) {
                    item.set__value_int(jsonParamItem.at("value_int"));
                }
                if (jsonParamItem.contains("value_float")) {
                    item.set__value_float(jsonParamItem.at("value_float"));
                }
                if (jsonParamItem.contains("value_str")) {
                    item.set__value_str(jsonParamItem.at("value_str"));
                }
                m_param[imageId].item.push_back(item);
            }
        }
    } catch (ordered_json::exception &e) {
        CCError(u8"流程节点%s参数解析失败: %s", u8"请检查对应节点参数配置", this->id().data(), e.what());
        return RET_FAILED;
    }

    std::string nodeName = utility::ccRosNodeName(ms_type.type, m_nodeId);
//    nodeName = "node_" + nodeName + "_" + this->id() + "_dataflow";;
    auto rosNode = std::make_shared<CCCameraSetParamRosNode>(nodeName, m_transmitType);
    rosNode->setTopicTransmitType(m_topicTransmitType);

    std::string topicName = driver::ccRosTopicNameSetParam(devType, devObject.deviceObjId);
    rosNode->createPublishSetParam(topicName, 10);

    topicName = driver::ccRosTopicNameSetParamBack(devType, devObject.deviceObjId);
    rosNode->createSubscribeParam(topicName, m_nodeId,
                                    std::bind(&CCCameraSetParamFlowNode::onSubscribeParam, this, _1), 1024);

    m_rosNode = rosNode;

    return RET_SUCCESS;
}

int CCCameraSetParamFlowNode::updateInput()
{
    int32_t inputCount{0};
    auto rosNode = std::dynamic_pointer_cast<CCCameraSetParamRosNode>(m_rosNode);
    for (const auto &inputData : m_inputData) {
        if (!checkInputData({inputData.preValueType})) {
            return RET_FAILED;
        }

        const auto &topic = inputData.topic;
        auto pos = inputData.selfValueType.find(".");
        const auto &selfType = pos == std::string::npos ? inputData.selfValueType : inputData.selfValueType.substr(pos + 1);
        if (selfType == "image_id") {
            rosNode->createSubscribeImageId(
                topic,
                std::bind(&CCCameraSetParamFlowNode::onSubscribeImageId, this, _1), 1024);
            inputCount += 1;
        }
        else {
            CCError(u8"流程节点%s配置不正确", u8"请检查参数input_data/self_type配置是否是value_array.image_id", m_nodeId.c_str());
            return RET_FAILED;
        }
    }
    m_eventCv->setCount(inputCount);
    return RET_SUCCESS;
}

utility::CCFlowNodeType CCCameraSetParamFlowNode::type()
{
    return ms_type;
}

void CCCameraSetParamFlowNode::onSubscribeParam(const CCCameraParam::SharedPtr param)
{
    CCDebug("subscribe set camera data end...");
    if (param->header.node_id == m_nodeId) {
        m_eventCv->wakeup(true);
    }
}

void CCCameraSetParamFlowNode::onSubscribeImageId(const CCModbusData::SharedPtr data)
{
    CCInfo("%s enter.", __FUNCTION__);
    auto it = std::find_if(m_inputData.begin(), m_inputData.end(),
                           [](const auto &item){
                               const auto &topic = item.topic;
                               auto pos = item.selfValueType.find(".");
                               const auto &selfType = pos == std::string::npos ? item.selfValueType : item.selfValueType.substr(pos + 1);
                               return (selfType == "image_id");
                           });
    if (it == m_inputData.end() || it->nodeId != data->header.node_id) {
        return;
    }
    if (data->item.size() != 1) {
        CCError(u8"%s接收图号为空的值", u8"请检查流程输入是否正确", m_nodeId.c_str());
        return;
    }

    CCInfo("%s SetParam onSubscribeImageId enter", m_nodeId.c_str());
    const auto &item = data->item[0];
    switch (item.data_type) {
    case CCModbusData::DATA_INT16:
        m_imageId = item.value_int16;
        m_eventCv->wakeup();
        break;
    case CCModbusData::DATA_INT32:
        m_imageId = item.value_int32;
        m_eventCv->wakeup();
        break;
    case CCModbusData::DATA_INT64:
        m_imageId = item.value_int64;
        m_eventCv->wakeup();
        break;
    default:
        CCInfo("SetParam onSubscribeImageId this is unknow:%d", item.data_type);
        break;
    }
    CCInfo("%s SetParam onSubscribeImageId exit: %d", m_nodeId.c_str(), m_imageId);
}
