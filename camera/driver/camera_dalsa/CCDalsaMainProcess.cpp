/**
 * Basler相机驱动主程序
 *
 */
#include "CCDalsaMainProcess.h"

#include <rclcpp/rclcpp.hpp>
#include <boost/format.hpp>
#include "CCDalsaCameraRosNode.h"
#include "CCDalsaCameraImp.h"
#include "node/camera/CCCameraTopicName.hpp"
#include "utility/core/CCTime.hpp"
#include "ros_msg/msg/cc_memory_type.hpp"
#include "utility/core/CCUtilityDefine.h"
#include "LogClientCommon.h"
#include "cc_data_id_meta.hpp"
#include "utility/core/datetime/CCDateTime.h"
#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "utility/cc_message/CCMessageFunction.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
#define ZERO 0.000001f

CCDalsaMainProcess::CCDalsaMainProcess(  const std::string &mainType
                             , const std::string &subType
                             , const std::string &objId
                             , int index
                             , const std::string &info
                             , int transmitType)
    : m_objId(objId)
    , m_cameraId(index)
    , m_image(std::make_shared<CCImage>())
    , m_camera(std::make_shared<CCDalsaCameraImp>(index, info))
{
    CCInfo("%s enter.", __FUNCTION__);
    utility::CCDeviceType type;
    type.mainType = mainType;
    type.subType = subType;
    memset(m_cameraParam, 0, sizeof(float) * CCCameraParam::PARAM_COUNT);
    m_camera->setImageCapturedCallback(std::bind(&CCDalsaMainProcess::onImageCaptured, this, _1, _2, _3, _4));

    std::string nodeName = utility::ccRosNodeName(type, objId);
    nodeName = "driver_" + nodeName;
    m_node = std::make_shared<CCDalsaCameraRosNode>(nodeName, transmitType);

    std::string topicName = driver::ccRosTopicNameDriverImageData(type, objId);
    CCInfo("dalsa camera created param topic: %s", topicName.c_str());
    m_node->createPublishImage(topicName, 1024);

    topicName = driver::ccRosTopicNameGetImage(type, objId);
    CCInfo("dalsa camera created image subscribe: %s", topicName.c_str());
    m_node->createSubscribeGetImage(topicName,
                                    std::bind(&CCDalsaMainProcess::onSubscribeGetImage, this, _1), 1024);

    topicName = driver::ccRosTopicNameSetParamBack(type, objId);
    CCInfo("dalsa camera created param subscribe: %s", topicName.c_str());
    m_node->createPublishParam(topicName, 1024);

    topicName = driver::ccRosTopicNameSetParam(type, objId);
    CCInfo("dalsa camera created param subscribe: %s", topicName.c_str());
    m_node->createSubscribeSetParam(topicName,
                                    std::bind(&CCDalsaMainProcess::onSubscribeSetParam, this, _1), 1024);

    m_getImage = std::make_shared<CCGetImage>();
    CCInfo("init successed");
    CCInfo("%s exit.", __FUNCTION__);
}

bool CCDalsaMainProcess::openCamera(int triggerType)
{
    CCInfo("%s enter.", __FUNCTION__);
    m_getImage->trigger_type = triggerType;
    bool ret = true;
#ifdef DALSA_CL_LINE
    ret &= m_camera->init();
#elif
#endif
    return ret;
}

void CCDalsaMainProcess::run()
{
    CCInfo("%s enter.", __FUNCTION__);
    utility::cc_message::spin(m_node);
    CCInfo("%s exit.", __FUNCTION__);
}

std::shared_ptr<utility::CCMessageNode> CCDalsaMainProcess::rosNode()
{
    return m_node;
}

std::shared_ptr<CCDalsaCameraImp>   CCDalsaMainProcess::cameraIO()
{
    return m_camera;
}

void CCDalsaMainProcess::setCameraParam(const CCCameraParam &param)
{

}

void CCDalsaMainProcess::onSubscribeGetImage(const CCGetImage::SharedPtr getImage)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    CCDebug("camera [%d] subscribe image enter", m_cameraId);

    if (-1 == getImage->trigger_type) {
        getImage->trigger_type = m_getImage->trigger_type;
    }
    int triggerType = m_getImage->trigger_type;
    m_getImage = getImage;
    m_getImage->id.set__camera_id(m_cameraId);
    m_imageIndex = 0;

    // 拍照点就绪开始&结束时间
    m_image->header.times.clear();
    m_image->header.times.emplace_back(0);
    m_image->header.times.emplace_back(m_getImage->header.times.back());
    m_image->header.times.emplace_back(1);
    m_image->header.times.emplace_back(m_getImage->header.times.back());

    // 设置参数
    if (m_getImage->param.size() > 0) {
        setCameraParam(m_getImage->param.at(0));
    }

    // 相机取图
    if (triggerType != m_getImage->trigger_type) {
        if (m_getImage->trigger_type == 0) {
            m_camera->setSoftwareTrigger();
        } else {
            m_camera->setHardwareTrigger();
        }
    }
    if (m_getImage->trigger_type == 0) { // 软触发
        m_camera->sendSoftTriggerCommand();
    }

    // 开始收图时间 软触发&硬触发
    m_image->header.times.emplace_back(4);
    m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    CCInfo("driver subscribe image read");

    return;
}

