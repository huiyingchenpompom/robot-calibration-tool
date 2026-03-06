#include "CCOptCameraImp.h"
#include "LogClientCommon.h"

CCOptCameraImp::CCOptCameraImp(int cameraId, const std::string &cameraInfo)
    :m_cameraId(cameraId)
{
    initDevice(cameraInfo);
}

CCOptCameraImp::~CCOptCameraImp()
{
    if (m_handle) {   //停止采集
        if (m_isGrabbing) {
            stopGrabbing();
        }
        //关闭设备
        if (m_isOpened) {
            closeDevice();
        }
    }
}

bool CCOptCameraImp::initDevice(const std::string &deviceInfo)
{
#ifdef OPT_USB_AREA
    m_cameraType = "SN";
#else
    m_cameraType = "IP";
#endif
    m_cameraInfo = deviceInfo;
    //Step1 获取所有设备及设备信息
    SCI_DEVICE_INFO_LIST deviceInfoList;
    unsigned int nReVal;
#ifdef OPT_USB_AREA
    nReVal = SciCam_DiscoveryDevices(&deviceInfoList, SciCamTLType::SciCam_TLType_Usb3);
#else
    nReVal = SciCam_DiscoveryDevices(&deviceInfoList, SciCamTLType::SciCam_TLType_Gige);
#endif

    if (nReVal != SCI_CAMERA_OK)
    {
        CCError(u8"扫描相机失败", u8"%s", getErrorInfo(nReVal).c_str());
        return false;
    }
    auto i4tos = [](uint32_t ip){
        struct in_addr addr;
        addr.s_addr = ip;
        char buf[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN is defined to be 16

        if (inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN) == nullptr) {
            // Handle error case if inet_ntop fails
            return std::string();
        }

        return std::string(buf);
    };

    CCInfo(u8"扫描到相机数量：%d", deviceInfoList.count);
    for (unsigned int i = 0; i < deviceInfoList.count; i++)
    {
        if (deviceInfoList.pDevInfo[i].tlType == SciCam_TLType_Gige && m_cameraType == "IP") {
            std::string ip = i4tos(deviceInfoList.pDevInfo[i].info.gigeInfo.ip);
            CCInfo(u8"扫描到网络相机IP：%s", ip.c_str());
            if (ip == m_cameraInfo) {
                CCInfo(u8"匹配到当前设备IP");
                memcpy(&m_deviceInfo, &deviceInfoList.pDevInfo[i], sizeof(SCI_DEVICE_INFO));
                return true;
            }
        } else if (deviceInfoList.pDevInfo[i].tlType == SciCam_TLType_Usb3 && m_cameraType == "SN") {
            std::string sn = std::string(reinterpret_cast<char*>(deviceInfoList.pDevInfo[i].info.usb3Info.serialNumber));
            CCInfo(u8"扫描到USB相机SN：%s", sn.c_str());
            if (sn == m_cameraInfo) {
                CCInfo(u8"匹配到当前设备SN");
                memcpy(&m_deviceInfo, &deviceInfoList.pDevInfo[i], sizeof(SCI_DEVICE_INFO));
                return true;
            }
        }
    }
    return false;
}

