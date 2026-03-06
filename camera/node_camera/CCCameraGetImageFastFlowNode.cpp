/**
 * 流程取图节点
 *
 */
#include "CCCameraGetImageFastFlowNode.h"
#include <filesystem>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>

#include "CCCameraGetImageRosNode.h"
#include "CCCameraGetImage.h"
#include "utility/core/CCUtilityDefine.h"
#include "utility/utility/CCTime.hpp"
#include "utility/core/device/CCDeviceManage.h"
#include "LogClientCommon.h"
#include "node/camera/CCCameraTopicName.hpp"
#include "utility/core/datetime/CCDateTime.h"
#include "utility/core/CCFileSystem.hpp"
#include "utility/core/event/CCEventCondVar.h"
#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "utility/core/setting/CCGlobalSettingManager.h"

#include "cc_data_id_meta.hpp"
#include "ros_msg/msg/cc_memory_type.hpp"

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace driver;
using namespace utility;
using namespace ros_msg::msg;
using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

CCFlowNodeType CCCameraGetImageFastFlowNode::ms_type = CCFlowNodeType{"camera_get_image_fast", u8"快拍取图"};

CCCameraGetImageFastFlowNode::CCCameraGetImageFastFlowNode(const std::string &id, const std::string &name)
    : utility::CCFlowNode(id, name)
{
    CCDebug("CCCameraGetImageSingleFlowNode enter");
    m_pGetImage = std::make_shared<CCGetImage>();
    m_pGetImage->header.set__node_id(m_nodeId);
    auto productCodeSet = CCDeviceManage::instance().productCodeSet();
    if (!productCodeSet.empty()) {
        m_productCode = *(productCodeSet.begin());
    }
    m_transmitType = utility::driverTransmitType();

    m_numberImageCount = std::make_shared<CCModbusData>();
    m_numberImageCount->header.set__node_id(id);
    m_numberImageCount->item.resize(1);
    m_numberImageCount->item[0].set__data_type(CCModbusData::DATA_INT32);
    CCDebug("CCCameraGetImageSingleFlowNode exit");
}

CCCameraGetImageFastFlowNode::~CCCameraGetImageFastFlowNode()
{
}

int CCCameraGetImageFastFlowNode::start()
{
    m_eventCv->reset();
    CC_INIT_CC_DATA_ID(m_pGetImage->id);
    CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.task_id, utility::taskId());
    if (!m_productCode.empty()) {
        std::vector<uint8_t> code(m_productCode.begin(), m_productCode.end());
        CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.product_code, code);
    }
    CCDebug("Current task id=%d", utility::taskId());
    return RET_SUCCESS;
}

void CCCameraGetImageFastFlowNode::stop()
{
    m_eventCv->wakeup(true);
}

