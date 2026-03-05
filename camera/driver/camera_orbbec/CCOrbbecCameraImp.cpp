#include "CCOrbbecCameraImp.h"
#include "LogClientCommon.h"
#include <sstream>
#include <fstream>
#include "opencv2/opencv.hpp"

CCOrbbecCameraImp::CCOrbbecCameraImp(int cameraId, const std::string &cameraIp)
    :m_cameraId(cameraId)
{
    // 设置错误码映射信息
    setErrorInfoMap();
//    initDevice(cameraIp);
}

CCOrbbecCameraImp::~CCOrbbecCameraImp()
{
    //关闭设备
    closeDevice();
}


bool CCOrbbecCameraImp::initDevice(const std::string &deviceIp)
{
    CCInfo("enter %s", __FUNCTION__);
//    ob::Context::setLoggerToFile(OB_LOG_SEVERITY_DEBUG, "d:/orbbec_log/");
//    ob::Context::setLoggerToCallback(OB_LOG_SEVERITY_DEBUG, [&](OBLogSeverity severity, const char *logMsg){
//        CCInfo("%s", logMsg);
//    });

    m_cameraIp = deviceIp;
    m_pDeviceCfg = std::make_shared<ob::Config>();
    m_pDeviceCfg->disableAllStream();
    m_pDeviceCfg->enableStream(OB_STREAM_COLOR);
    m_pDeviceCfg->setFrameAggregateOutputMode(OB_FRAME_AGGREGATE_OUTPUT_COLOR_FRAME_REQUIRE);  // ???

    m_pPipeline = std::make_shared<ob::Pipeline>();
    m_pPipeline->start(m_pDeviceCfg, [this](std::shared_ptr<ob::FrameSet> frameSet){
        handleFrameSetCallback(frameSet);
    });
//    m_pPipeline->start(m_pDeviceCfg);

    m_pDevice = m_pPipeline->getDevice();
//    m_pPipeline->stop();	// 先关闭收图通道，等到用的时候再重新打开
//    setHardwareTrigger();
    m_pDeviceInfo = m_pDevice->getDeviceInfo();
    m_cameraIp = m_pDeviceInfo->getIpAddress();
//    m_pDevice->setIntProperty(OB_PROP_CAPTURE_IMAGE_FRAME_NUMBER_INT, 1);
//    m_pPipeline->stop();	// 先关闭收图通道，等到用的时候再重新打开

    CCInfo("leave %s, camera ip : %s", __FUNCTION__, m_cameraIp.c_str());
    return true;
}

bool CCOrbbecCameraImp::openDevice()
{
    CCInfo(u8"enter %s", __FUNCTION__);

    ob::Context::setLoggerToFile(OB_LOG_SEVERITY_DEBUG, "d:/orbbec_log/");
//    ob::Context::setLoggerToCallback(OB_LOG_SEVERITY_DEBUG, [&](OBLogSeverity severity, const char *logMsg){
//        CCInfo("%s", logMsg);
//    });

    if (nullptr == m_pDeviceCfg){
        m_pDeviceCfg = std::make_shared<ob::Config>();
        m_pDeviceCfg->disableAllStream();
        m_pDeviceCfg->enableStream(OB_STREAM_COLOR);
        m_pDeviceCfg->setFrameAggregateOutputMode(OB_FRAME_AGGREGATE_OUTPUT_COLOR_FRAME_REQUIRE);
    }

    if (nullptr == m_pPipeline){
        m_pPipeline = std::make_shared<ob::Pipeline>();
        m_pPipeline->start(m_pDeviceCfg, [this](std::shared_ptr<ob::FrameSet> frameSet){
            handleFrameSetCallback(frameSet);
        });
        m_pDevice = m_pPipeline->getDevice();
//        setHardwareTrigger();
        m_pDeviceInfo = m_pDevice->getDeviceInfo();
        m_cameraIp = m_pDeviceInfo->getIpAddress();
//        m_pDevice->setIntProperty(OB_PROP_CAPTURE_IMAGE_FRAME_NUMBER_INT, 1);
    }
//        m_pPipeline->enableFrameSync();

    if (!m_isGrabbing){
        startGrabbing();
    }

    CCInfo(u8"leave %s, camera ip : %s", __FUNCTION__, m_cameraIp.c_str());
    return true;
}

bool CCOrbbecCameraImp::getPayloadSize()
{
    return true;
}

