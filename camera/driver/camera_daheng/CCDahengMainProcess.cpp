/**
 * Daheng相机驱动主程序
 *
 */
#include "CCDahengMainProcess.h"
#include <boost/format.hpp>
#include "CCDahengCameraRosNode.h"
#include "CCDahengCameraImp.h"
#include "node/camera/CCCameraTopicName.hpp"
#include "utility/utility/CCTime.hpp"
#include "ros_msg/msg/cc_memory_type.hpp"
#include "utility/core/CCUtilityDefine.h"
#include "LogClientCommon.h"
#include "cc_data_id_meta.hpp"

#include "utility/core/datetime/CCDateTime.h"
#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "utility/cc_message/CCMessageFunction.h"
#include <boost/dll.hpp>
#include "utility/core/CCFileSystem.hpp"
#include "node/lens/CCAbstractLens.h"
#include "node/lens/lens_computar_usb/CCComputarUsbLens.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
#define ZERO 0.000001f

CCDahengMainProcess::CCDahengMainProcess(  const std::string &mainType
                             , const std::string &subType
                             , const std::string &objId
                             , int index
                             , const std::string &info
                             , int transmitType
                             , const nlohmann::json &json)
    : m_objId(objId)
    , m_cameraId(index)
    , m_image(std::make_shared<CCImage>())
    , m_camera(std::make_shared<CCDahengCameraImp>(index, info))
    , m_lens(nullptr)
    , m_lensEnabled(false)
{
    CCInfo("%s enter.", __FUNCTION__);
    utility::CCDeviceType type;
    type.mainType = mainType;
    type.subType = subType;
    std::fill(std::begin(m_cameraParam), std::end(m_cameraParam), -1);
    m_camera->setImageCapturedCallback(std::bind(&CCDahengMainProcess::onImageCaptured, this, _1, _2, _3, _4));

    initializeLens(json);
    std::string nodeName = utility::ccRosNodeName(type, objId);
    // nodeName = "driver_" + nodeName;
    m_node = std::make_shared<CCDahengCameraRosNode>(nodeName, transmitType);

    std::string topicName = driver::ccRosTopicNameDriverImageData(type, objId);
    CCInfo("daheng camera created param topic: %s", topicName.c_str());
    m_node->createPublishImage(topicName, 1024);

    topicName = driver::ccRosTopicNameGetImage(type, objId);
    CCInfo("daheng camera created image subscribe: %s", topicName.c_str());
    m_node->createSubscribeGetImage(topicName,
                                    std::bind(&CCDahengMainProcess::onSubscribeGetImage, this, _1), 1024);

    topicName = driver::ccRosTopicNameSetParamBack(type, objId);
    CCInfo("daheng camera created param subscribe: %s", topicName.c_str());
    m_node->createPublishParam(topicName, 1024);

    topicName = driver::ccRosTopicNameSetParam(type, objId);
    CCInfo("daheng camera created param subscribe: %s", topicName.c_str());
    m_node->createSubscribeSetParam(topicName,
                                    std::bind(&CCDahengMainProcess::onSubscribeSetParam, this, _1), 1024);

    m_getImage = std::make_shared<CCGetImage>();
    CCInfo("init successed");
    CCInfo("%s exit.", __FUNCTION__);
}

CCDahengMainProcess::~CCDahengMainProcess()
{
    // 关闭镜头设备
    if (m_isEnableLens && m_lens) {
        closeLens();
    }

    // 释放镜头实例
    m_lens.reset();

    // 释放动态库资源
    for (auto* lib : m_sharedLibs) {
        try {
            delete lib;
        } catch (const std::exception& e) {
            CCError(u8"释放变焦镜头[%s]库失败: %s, %s, %s", u8"请参考错误信息解决",
                    m_lensId.c_str(), m_lensBrand.c_str(), m_lensType, e.what());
        }
    }
    m_sharedLibs.clear();

    m_image = nullptr;
    m_camera = nullptr;
    m_getImage = nullptr;
    CCInfo("%s destructor", __FUNCTION__);
}

bool CCDahengMainProcess::openCamera(int triggerType)
{
    CCInfo("%s enter.", __FUNCTION__);
    m_getImage->trigger_type = triggerType;
    bool result = true;
    result &= m_camera->openCamera(triggerType);
    result &= openLens();
    return result;
}

void CCDahengMainProcess::run()
{
    CCInfo("%s enter.", __FUNCTION__);
    utility::cc_message::spin(m_node);
    CCInfo("%s exit.", __FUNCTION__);
}