void CCCameraGetImageFastFlowNode::run()
{
    CCDebug("%s run publish get image enter.", m_nodeId.c_str());

    // 判断是否收到所有输入
    m_eventCv->wait();
    CCDebug("%s run publish get image input: %d", m_nodeId.c_str(), m_pGetImage->id.plc_cycle_id);

    // 相机参数
    std::map<int64_t, ros_msg::msg::CCCameraParam> cameraParam;
    if (m_cameraParamFromJson.empty()) {
        if (m_cameraParamFromDb.count(m_productCode) != 0) {
            cameraParam = m_cameraParamFromDb[m_productCode];
        }
    } else {
        cameraParam = m_cameraParamFromJson;
    }

    m_pGetImage->param.resize(m_pGetImage->camera_image_id.size());
    for (int i = 0; i < m_pGetImage->camera_image_id.size(); i++) {
        if (cameraParam.count(m_pGetImage->camera_image_id[i]) == 0) {
            CCError(u8"%s未找到相机图号%d的相关节点参数", u8"1.请检查图号是否正确 2.若图号不正确请检查流程中图号输入 3.若图号正确请检查节点参数是否配置", m_nodeId.c_str(), m_pGetImage->camera_image_id[i]);
        } else {
            m_pGetImage->param[i] = cameraParam[m_pGetImage->camera_image_id[i]];
            m_pGetImage->param[i].header.set__node_id(m_nodeId);
        }
    }

    // publish getImage(dataId)
    m_pGetImage->header.set__time(utility::ccNow());
    m_pGetImage->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.task_id, utility::taskId());
    auto transmitType = utility::driverTransmitType();
    auto flagUseShmMem = m_flagUseShmMem && !(transmitType & TransmitCrossComputer);
    m_pGetImage->set__memory_type(flagUseShmMem ? CCMemoryType::MEM_SHARE : CCMemoryType::MEM_DATA);
    auto imageIdList = m_pGetImage->camera_image_id;
    m_pGetImage->camera_image_id.clear();
    CCInfo(u8"图号列表%s, 取图图片数量：%d", utility::toString(imageIdList).c_str(), m_imageCount);
    if (imageIdList.size() == 1) { // 兼容旧逻辑
        for (int i = 0; i < m_imageCount; i++) {
            m_pGetImage->camera_image_id.emplace_back(imageIdList[0] + i);
        }
    } else {                        // 新逻辑，快拍收到具体的图号列表
        if (m_imageCount > imageIdList.size()) {
            CCError(u8"取图节点%s的取图数量%d大于收到的图号数量%d", u8"请检查图号、取图数量输入情况", m_nodeId.c_str(), m_imageCount, imageIdList.size());
            return;
        }
        for (int i = 0; i < m_imageCount; i++) {
            m_pGetImage->camera_image_id.emplace_back(imageIdList[i]);
        }
    }

    // publish
    auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>(m_rosNode);
    rosNode->publishGetImage(m_pGetImage);
    if (!m_pGetImage->param.empty()) {
        m_eventCv->sleep();
    }

    // publish image count
    m_numberImageCount->header.set__time(utility::ccNow());
    m_numberImageCount->item[0].set__value_int32(m_imageCount);
    if (!m_productCode.empty()) {
        m_numberImageCount->item.resize(1 + m_productCode.size());
        for (size_t i = 0; i < m_productCode.size(); ++i) {
            m_numberImageCount->item[i + 1].set__data_type(CCValueItem::DATA_INT8);
            m_numberImageCount->item[i + 1].set__value_int8(m_productCode[i]);
        }
    }
    rosNode->publishImageCount(m_numberImageCount);

    CCDebug("%s run publish get image exit: %d", m_nodeId.c_str(), m_pGetImage->id.plc_cycle_id);
}

int CCCameraGetImageFastFlowNode::createOutput(const ordered_json &, const ordered_json &)
{
    const auto topicName = driver::ccRosTopicNameNodeImageData(ms_type.type, m_nodeId);
    CCFlowNodeOutputData data;
    data.topic = topicName;
    data.valueType = CCFlowNodeInOutValueType::ImageType;
    m_outputData.emplace_back(data);

    data.topic = ccRosTopicMerge(ms_type.type, m_nodeId) + "/image_count";
    data.valueType = CCFlowNodeInOutValueType::ModbusNodeType + std::string(".image_count");
    m_outputData.emplace_back(data);

    return RET_SUCCESS;
}

int CCCameraGetImageFastFlowNode::parseJsonValue(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam)
{
    CCInfo("node(%s) parseJsonValue enter.", m_nodeId.c_str());

    // parse json
    utility::CCDeviceType devType;
    utility::CCDeviceObject devObject;
    try {
        std::string deviceId = jsonDevice.at("id");
        CCDeviceManage::instance().getDeviceObj(deviceId, devType, devObject);
        if (devType.mainType.empty()) {
            CCError(u8"节点%s设备[%s]不存在", u8"请确定节点设备配置是否正确", m_nodeId.c_str(), deviceId.data());
            return RET_FAILED;
        }

        // param
        m_pGetImage->param.resize(1);
        m_pGetImage->camera_image_id.resize(1);
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
                int focus = item.at("focal_length_step");
                CCInfo(u8"获取到图号%d的焦距参数：%d", imageId, focus);
                param.params.push_back(CCCameraParam::PARAM_FOCUS);
                param.values.push_back(item.at("focal_length_step"));
            }
            if (item.contains("aperture_step") && !item.at("aperture_step").is_null()) {
                int iris = item.at("aperture_step");
                CCInfo(u8"获取到图号%d的光圈参数：%d", imageId, iris);
                param.params.push_back(CCCameraParam::PARAM_IRIS);
                param.values.push_back(item.at("aperture_step"));
            }
            m_cameraParamFromJson[imageId] = param;
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
                m_cameraParamFromJson[imageId].item.push_back(item);
            }
        }

        // trigger type
        const auto &jsonTriggerType = jsonNodeParam.at("trigger_type");
        jsonTriggerType.get_to(m_pGetImage->trigger_type);
    } catch (ordered_json::exception &e) {
        CCError(u8"流程节点%s参数解析失败: %s", u8"请检查对应节点参数配置", this->id().data(), e.what());
        return RET_FAILED;
    }

    // 如果flow_node.json未定义camera_param，从数据库读取
    if (m_cameraParamFromJson.empty()) {
        m_cameraParamFromDb = getCameraParamFromDb();
    }

    // create node
    std::string nodeName = utility::ccRosNodeName(ms_type.type, m_nodeId);
