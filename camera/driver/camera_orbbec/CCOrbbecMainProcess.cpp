/**
 * Basler网络相机驱动主程序
 *
 */
#include "CCOrbbecMainProcess.h"

#include <boost/format.hpp>
#include "CCOrbbecCameraRosNode.h"
#include "CCOrbbecCameraImp.h"
#include "node/camera/CCCameraTopicName.hpp"
#include "utility/utility/CCTime.hpp"
#include "ros_msg/msg/cc_memory_type.hpp"
#include "utility/core/CCUtilityDefine.h"
#include "LogClientCommon.h"
#include "cc_data_id_meta.hpp"
#include "utility/core/datetime/CCDateTime.h"
#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "utility/cc_message/CCMessageFunction.h"
#include "ros_msg/msg/cc_msg_reply.hpp"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
#define ZERO 0.000001f

CCOrbbecMainProcess::CCOrbbecMainProcess(  const std::string &mainType
                             , const std::string &subType
                             , const std::string &objId
                             , int index
                             , const std::string &info
                             , int transmitType)
    : m_objId(objId)
    , m_cameraId(index)
    , m_image(std::make_shared<CCImage>())
    , m_camera(std::make_shared<CCOrbbecCameraImp>(index, info))
{
    CCInfo("orbbec %s enter.", __FUNCTION__);
    utility::CCDeviceType type;
    type.mainType = mainType;
    type.subType = subType;
    memset(m_cameraParam, 0, sizeof(float) * CCCameraParam::PARAM_COUNT);
    m_camera->setImageCapturedCallback(std::bind(&CCOrbbecMainProcess::onImageCaptured, this, _1, _2, _3, _4));

    std::string nodeName = utility::ccRosNodeName(type, objId);
    nodeName = "driver_" + nodeName;
    m_node = std::make_shared<CCOrbbecCameraRosNode>(nodeName, transmitType);

    std::string topicName = driver::ccRosTopicNameDriverImageData(type, objId);
    CCInfo("orbbec camera created param topic: %s", topicName.c_str());
    m_node->createPublishImage(topicName, 1024);

    topicName = driver::ccRosTopicNameGetImage(type, objId);
    CCInfo("orbbec camera created image subscribe: %s", topicName.c_str());
    m_node->createSubscribeGetImage(topicName,
                                    std::bind(&CCOrbbecMainProcess::onSubscribeGetImage, this, _1), 1024);

    topicName = driver::ccRosTopicNameSetParamBack(type, objId);
    CCInfo("orbbec camera created param subscribe: %s", topicName.c_str());
    m_node->createPublishParam(topicName, 1024);

    topicName = driver::ccRosTopicNameSetParam(type, objId);
    CCInfo("orbbec camera created param subscribe: %s", topicName.c_str());
    m_node->createSubscribeSetParam(topicName,
                                    std::bind(&CCOrbbecMainProcess::onSubscribeSetParam, this, _1), 1024);

    topicName = driver::ccRosTopicNameCameraSwitchReply(type, objId);
    CCInfo(u8"obbec camera publish camera switch topic : %s", topicName.c_str());
    m_node->createPublishCameraSwitchReply(topicName, 1024);

    topicName = driver::ccRosTopicNameCameraSwitch(type, objId);
    CCInfo(u8"orbbec camera switch subscribe topicName : %s", topicName.c_str());
    m_node->createSubscribeCameraReply(topicName,
                                    std::bind(&CCOrbbecMainProcess::onSubscribeCameraSwitch, this, _1), 1024);

    m_getImage = std::make_shared<CCGetImage>();
    CCInfo(u8"init successed");
    CCInfo(u8"%s exit.", __FUNCTION__);
}

bool CCOrbbecMainProcess::openCamera(int triggerType)
{
    CCInfo(u8"%s enter.", __FUNCTION__);
    m_getImage->trigger_type = triggerType;
    bool result = true;
//    result &= m_camera->createHandle();	//  在openDevice中创建
//    result &= m_camera->registerImageCallBack();	// 在openDevice中注册
    result &= m_camera->openDevice();
//    result &= m_camera->setImageBuffer(16);	//
//    result &= m_camera->setResend();	//
//    result &= m_camera->setHeartbeatTime(3000);	//
    result &= 0 == triggerType ? m_camera->setSoftwareTrigger(): m_camera->setHardwareTrigger();
//    result &= m_camera->setAutoPacketSize(); /* 做了修改 函数setPacketSize  -->  函数setAutoPacketSize*/		//
//    result &= m_camera->startGrabbing();
//    result &= m_camera->setGevSCPD(10) ;     /* 新增，设置延迟发包间隔为10，可以降低相机采集速度*/
    result &= m_camera->getPayloadSize();    // 获取数据包大小
    return result;
}

void CCOrbbecMainProcess::run()
{
    CCInfo("%s enter.", __FUNCTION__);
    utility::cc_message::spin(m_node);
    CCInfo("%s exit.", __FUNCTION__);
}

std::shared_ptr<utility::CCMessageNode> CCOrbbecMainProcess::rosNode()
{
    return m_node;
}