std::shared_ptr<utility::CCMessageNode> CCDahengMainProcess::rosNode()
{
    return m_node;
}

std::shared_ptr<CCDahengCameraImp>   CCDahengMainProcess::cameraIO()
{
    return m_camera;
}

void CCDahengMainProcess::onSubscribeGetImage(const CCGetImage::SharedPtr getImage)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 相机取图
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
        auto param = m_getImage->param.at(0);
        param.header.set__device_id(m_objId);
        param.header.set__node_id(m_getImage->header.node_id);
        // setCameraParam(m_getImage->param.at(0));
        setCameraParam(param);
    }

    if (triggerType != m_getImage->trigger_type) {
        if (m_getImage->trigger_type == 0) {
            m_camera->setTriggerMode(CCDahengCameraImp::SoftwareTriggerMode);
        } else {
            m_camera->setTriggerMode(CCDahengCameraImp::HardwareTriggerMode);
        }
    }
    if (m_getImage->trigger_type == 0) { // 软触发
        m_camera->sendSoftwareTriggerCommand();
    }

    // 开始收图时间 软触发&硬触发
    m_image->header.times.emplace_back(4);
    m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    CCDebug("driver %s subscribe image exit", m_objId.c_str());
}

void CCDahengMainProcess::onSubscribeSetParam(const CCCameraParam::SharedPtr param)
{
    setCameraParam(*param);
}