//    nodeName = "node_" + nodeName + "_" + this->id() + "_image";
    std::shared_ptr<CCCameraGetImageRosNode> rosNode = std::make_shared<CCCameraGetImageRosNode>(nodeName, m_transmitType);
    rosNode->setTopicTransmitType(m_topicTransmitType);
    m_rosNode = rosNode;

    // create publish getImage & subscribe getImageBack
    std::string  topicName = driver::ccRosTopicNameGetImage(devType, devObject.deviceObjId);
    rosNode->createPublishGetImage(topicName, 1024);
    topicName = driver::ccRosTopicNameDriverImageData(devType, devObject.deviceObjId);
    rosNode->createSubscribeGetImage(topicName, m_nodeId, std::bind(&CCCameraGetImageFastFlowNode::onSubscribeImage, this, _1), 128);
    topicName = driver::ccRosTopicNameSetParamBack(devType, devObject.deviceObjId);
    rosNode->createSubscribeParam(topicName, m_nodeId, std::bind(&CCCameraGetImageFastFlowNode::onSubscribeParam, this, _1), 1024);

    for (const auto &outputData : m_outputData) {
        topicName = outputData.topic;
        const auto valueType = outputData.valueType;
        if (valueType == CCFlowNodeInOutValueType::ImageType) {
            rosNode->createPublishImage(topicName, 1024);
        } else if (valueType == CCFlowNodeInOutValueType::ModbusNodeType + std::string(".image_count")) {
            rosNode->createPublishImageCount(topicName, 1024);
        }
    }

    CCDebug("node(%s) parseJsonValue exit.", m_nodeId.c_str());
    return RET_SUCCESS;
}

int CCCameraGetImageFastFlowNode::updateInput()
{
    int32_t inputCount{0};
    auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>(m_rosNode);
    for (const auto &inputData : m_inputData) {
        if (!checkInputData({inputData.preValueType})) {
            return RET_FAILED;
        }

        const auto &topic = inputData.topic;
        auto pos = inputData.selfValueType.find(".");
        const auto &selfType = pos == std::string::npos ? inputData.selfValueType : inputData.selfValueType.substr(pos + 1);
        if (selfType == "cycle") {
            rosNode->createSubscribeCycleId(
                topic,
                std::bind(&CCCameraGetImageFastFlowNode::onSubscribeCycleId, this, _1), 1024);
            inputCount += 1;
        }
        else if (selfType == "image_id") {
            rosNode->createSubscribeImageId(
                topic,
                std::bind(&CCCameraGetImageFastFlowNode::onSubscribeImageId, this, _1), 1024);
            inputCount += 1;
        }
        else if (selfType == "image_count") {
            rosNode->createSubscribeImageCount(
                topic,
                std::bind(&CCCameraGetImageFastFlowNode::onSubscribeImageCount, this, _1), 1024);
            inputCount += 1;
        }
        else if (selfType == "product_code") {
            rosNode->createSubscribeProductCode(
                topic,
                std::bind(&CCCameraGetImageFastFlowNode::onSubscribeProductCode, this, _1), 1024);
            inputCount += 1;
        }
        else {
            CCError(u8"流程节点%s配置不正确", u8"请检查参数input_data/self_type配置", m_nodeId.c_str());
            return RET_FAILED;
        }
    }
    m_eventCv->setCount(inputCount);
    return RET_SUCCESS;
}

