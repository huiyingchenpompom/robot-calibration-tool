/**
 * usb相机驱动主程序
 *
 */
#include "CCSimulatorCameraMainProcess.h"

#include <future>
#include <boost/format.hpp>

#include "CCSimulatorCameraRosNode.h"
#include "CCSimulatorCameraCameraIO.h"
#include "CCSimulatorCameraUdpServer.h"

#include "node/camera/CCCameraTopicName.hpp"
#include "utility/utility/CCTime.hpp"
#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "ros_msg/msg/cc_memory_type.hpp"
#include "cc_data_id_meta.hpp"

#include "utility/core/CCUtilityDefine.h"
#include "utility/core/datetime/CCDateTime.h"
#include "utility/core/event/CCEventCondVar.h"
#include "LogClientCommon.h"
#include "utility/cc_message/CCMessageFunction.h"

using std::placeholders::_1;

CCSimulatorCameraMainProcess::CCSimulatorCameraMainProcess(const std::string &mainType, const std::string &subType,
                         const std::string &objId, int index, int triggerType, int transmitType, const std::string &imagePath)
    : m_objId(objId)
    , m_cameraId(index)
    , m_triggerType(triggerType)
    , m_camera(std::make_shared<CCSimulatorCameraCameraIO>(index, utility::configPath() + imagePath + "/"))
{
    CCDebug("CCSimulatorCameraMainProcess enter.");
    m_eventCv = std::make_unique<utility::CCEventCondVar>();

    utility::CCDeviceType type;
    type.mainType = mainType;
    type.subType = subType;

    std::string nodeName = utility::ccRosNodeName(type, objId);
    // nodeName = "driver_" + nodeName;
    m_node = std::make_shared<CCSimulatorCameraRosNode>(nodeName, transmitType);

    // create publish getImage & subscibe getImage
    auto topicName = driver::ccRosTopicNameGetImage(type, objId);
    m_node->createSubscribeGetImage(topicName,
                                    std::bind(&CCSimulatorCameraMainProcess::onSubscribeGetImage, this, _1), 1024);
    topicName = driver::ccRosTopicNameDriverImageData(type, objId);
    m_node->createPublishImage(topicName, 1024);

    // create publish paramData & subscribe paramData
    topicName = driver::ccRosTopicNameSetParamBack(type, objId);
    m_node->createPublishParam(topicName, 1024);
    topicName = driver::ccRosTopicNameSetParam(type, objId);
    m_node->createSubscribeSetParam(topicName,
                                    std::bind(&CCSimulatorCameraMainProcess::onSubscribeSetParam, this, _1), 1024);

    if (m_triggerType == 1) {
        m_server = std::make_shared<CCSimulatorCameraUdpServer>(
            m_context, 42000 + m_cameraId);
        m_thread = std::thread([this](){m_context.run();});
        m_server->setCallFun(std::bind(&CCSimulatorCameraMainProcess::doHardGetImage, this, std::placeholders::_1));
    }

    CCDebug("simulator camera init successed");
}

CCSimulatorCameraMainProcess::~CCSimulatorCameraMainProcess()
{
    if (m_thread.joinable()) {
        m_context.stop();
        m_thread.join();
    }
    if (m_threadSoftGetImage.joinable()) {
        m_threadSoftGetImage.join();
    }
    CCDebug("CCSimulatorCameraMainProcess destructor, ros node refcout: %d", m_node.use_count());
}

void CCSimulatorCameraMainProcess::run()
{
    utility::cc_message::spin(m_node);
}

std::shared_ptr<utility::CCMessageNode> CCSimulatorCameraMainProcess::rosNode()
{
    return m_node;
}

void CCSimulatorCameraMainProcess::onSubscribeGetImage(const CCGetImage::SharedPtr getImage)
{
    CCDebug("driver(%s) onSubscribeGetImage enter", m_objId.c_str());
    auto cycleId = getImage->id.plc_cycle_id;
    m_cameraImageIndex[cycleId] = 0;
    for (int i = 0; i < getImage->camera_image_id.size(); i++) {
        getImage->delay_time.emplace_back(100);
    }
    m_getImageList.pushBack(getImage);
    int triggerType = m_triggerType;
    if (getImage->trigger_type != -1) {
        triggerType = getImage->trigger_type;
    }
    CC_SET_CCDATAID_FIELD_VALUE(getImage->id.camera_id,m_cameraId);

    // publish param
    auto param = std::make_shared<CCCameraParam>();
    param->header.set__device_id(m_objId);
    param->header.set__node_id(getImage->header.node_id);
    m_node->publishParam(param);

    m_eventCv->wakeup(true);

    if (triggerType == 0) {
        if (getImage->delay_time.empty()) {
            onGrabImage(getImage);
        }
        else {
            doSoftGetImage();
        }
    }
}

