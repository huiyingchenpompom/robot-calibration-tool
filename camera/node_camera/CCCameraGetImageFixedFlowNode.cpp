/**
 * 流程取图节点
 *
 */
#include "CCCameraGetImageFixedFlowNode.h"
#include <filesystem>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include "CCCameraGetImageRosNode.h"
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

CCFlowNodeType CCCameraGetImageFixedFlowNode::ms_type = CCFlowNodeType{"camera_get_image_fixed", u8"定拍取图"};

CCCameraGetImageFixedFlowNode::CCCameraGetImageFixedFlowNode(const std::string &id, const std::string &name)
    : utility::CCFlowNode(id, name)
{
    CCDebug("CCCameraGetImageSingleFlowNode enter");
    m_pGetImage = std::make_shared<CCGetImage>();
    m_pGetImage->header.set__node_id(m_nodeId);
    m_transmitType = utility::driverTransmitType();
    CCDebug("CCCameraGetImageSingleFlowNode exit");
}

CCCameraGetImageFixedFlowNode::~CCCameraGetImageFixedFlowNode()
{
}

int CCCameraGetImageFixedFlowNode::start()
{
    m_eventCv->reset();
    CC_INIT_CC_DATA_ID(m_pGetImage->id);
    CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.task_id, utility::taskId());
    CCDebug("Current task id=%d", utility::taskId());
    return RET_SUCCESS;
}

void CCCameraGetImageFixedFlowNode::stop()
{
    m_eventCv->wakeup(true);
}

void CCCameraGetImageFixedFlowNode::run()
{
    CCDebug("%s run publish get image enter.", m_nodeId.c_str());

    // 判断是否收到所有输入
    int32_t index = m_eventCv->wait();
    if (index > 0) {
        CCError(u8"%s未接收所有输入数据", u8"请检查流程是否正确", m_nodeId.c_str());
    }
    CCDebug("%s run publish get image input: %d", m_nodeId.c_str(), m_pGetImage->id.plc_cycle_id);

    // 相机参数
    if (m_cameraParam.count(m_pGetImage->camera_image_id[0]) == 0) {
        CCError(u8"输入相机原图id和流程配置id不一致", u8"请检查流程配置正确性");
    } else {
        m_pGetImage->param[0] = m_cameraParam[m_pGetImage->camera_image_id[0]];
    }

    // publish getImage(dataId)
    m_pGetImage->header.set__time(utility::ccNow());
    m_pGetImage->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.task_id, utility::taskId());
    auto transmitType = utility::driverTransmitType();
    auto flagUseShmMem = m_flagUseShmMem && !(transmitType & TransmitCrossComputer);
    m_pGetImage->set__memory_type(
        flagUseShmMem ? CCMemoryType::MEM_SHARE : CCMemoryType::MEM_DATA);

    // publish
    auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>(m_rosNode);
    rosNode->publishGetImage(m_pGetImage);

    CCDebug("%s run publish get image exit: %d", m_nodeId.c_str(), m_pGetImage->id.plc_cycle_id);
}

int CCCameraGetImageFixedFlowNode::createOutput(const ordered_json &, const ordered_json &)
{
    const auto topicName = driver::ccRosTopicNameNodeImageData(ms_type.type, m_nodeId);
    CCFlowNodeOutputData data;
    data.topic = topicName;
    data.valueType = CCFlowNodeInOutValueType::ImageType;
    m_outputData.emplace_back(std::move(data));

    return RET_SUCCESS;
}

int CCCameraGetImageFixedFlowNode::parseJsonValue(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam)
{
    CCInfo("node(%s) parseJsonValue enter.", m_nodeId.c_str());

    // parse json
    utility::CCDeviceType devType;
    utility::CCDeviceObject devObject;
    try {
        std::string deviceId = jsonDevice.at("id");
        CCDeviceManage::instance().getDeviceObj(deviceId, devType, devObject);
        if (devType.mainType.empty()) {
            CCError(u8"设备id: %s不存在", u8"请检查device配置", deviceId.data());
            return RET_FAILED;
        }

        // param
        m_pGetImage->param.resize(1);
        m_pGetImage->camera_image_id.resize(1);
        const auto &jsonParam = jsonNodeParam.at("camera_param");
        for (const auto &item : jsonParam) {
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
            m_cameraParam[imageId] = param;
        }
        // trigger type
        const auto &jsonTriggerType = jsonNodeParam.at("trigger_type");
        jsonTriggerType.get_to(m_pGetImage->trigger_type);
    } catch (ordered_json::exception &e) {
        CCError(u8"流程节点%s参数解析失败: %s", u8"请检查对应节点参数配置", this->id().data(), e.what());
        return RET_FAILED;
    }

    // create node
    std::string nodeName = utility::ccRosNodeName(devType, devObject.deviceObjId);
    nodeName = "node_" + nodeName + "_" + this->id() + "_image";
    std::shared_ptr<CCCameraGetImageRosNode> rosNode = std::make_shared<CCCameraGetImageRosNode>(nodeName, m_transmitType);
    rosNode->setTopicTransmitType(m_topicTransmitType);
    m_rosNode = rosNode;

    // create publish getImage & subscribe getImageBack
    std::string  topicName = driver::ccRosTopicNameGetImage(devType, devObject.deviceObjId);
    rosNode->createPublishGetImage(topicName, 1024);
    topicName = driver::ccRosTopicNameDriverImageData(devType, devObject.deviceObjId);
    rosNode->createSubscribeGetImage(topicName, std::bind(&CCCameraGetImageFixedFlowNode::onSubscribeImage, this, _1), 128);

    if (!m_outputData.empty()) {
        topicName = m_outputData.front().topic;
        rosNode->createPublishImage(topicName, 1024);
    }

    CCDebug("node(%s) parseJsonValue exit.", m_nodeId.c_str());
    return RET_SUCCESS;
}