void CCDahengMainProcess::setCameraParam(const CCCameraParam &param)
{
    CCInfo("%s enter.", __FUNCTION__);

    // 开始设置参数时间
    m_image->header.times.emplace_back(2);
    m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    if (!m_camera->isDeviceAccessible()) {
        CCError(u8"大恒相机[%d]掉线，设置参数失败", u8"请检查相机网线、USB线、电源线两头连接是否正常", m_cameraId);
        return;
    }

    float cameraParam[CCCameraParam::PARAM_COUNT];
    bool hasRoi = false;
    if (param.params.size() == param.values.size()) {
        for (uint8_t i = 0; i < param.params.size(); i++) {
            // 曝光时间
            if (param.params[i] == CCCameraParam::PARAM_EXPOSURE) {
                cameraParam[CCCameraParam::PARAM_EXPOSURE] = param.values[i];
                if (cameraParam[CCCameraParam::PARAM_EXPOSURE] > -ZERO
                    && std::fabs(cameraParam[CCCameraParam::PARAM_EXPOSURE] - m_cameraParam[CCCameraParam::PARAM_EXPOSURE]) > ZERO) {
                    m_camera->setExposureTime(cameraParam[CCCameraParam::PARAM_EXPOSURE]);
                    m_cameraParam[CCCameraParam::PARAM_EXPOSURE] = cameraParam[CCCameraParam::PARAM_EXPOSURE];
                }
            }
            // GAMMA
            else if (param.params[i] == CCCameraParam::PARAM_GAMMA) {
                cameraParam[CCCameraParam::PARAM_GAMMA] = param.values[i];
                if (cameraParam[CCCameraParam::PARAM_GAMMA] > -ZERO
                    && std::fabs(cameraParam[CCCameraParam::PARAM_GAMMA] - m_cameraParam[CCCameraParam::PARAM_GAMMA]) > ZERO) {
                    m_camera->setGamma(cameraParam[CCCameraParam::PARAM_GAMMA]);
                    m_cameraParam[CCCameraParam::PARAM_GAMMA] = cameraParam[CCCameraParam::PARAM_GAMMA];
                }
            }
            // 增益
            else if (param.params[i] == CCCameraParam::PARAM_GAIN) {
                cameraParam[CCCameraParam::PARAM_GAIN] = param.values[i];
                if (cameraParam[CCCameraParam::PARAM_GAIN] > -ZERO
                    && std::fabs(cameraParam[CCCameraParam::PARAM_GAIN] - m_cameraParam[CCCameraParam::PARAM_GAIN]) > ZERO) {
                    m_camera->setGain(cameraParam[CCCameraParam::PARAM_GAIN]);
                    m_cameraParam[CCCameraParam::PARAM_GAIN] = cameraParam[CCCameraParam::PARAM_GAIN];
                }
            }
            // X偏移
            else if (param.params[i] == CCCameraParam::PARAM_OFFSETX) {
                hasRoi = true;
                cameraParam[CCCameraParam::PARAM_OFFSETX] = param.values[i];
            }
            // Y偏移
            else if (param.params[i] == CCCameraParam::PARAM_OFFSETY) {
                hasRoi = true;
                cameraParam[CCCameraParam::PARAM_OFFSETY] = param.values[i];
            }
            // 宽
            else if (param.params[i] == CCCameraParam::PARAM_WIDTH) {
                hasRoi = true;
                cameraParam[CCCameraParam::PARAM_WIDTH] = param.values[i];
            }
            // 高
            else if (param.params[i] == CCCameraParam::PARAM_HEIGHT) {
                hasRoi = true;
                cameraParam[CCCameraParam::PARAM_HEIGHT] = param.values[i];
            }
            // 焦距
            else if (param.params[i] == CCCameraParam::PARAM_FOCUS) {
                cameraParam[CCCameraParam::PARAM_FOCUS] = param.values[i];
                if (cameraParam[CCCameraParam::PARAM_FOCUS] > -ZERO
                    && std::fabs(cameraParam[CCCameraParam::PARAM_FOCUS] - m_cameraParam[CCCameraParam::PARAM_FOCUS]) > ZERO) {
                    if(m_lensEnabled) {
                        m_lens->setFocus(cameraParam[CCCameraParam::PARAM_FOCUS]);
                    }
                    m_cameraParam[CCCameraParam::PARAM_FOCUS] = cameraParam[CCCameraParam::PARAM_FOCUS];
                }
            }
            // 光圈
            else if (param.params[i] == CCCameraParam::PARAM_IRIS) {
                cameraParam[CCCameraParam::PARAM_IRIS] = param.values[i];
                if (cameraParam[CCCameraParam::PARAM_IRIS] > -ZERO
                    && std::fabs(cameraParam[CCCameraParam::PARAM_IRIS] - m_cameraParam[CCCameraParam::PARAM_IRIS]) > ZERO) {
                    if(m_lensEnabled){
                        m_lens->setIris(cameraParam[CCCameraParam::PARAM_IRIS]);
                    }
                    m_cameraParam[CCCameraParam::PARAM_IRIS] = cameraParam[CCCameraParam::PARAM_IRIS];
                }
            }
        }
    } else {
        CCError(u8"相机[%d]参数配置个数不一致: %d, %d", u8"请检查参数配置是否正确", m_cameraId, param.params.size(), param.values.size());
    }

    // roi
    if (hasRoi) {
        if (cameraParam[CCCameraParam::PARAM_OFFSETX] > -ZERO &&
            cameraParam[CCCameraParam::PARAM_OFFSETY] > -ZERO &&
            cameraParam[CCCameraParam::PARAM_WIDTH] > -ZERO &&
            cameraParam[CCCameraParam::PARAM_HEIGHT] > -ZERO &&
            (std::fabs(cameraParam[CCCameraParam::PARAM_OFFSETX] - m_cameraParam[CCCameraParam::PARAM_OFFSETX]) > -ZERO ||
             std::fabs(cameraParam[CCCameraParam::PARAM_OFFSETY] - m_cameraParam[CCCameraParam::PARAM_OFFSETY]) > -ZERO ||
             std::fabs(cameraParam[CCCameraParam::PARAM_WIDTH] - m_cameraParam[CCCameraParam::PARAM_WIDTH]) > ZERO ||
             std::fabs(cameraParam[CCCameraParam::PARAM_HEIGHT] - m_cameraParam[CCCameraParam::PARAM_HEIGHT]) > ZERO)) {
            m_camera->stopGrabbing();
            m_camera->setRoi(cameraParam[CCCameraParam::PARAM_OFFSETX], cameraParam[CCCameraParam::PARAM_OFFSETY], cameraParam[CCCameraParam::PARAM_WIDTH], cameraParam[CCCameraParam::PARAM_HEIGHT]);
            m_camera->startGrabbing();
            m_cameraParam[CCCameraParam::PARAM_OFFSETX] = cameraParam[CCCameraParam::PARAM_OFFSETX];
            m_cameraParam[CCCameraParam::PARAM_OFFSETY] = cameraParam[CCCameraParam::PARAM_OFFSETY];
            m_cameraParam[CCCameraParam::PARAM_WIDTH] = cameraParam[CCCameraParam::PARAM_WIDTH];
            m_cameraParam[CCCameraParam::PARAM_HEIGHT] = cameraParam[CCCameraParam::PARAM_HEIGHT];
        }
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
                m_camera->setIntValue(featureName.c_str(), paramItem.value_int);
                it->value_int = paramItem.value_int;
            }
            break;
        case CCCameraParamItem::TYPE_FLOAT:
            if (paramItem.value_float > -ZERO &&
                std::fabs(paramItem.value_float - it->value_float) > ZERO) {
                float value = static_cast<float>(paramItem.value_float);
                m_camera->setFloatValue(featureName.c_str(), value);
                it->value_float = paramItem.value_float;
            }
            break;
        case CCCameraParamItem::TYPE_BOOL:
            if (paramItem.value_int != it->value_int) {
                bool flag = paramItem.value_int == 1;
                m_camera->setBoolValue(featureName.c_str(), flag);
                it->value_int = paramItem.value_int;
            }
            break;
        case CCCameraParamItem::TYPE_ENUM:
            if (paramItem.value_str != it->value_str) {
                m_camera->setEnumValue(featureName.c_str(), paramItem.value_str);
                it->value_str = paramItem.value_str;
            }
            break;
        case CCCameraParamItem::TYPE_STRING:
            if (paramItem.value_str != it->value_str) {
                m_camera->setStringValue(featureName.c_str(), paramItem.value_str);
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

    paramBack->params.push_back(CCCameraParam::PARAM_EXPOSURE);
    paramBack->values.push_back(m_camera->getExposureTime());
    paramBack->params.push_back(CCCameraParam::PARAM_GAMMA);
    paramBack->values.push_back(m_camera->getGamma());
    paramBack->params.push_back(CCCameraParam::PARAM_GAIN);
    paramBack->values.push_back(m_camera->getGain());

    // 返回参数
    m_node->publishParam(paramBack);

    // 结束设置参数时间
    m_image->header.times.emplace_back(3);
    m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    CCInfo("%s exit.", __FUNCTION__);
}

void CCDahengMainProcess::onImageCaptured(void *buffer, int width, int height, int channel)
{
    CCInfo("%s enter.", __FUNCTION__);
    CC_ASSERT(buffer);
//    CCImage::SharedPtr ccImage = m_image;

    // 结束收图时间
    m_image->header.times.emplace_back(5);
    m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());

    m_image->header.set__time(utility::ccNow());
    m_image->header.device_id = m_objId;
    m_image->header.set__node_id(m_getImage->header.node_id);
    m_image->set__id(m_getImage->id);
    CC_SET_CCDATAID_FIELD_VALUE(m_image->id.camera_id, m_cameraId);
    CC_SET_CCDATAID_FIELD_VALUE(m_image->id.image_type, CCImage::RAW_IMAGE);

    m_image->set__memory_type(m_getImage->memory_type);

    m_image->img_width  = width;
    m_image->img_height = height;
    m_image->set__img_channel(channel);
    m_image->set__img_format(channel == 1 ? 0 : 16);

    if (nullptr == buffer) {
        m_image->is_data_valid = false;
    } else {
        m_image->is_data_valid = true;
        const uint32_t size = m_image->img_width * m_image->img_height * m_image->img_channel;

        if (m_image->memory_type == CCMemoryType::MEM_DATA) {
            if (m_image->data.size() != size) {
                m_image->data.resize(size);
            }
            memcpy(m_image->data.data(), buffer, size);
        }
        else {
            CCRosMsgShm msgShm;
            m_image->data.clear();
            auto index = msgShm.createShmIndexWithData(static_cast<uint8_t *>(buffer), size);
            m_image->set__shm_index(index);
            if (!msgShm.isValid(index)) {
                CCError(u8"共享内存分配失败", u8"请检查当前系统各进程内存使用量，并确认节点（推理、前处理、环切、MARK图、存图）是否存在图片积压");
                return;
            }
        }
        m_image->set__data_size(size);
    }
    // 发布图片
    if (m_getImage->camera_image_id.size() > 0) {
        if (m_imageIndex >= 0 && m_imageIndex < m_getImage->camera_image_id.size()) {
            CC_SET_CCDATAID_FIELD_VALUE(m_image->id.camera_image_id, m_getImage->camera_image_id[m_imageIndex]);
        }
        else {
            CC_SET_CCDATAID_FIELD_VALUE(m_image->id.camera_image_id, 0);
            if (m_getImage->error_image_count == 1) {
                CCError(u8"收图张数大于配置数量:%d>%d", u8"1.增大相机防抖参数，2.确认PLC触发逻辑是否和需求一致，3.确认流程配置是否存在错误", m_imageIndex+1, m_getImage->camera_image_id.size());
            } else {
                CCWarn(u8"收图张数大于配置数量:%d>%d", m_imageIndex+1, m_getImage->camera_image_id.size());
            }
        }
    }
    else {
        CC_SET_CCDATAID_FIELD_VALUE(m_image->id.camera_image_id, m_imageIndex + 1);
    }

    ++ m_imageIndex;
    if (m_imageIndex < m_getImage->camera_image_id.size() && m_getImage->trigger_type == 0) {
        m_camera->sendSoftwareTriggerCommand();
    }
    CCInfo(u8"驱动发送图片开始，相机号：%d，周期号：%d，图号：%d", m_image->id.camera_id,m_image->id.plc_cycle_id, m_image->id.camera_image_id);
    m_node->publishImage(m_image);
    CCInfo(u8"驱动发送图片结束，相机号：%d，周期号：%d，图号：%d", m_image->id.camera_id,m_image->id.plc_cycle_id, m_image->id.camera_image_id);

    if (m_imageIndex) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 更新时间
        int size = m_image->header.times.size();
        for (int i = 0; i < size - 4; i++) {
            m_image->header.times.pop_back();
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
        m_image->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    }

    CCInfo("%s exit %s.", __FUNCTION__, m_image->header.node_id.c_str());
}