int CCCameraGetImageFastFlowNode::getDefaultExtraSetting(std::vector<utility::CCExtraSetting> &nodeExtraSetting)
{
    nodeExtraSetting.clear();
    utility::CCExtraSetting settingErrorImageCount;
    settingErrorImageCount.configurationKey = "flag_error_image_count";
    settingErrorImageCount.configurationName = "取图数量错误时是否弹出报错框";
    settingErrorImageCount.configurationType = ConfigurationValueType::Switch;
    settingErrorImageCount.configurationValue = "true";
    settingErrorImageCount.configurationOptions = "";
    settingErrorImageCount.configurationTips = "false: 不弹出错误框; true: 弹出错误框";
    settingErrorImageCount.configurationIndex = 1;
    settingErrorImageCount.permission = 0;
    settingErrorImageCount.helpLink = "";
    nodeExtraSetting.push_back(settingErrorImageCount);

    utility::CCExtraSetting dropImageNumberZero;
    dropImageNumberZero.configurationKey = "drop_image_number_zero";
    dropImageNumberZero.configurationName = "是否丢弃图号为0的图像";
    dropImageNumberZero.configurationType = ConfigurationValueType::Switch;
    dropImageNumberZero.configurationValue = "false";
    dropImageNumberZero.configurationOptions = "";
    dropImageNumberZero.configurationTips = "false: 不丢弃; true: 丢弃";
    dropImageNumberZero.configurationIndex = 1;
    dropImageNumberZero.permission = 0;
    dropImageNumberZero.helpLink = "";
    nodeExtraSetting.push_back(dropImageNumberZero);

    return utility::DefaultExtraSettingResult::Success;
}

int CCCameraGetImageFastFlowNode::setExtraSettingValue(const std::vector<utility::CCExtraSetting> &nodeExtraSetting)
{
    for(const auto &setting : nodeExtraSetting) {
        if(setting.configurationKey == "flag_error_image_count") {
            m_pGetImage->set__error_image_count(setting.configurationValue == "true" ? 1 : 0);
        }

        if(setting.configurationKey == "drop_image_number_zero") {
            m_bDropImageNumberZero = setting.configurationValue == "true";
        }
    }
    return utility::DefaultExtraSettingResult::Success;
}

utility::CCFlowNodeType CCCameraGetImageFastFlowNode::type()
{
    return ms_type;
}

void CCCameraGetImageFastFlowNode::onSubscribeImage(const CCImage::SharedPtr image)
{
    CCRosMsgShm msgShm(image->shm_index, m_nodeId);
    if (image->header.node_id == m_nodeId) {
        auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>(m_rosNode);

        bool replaceImage = utility::CCGlobalSettingManager::instance()->getBoolValue("run_project", "replace_image", false);
        if (replaceImage) {
            auto stringToInt = [](const std::string& str, int& result) {
                try {
                    result = std::stoi(str);
                    return true;
                } catch (const std::invalid_argument&) {
                    // 字符串不是有效的整数表示
                    return false;
                } catch (const std::out_of_range&) {
                    // 转换结果超出了 int 范围
                    return false;
                }
            };
            int imageId = image->id.camera_image_id;
            std::string imagePath = utility::getProjectConfigPath() + "replace_image";
            const auto &imageFiles = utility::ccFilesOfPath(imagePath);
            bool findImage = false;
            for (const auto& fileName: imageFiles) {
                std::filesystem::path filePath = fileName;
                const auto &imageIdStr = filePath.stem().string();
                CCInfo(u8"搜索到替换图号文件：%s", imageIdStr.c_str());
                int imageId2;
                if (stringToInt(imageIdStr, imageId2) && imageId2 == imageId) {
                    cv::Mat cvImage;
                    if (m_replaceImageCache.count(imageId2) > 0) {
                        cvImage = m_replaceImageCache[imageId2];
                    } else {
                        cvImage = cv::imread(fileName);
                        m_replaceImageCache[imageId2] = cvImage;
                    }
                    if (cvImage.empty()) {
                        CCWarn(u8"路径%s找到需要替换的图号：%d读取失败", u8"请检查图片", imagePath.c_str(), imageId);
                        break;
                    }
                    CCInfo(u8"取图节点%s已替换文件%s", m_nodeId.c_str(), fileName.c_str());
                    size_t size = cvImage.cols * cvImage.rows * cvImage.channels();
                    if (image->memory_type == CCMemoryType::MEM_DATA) {
                        if (image->data_size < size) {
                            image->data.resize(size);
                        }
                        memcpy(image->data.data(), cvImage.data, size);
                    } else {
                        if (image->data_size < size) {
                            CCInfo(u8"图片%d大小%d小于替换图大小%d,重新开辟内存后开始拷贝内存", imageId, image->data_size, size);
                            CCRosMsgShm msgShmNew;
                            auto index = msgShmNew.createShmIndexWithData(cvImage.data, size);
                            image->set__shm_index(index);
                            if (!msgShmNew.isValid(index)) {
                                CCError( u8"图片替换功能：替换图片与原图大小不一致，重新分配共享内存错误", u8"请检查当前系统各进程内存使用量，并确认节点（推理、前处理、环切、MARK图、存图）是否存在图片积压");
                                return;
                            }
                        } else {
                            CCInfo(u8"图片%d大小%d大于等于替换图大小%d,直接拷贝内存", imageId, size);
                            memcpy(msgShm.data(image->shm_index), cvImage.data, size);
                        }
                        CCInfo(u8"图片%d大小%d拷贝内存完成", imageId, size);
                    }
                    image->set__img_width(cvImage.cols);
                    image->set__img_height(cvImage.rows);
                    image->set__img_channel(cvImage.channels());
                    image->set__img_format(cvImage.channels() == 1 ? 0 : 16);
                    image->set__data_size(size);
                    findImage = true;
                    break;
                }
            }
            if (!findImage) {
                CCWarn(u8"路径%s未找到需要替换的图号：%d", u8"请检查图片", imagePath.c_str(), imageId);
            }
        }
        if (!m_bDropImageNumberZero || (m_bDropImageNumberZero && image->id.camera_image_id)) {
            rosNode->publishImage(image);
        } else {
            CCInfo(u8"取图节点%s收到图号为0的图片，丢弃。taskid:%d,周期号:%d,相机号：%d ", this->id().c_str(), image->id.task_id, image->id.plc_cycle_id, image->id.camera_id);
        }
        CCInfo(u8"取图节点%s收到图片, taskid:%d,周期号:%d,相机号：%d 图号：%d ", this->id().c_str(), image->id.task_id, image->id.plc_cycle_id, image->id.camera_id, image->id.camera_image_id);
        CCDebug("CCCameraGetImageSingleFlowNode(%s) onSubscribeImage enter.", this->id().c_str());
    }
}