void CCSimulatorCameraMainProcess::onGrabImage(const CCGetImage::SharedPtr getImage)
{
    CCInfo("%s driver publish image enter", m_objId.c_str());
    auto cycleId = getImage->id.plc_cycle_id;

    CCImage::SharedPtr image = std::make_shared<CCImage>();
    image->header.times.clear();

    // 拍照点就绪开始&结束时间
    image->header.times.emplace_back(0);
    image->header.times.emplace_back(getImage->header.times.back());
    image->header.times.emplace_back(1);
    image->header.times.emplace_back(getImage->header.times.back());

    // 相机参数设置
    image->header.times.emplace_back(2);
    image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    CCCameraParam param;
    if (m_param != nullptr) {
        param = *m_param;
        m_param.reset();
    }
    else if (m_cameraImageIndex[cycleId] < getImage->param.size()) {
        param = getImage->param[m_cameraImageIndex[cycleId]];
    }
    const size_t Count = std::min<size_t>(
        param.params.size(),
        param.values.size());
    for (size_t i = 0; i < Count; ++i) {
        int key = param.params.at(i);
//        float value = param.values.at(i);
        switch (key) {
        case CCCameraParam::PARAM_EXPOSURE:
            break;
        case CCCameraParam::PARAM_GAMMA:
        {
//            cv::convertScaleAbs(m_frame, m_frame, 1, value);
            break;
        }
        case CCCameraParam::PARAM_GAIN:
            break;
        default:
            break;
        }
    }

    image->header.times.emplace_back(3);
    image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    image->header.set__time(utility::ccNow());

    // 相机取图
    image->header.times.emplace_back(4);
    image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    m_camera->getFrame(m_frame);

    image->header.set__node_id(getImage->header.node_id);
    image->header.set__device_id(m_objId);
    image->set__memory_type(getImage->memory_type);
    image->set__id(getImage->id);
    CC_SET_CCDATAID_FIELD_VALUE(image->id.image_type, CCImage::RAW_IMAGE);

    image->set__img_width(m_frame.cols);
    image->set__img_height(m_frame.rows);
    image->set__img_channel(m_frame.channels());
    image->set__img_format(m_frame.type());
    image->set__is_data_valid(true);

    const int Size = image->img_width * image->img_height * image->img_channel;

    if (image->memory_type == CCMemoryType::MEM_DATA) {
        if (image->data.size() != Size) {
            image->data.resize(Size);
        }
        memcpy(image->data.data(), m_frame.data, Size);
    }
    else {
        CCRosMsgShm msgShm;
        image->data.clear();
        auto index = msgShm.createShmIndexWithData(m_frame.data, Size);
        image->set__shm_index(index);
        if (!msgShm.isValid(index)) {
            return;
        }
    }
    image->set__data_size(Size);

    // 发布图片
    if (getImage->camera_image_id.size() > 0) {
        if (m_cameraImageIndex[cycleId] >= 0 && m_cameraImageIndex[cycleId] < getImage->camera_image_id.size()) {
            CC_SET_CCDATAID_FIELD_VALUE(image->id.camera_image_id,
                                        getImage->camera_image_id[m_cameraImageIndex[cycleId]]);
        }
        else {
            CC_SET_CCDATAID_FIELD_VALUE(image->id.camera_image_id, 0);
            if (getImage->error_image_count == 1) {
                CCError(u8"收图张数大于配置数量:%d,%d", u8"请检查相机或流程配置",
                    m_cameraImageIndex[cycleId], getImage->camera_image_id.size());
            } else {
                CCWarn(u8"收图张数大于配置数量:%d,%d",
                    m_cameraImageIndex[cycleId], getImage->camera_image_id.size());
            }
        }
    }
    else {
        CC_SET_CCDATAID_FIELD_VALUE(image->id.camera_image_id, m_cameraImageIndex[cycleId] + 1);
    }
    image->header.times.emplace_back(5);
    image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    m_node->publishImage(image);
    ++m_cameraImageIndex[cycleId];
    if (m_cameraImageIndex[cycleId] >= getImage->camera_image_id.size()) {
        m_cameraImageIndex.erase(cycleId);
        m_getImageList.popFront();
    }
    CCInfo("%s driver publish image", m_objId.c_str());
}

void CCSimulatorCameraMainProcess::onSubscribeSetParam(const CCCameraParam::SharedPtr param)
{
    // 返回参数
    m_param = param;
    m_node->publishParam(param);
}

void CCSimulatorCameraMainProcess::doHardGetImage(const std::string &text)
{
    CCInfo("driver(%s) do get image: %s", m_objId.c_str(), text.c_str());
    if (text == m_objId) {
        if (m_getImageList.empty()) {
            m_eventCv->sleep();
        }
        auto getImage = m_getImageList.front();
        onGrabImage(getImage);
    }
}

void CCSimulatorCameraMainProcess::doSoftGetImage()
{
    if (m_threadSoftGetImage.joinable()) {
        m_threadSoftGetImage.join();
    }
    m_threadSoftGetImage = std::thread(
        [this](){
            if (m_getImageList.empty()) {
                m_eventCv->sleep();
            }

            auto getImage = m_getImageList.front();
            const size_t delayTimeCount = getImage->delay_time.size();
            const size_t count = std::max(delayTimeCount, getImage->camera_image_id.size());
            for (size_t i = 0; i < count; ++i) {
                auto ms = i < delayTimeCount ? getImage->delay_time[i] : getImage->delay_time[delayTimeCount - 1];
                onGrabImage(getImage);
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
        }
        );
}