bool CCDahengMainProcess::openLens()
{
    CCInfo("%s enter.", __FUNCTION__);

    if(!m_isEnableLens){
        CCInfo(u8"未启用镜头");
        return true;
    }

    std::lock_guard<std::mutex> lock(m_lensMutex);

    if (!m_lens) {
        CCInfo(u8"镜头初始化失败");
        return false;
    }

    if(m_lens->openLens(m_lensJson)){
        CCInfo("Lens instance created successfully");
        m_isEnableLens = true;
        m_lensEnabled = true;
    } else {
        // openLens已有错误信息处理
    }

    CCInfo("%s exit.", __FUNCTION__);
    return true;
}

std::shared_ptr<driver::CCAbstractLens> CCDahengMainProcess::loadLens(const std::string &fileName)
{
    CCDebug(u8"%s: %s enter", __FUNCTION__, fileName.c_str());
    std::shared_ptr<driver::CCAbstractLens> lens;

    if (!std::filesystem::exists(fileName)) {
        CCError(u8"相机[%d]镜头[%s,%s]驱动文件%s不存在",
                u8"1.请确认镜头配置是否正确，2.重新安装SC程序[1.9.5.1及以上]",
                m_cameraId, m_lensBrand.c_str(), m_lensType.c_str(), fileName.c_str());
        return lens;
    }else{
        CCInfo(u8"驱动文件存在，准备加载");
    }

    try {
        auto devLib = new boost::dll::shared_library(fileName);

        // 检查是否有创建镜头的函数
        if (!devLib->has("createLensImpl")) {
            CCError(u8"相机[%d]镜头[%s,%s]加载设备库失败", u8"重新安装SC程序[1.9.5.1及以上]",
                    m_cameraId, m_lensBrand.c_str(), m_lensType.c_str(), fileName.c_str());
            delete devLib;
            return lens;
        }

        // 获取创建镜头实例的函数
        const auto &createLensFunc = devLib->get<driver::CCAbstractLens* __cdecl(const std::string&, const std::string&)>("createLensImpl");

        // 根据镜头品牌和类型构造参数
        std::string lensTypeStr =  m_lensBrand + "_" + m_interfaceType;
        std::string lensParams = m_lensJson.dump();  // 将JSON配置转为字符串

        CCInfo(u8"加载设备库 调用函数创建镜头实例 createLensImpl");

        // 调用函数创建镜头实例
        driver::CCAbstractLens* rawLens = createLensFunc(lensTypeStr, lensParams);
        CCInfo(u8"加载设备库 调用函数创建镜头实例 success");

        if (rawLens != nullptr) {
            // 使用自定义删除器，确保通过DLL的销毁函数来释放
            lens = std::shared_ptr<driver::CCAbstractLens>(rawLens, [devLib](driver::CCAbstractLens* p) {
                if (p && devLib->has("destroyLensImpl")) {
                    const auto &destroyLensFunc = devLib->get<void __cdecl(driver::CCAbstractLens*)>("destroyLensImpl");
                    destroyLensFunc(p);
                }
            });

            // 保存动态库指针，防止自动释放
            m_sharedLibs.push_back(devLib);

            CCInfo("Successfully loaded lens from %s, type: %s", fileName.c_str(), lensTypeStr.c_str());
        } else {
            CCError(u8"相机[%d]镜头[%s,%s]创建镜头实例失败", u8"请检查镜头厂家驱动是否安装正确",
                    m_cameraId, m_lensBrand.c_str(), m_lensType.c_str());
            delete devLib;
        }

    }
    catch (const boost::system::system_error& e) {
        const std::string &errMsg = utility::gbkToUtf8(e.what());
        CCError(u8"相机[%d]镜头[%s,%s]加载设备库失败: %s", u8"请检查镜头厂家驱动是否安装正确",
                    m_cameraId, m_lensBrand.c_str(), m_lensType.c_str(), errMsg.c_str());
    }
    catch (const std::exception& e) {
        CCError(u8"相机[%d]镜头[%s,%s]加载设备库失败: %s", u8"请检查镜头厂家驱动是否安装正确",
                    m_cameraId, m_lensBrand.c_str(), m_lensType.c_str(), e.what());
    }

    CCDebug("%s: %s exit", __FUNCTION__, fileName.c_str());
    return lens;
}