void CCCameraGetImageFastFlowNode::onSubscribeCycleId(const CCModbusData::SharedPtr data)
{
    auto it = std::find_if(m_inputData.begin(), m_inputData.end(),
                           [](const auto &item){
                               const auto &topic = item.topic;
                               auto pos = item.selfValueType.find(".");
                               const auto &selfType = pos == std::string::npos ? item.selfValueType : item.selfValueType.substr(pos + 1);
                               return (selfType == "cycle");

    });
    if (it == m_inputData.end() || it->nodeId != data->header.node_id) {
        return;
    }
    if (data->item.size() != 1) {
        CCError(u8"%s接收非法周期号输入，数组数量小于1", u8"请联系SC开发人员排查", m_nodeId.c_str(), data->header.node_id);
        return;
    }

    CCDebug("%s onSubscribeCycleId enter", this->id().c_str());
    const auto &item = data->item[0];
    switch (item.data_type) {
    case CCModbusData::DATA_INT16:
        CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.plc_cycle_id, static_cast<int32_t>(item.value_int16));
        m_eventCv->wakeup();
        break;
    case CCModbusData::DATA_INT32:
    {
        CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.plc_cycle_id, item.value_int32);
        m_eventCv->wakeup();
        break;
    }
    case CCModbusData::DATA_INT64:
    {
        CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.plc_cycle_id, item.value_int64);
        m_eventCv->wakeup();
        break;
    }
    default:
        break;
    }
    CCDebug( u8"%s 更新周期号 exit: %d", this->id().c_str(), m_pGetImage->id.plc_cycle_id);
}