void CCDalsaMainProcess::onSubscribeSetParam(const CCCameraParam::SharedPtr param)
{
    CCInfo("%s enter.", __FUNCTION__);

    // 返回参数
    CCCameraParam::SharedPtr paramBack = std::make_shared<CCCameraParam>();
    m_node->publishParam(paramBack);
    CCInfo("%s exit.", __FUNCTION__);
}

void CCDalsaMainProcess::onImageCaptured(void *buffer, int width, int height, int channel)
{
    CCInfo("%s enter.", __FUNCTION__);
    CC_ASSERT(buffer);
    CCImage::SharedPtr ccImage = m_image;

    // 结束收图时间
    m_image->header.times.emplace_back(5);
    ccImage->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    ccImage->header.set__time(utility::ccNow());
    ccImage->header.device_id = m_objId;
    ccImage->header.set__node_id(m_getImage->header.node_id);
    ccImage->set__id(m_getImage->id);
    CC_SET_CCDATAID_FIELD_VALUE(m_image->id.image_type, CCImage::RAW_IMAGE);

    ccImage->set__memory_type(m_getImage->memory_type);

    ccImage->img_width  = width;
    ccImage->img_height = height;
    ccImage->set__img_channel(channel);
    ccImage->set__img_format(channel == 1 ? 0 : 16);

    if (nullptr == buffer) {
        ccImage->is_data_valid = false;
    } else {
        ccImage->is_data_valid = true;
        const uint32_t size = m_image->img_width * m_image->img_height * m_image->img_channel;

        if (ccImage->memory_type == CCMemoryType::MEM_DATA) {
            if (ccImage->data.size() != size) {
               ccImage->data.resize(size);
            }
            memcpy(ccImage->data.data(), buffer, size);
        }
        else {
            CCRosMsgShm msgShm;
            ccImage->data.clear();
            auto index = msgShm.createShmIndexWithData(static_cast<uint8_t *>(buffer), size);
            ccImage->set__shm_index(index);
            if (!msgShm.isValid(index)) {
               CCError(u8"共享内存分配错误", u8"请联系开发人员");
               return;
            }
        }
        ccImage->set__data_size(size);
    }
    // 发布图片
    if (m_getImage->camera_image_id.size() > 0) {
        if (m_imageIndex >= 0 && m_imageIndex < m_getImage->camera_image_id.size()) {
            CC_SET_CCDATAID_FIELD_VALUE(m_image->id.camera_image_id, m_getImage->camera_image_id[m_imageIndex]);
        }
        else {
            CCError(u8"收图张数大于配置数量:%d,%d", u8"请检查相机或流程配置", m_imageIndex, m_getImage->camera_image_id.size());
        }
    }
    else {
        CC_SET_CCDATAID_FIELD_VALUE(m_image->id.camera_image_id, m_imageIndex + 1);
    }

    ++ m_imageIndex;
    if (m_imageIndex < m_getImage->camera_image_id.size() && m_getImage->trigger_type == 0) {
        m_camera->sendSoftTriggerCommand();
    }
    CCInfo(u8"驱动发送图片开始，相机号：%d，周期号：%d，图号：%d", ccImage->id.camera_id,ccImage->id.plc_cycle_id, ccImage->id.camera_image_id);
    m_node->publishImage(ccImage);
    CCInfo(u8"驱动发送图片结束，相机号：%d，周期号：%d，图号：%d", ccImage->id.camera_id,ccImage->id.plc_cycle_id, ccImage->id.camera_image_id);

    if (m_imageIndex) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 更新时间
        int size = ccImage->header.times.size();
        for (int i = 0; i < size - 4; i++) {
            ccImage->header.times.pop_back();
        }

        // set camera param
        if (m_imageIndex < m_getImage->param.size()) {
            setCameraParam(m_getImage->param.at(m_imageIndex));
        } else {
            // 添加设置参数时间
            m_image->header.times.emplace_back(2);
            m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
            m_image->header.times.emplace_back(3);
            m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
        }

        // 开始收图时间
        m_image->header.times.emplace_back(4);
        ccImage->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    }
    CCInfo("%s exit %s.", __FUNCTION__, ccImage->header.node_id.c_str());
}