bool CCDahengMainProcess::initializeLens(const nlohmann::json &json)
{
    CCInfo("%s enter.", __FUNCTION__);

    m_lensJson = json;
    int lensMode = 0;
    m_lensBrand.clear();
    m_lensType.clear();
    m_lensId.clear();
    m_interfaceType.clear();
    if (json.contains("lens_mode"))
    {
        lensMode = json.at("lens_mode");
    }

    if (json.contains("lens_brand"))
    {
        m_lensBrand = json.at("lens_brand");
    }

    if (json.contains("lens_type"))
    {
        m_lensType = json.at("lens_type");
    }

    if (json.contains("lens_id"))
    {
        m_lensId = json.at("lens_id");
    }

    if(json.contains("interface_type")){
        m_interfaceType = json.at("interface_type");
    }

    if(lensMode == 0){
        m_isEnableLens = false;
        m_lensEnabled = false;
    } else {
        if (m_lensBrand.empty() || m_lensType.empty() || m_lensId.empty() || m_interfaceType.empty()) {
            CCError(u8"打开了镜头模式，但是镜头参数相关参数存在空值, lens_brand:%s, lens_type:%s, lens_id:%s, interface_type:%s",
                    u8"1.若是有网状态下,请通过ST修改相关配置后重新发起调试,会自动下发"
                    u8"2.若是无网状态下,可直接修改device.json中相机下面的相关字段",
                    m_lensBrand.c_str(), m_lensType.c_str(), m_lensId.c_str(), m_interfaceType.c_str());
            m_isEnableLens = false;
            m_lensEnabled = false;
        } else {
            m_isEnableLens = true;
        }
    }

    CCInfo(u8"解析下发的镜头参数,lensClassify: %d, lens_brand:%s, lens_type:%s,lens_id:%s ", lensMode, m_lensBrand.c_str(), m_lensType.c_str(), m_lensId.c_str());
    if(m_isEnableLens){
        std::string fileName =  "plugin/driver/lens_" + m_lensBrand + "_" + m_interfaceType + ".dll";
        CCInfo("Loading lens library - Brand: %s, interface: %s, Library: %s",
               m_lensBrand.c_str(), m_interfaceType.c_str(), fileName.c_str());

        // 动态加载镜头库
        m_lens = this->loadLens(fileName);
        if (m_lens) {
            CCInfo("Successfully created lens instance - Brand: %s, interface: %s",
                   m_lensBrand.c_str(), m_interfaceType.c_str());
            m_lensEnabled = true;
        } else {
            CCError(u8"相机[%d]镜头[%s,%s]创建镜头实例失败", u8"请检查系统内存使用情况",
                    m_cameraId, m_lensBrand.c_str(), m_lensType.c_str());
            m_isEnableLens = false;
            return false;
        }
    }else{
        CCInfo(u8"未启用变焦镜头");
        return false;
    }

    CCInfo("%s exit.", __FUNCTION__);
    return true;
}

bool CCDahengMainProcess::closeLens()
{
    CCInfo("%s enter.", __FUNCTION__);

    if(m_isEnableLens){
        CCInfo("No lens need to close");
        return true;
    }

    if (!m_lens) {
        CCInfo("No lens to close");
        return true;
    }

    bool result = m_lens->closeLens();
    m_lens.reset();
    m_lensEnabled = false;

    CCInfo("%s exit.", __FUNCTION__);
    return result;
}