int CCCameraGetImageFixedFlowNode::updateInput()
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
                std::bind(&CCCameraGetImageFixedFlowNode::onSubscribeCycleId, this, _1), 1024);
            inputCount += 1;
        }
        else if (selfType == "image_id") {
            rosNode->createSubscribeImageId(
                topic,
                std::bind(&CCCameraGetImageFixedFlowNode::onSubscribeImageId, this, _1), 1024);
            inputCount += 1;
        }
        else {
            CCError(u8"流程节点%s配置不正确", u8"请检查参数input_data:self_type配置", m_nodeId.c_str());
            return RET_FAILED;
        }
    }
    m_eventCv->setCount(inputCount);
    return RET_SUCCESS;
}


utility::CCFlowNodeType CCCameraGetImageFixedFlowNode::type()
{
    return ms_type;
}

void CCCameraGetImageFixedFlowNode::onSubscribeImage(const CCImage::SharedPtr image)
{
    if (image->header.node_id == m_nodeId) {
        auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>(m_rosNode);


        CCRosMsgShm msgShm(image->shm_index, m_nodeId);
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
                    const auto &cvImage = cv::imread(fileName);
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
                                CCError(u8"图片替换功能：替换图片与原图大小不一致，重新分配共享内存错误", u8"请联系开发人员");
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
        rosNode->publishImage(image);
        CCInfo(u8"取图节点%s收到图片, taskid:%d,周期号:%d,相机号：%d 图号：%d ", this->id().c_str(), image->id.task_id, image->id.plc_cycle_id, image->id.camera_id, image->id.camera_image_id);
        CCDebug("CCCameraGetImageSingleFlowNode(%s) onSubscribeImage enter.", this->id().c_str());
    }
}

void CCCameraGetImageFixedFlowNode::onSubscribeCycleId(const CCModbusData::SharedPtr data)
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
        CCError(u8"%s接收数据非法周期号", u8"请联系开发人员", m_nodeId.c_str(), data->header.node_id);
        return;
    }

    CCDebug("%s onSubscribeCycleId enter: %d", this->id().c_str(), CC_GET_DATA_ID_VALUE(m_pGetImage->id.plc_cycle_id));
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
    CCDebug("%s onSubscribeCycleId exit: %d", this->id().c_str(), CC_GET_DATA_ID_VALUE(m_pGetImage->id.plc_cycle_id));
}

void CCCameraGetImageFixedFlowNode::onSubscribeImageId(const CCModbusData::SharedPtr data)
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
        CCError(u8"%s接收数据非法图片id", u8"请联系开发人员", m_nodeId.c_str());
        return;
    }

    CCInfo("%s onSubscribeImageId enter", m_nodeId.c_str());
    const auto &item = data->item[0];
    switch (item.data_type) {
    case CCModbusData::DATA_INT16:
        CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->camera_image_id[0], item.value_int16);
        m_eventCv->wakeup();
        break;
    case CCModbusData::DATA_INT32:
        CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->camera_image_id[0], item.value_int32);
        m_eventCv->wakeup();
        break;
    case CCModbusData::DATA_INT64:
        CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->camera_image_id[0], item.value_int64);
        m_eventCv->wakeup();
        break;
    default:
        break;
    }
    CCInfo("%s onSubscribeImageId exit: %lld", m_nodeId.c_str(), CC_GET_DATA_ID_VALUE(m_pGetImage->id.camera_image_id));
}

void CCCameraGetImageFixedFlowNode::registerDataIdMeta(void * dataIdMeta)
{
    CC_DATA_ID_META_REGISTER(dataIdMeta);
    m_pDataIdErrorHandle = CC_DATA_ID_NODE_REGISTER( m_nodeId );
}

const utility::CCDataIdErrorHandle *CCCameraGetImageFixedFlowNode::getDataIdErrorObj() const
{
    return m_pDataIdErrorHandle;
}