bool CCOrbbecCameraImp::closeDevice()
{
    CCInfo(u8"enter %s", __FUNCTION__);

    if (m_isGrabbing) {
        stopGrabbing();
    }

    if (m_pPipeline){
        m_pPipeline->stop();
        m_pPipeline = nullptr;

        m_pDevice = nullptr;
        m_pDeviceCfg = nullptr;
        m_pDeviceInfo = nullptr;

        CCInfo(u8"为IP = %s的相机关闭成功", m_cameraIp.c_str());
    }

    CCInfo(u8"leave %s", __FUNCTION__);
    return true;
}

bool CCOrbbecCameraImp::startGrabbing()
{
    CCInfo(u8"enter startGrabbing");

    if (m_isGrabbing) {
        CCWarn(u8"为IP = %s的相机开始采集失败，错误原因：该相机已经开启采集", m_cameraIp.c_str());
        return true;
    }

    if (m_pPipeline){
//        m_pPipeline->enableFrameSync();   /// ???
        m_isGrabbing = true;
        CCInfo(u8"为IP = %s的相机开始采集成功", m_cameraIp.c_str());
        return true;
    }else{
        CCError(u8"为IP = %s的相机开始采集失败", u8"失败原因：相机设备不存在", m_cameraIp.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::stopGrabbing()
{
    CCInfo(u8"enter stopGrabbing");

    if (!m_isGrabbing) {
        CCWarn(u8"为IP = %s的相机停止采集失败，错误原因：该相机已经停止采集", m_cameraIp.c_str());
        return true;
    }

    if (m_pPipeline){
//        m_pPipeline->disableFrameSync();
        m_isGrabbing = false;
        return true;
    }else{
        CCError(u8"为IP = %s的相机停止采集失败", u8"失败原因：相机设备不存在", m_cameraIp.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::setHeartbeatEnable(bool enable)
{
    m_pDevice->enableHeartbeat(enable);
    return true;
}

bool CCOrbbecCameraImp::setHardwareTrigger()
{
    CCInfo(u8"enter %s : 设置硬触发命令", __FUNCTION__);
    if (nullptr == m_pDevice){
//        openDevice();
        CCError(u8"为IP = %s的相机设置硬触发失败", u8"错误原因：该相机未被打开", m_cameraIp.c_str());
        return false;
    }

    auto multiDeviceSyncCfg = m_pDevice->getMultiDeviceSyncConfig();
    multiDeviceSyncCfg.syncMode = OB_MULTI_DEVICE_SYNC_MODE_HARDWARE_TRIGGERING;
    m_pDevice->setMultiDeviceSyncConfig(multiDeviceSyncCfg);

    CCInfo(u8"为IP = %s的相机设置硬触发成功", m_cameraIp.c_str());
    return true;
}

bool CCOrbbecCameraImp::setSoftwareTrigger()
{
    CCInfo(u8"enter %s", __FUNCTION__);
    if (nullptr == m_pDevice){
        CCError(u8"为IP = %s的相机设置软触发失败", u8"错误原因：该相机未被打开", m_cameraIp.c_str());
        return false;
    }

    auto multiDeviceSyncCfg = m_pDevice->getMultiDeviceSyncConfig();
    multiDeviceSyncCfg.syncMode = OB_MULTI_DEVICE_SYNC_MODE_SOFTWARE_TRIGGERING;
    m_pDevice->setMultiDeviceSyncConfig(multiDeviceSyncCfg);

    CCInfo(u8"enter %s", __FUNCTION__);
    return true;
}

bool CCOrbbecCameraImp::sendSoftTriggerCommand()
{
    CCInfo(u8"enter sendSoftTriggerCommand");

    if (m_pDevice){
        m_pDevice->triggerCapture();
        CCInfo(u8"为IP = %s的相机发送软触发命令成功", m_cameraIp.c_str());
        return true;
    }else{
        CCError(u8"为IP = %s的相机发送软触发命令失败", u8"失败原因：相机设备不存在", m_cameraIp.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::setTriggerDelayTime(float delayTime)
{
    CCInfo(u8"enter setTriggerDelayTime");
//    return setIntValueByID()		???
//    return setFloatValue("TriggerDelay", delayTime);
    return true;
}

bool CCOrbbecCameraImp::isGrabbing()
{
    return m_isGrabbing;
}

bool CCOrbbecCameraImp::setExposureTime(int exposureTime)
{
    return setIntValueById(OB_PROP_COLOR_EXPOSURE_INT, exposureTime);
}

bool CCOrbbecCameraImp::setGammaEnable(bool enable)
{
    if (nullptr != m_pDevice){
//        m_pDevice->setBoolProperty(, enable);		???
        return true;
    }else{
        return false;
    }
}

bool CCOrbbecCameraImp::setGamma(int gamma)
{
    return setIntValueById(OB_PROP_COLOR_GAMMA_INT, gamma);
}

bool CCOrbbecCameraImp::setGain(int gain)
{
    return setIntValueById(OB_PROP_COLOR_GAIN_INT, gain);
}

bool CCOrbbecCameraImp::setPreampGain(int preGain)
{
    if (nullptr != m_pDevice){
//        m_pDevice->setIntProperty(, preGain);			???
        return true;
    }else{
        return false;
    }
}

bool CCOrbbecCameraImp::setLineRate(int rate)
{
    return setIntValueById(OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT, rate);
}

int CCOrbbecCameraImp::getExposureTime()
{
    return getIntValueById(OB_PROP_COLOR_EXPOSURE_INT);
}

bool CCOrbbecCameraImp::getGammaEnable()
{
    bool enable = false;
    if (nullptr != m_pDevice){
//        enable = m_pDevice->getBoolProperty();		???
    }
    return enable;
}

int CCOrbbecCameraImp::getGamma()
{
    return getIntValueById(OB_PROP_COLOR_GAMMA_INT);
}

int CCOrbbecCameraImp::getGain()
{
    return getIntValueById(OB_PROP_COLOR_GAIN_INT);
}

int CCOrbbecCameraImp::getPreampGain()
{
    int preGain = 0;
    if (nullptr != m_pDevice){
//        preGain = m_pDevice->getIntProperty();		???
    }
    return preGain;
}

int CCOrbbecCameraImp::getLineRate()
{
    return getIntValueById(OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT);
}

int CCOrbbecCameraImp::getIntValueById(OBPropertyID id){
    return nullptr == m_pDevice ? 0 : m_pDevice->getIntProperty(id);
}

float CCOrbbecCameraImp::getFloatValueById(OBPropertyID id){
    return nullptr == m_pDevice ? float(0.0) : m_pDevice->getFloatProperty(id);
}

bool CCOrbbecCameraImp::getBoolValueById(OBPropertyID id){
    return nullptr == m_pDevice ? false : m_pDevice->getBoolProperty(id);
}

bool CCOrbbecCameraImp::setIntValueById(OBPropertyID id, int value){
    if (nullptr != m_pDevice){
        m_pDevice->setIntProperty(id, value);
        return true;
    }else{
        return false;
    }
}

bool CCOrbbecCameraImp::setFloatValueById(OBPropertyID id, float value){
    if (nullptr != m_pDevice){
        m_pDevice->setFloatProperty(id, value);
        return true;
    }else{
        return false;
    }
}

bool CCOrbbecCameraImp::setBoolValueById(OBPropertyID id, bool value){
    if (nullptr != m_pDevice){
        m_pDevice->setBoolProperty(id, value);
        return true;
    }else{
        return false;
    }
}

bool CCOrbbecCameraImp::setIntValue(const std::string &featureName, unsigned int value)
{
    if (nullptr != m_pDevice){
        OBPropertyID id;
        if (!checkPropertyItem(featureName, true, id)){
            return false;
        }
        m_pDevice->setIntProperty(id, value);

        CCInfo(u8"为IP = %s的相机设置参数%s成功。", m_cameraIp.c_str(), featureName.c_str());
        return true;
    }else{
        CCWarn(u8"为IP = %s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::setFloatValue(const std::string &featureName, float value)
{
    if (nullptr != m_pDevice){
        OBPropertyID id;
        if (!checkPropertyItem(featureName, true, id)){
            return false;
        }
        m_pDevice->setFloatProperty(id, value);

        CCInfo(u8"为IP = %s的相机设置参数%s成功。", m_cameraIp.c_str(), featureName.c_str());
        return true;
    }else{
        CCWarn(u8"为IP = %s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::setBoolVaule(const std::string &featureName, bool value)
{
    if (nullptr != m_pDevice){
        OBPropertyID id;
        if (!checkPropertyItem(featureName, true, id)){
            return false;
        }
        m_pDevice->setBoolProperty(id, value);

        CCInfo(u8"为IP = %s的相机设置参数%s成功。", m_cameraIp.c_str(), featureName.c_str());
        return true;
    }else{
        CCWarn(u8"为IP = %s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::getIntValue(const std::string &featureName, unsigned int &value)
{
    if (nullptr != m_pDevice){
        OBPropertyID id;
        if (!checkPropertyItem(featureName, false, id)){
            return false;
        }
        value = m_pDevice->getIntProperty(id);

        CCInfo(u8"为IP = %s的相机获取参数%s成功。", m_cameraIp.c_str(), featureName.c_str());
        return true;
    }else{
        CCWarn(u8"为IP = %s的相机获取参数%s失败，错误原因：该相机未被打开", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::getFloatValue(const std::string &featureName, float &value)
{
    if (nullptr != m_pDevice){
        OBPropertyID id;
        if (!checkPropertyItem(featureName, false, id)){
            return false;
        }
        value = m_pDevice->getFloatProperty(id);

        CCInfo(u8"为IP = %s的相机获取参数%s成功。", m_cameraIp.c_str(), featureName.c_str());
        return true;
    }else{
        CCWarn(u8"为IP = %s的相机获取参数%s失败，错误原因：该相机未被打开", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOrbbecCameraImp::getBoolVaule(const std::string &featureName, bool &value)
{
    if (nullptr != m_pDevice){
        OBPropertyID id;
        if (!checkPropertyItem(featureName, false, id)){
            return false;
        }
        value = m_pDevice->getBoolProperty(id);

        CCInfo(u8"为IP = %s的相机获取参数%s成功。", m_cameraIp.c_str(), featureName.c_str());
        return true;
    }else{
        CCWarn(u8"为IP = %s的相机获取参数%s失败，错误原因：该相机未被打开", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }
}

void CCOrbbecCameraImp::setImageCapturedCallback(std::function<void(void*, int, int, int)> func)
{
    CCInfo(u8"enter setImageCapturedCallback");
    m_capturedFunc = func;
}

long long curTimeStamp()
{
    auto now = std::chrono::system_clock::now(); // 获取当前时间点
    auto epoch = now.time_since_epoch(); // 自纪元（1970-01-01）的时长
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    return ms;
    //    std::cout << "毫秒时间戳: " << ms << std::endl; // 输出：1688321542123
}

void CCOrbbecCameraImp::handleFrameSetCallback(std::shared_ptr<ob::FrameSet> frameSet)
{
    CCInfo(u8"enter %s", __FUNCTION__);
    if (nullptr == frameSet || nullptr == frameSet->colorFrame()){
        CCInfo(u8"frameSet is nullptr，or frameSet without colorFrame");
        return;
    }

    if (!m_isGrabbing){
        CCWarn("get frame when m_isGrabbing is false");
        return;
    }

    auto count = frameSet->getCount();
    CCInfo(u8"get frame count : %d", count);
    for (unsigned int i = 0; i < count; ++i)
    {
        auto frame = frameSet->getFrameByIndex(i);
        CCInfo("frame[%d] type : %d", i, frame->getFormat());
        std::shared_ptr<ob::ColorFrame> colorFrame = frame->as<ob::ColorFrame>();

        /*
        {	// for tst begin
            CCInfo("save picture start");
            auto dataLen = colorFrame->getDataSize();
            auto colorFrameData = colorFrame->getData();

            // write file
            auto filenameOut = std::string("d:\\color_frame_") + std::to_string(curTimeStamp()) + std::string(".jpg");
            std::ofstream output_file(filenameOut.c_str(), std::ios::binary);
            if (!output_file.is_open()){
                return;
            }
            output_file.write(reinterpret_cast<const char*>(colorFrameData), dataLen);
            output_file.close();
            CCInfo("save picture end");
        }	// for tst end
        */

        // decode by format
        cv::Mat rstMat;
        switch(colorFrame->getFormat()) {
        case OB_FORMAT_MJPG: {
            cv::Mat rawMat(1, colorFrame->getDataSize(), CV_8UC1, colorFrame->getData());
            rstMat = cv::imdecode(rawMat, 1);
        } break;
        case OB_FORMAT_NV21: {
            cv::Mat rawMat(colorFrame->getHeight() * 3 / 2, colorFrame->getWidth(), CV_8UC1, colorFrame->getData());
            cv::cvtColor(rawMat, rstMat, cv::COLOR_YUV2BGR_NV21);
        } break;
        case OB_FORMAT_YUYV:
        case OB_FORMAT_YUY2: {
            cv::Mat rawMat(colorFrame->getHeight(), colorFrame->getWidth(), CV_8UC2, colorFrame->getData());
            cv::cvtColor(rawMat, rstMat, cv::COLOR_YUV2BGR_YUY2);
        } break;
        case OB_FORMAT_RGB: {
            cv::Mat rawMat(colorFrame->getHeight(), colorFrame->getWidth(), CV_8UC3, colorFrame->getData());
            cv::cvtColor(rawMat, rstMat, cv::COLOR_RGB2BGR);
        } break;
        case OB_FORMAT_RGBA: {
            cv::Mat rawMat(colorFrame->getHeight(), colorFrame->getWidth(), CV_8UC4, colorFrame->getData());
            cv::cvtColor(rawMat, rstMat, cv::COLOR_RGBA2BGR);
        } break;
        case OB_FORMAT_BGRA: {
            cv::Mat rawMat(colorFrame->getHeight(), colorFrame->getWidth(), CV_8UC4, colorFrame->getData());
            cv::cvtColor(rawMat, rstMat, cv::COLOR_BGRA2BGR);
        } break;
        case OB_FORMAT_UYVY: {
            cv::Mat rawMat(colorFrame->getHeight(), colorFrame->getWidth(), CV_8UC2, colorFrame->getData());
            cv::cvtColor(rawMat, rstMat, cv::COLOR_YUV2BGR_UYVY);
        } break;
        case OB_FORMAT_I420: {
            cv::Mat rawMat(colorFrame->getHeight() * 3 / 2, colorFrame->getWidth(), CV_8UC1, colorFrame->getData());
            cv::cvtColor(rawMat, rstMat, cv::COLOR_YUV2BGR_I420);
        } break;
        default:
            CCWarn(u8"unknown format : %d", colorFrame->getFormat());
            break;
        }

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t newBuffSize = rstMat.total()*rstMat.elemSize();
        this->m_payloadSize = newBuffSize;
        width = rstMat.cols;
        height = rstMat.rows;
        auto channels = rstMat.channels();
        unsigned char * pNewBuff = new unsigned char[newBuffSize];
        if (nullptr == pNewBuff){
            CCError(u8"new failed.", "");
            return;
        }
        memcpy(pNewBuff, rstMat.data, newBuffSize);
        m_capturedFunc(pNewBuff, width, height, channels);

        if (nullptr != pNewBuff){
            delete []pNewBuff;
            pNewBuff = nullptr;
        }
    }

    CCInfo(u8"leave %s", __FUNCTION__);
}

bool CCOrbbecCameraImp::checkPropertyItem(const std::string &featureName, bool write, OBPropertyID &id)
{
    auto iter = m_mapOBPropertyItem.find(featureName);
    if (iter == m_mapOBPropertyItem.end()){
        CCWarn(u8"为IP = %s的相机设置参数%s失败，该相机没有该参数。", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }

    if (OB_PERMISSION_READ == iter->second.permission  && write){
        CCWarn(u8"为IP = %s的相机设置参数%s失败，该参数权限为只读。", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }else if (OB_PERMISSION_WRITE == iter->second.permission && !write){
        CCWarn(u8"为IP = %s的相机设置参数%s失败，该参数权限为只写。", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }else if (OB_PERMISSION_DENY == iter->second.permission){
        CCWarn(u8"为IP = %s的相机设置参数%s失败，该参数权限为DENY。", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }else if (OB_PERMISSION_ANY == iter->second.permission){
        CCWarn(u8"为IP = %s的相机设置参数%s失败，该参数权限为ANY, 存在失败风险。", m_cameraIp.c_str(), featureName.c_str());
        return false;
    }else{
        id = iter->second.id;
        return true;
    }
}

void CCOrbbecCameraImp::setErrorInfoMap()
{
    m_mapErrorInfo[OB_EXCEPTION_TYPE_UNKNOWN]=          u8"Unknown error, an error not clearly defined by the SDK ";
    m_mapErrorInfo[OB_EXCEPTION_STD_EXCEPTION]=         u8"Standard exception, an error caused by the standard library ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_CAMERA_DISCONNECTED]=    u8"Camera/Device has been disconnected, the camera/device is not available ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_PLATFORM]=         u8"An error in the SDK adaptation platform layer, which means an error in the implementation of a specific system platform ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_INVALID_VALUE]=    u8"Invalid parameter type exception, need to check input parameter ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE]=  u8"Wrong API call sequence, the API is called in the wrong order or the wrong parameter is passed ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_NOT_IMPLEMENTED]=      u8"SDK and firmware have not yet implemented this function or feature ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_IO]=                   u8"SDK access IO exception error ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_MEMORY]=               u8"SDK access and use memory errors. For example, the frame fails to allocate memory ";
    m_mapErrorInfo[OB_EXCEPTION_TYPE_UNSUPPORTED_OPERATION]=   u8"Unsupported operation type error by SDK or device ";
}

std::string CCOrbbecCameraImp::errorNoToHex(int error)
{
    std::stringstream ss;
    ss << std::hex << error;
    return ss.str();
}

std::string CCOrbbecCameraImp::getErrorInfo(int errorCode)
{
    uint32_t error = static_cast<uint32_t>(errorCode);
    if (m_mapErrorInfo.find(error) != m_mapErrorInfo.end()) {
        return m_mapErrorInfo[error];
    } else {
        return "未知错误码";
    }
}