std::shared_ptr<CCOrbbecCameraImp>   CCOrbbecMainProcess::cameraIO()
{
    return m_camera;
}

void CCOrbbecMainProcess::onSubscribeGetImage(const CCGetImage::SharedPtr getImage)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    CCDebug("camera [%d] subscribe image enter", m_cameraId);
    CCInfo("xxx camera [%d] subscribe image enter", m_cameraId);

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

void CCOrbbecMainProcess::onSubscribeSetParam(const CCCameraParam::SharedPtr param)
{
    setCameraParam(*param);
}

void CCOrbbecMainProcess::onSubscribeCameraSwitch( const CCDriverSwitch::SharedPtr param)
{
    CCInfo(u8"enter %s.", __FUNCTION__);

    std::shared_ptr<CCMsgReply> pReply = std::make_shared<CCMsgReply>();
    bool bRet = false;
    if (param->OFF == param->instruction){
        bRet = this->m_camera->closeDevice();
    }else if (param->ON == param->instruction){
        bRet = this->m_camera->openDevice();
    }

    if (bRet){
        pReply->reply = pReply->SUCCESS;
    }else{
        pReply->reply = pReply->FAILED;
    }
    pReply->header.set__node_id(param->header.node_id);
    this->m_node->publishCameraSwitchReply(pReply);

    CCInfo(u8"leave %s.", __FUNCTION__);
}