bool CCOptCameraImp::createHandle()
{
    if (nullptr != m_handle) {
        CCWarn(u8"%s为%s的相机创建句柄失败，错误原因：该句柄已经创建", m_cameraType.c_str(), m_cameraInfo.c_str());
        return true;
    }
    unsigned int ret = SciCam_CreateDevice(&m_handle, &m_deviceInfo);
    if (SCI_CAMERA_OK != ret) {
        CCError(u8"%s为%s的相机创建句柄失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(ret).c_str());
        m_handle = nullptr;
        return false;
    }
    CCInfo(u8"%s为%s的相机创建句柄成功", m_cameraType.c_str(), m_cameraInfo.c_str());
    return true;
}

bool CCOptCameraImp::destroyHandle()
{
    if (nullptr == m_handle) {
        CCWarn(u8"%s为%s的相机释放句柄失败，错误原因：该句柄未被创建", m_cameraType.c_str(), m_cameraInfo.c_str());
        return true;
    }
    int ret = SciCam_DeleteDevice(m_handle);
    if (SCI_CAMERA_OK != ret) {
        CCError(u8"%s为%s的相机释放句柄失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(ret).c_str());
        return false;
    }
    m_handle = nullptr;
    CCInfo(u8"为%s为%s的相机释放句柄成功", m_cameraType.c_str(), m_cameraInfo.c_str());
    return true;
}

bool CCOptCameraImp::openDevice()
{
    if (nullptr == m_handle) {
        CCError(u8"%s为%s的相机打开失败", u8"错误原因：该相机句柄未被创建", m_cameraType.c_str(), m_cameraInfo.c_str());
        m_isOpened = false;
        return false;
    }
    if (m_isOpened) {
        CCWarn(u8"%s为%s的相机打开失败，错误原因：该相机已被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return true;
    }
    bool result = false;
    int resultCode = SciCam_OpenDevice(m_handle);
    if (SCI_CAMERA_OK == resultCode) {
        result = true;
        m_isOpened = true;
        CCInfo(u8"%s为%s的相机打开成功", m_cameraType.c_str(), m_cameraInfo.c_str());
    } else {
        m_isOpened = false;
        CCError(u8"%s为%s的相机打开失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(resultCode).c_str());
    }

    if (m_cameraType == "IP") {
        result &= setHeartbeatTime();
    }

    if (!isColor(m_isColor, m_pixelFormat)) {
        return false;
    }

    return result;
}

bool CCOptCameraImp::closeDevice()
{
    if (!m_isOpened) {
        CCWarn(u8"%s为%s的相机关闭失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return true;
    }

    if (m_isGrabbing) {
        stopGrabbing();
    }

    int resultCode = SciCam_CloseDevice(m_handle);
    bool result = false;
    if (SCI_CAMERA_OK == resultCode) {
        result = true;
        m_isOpened = false;
        CCInfo(u8"%s为%s的相机关闭成功", m_cameraType.c_str(), m_cameraInfo.c_str());
    } else {
        CCError(u8"%s为%s的相机关闭失败", u8"%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(resultCode).c_str());
    }
    return result;
}

bool CCOptCameraImp::startGrabbing()
{
    if (!m_isOpened) {
        CCError(u8"%s为%s的相机开始采集失败", u8"错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }

    if (m_isGrabbing) {
        CCWarn(u8"%s为%s的相机开始采集失败，错误原因：该相机已经开启采集", m_cameraType.c_str(), m_cameraInfo.c_str());
        return true;
    }

    /* 2000万像素相机偶尔出现SciCam_StartGrabbing调用失败的情况，当失败时，重试10次 */
    bool result = false;
    int resultCode = SCI_CAMERA_OK;
    int maxTryTimes = 10;
    do {
        resultCode = SciCam_StartGrabbing(m_handle);
        if (SCI_CAMERA_OK == resultCode) {
            result = true;
            m_isGrabbing = true;
            CCInfo(u8"%s为%s的相机开始采集成功", m_cameraType.c_str(), m_cameraInfo.c_str());
            break;
        } else {
            result = false;
            (maxTryTimes == 0 ?
            CCError(u8"%s为%s的相机开始采集失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(resultCode).c_str()) :
            CCWarn(u8"%s为%s的相机开始采集失败，错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(resultCode).c_str()));
        }
    } while (maxTryTimes-- > 0);
    return result;
}

bool CCOptCameraImp::stopGrabbing()
{
    if (!m_isOpened) {
        CCError(u8"%s为%s的相机停止采集失败", u8"错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }

    if (!m_isGrabbing) {
        CCWarn(u8"%s为%s的相机停止采集失败，错误原因：该相机已经停止采集", m_cameraType.c_str(), m_cameraInfo.c_str());
        return true;
    }

    bool result = false;
    int resultCode = SciCam_StopGrabbing(m_handle);
    if (SCI_CAMERA_OK == resultCode) {
        m_isGrabbing = false;
        result = true;
        CCInfo(u8"%s为%s的相机停止采集成功", m_cameraType.c_str(), m_cameraInfo.c_str());
    } else {
        CCError(u8"%s为%s的相机停止采集失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(resultCode).c_str());
    }
    return result;
}

bool CCOptCameraImp::setImageBuffer(unsigned int imageBuffer)
{
    if (!m_isOpened) {
        CCError(u8"%s为%s的相机,设置图像缓冲区数量失败", u8"错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }

    // 设置图像缓冲区数量，以解决偶发性的图片丢帧、错位的问题
    int ret = SciCam_SetGrabBufferCount(m_handle, imageBuffer);
    if (SCI_CAMERA_OK != ret) {
        CCError(u8"%s为%s的相机,设置图像缓冲区数量失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(ret).c_str());
        return false;
    } else {
        CCInfo(u8"%s为%s的相机，设置图像缓冲区数量成功%d", m_cameraType.c_str(), m_cameraInfo.c_str(), imageBuffer);
        return true;
    }
}

bool CCOptCameraImp::setResend(unsigned int maxResendPercent, unsigned int resendTimeout)
{
    //if (!m_isOpened) {
    //    CCError(u8"%s为%s的相机设置重发失败", u8"错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
    //    return false;
    //}

    //int ret = SciCam_GIGE_SetResend(m_handle, 1, maxResendPercent, resendTimeout);
    //if (SCI_CAMERA_OK != ret) {
    //    // CCError(u8"%s为%s的相机设置重发失败", u8"错误码%s,错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
    //    return true;
    //} else {
    //    CCInfo(u8"%s为%s的相机设置重发成功", m_cameraType.c_str(), m_cameraInfo.c_str());
    //    return true;
    //}
    return true;
}

bool CCOptCameraImp::setGevSCPD(unsigned int gevSCPD)
{
    return setIntValue("GevSCPD", gevSCPD);
}

bool CCOptCameraImp::setPacketSize(unsigned int packetSize)
{
    return setIntValue("PayloadSize", packetSize);
}

bool CCOptCameraImp::setAutoPacketSize()
{
    if (!m_isOpened) {
        CCError(u8"%s为%s的相机设置自动包长失败", u8"错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }
    unsigned int payloadSize;
    if (getIntValue("SIPayloadTransferSize", payloadSize)) {
        return setIntValue("PayloadSize", payloadSize);
    }
    return false;
}

bool CCOptCameraImp::setHeartbeatTime(unsigned int heartbeatTime)
{
    return setIntValue("DeviceHeartbeatTimeout", heartbeatTime);
}

bool CCOptCameraImp::setAcquisitionMode(AcquisitionMode acquisitionMode)
{
    return setEnumValue("AcquisitionMode", acquisitionMode);
}

bool CCOptCameraImp::getPayloadSize()
{
    if (!getIntValue("PayloadSize", m_payloadSize)) {
        return false;
    }
    CCInfo(u8"%s为%s的相机获取数据包大小 = %d 成功", m_cameraType.c_str(), m_cameraInfo.c_str(), m_payloadSize);
    return true;
}

bool CCOptCameraImp::setHardwareTrigger()
{
    bool ret = true;
    ret &= setEnumValue("TriggerMode", "On");
    ret &= setEnumValue("TriggerSource", "Line1");
    return ret;
}

bool CCOptCameraImp::setSoftwareTrigger()
{
    bool ret = true;
    ret &= setEnumValue("TriggerMode", "On");
    ret &= setEnumValue("TriggerSource", "Software");
    return ret;
}

bool CCOptCameraImp::sendSoftTriggerCommand()
{
    if (!m_isOpened) {
        CCError(u8"%s为%s的相机发送软触发命令失败", u8"错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }

    if (!m_isGrabbing) {
        CCError(u8"%s为%s的相机发送软触发命令失败", u8"错误原因：该相机已经停止采集", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }

    int ret = SciCam_SetCommandValue(m_handle, "TriggerSoftware");
    if (SCI_CAMERA_OK == ret) {
        CCInfo(u8"%s为%s的相机发送软触发命令成功", m_cameraType.c_str(), m_cameraInfo.c_str());
        return true;
    } else {
        CCError(u8"%s为%s的相机发送软触发命令失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(ret).c_str());
        return false;
    }
}

bool CCOptCameraImp::setTriggerActivation(TriggerActivation triggerActivation)
{
    return setEnumValue("TriggerActivation", triggerActivation);
}

bool CCOptCameraImp::setlineDebouncerTime(unsigned int debouncerTime)
{
    //老4K和面阵相机
    bool result = setIntValue("LineDebouncerTime", debouncerTime);
    if (!result) {
        //新4K相机配置
        result = setIntValue("LineDebouncerTimeNs", debouncerTime);
    }
    return result;
}

bool CCOptCameraImp::setTriggerDelayTime(float delayTime)
{
    return setFloatValue("TriggerDelay", delayTime);
}

bool CCOptCameraImp::isDeviceAccessible()
{
    if (!m_isOpened) {
        CCError(u8"%s为%s的相机获取相机状态失败", u8"错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }

    if (SCI_CAMERA_OK != SciCam_IsDeviceOpen(m_handle)) {
        return false;
    }
    return true;
}

bool CCOptCameraImp::isDeviceOpenned()
{
    return m_isOpened;
}

bool CCOptCameraImp::isGrabbing()
{
    return m_isGrabbing;
}

bool CCOptCameraImp::isAreaScan(bool &isAreaScanCamera)
{
    unsigned int iCameraType = 0;
    bool result = getEnumValue("DeviceScanType", iCameraType);

    if(result) {
        if(1 == iCameraType) { // 1是线扫
            isAreaScanCamera = false;
        } else {
            isAreaScanCamera = true;
        }
    }
    return result;
}

bool CCOptCameraImp::isColor(bool &isColorCamera, std::string &pixFormat)
{
    std::string pixelFormat = "";
    bool result = getEnumValue("PixelFormat", pixelFormat);
    if (pixelFormat.find("Mono") != std::string::npos) {
        isColorCamera = false;
    } else {
        isColorCamera = true;
    }
    pixFormat = pixelFormat;
    return result;
}

void payloadReceived(void* payload, void* pUser)
{
    if (pUser) {
        CCOptCameraImp *self = static_cast<CCOptCameraImp*>(pUser);
        self->photoReceivedCallBack(payload, pUser);
    }
}

bool CCOptCameraImp::registerImageCallBack()
{
    if (nullptr == m_handle)
    {
        CCError(u8"%s为%s的相机注册回调函数失败", u8"错误原因：相机句柄未被创建", m_cameraType.c_str(), m_cameraInfo.c_str());
        return false;
    }

    int ret = SciCam_RegisterPayloadCallBack(m_handle, (fnOnPayload)payloadReceived, this, true);
    if (SCI_CAMERA_OK != ret)
    {
        CCError(u8"%s为%s的相机注册回调函数失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(ret).c_str());
        return false;
    }
    CCInfo(u8"%s为%s的相机注册回调函数成功", m_cameraType.c_str(), m_cameraInfo.c_str());
    return true;
}

bool CCOptCameraImp::setRoi(int offsetX, int offsetY, int width, int height)
{
    bool result = true;

    result &= setIntValue("OffsetX", 0);
#ifndef OPT_GIGE_LINE
    result &= setIntValue("OffsetY", 0);
#endif

    if (width >= 0) {
        result &= setIntValue("Width", static_cast<uint32_t>(width));
    }
    if (height >= 0) {
        result &= setIntValue("Height", static_cast<uint32_t>(height));
    }
    if (offsetX >= 0) {
        result &= setIntValue("OffsetX", static_cast<uint32_t>(offsetX));
    }
#ifndef OPT_GIGE_LINE
    if (offsetY >= 0) {
        result &= setIntValue("OffsetY", static_cast<uint32_t>(offsetY));
    }
#endif
    return result;
}

bool CCOptCameraImp::setOffsetX(unsigned int offsetX)
{
    return setIntValue("OffsetX", offsetX);
}

bool CCOptCameraImp::setImageWidth(unsigned int width)
{
    return setIntValue("Width", width);
}

bool CCOptCameraImp::setImageHeight(unsigned int height)
{
    return setIntValue("Height", height);
}

bool CCOptCameraImp::setTriggerMode(bool triggerMode)
{
    bool result = true;
    if (triggerMode)
    {
        result = setEnumValue("TriggerMode", 1);
    }
    else
    {
        result = setEnumValue("TriggerMode", 0);
    }

    return result;
}

bool CCOptCameraImp::setExposureTime(float exposureTime)
{
    const char* nodeName[3] =
    {
        "ExposureTime",
        "ExposureTimeAbs",
        "ExposureTimeRaw"
    };
    for (int i = 0; i < 3; i++) {
        if (!setFloatValue(nodeName[i], exposureTime, true)) {
            bool warn = (i != 2);
            if (setIntValue(nodeName[i], exposureTime, warn)) {
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

bool CCOptCameraImp::setGammaEnable(bool enable)
{
    return setBoolVaule("GammaEnable", enable);
}

bool CCOptCameraImp::setGamma(float gamma)
{
    return setFloatValue("Gamma", gamma);
}

bool CCOptCameraImp::setGain(float gain)
{
    const char* nodeName[2] =
    {
        "Gain",
        "GainRaw"
    };
    for (int i = 0; i < 2; i++) {
        if (!setFloatValue(nodeName[i], gain, true)) {
            bool warn = (i != 1);
            if (setIntValue(nodeName[i], gain, warn)) {
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

bool CCOptCameraImp::setPixelFormat(const std::string &pixelFormat)
{
    int ret = setEnumValue("PixelFormat", pixelFormat);
    if (SCI_CAMERA_OK == ret) {
        CCInfo(u8"%s为%s的相机设置图片格式%s成功", m_cameraType.c_str(), m_cameraInfo.c_str(), pixelFormat.c_str());
        return true;
    } else {
        CCError(u8"%s为%s的相机设置图片格式%s失败", u8"错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), pixelFormat.c_str(), getErrorInfo(ret).c_str());
        return false;
    }
}

bool CCOptCameraImp::setPreampGain(float preampGain)
{
    return setEnumValue("PreampGain", static_cast<unsigned int>(preampGain * 1000));
}

bool CCOptCameraImp::setLineRate(unsigned int rate)
{
    return setIntValue("AcquisitionLineRate", rate);
}

bool CCOptCameraImp::getRoi(unsigned int &offsetX, unsigned int &offsetY, unsigned int &width, unsigned int &height)
{
    bool result = true;
    result &= getIntValue("OffsetX", offsetX);
    result &= getIntValue("OffsetY", offsetY);
    result &= getIntValue("Width",   width);
    result &= getIntValue("Height",  height);
    return result;
}

bool CCOptCameraImp::getOffsetX(unsigned int &offsetX)
{
    return getIntValue("OffsetX", offsetX);
}

bool CCOptCameraImp::getImageWidth(unsigned int &width)
{
    return getIntValue("Width", width);
}

bool CCOptCameraImp::getImageHeight(unsigned int &height)
{
    return getIntValue("Height", height);
}

bool CCOptCameraImp::getMaxImageWidth(unsigned int &maxWidth)
{
    return getIntValue("WidthMax", maxWidth);
}

bool CCOptCameraImp::getMaxImageHeight(unsigned int &maxHeight)
{
    return getIntValue("HeightMax", maxHeight);
}

bool CCOptCameraImp::getTriggerMode(bool &triggerMode)
{
    unsigned int cameraTriggerModee = 0;
    bool result = getEnumValue("TriggerMode", cameraTriggerModee);

    if(cameraTriggerModee == 1) {
        triggerMode = true;
    } else {
        triggerMode = false;
    }

    return result;
}

float CCOptCameraImp::getExposureTime()
{
    const char* nodeName[3] =
    {
        "ExposureTime",
        "ExposureTimeAbs",
        "ExposureTimeRaw"
    };
    float exposureTime = 0.0;
    for (int i = 0; i < 3; i++) {
        if (!getFloatValue(nodeName[i], exposureTime, true)) {
            bool warn = (i != 2);
            unsigned int exposureTimeInt;
            if (getIntValue(nodeName[i], exposureTimeInt, warn)) {
                exposureTime = static_cast<float>(exposureTimeInt);
                break;
            }
        } else {
            break;
        }
    }
    return exposureTime;
}

bool CCOptCameraImp::getGammaEnable(bool &enable)
{
    return getBoolVaule("GammaEnable", enable);
}

float CCOptCameraImp::getGamma()
{
    float gamma = 0.0;
    getFloatValue("Gamma", gamma);
    return gamma;
}

float CCOptCameraImp::getGain()
{
    const char* nodeName[2] =
    {
        "Gain",
        "GainRaw"
    };
    float gain = 0.0;
    for (int i = 0; i < 2; i++) {
        if (!getFloatValue(nodeName[i], gain, true)) {
            bool warn = (i != 1);
            unsigned int gainInt;
            if (getIntValue(nodeName[i], gainInt, warn)) {
                gain = static_cast<float>(gainInt);
                break;
            }
        } else {
            break;
        }
    }
    return gain;
}

bool CCOptCameraImp::setReverseX(bool enable)
{
    return setBoolVaule("ReverseX", enable);
}

bool CCOptCameraImp::setReverseY(bool enable)
{
    return setBoolVaule("ReverseY", enable);
}

bool CCOptCameraImp::setIntValue(const std::string &featureName, unsigned int value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s为%s的相机设置参数%s=%d开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        int ret = SciCam_SetIntValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value, u8"设置", ret, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::setFloatValue(const std::string &featureName, float value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s为%s的相机设置参数%s=%f开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        int ret = SciCam_SetFloatValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, static_cast<double>(value), u8"设置", ret, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::setEnumValue(const std::string &featureName, unsigned int value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            CCWarn(u8"%s为%s的相机未设置参数%s, 直接返回", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
            return true;
        }
        CCInfo(u8"%s为%s的相机设置参数%s=%d开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        int ret = SciCam_SetEnumValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value, u8"设置", ret, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::setEnumValue(const std::string &featureName, const std::string &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            CCWarn(u8"%s为%s的相机未设置参数%s, 直接返回", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
            return true;
        }
        CCInfo(u8"%s为%s的相机设置参数%s=%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value.c_str());
        int ret = SciCam_SetEnumValueByString(m_handle, featureName.c_str(), value.c_str());
        return printCameraSetParamInfo(featureName, value, u8"设置", ret, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::setBoolVaule(const std::string &featureName, bool value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s为%s的相机设置参数%s=%d开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        int ret = SciCam_SetBoolValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value ? "true" : "false", u8"设置", ret, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::setStringValue(const std::string &featureName, const std::string &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s为%s的相机设置参数%s=%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value.c_str());
        int ret = SciCam_SetStringValue(m_handle, featureName.c_str(), value.c_str());
        return printCameraSetParamInfo(featureName, value, u8"设置", ret, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::getIntValue(const std::string &featureName, unsigned int &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        SCI_NODE_VAL_INT getParam;
        memset(&getParam, 0, sizeof(SCI_NODE_VAL_INT));
        CCInfo(u8"%s为%s的相机获取参数%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        int result = SciCam_GetIntValue(m_handle, featureName.c_str(), &getParam);
        if (SCI_CAMERA_OK == result) {
            value = getParam.nVal;
            CCInfo(u8"%s为%s的相机获取参数%s=%d", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        }
        return printCameraSetParamInfo(featureName, value, u8"获取", result, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::getFloatValue(const std::string &featureName, float &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        SCI_NODE_VAL_FLOAT getParam;
        memset(&getParam, 0, sizeof(SCI_NODE_VAL_FLOAT));
        CCInfo(u8"%s为%s的相机获取参数%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        int result = SciCam_GetFloatValue(m_handle, featureName.c_str(), &getParam);
        if (SCI_CAMERA_OK == result)
        {
            value = getParam.dVal;
            CCInfo(u8"%s为%s的相机获取参数%s=%f", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        }
        return printCameraSetParamInfo(featureName, static_cast<double>(value), u8"获取", result, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::getEnumValue(const std::string &featureName, unsigned int &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        SCI_NODE_VAL_ENUM getParam;
        memset(&getParam, 0, sizeof(SCI_NODE_VAL_ENUM));
        CCInfo(u8"%s为%s的相机获取参数%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        int result = SciCam_GetEnumValue(m_handle, featureName.c_str(), &getParam);
        if (SCI_CAMERA_OK == result) {
            value = getParam.nVal;
            CCInfo(u8"%s为%s的相机获取参数%s=%d", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        }
        return printCameraSetParamInfo(featureName, value, u8"获取", result, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::getEnumValue(const std::string &featureName, std::string &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        SCI_NODE_VAL_ENUM getParam;
        memset(&getParam, 0, sizeof(SCI_NODE_VAL_ENUM));
        CCInfo(u8"%s为%s的相机获取参数%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        int result = SciCam_GetEnumValue(m_handle, featureName.c_str(), &getParam);
        if (SCI_CAMERA_OK == result) {
            for (unsigned int i = 0; i < getParam.itemCount; i++) {
                if (getParam.nVal == getParam.items[i].val) {
                    value = std::string(getParam.items[i].desc);
                    CCInfo(u8"%s为%s的相机获取参数%s=%s", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value.c_str());
                }
            }
        }
        return printCameraSetParamInfo(featureName, value, u8"获取", result, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::getBoolVaule(const std::string &featureName, bool &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        CCInfo(u8"%s为%s的相机获取参数%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        int result = SciCam_GetBoolValue(m_handle, featureName.c_str(), &value);
        CCInfo(u8"%s为%s的相机获取参数%s=%d", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value ? "true" : "false", u8"获取", result, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

bool CCOptCameraImp::getStringValue(const std::string &featureName, std::string &value, bool warn)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        SCI_NODE_VAL_STRING getParam;
        memset(&getParam, 0, sizeof(SCI_NODE_VAL_STRING));
        CCInfo(u8"%s为%s的相机获取参数%s开始。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        int result = SciCam_GetStringValue(m_handle, featureName.c_str(), &getParam);
        value = std::to_string(-1);
        if (SCI_CAMERA_OK == result) {
            value = std::string(getParam.val);
            CCInfo(u8"%s为%s的相机获取参数%s=%s", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str(), value.c_str());
        }
        return printCameraSetParamInfo(featureName, value, u8"获取", result, warn);
    } else {
        CCWarn(u8"%s为%s的相机设置参数%s失败，错误原因：该相机未被打开", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
}

void CCOptCameraImp::photoReceivedCallBack(void *payload, void *)
{
    if (payload == nullptr) {
        CCError(u8"%s为%s的相机[%d]收图失败，图片内容为空", "请检查相机状态",
        m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId);
        return;
    }

    SCI_CAM_PAYLOAD_ATTRIBUTE payloadAttribute;
    memset(&payloadAttribute, 0, sizeof(SCI_CAM_PAYLOAD_ATTRIBUTE));
    auto result = SciCam_Payload_GetAttribute(payload, &payloadAttribute);
    if (SCI_CAMERA_OK != result) {
        CCError(u8"%s为%s的相机[%d]回调函数中获取相机参数失败", "错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId, getErrorInfo(result).c_str());
        return;
    }
    bool imgIsComplete = payloadAttribute.isComplete;
    SciCamPayloadMode payloadMode = payloadAttribute.payloadMode;
    uint64_t imgWidth = payloadAttribute.imgAttr.width;
    uint64_t imgHeight = payloadAttribute.imgAttr.height;
    uint64_t framID = payloadAttribute.frameID;
    if (SciCam_PayloadMode_2D != payloadMode) {
        CCError(u8"%s为%s的相机[%d]收到不为2D相机的图片", u8"当前仅支持2D相机", m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId);
    }

    CCInfo(u8"%s为%s的相机[%d]收到图片，帧号：%d, 图片格式：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId, framID, m_pixelFormat.c_str());
    int channel = m_isColor ? 3 : 1;
    if (imgIsComplete) {
        void* imgData = nullptr;
        result = SciCam_Payload_GetImage(payload, &imgData);
        if (result != SCI_CAMERA_OK) {
            CCError(u8"%s为%s的相机[%d]获取相机数据失败", "错误原因：%s", m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId, getErrorInfo(result).c_str());
            return;
        }

        BYTE *dstData = nullptr;
        uint64_t dstImgSize = 0;
        SciCamPixelType desPixelFoemat = m_isColor ? SciCamPixelType::BGR8 : SciCamPixelType::Mono8;
        result = SciCam_Payload_ConvertImageEx(&payloadAttribute.imgAttr, imgData, desPixelFoemat, nullptr, &dstImgSize, true, 0);
        if (result == SCI_CAMERA_OK) {
            if (dstImgSize != imgWidth * imgHeight) {
                CCWarn(u8"%s为%s的相机[%d]当前图片大小%d与预期宽%d * 高%d的大小%d不一致",
                       m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId, dstImgSize, imgWidth, imgHeight, imgWidth * imgHeight);
            }
            dstData = new BYTE[dstImgSize];
            result = SciCam_Payload_ConvertImageEx(&payloadAttribute.imgAttr, imgData, desPixelFoemat, dstData, &dstImgSize, true, 0);
            if (result != SCI_CAMERA_OK) {
                CCError(u8"%s为%s的相机[%d]图片格式转化失败", u8"错位原因:%s", m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId, getErrorInfo(result).c_str());
                return;
            }
        } else {
            CCError(u8"%s为%s的相机[%d]图片格式转化失败", u8"错位原因:%s", m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId, getErrorInfo(result).c_str());
            return;
        }
        m_capturedFunc(dstData, imgWidth, imgHeight, channel);
        if (dstData) {
            delete [] dstData;
            dstData = nullptr;
        }
    } else {
        CCError(u8"%s为%s的相机[%d]收到残帧，", u8"请检查相机传输状态", m_cameraType.c_str(), m_cameraInfo.c_str(), m_cameraId);
        std::unique_ptr<char[]> buffer = std::unique_ptr<char[]>(new char[imgWidth * imgHeight * channel], std::default_delete<char[]>());
        m_capturedFunc(buffer.get(), imgWidth, imgHeight, channel);
    }
}

bool CCOptCameraImp::checkNode(const std::string &featureName, bool write)
{
    SciCamNodeAccessMode penAccessMode;
    unsigned int resultCode = SciCam_GetNodeAccessMode(m_handle, featureName.c_str(), &penAccessMode);
    if (SCI_CAMERA_OK != resultCode) {
        CCError(u8"%s为%s的相机获取节点参数%s失败", u8"%s", m_cameraType.c_str(), m_cameraInfo.c_str(), getErrorInfo(resultCode).c_str());
        return false;
    }

    if (penAccessMode == SciCam_NodeAccessMode_NI || penAccessMode == SciCam_NodeAccessMode_NA) {
        CCWarn(u8"%s为%s的相机设置参数%s失败，该相机的该参数不可用。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    } else if (write && penAccessMode == SciCam_NodeAccessMode_RO) {
        CCWarn(u8"%s为%s的相机设置参数%s失败，该相机的该参数只读。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    } else if (!write && penAccessMode == SciCam_NodeAccessMode_WO) {
        CCWarn(u8"%s为%s的相机设置参数%s失败，该相机的该参数只写。", m_cameraType.c_str(), m_cameraInfo.c_str(), featureName.c_str());
        return false;
    }
    return true;
}

std::string CCOptCameraImp::getErrorInfo(int errorCode)
{
    std::string msg;
    switch (errorCode)
    {
    case SCI_CAMERA_OK: msg += u8"成功"; break;
    case SCI_ERR_CAMERA_INCORRECT_INIT_OBJECT: msg += u8"对象初始化错误"; break;
    case SCI_ERR_CAMERA_PARAM_INVALID: msg += u8"参数无效错误"; break;
    case SCI_ERR_CAMERA_SDK_VERSION_MISMATCH: msg += u8"SDK 版本不匹配错误"; break;
    case SCI_ERR_CAMERA_UNKNOW: msg += u8"未知错误 "; break;
    case SCI_ERR_CAMERA_FUNCTION_CONFLICT: msg += u8"功能或接口调用存在冲突"; break;
    case SCI_ERR_CAMERA_ENUM_DEVICES_FAILED: msg += u8"  搜索相机错误"; break;
    case SCI_ERR_CAMERA_MODIFY_IP_ADDRESS_FAILED: msg += u8"设置相机 IP 失败"; break;
    case SCI_ERR_CAMERA_SET_PERSISTENT_IP_FAILED: msg += u8"设置永久性 IP 失败"; break;
    case SCI_ERR_CAMERA_CREATE_FAILED: msg += u8"创建相机对象失败"; break;
    case SCI_ERR_CAMERA_EXCEPTION: msg += u8"相机操作异常错误"; break;
    case SCI_ERR_CAMERA_NOT_FOUND: msg += u8"查找相机失败错误"; break;
    case SCI_ERR_CAMERA_OFFLINE: msg += u8"相机已掉线"; break;
    case SCI_ERR_CAMERA_OPEN_FAILED: msg += u8"打开相机失败"; break;
    case SCI_ERR_CAMERA_NOT_MISMATCHACQ: msg += u8"相机与采集库不匹配"; break;
    case SCI_ERR_CAMERA_NOT_SUPPORT: msg += u8"相机不支持"; break;
    case SCI_ERR_CAMERA_NOT_OPEN: msg += u8"相机未打开"; break;
    case SCI_ERR_CAMERA_ALLREADY_OPEN: msg += u8"相机已打开"; break;
    case SCI_ERR_CAMERA_OCCUPANCY: msg += u8"相机被占用"; break;
    case SCI_ERR_CAMERA_ROMOVED: msg += u8"相机已被移除或不存在"; break;
    case SCI_ERR_CAMERA_START_GRAB_FAILED: msg += u8"开始采集失败"; break;
    case SCI_ERR_CAMERA_NOT_GRABBING: msg += u8" 相机未打开采集"; break;
    case SCI_ERR_CAMERA_ALREADY_GRABBING: msg += u8"相机已开启采集"; break;
    case SCI_ERR_CAMERA_GRAB_FAILED: msg += u8"相机采集失败"; break;
    case SCI_ERR_CAMERA_GRAB_TIMEOUT: msg += u8"相机采集超时"; break;
    case SCI_ERR_CAMERA_TRIGGER_WAIT_FAILED: msg += u8"等待触发错误"; break;
    case SCI_ERR_CAMERA_SOFTWARE_TRIGGER_FAILED: msg += u8"软触发失败"; break;
    case SCI_ERR_CAMERA_GRAB_EXCEPTION: msg += u8"相机采集其他异常"; break;
    case SCI_ERR_CAMERA_GRAB_INTERRUPTED: msg += u8"采集被中断"; break;
    case SCI_ERR_CAMERA_GRAB_INSUFFICIENT_BUFFER: msg += u8"采集缓冲区不足"; break;
    case SCI_ERR_CAMERA_SET_VALUE_FAILED: msg += u8"设置参数错误"; break;
    case SCI_ERR_CAMERA_GET_VALUE_FAILED: msg += u8"获取参数错误"; break;
    case SCI_ERR_CAMERA_SEND_CMD_FAILED: msg += u8"发送命令错误"; break;
    case SCI_ERR_CAMERA_WRONG_ACK: msg += u8" 接受应答错误"; break;
    case SCI_ERR_CAMERA_FILE_OPEN_FAILED: msg += u8"打开相机配置文件错误"; break;
    case SCI_ERR_CAMERA_NODE_XML_TYPE: msg += u8"xml 类型错误"; break;
    case SCI_ERR_CAMERA_NODE_NAME_INVALID: msg += u8"节点名称无效"; break;
    case SCI_ERR_CAMERA_NODE_READ_FORBIDDEN: msg += u8"该节点禁止读取"; break;
    case SCI_ERR_CAMERA_NODE_WRITE_FORBIDDEN: msg += u8"该节点禁止写入"; break;
    case SCI_ERR_CAMERA_NODE_TYPE_INVALID: msg += u8"该节点类型无效"; break;
    case SCI_ERR_CAMERA_NODE_TYPE_UNMATCH: msg += u8"该节点类型不匹配"; break;
    case SCI_ERR_CAMERA_NODE_VALUE: msg += u8"该节点值错误"; break;
    case SCI_ERR_CAMERA_NODE_MAP_INVALID: msg += u8"节点树无效"; break;
    case SCI_ERR_CAMERA_NODE_GET_ROOT_NODE_FAILED: msg += u8"获取 Root 节点失败"; break;
    case SCI_ERR_CAMERA_NODE_TRAVERSE: msg += u8"遍历节点失败"; break;
    case SCI_ERR_CAMERA_IMAGE_INVALID: msg += u8"图像数据无效"; break;
    case SCI_ERR_CAMERA_IMAGE_CREATE_FAILED: msg += u8"创建图像失败"; break;
    case SCI_ERR_CAMERA_IMAGE_TYPE_NOT_SUPPORT: msg += u8"图像类型不支持"; break;
    case SCI_ERR_CAMERA_IMAGE_NOT_COMPLETE: msg += u8"采集图像不完整"; break;
    case SCI_ERR_CAMERA_IMAGE_DATACONVERT_FAILED: msg += u8"图像转换失败"; break;
    case SCI_ERR_CAMERA_IMAGE_FORMAT: msg += u8"图像格式错误"; break;
    case SCI_ERR_CAMERA_IMAGE_GET_PROPERTY: msg += u8"获取图像属性失败"; break;
    case SCI_ERR_CAMERA_IMAGE_OUT_OF_BOUNDS: msg += u8"图像数据越界"; break;
    case SCI_ERR_CAMERA_CHUNKDATA_EMPTY: msg += u8"ChunkData 为空"; break;
    case SCI_ERR_CAMERA_CHUNKDATA_LENGTH: msg += u8"ChunkData 长度异常"; break;
    case SCI_ERR_CAMERA_CHUNKDATA_PARSING_EXCEPTION: msg += u8"解析 ChunkData 异常"; break;
    case SCI_ERR_CAMERA_MEMORY_ALLOCATION_FAILED: msg += u8"申请内存失败"; break;
    case SCI_ERR_CAMERA_APPEND_BUFFER_FAILED: msg += u8"追加缓存失败"; break;
    case SCI_ERR_CAMERA_SYSTEM_RESOURCE_EXHAUSTED: msg += u8"系统资源耗尽"; break;
    case SCI_ERR_CAMERA_INSUFFICIENT_MEMORY_LENGTH: msg += u8"目标地址内存长度不足"; break;
    case SCI_ERR_CAMERA_GET_CTI_OBJECT_NULL: msg += u8" 获取 CTI 对象为空"; break;
    case SCI_ERR_CAMERA_GET_ANY_PORT_FAILED: msg += u8"获取采集卡端口或数据流端口或设备端口为空"; break;
    case SCI_ERR_CAMERA_GET_IF_PORT_FAILED: msg += u8"获取 IF 端口为空"; break;
    case SCI_ERR_CAMERA_GET_DEVICE_PORT_FAILED: msg += u8"获取设备端口失败"; break;
    case SCI_ERR_CAMERA_GET_DEVICE_DATA_STREAM_PORT_FAILED: msg += u8"获取设备数据流端口失败"; break;
    case SCI_ERR_CAMERA_GET_DEVICE_DATA_STREAM_FAILED: msg += u8"获取设备数据流失败"; break;
    case SCI_ERR_CAMERA_GET_DEVICE_DATA_STREAM_COUNT_ZERO: msg += u8"获取设备数据流个数为 0"; break;
    case SCI_ERR_CAMERA_GET_DATA_STREAM_ID_FAILED: msg += u8"获取设备数据流 ID 失败"; break;
    case SCI_ERR_CAMERA_OPEN_DATA_STREAM_FAILED: msg += u8"打开数据流失败"; break;
    case SCI_ERR_CAMERA_NODEMAP_NULL: msg += u8"获取设备 nodemap 为空"; break;
    case SCI_ERR_CAMERA_PAYLOAD_SIZE_INVALID: msg += u8" 获取 payloadsize 无效"; break;
    case SCI_ERR_CAMERA_REGISTER_IMAGE_EVENT_FAILED: msg += u8"  注册图像事件失败"; break;
    case SCI_ERR_CAMERA_CTI: msg += u8"CTI 错误"; break;
    case SCI_ERR_CAMERA_DS: msg += u8"DS 错误"; break;
    case SCI_ERR_CAMERA_GEN_TL_EVENT: msg += u8"GenTL Event 错误"; break;
    case SCI_ERR_CAMERA_BUF: msg += u8"BUF 错误"; break;
    default: msg += (u8"未知的错误码" + std::to_string(errorCode)); break;
    }

    return msg;
}

template<typename T>
bool CCOptCameraImp::printCameraSetParamInfo(const std::string &featureName, T t, const std::string &action, int status, bool warn)
{
    if(SCI_CAMERA_OK == status) {
        CCInfo(u8"%s为%s的相机%s参数%s成功。", m_cameraType.c_str(), m_cameraInfo.c_str(), action.c_str(), featureName.c_str());
        return true;
    } else {
        if (warn) {
            CCWarn(u8"%s为%s的相机%s参数%s失败, 错误原因：%s。", m_cameraType.c_str(), m_cameraInfo.c_str(), action.c_str(), featureName.c_str(), getErrorInfo(status).c_str());
        } else {
            CCError(u8"%s为%s的相机%s参数%s失败", u8"错误原因：%s。", m_cameraType.c_str(), m_cameraInfo.c_str(), action.c_str(), featureName.c_str(), getErrorInfo(status).c_str());
        }
        return false;
    }
}