void CCCameraGetImageFastFlowNode::onSubscribeImageId(const CCModbusData::SharedPtr data)
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

    CCInfo("%s onSubscribeImageId enter, data size:%d", m_nodeId.c_str(), data->item.size());
    m_pGetImage->camera_image_id.clear();
    bool wakeup = true;
    for (int i = 0; i < data->item.size(); i++) {
        const auto &item = data->item[i];
        uint64 value = 0;
        switch (item.data_type) {
        case CCModbusData::DATA_INT8:
            wakeup &= true;
            value = item.value_int8;
            m_pGetImage->camera_image_id.emplace_back(item.value_int8);
            break;
        case CCModbusData::DATA_INT16:
            wakeup &= true;
            value = item.value_int16;
            m_pGetImage->camera_image_id.emplace_back(item.value_int16);
            break;
        case CCModbusData::DATA_INT32:
            wakeup &= true;
            value = item.value_int32;
            m_pGetImage->camera_image_id.emplace_back(item.value_int32);
            break;
        case CCModbusData::DATA_INT64:
            wakeup &= true;
            value = item.value_int64;
            m_pGetImage->camera_image_id.emplace_back(item.value_int64);
            break;
        default:
            wakeup &= false;
            CCError(u8"取图节点%s图号接收收到未知的数据类型:%d", u8"请检查图号输入节点，目前仅支持int8/int16/int32/int64类型", m_nodeId.c_str(), item.data_type);
            break;
        }
        if (0 == value) {
            break;
        }
    }

    std::unordered_set<int> imageIdSet;
    for (const auto &imageId: m_pGetImage->camera_image_id) {
        auto [it, inserted] = imageIdSet.insert(imageId);
        if (!inserted) {
             CCError(u8"更新图号%s失败，存在重复的图号%d", u8"请检查图号输入来源", utility::toString(m_pGetImage->camera_image_id).c_str(), imageId);
            return;
        }
    }

    if (wakeup) {
        m_eventCv->wakeup();
    }
    CCInfo( u8"%s 更新图号 exit: %s", m_nodeId.c_str(), utility::toString(m_pGetImage->camera_image_id).c_str());
}

void CCCameraGetImageFastFlowNode::onSubscribeImageCount(const ros_msg::msg::CCModbusData::SharedPtr data)
{
    CCInfo("%s enter.", __FUNCTION__);
    auto it = std::find_if(m_inputData.begin(), m_inputData.end(),
                           [](const auto &item){
                               const auto &topic = item.topic;
                               auto pos = item.selfValueType.find(".");
                               const auto &selfType = pos == std::string::npos ? item.selfValueType : item.selfValueType.substr(pos + 1);
                               return (selfType == "image_count");
                           });
    if (it == m_inputData.end() || it->nodeId != data->header.node_id) {
        return;
    }
    if (data->item.size() != 1) {
        CCError(u8"%s接收非法图片数量输入，数组数量小于1", u8"请联系SC开发人员排查", m_nodeId.c_str());
        return;
    }

    CCInfo("%s onSubscribeImageCount enter", m_nodeId.c_str());
    const auto &item = data->item[0];
    switch (item.data_type) {
    case CCModbusData::DATA_INT16:
        m_imageCount = item.value_int16;
        m_eventCv->wakeup();
        break;
    case CCModbusData::DATA_INT32:
        m_imageCount = item.value_int32;
        m_eventCv->wakeup();
        break;
    case CCModbusData::DATA_INT64:
        m_imageCount = static_cast<int32_t>(item.value_int64);
        m_eventCv->wakeup();
        break;
    default:
        break;
    }
    if (m_imageCount < 1) {
        CCWarn(u8"%s收到异常的图片数量%d,已将图片数量改为1", m_nodeId.c_str(), m_imageCount);
        m_imageCount = 1;
    }
    CCInfo( u8"%s 更新取图数量 exit: %d", m_nodeId.c_str(), m_imageCount);
}

void CCCameraGetImageFastFlowNode::onSubscribeProductCode(const ros_msg::msg::CCString::SharedPtr productCode)
{
    CCDebug( u8"%s 收到产品号: %s", m_nodeId.c_str(), productCode->msg.c_str() );
    m_productCode = productCode->msg;
    std::vector<uint8_t> code(m_productCode.begin(), m_productCode.end());
    CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.product_code, code);
    m_eventCv->wakeup();
}

void CCCameraGetImageFastFlowNode::registerDataIdMeta(void * dataIdMeta)
{
    CC_DATA_ID_META_REGISTER(dataIdMeta);
    m_pDataIdErrorHandle = CC_DATA_ID_NODE_REGISTER( m_nodeId );
}

const utility::CCDataIdErrorHandle *CCCameraGetImageFastFlowNode::getDataIdErrorObj() const
{
    return m_pDataIdErrorHandle;
}


void CCCameraGetImageFastFlowNode::onSubscribeParam(const CCCameraParam::SharedPtr param)
{
    CCInfo(u8"快拍取图节点%s收到%s参数修改返回...", m_nodeId.c_str(), param->header.node_id.c_str());
    if (param->header.node_id == m_nodeId) {
        m_eventCv->wakeup(true);
    }
}