void CCOrbbecMainProcess::setCameraParam(const CCCameraParam &param)
{
    CCInfo("%s enter.", __FUNCTION__);

    // 开始设置参数时间
    m_image->header.times.emplace_back(2);
    m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    float cameraParam[CCCameraParam::PARAM_COUNT];
    if (param.params.size() == param.values.size()) {
       for (uint8_t i = 0; i < param.params.size(); i++) {
           // 曝光时间
           if (param.params[i] == CCCameraParam::PARAM_EXPOSURE) {
               cameraParam[CCCameraParam::PARAM_EXPOSURE] = param.values[i];
           }
           // GAMMA
           else if (param.params[i] == CCCameraParam::PARAM_GAMMA) {
               cameraParam[CCCameraParam::PARAM_GAMMA] = param.values[i];
           }
           // 增益
           else if (param.params[i] == CCCameraParam::PARAM_GAIN) {
               cameraParam[CCCameraParam::PARAM_GAIN] = param.values[i];
           }
           // X偏移
           else if (param.params[i] == CCCameraParam::PARAM_OFFSETX) {
               cameraParam[CCCameraParam::PARAM_OFFSETX] = param.values[i];
           }
           // Y偏移
           else if (param.params[i] == CCCameraParam::PARAM_OFFSETY) {
               cameraParam[CCCameraParam::PARAM_OFFSETY] = param.values[i];
           }
           // 宽
           else if (param.params[i] == CCCameraParam::PARAM_WIDTH) {
               cameraParam[CCCameraParam::PARAM_WIDTH] = param.values[i];
           }
           // 高
           else if (param.params[i] == CCCameraParam::PARAM_HEIGHT) {
               cameraParam[CCCameraParam::PARAM_HEIGHT] = param.values[i];
           }
           // 前置增益
           else if (param.params[i] == CCCameraParam::PARAM_PREAMP_GAIN) {
               cameraParam[CCCameraParam::PARAM_PREAMP_GAIN] = param.values[i];
           }
           // 线扫行频
           else if (param.params[i] == CCCameraParam::PARAM_LINERATE) {
               cameraParam[CCCameraParam::PARAM_LINERATE] = param.values[i];
           }
       }
    }

    // 曝光时间
    if (cameraParam[CCCameraParam::PARAM_EXPOSURE] > -ZERO
        && std::fabs(cameraParam[CCCameraParam::PARAM_EXPOSURE] - m_cameraParam[CCCameraParam::PARAM_EXPOSURE]) > ZERO) {
       m_camera->setExposureTime(cameraParam[CCCameraParam::PARAM_EXPOSURE]);
       m_cameraParam[CCCameraParam::PARAM_EXPOSURE] = cameraParam[CCCameraParam::PARAM_EXPOSURE];
    }
    // GAMMA
    if (std::fabs(cameraParam[CCCameraParam::PARAM_GAMMA] - m_cameraParam[CCCameraParam::PARAM_GAMMA]) > ZERO) {
        if (cameraParam[CCCameraParam::PARAM_GAMMA] > -ZERO) {
            m_camera->setGammaEnable(true);
            m_camera->setGamma(cameraParam[CCCameraParam::PARAM_GAMMA]);
        } else if ((cameraParam[CCCameraParam::PARAM_GAMMA] < (-1 - ZERO))) {
            m_camera->setGammaEnable(false);
        } else {
            // -1不做处理
        }
        m_cameraParam[CCCameraParam::PARAM_GAMMA] = cameraParam[CCCameraParam::PARAM_GAMMA];
    }
    // 增益
    if (cameraParam[CCCameraParam::PARAM_GAIN] > -ZERO
        && std::fabs(cameraParam[CCCameraParam::PARAM_GAIN] - m_cameraParam[CCCameraParam::PARAM_GAIN]) > ZERO) {
        m_camera->setGain(cameraParam[CCCameraParam::PARAM_GAIN]);
        m_cameraParam[CCCameraParam::PARAM_GAIN] = cameraParam[CCCameraParam::PARAM_GAIN];
    }

    // 前置增益
    CCDebug(u8"%s设置前置增益: %f, %f", m_objId.c_str(), cameraParam[CCCameraParam::PARAM_PREAMP_GAIN], m_cameraParam[CCCameraParam::PARAM_PREAMP_GAIN]);
    if (cameraParam[CCCameraParam::PARAM_PREAMP_GAIN] > -ZERO
        && std::fabs(cameraParam[CCCameraParam::PARAM_PREAMP_GAIN] - m_cameraParam[CCCameraParam::PARAM_PREAMP_GAIN]) > ZERO) {
        CCDebug(u8"%s开始设置前置增益: %f, %f", m_objId.c_str(), cameraParam[CCCameraParam::PARAM_PREAMP_GAIN], m_cameraParam[CCCameraParam::PARAM_PREAMP_GAIN]);
        m_camera->setPreampGain(cameraParam[CCCameraParam::PARAM_PREAMP_GAIN]);
        m_cameraParam[CCCameraParam::PARAM_PREAMP_GAIN] = cameraParam[CCCameraParam::PARAM_PREAMP_GAIN];
    }

    // 附加参数
    for (const auto &paramItem : param.item) {
        const auto &featureName = paramItem.feature_name;
        auto fun = [&featureName](const auto &oldItem){
            return featureName == oldItem.feature_name;};
        auto it = std::find_if(m_extraCameraParam.begin(), m_extraCameraParam.end(),
                               fun);
        if (it == m_extraCameraParam.end()) {
            CCCameraParamItem extraParam;
            extraParam.set__feature_name(featureName);
            extraParam.set__value_type(paramItem.value_type);
            m_extraCameraParam.push_back(extraParam);

            it = std::find_if(m_extraCameraParam.begin(), m_extraCameraParam.end(),
                              fun);
        }
        switch (paramItem.value_type) {
        case CCCameraParamItem::TYPE_INT:
            if (paramItem.value_int > 0 &&
                paramItem.value_int != it->value_int) {
               uint32_t value = static_cast<uint32_t>(paramItem.value_int);
               m_camera->setIntValue(featureName, value);
               it->value_int = paramItem.value_int;
            }
            break;
        case CCCameraParamItem::TYPE_FLOAT:
            if (paramItem.value_float > -ZERO &&
                std::fabs(paramItem.value_float - it->value_float) > ZERO) {
               float value = static_cast<float>(paramItem.value_float);
               m_camera->setFloatValue(featureName, value);
               it->value_float = paramItem.value_float;
            }
            break;
        case CCCameraParamItem::TYPE_BOOL:
            if (paramItem.value_int != it->value_int) {
               bool flag = paramItem.value_int == 1;
               m_camera->setBoolVaule(featureName.c_str(), flag);
               it->value_int = paramItem.value_int;
            }
            break;
        case CCCameraParamItem::TYPE_ENUM:
            if (paramItem.value_int != it->value_int) {
               uint32_t value = static_cast<uint32_t>(paramItem.value_int);
               m_camera->setEnumValue(featureName, value);
               it->value_int = paramItem.value_int;
            }
            break;
        case CCCameraParamItem::TYPE_STRING:
            if (paramItem.value_str != it->value_str) {
               m_camera->setStringValue(featureName, paramItem.value_str);
               it->value_str = paramItem.value_str;
            }
            break;
        default:
            break;
        }
    }

    // 设置参数
    CCCameraParam::SharedPtr paramBack = std::make_shared<CCCameraParam>();
    *paramBack = param;
    paramBack->header.set__device_id(m_objId);

    paramBack->params.push_back(param.PARAM_EXPOSURE);
    paramBack->values.push_back(m_camera->getExposureTime());
    paramBack->params.push_back(param.PARAM_GAMMA);
    paramBack->values.push_back(m_camera->getGamma());
    paramBack->params.push_back(param.PARAM_GAIN);
    paramBack->values.push_back(m_camera->getGain());

    // 返回参数
    m_node->publishParam(paramBack);

    // 结束设置参数时间
    m_image->header.times.emplace_back(3);
    m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    CCInfo("%s exit.", __FUNCTION__);
}

void CCOrbbecMainProcess::onImageCaptured(void *buffer, int width, int height, int channel)
{
    CCInfo("xxx %s enter.", __FUNCTION__);
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
            if (m_getImage->error_image_count == 1) {
                CCError(u8"收图张数大于配置数量:%d,%d", u8"请检查相机或流程配置", m_imageIndex, m_getImage->camera_image_id.size());
            } else {
                CCWarn(u8"收图张数大于配置数量:%d,%d", m_imageIndex, m_getImage->camera_image_id.size());
            }
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
