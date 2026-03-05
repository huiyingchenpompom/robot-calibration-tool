#include "CCHikCameraImp.h"
#include "MvCameraControl.h"
#include "MvErrorDefine.h"
#include "LogClientCommon.h"
#include <sstream>

CCHikCameraImp::CCHikCameraImp(int cameraId, const std::string &cameraInfo)
    :m_cameraId(cameraId)
{
    // 设置错误码映射信息
    setErrorInfoMap();
    initDevice(cameraInfo);
}

CCHikCameraImp::~CCHikCameraImp()
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

bool CCHikCameraImp::enumDevices(std::map<std::string, MV_CC_DEVICE_INFO> &cameraInfoList)
{
    MV_CC_DEVICE_INFO_LIST deviceInfoList;
    deviceInfoList.nDeviceNum = 0;

    int ret = MV_CC_EnumDevices(m_cameraType == "IP" ? MV_GIGE_DEVICE : MV_USB_DEVICE, &deviceInfoList);
    if (MV_OK == ret) {
        CCInfo(u8"扫描到%d个相机", deviceInfoList.nDeviceNum);
        for (uint32_t ii = 0; ii < deviceInfoList.nDeviceNum; ++ii) {
            MV_CC_DEVICE_INFO deviceInfo;
            memset(&deviceInfo, 0, sizeof(MV_CC_DEVICE_INFO));
            memcpy(&deviceInfo, deviceInfoList.pDeviceInfo[ii], sizeof(MV_CC_DEVICE_INFO));
            std::string cameraInfo;
            if (m_cameraType == "IP") {
                for (int i = 0; i < 4; ++i) {
                    cameraInfo += std::to_string((deviceInfo.SpecialInfo.stGigEInfo.nCurrentIp >> (8 * (3 - i))) & 0xFF);
                    if (i < 3) {
                        cameraInfo += ".";
                    }
                }
            } else {
                cameraInfo =  reinterpret_cast<char *>(deviceInfo.SpecialInfo.stUsb3VInfo.chSerialNumber);
            }
            cameraInfoList[cameraInfo] = deviceInfo;
            CCInfo(u8"当前获取到的%s相机是：%s", m_cameraType.c_str(), cameraInfo.c_str());
        }

        if (cameraInfoList.size() > 0) {
            return true;
        } else {
            CCError(u8"未扫描到%s", u8"请使用使用相机官方软件辅助检查相机是否连接正常，若不正常，请检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str());
            return false;
        }
    } else {
        CCError(u8"扫描相机失败，错误码%s, 错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。", errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    }
}

bool CCHikCameraImp::getCameraDeviceNum(unsigned int &deviceNum)
{
    bool result = false;
    std::map<std::string, MV_CC_DEVICE_INFO> cameraInfoList;
    result = enumDevices(cameraInfoList);
    deviceNum = static_cast<unsigned int>(cameraInfoList.size());
    return result;
}

bool CCHikCameraImp::initDeviceByAdress(const std::string &ipAddress)
{
    m_cameraInfo = ipAddress;
    //Step1 获取所有设备及IP
    std::map<std::string, MV_CC_DEVICE_INFO> deviceInfoList;
    if (!enumDevices(deviceInfoList)) {
        return false;
    }
    //Step2 匹配IP地址
    if (deviceInfoList.find(m_cameraInfo) == deviceInfoList.end()) {
        CCError(u8"未扫描到%s", u8"请打开MVS程序查看IP或SN,是否与ST上的相机配置一致", cameraInfo().c_str());
        return false;
    }
    //Step3 获取设备信息并生成
    memcpy(&m_deviceInfo, &deviceInfoList[m_cameraInfo], sizeof(MV_CC_DEVICE_INFO));
    return true;
}

bool CCHikCameraImp::initDevice(const std::string &deviceInfo)
{
#ifdef HIK_USB_AREA
    m_cameraType = "SN";
#else
    m_cameraType = "IP";
#endif
    m_cameraInfo = deviceInfo;
    //Step1 获取所有设备及设备信息
    std::map<std::string, MV_CC_DEVICE_INFO> deviceInfoList;
    if (!enumDevices(deviceInfoList)) {
        return false;
    }
    //Step2 匹配设备信息
    auto it = deviceInfoList.begin();
    while (it != deviceInfoList.end()) {
        if (it->first == m_cameraInfo) {
            break;
        }
        it++;
    }

    if (it == deviceInfoList.end()) {
        CCError(u8"未扫描到%s", u8"请打开MVS程序查看IP或SN,是否与ST上的相机配置一致", cameraInfo().c_str());
        return false;
    }

    //Step3 获取设备信息并生成
    memcpy(&m_deviceInfo, &deviceInfoList[m_cameraInfo], sizeof(MV_CC_DEVICE_INFO));
    return true;
}

bool CCHikCameraImp::createHandle()
{
    if (nullptr != m_handle) {
        CCWarn(u8"%s创建句柄失败，错误原因：该句柄已经创建", cameraInfo().c_str());
        return true;
    }
    int ret = MV_CC_CreateHandle(&m_handle, &m_deviceInfo);
    if (MV_OK != ret) {
        CCError(u8"%s创建句柄失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    }
    CCInfo(u8"%s创建句柄成功", cameraInfo().c_str());
    return true;
}

bool CCHikCameraImp::destroyHandle()
{
    if (nullptr == m_handle) {
        CCWarn(u8"%s释放句柄失败，错误原因：该句柄未被创建", cameraInfo().c_str());
        return true;
    }
    int ret = MV_CC_DestroyHandle(m_handle);
    if (MV_OK != ret) {
        CCError(u8"%s释放句柄失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    }
    m_handle = nullptr;
    CCInfo(u8"%s释放句柄成功", cameraInfo().c_str());
    return true;
}

bool CCHikCameraImp::openDevice()
{
    if (nullptr == m_handle) {
        m_isOpened = false;
        return false;
    }
    if (m_isOpened) {
        CCWarn(u8"%s打开失败，错误原因：该相机已被打开", cameraInfo().c_str());
        return true;
    }
    bool result = false;
    int resultCode = MV_CC_OpenDevice(m_handle);
    if (MV_OK == resultCode) {
        result = true;
        m_isOpened = true;
        CCInfo(u8"%s打开成功", cameraInfo().c_str());
    } else {
        m_isOpened = false;
        CCError(u8"%s打开失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(resultCode).c_str(), getErrorInfo(resultCode).c_str());
    }

    if (m_cameraType == "IP") {
        result &= setHeartbeatTime();
    }

    return result;
}

bool CCHikCameraImp::closeDevice()
{
    if (!m_isOpened) {
        CCWarn(u8"%s关闭失败，错误原因：该相机未被打开", cameraInfo().c_str());
        return true;
    }

    if (m_isGrabbing) {
        stopGrabbing();
    }

    int resultCode = MV_CC_CloseDevice(m_handle);
    bool result = false;
    if (MV_OK == resultCode) {
        result = true;
        m_isOpened = false;
        CCInfo(u8"%s关闭成功", cameraInfo().c_str());
    } else {
        CCError(u8"%s关闭失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(resultCode).c_str(), getErrorInfo(resultCode).c_str());
    }
    return result;
}

bool CCHikCameraImp::startGrabbing()
{
    if (!m_isOpened) {
        CCError(u8"%s开始采集失败, 该相机未被打开", u8"", cameraInfo().c_str());
        return false;
    }

    if (m_isGrabbing) {
        CCWarn(u8"%s开始采集失败，错误原因：该相机已经开启采集", cameraInfo().c_str());
        return true;
    }

    /* 2000万像素相机偶尔出现MV_CC_StartGrabbing调用失败的情况，当失败时，重试10次 */
    bool result = false;
    int resultCode = MV_OK;
    int maxTryTimes = 10;
    do {
        resultCode = MV_CC_StartGrabbing(m_handle);
        if (MV_OK == resultCode) {
            result = true;
            m_isGrabbing = true;
            CCInfo(u8"%s开始采集成功", cameraInfo().c_str());
            break;
        } else {
            result = false;
            (maxTryTimes == 0 ?
            CCError(u8"%s开始采集失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(resultCode).c_str(), getErrorInfo(resultCode).c_str()) :
            CCWarn(u8"%s开始采集失败,错误码%s,错误原因：%s", cameraInfo().c_str(), errorNoToHex(resultCode).c_str(), getErrorInfo(resultCode).c_str()));
        }
    } while (maxTryTimes-- > 0);
    return result;
}

bool CCHikCameraImp::stopGrabbing()
{
    if (!m_isOpened) {
        CCError(u8"%s停止采集失败", u8"错误原因：该相机未被打开", cameraInfo().c_str());
        return false;
    }

    if (!m_isGrabbing) {
        CCWarn(u8"%s停止采集失败，错误原因：该相机已经停止采集", cameraInfo().c_str());
        return true;
    }

    bool result = false;
    int resultCode = MV_CC_StopGrabbing(m_handle);
    if (MV_OK == resultCode) {
        m_isGrabbing = false;
        result = true;
        CCInfo(u8"%s停止采集成功", cameraInfo().c_str());
    } else {
        CCError(u8"%s停止采集失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(resultCode).c_str(), getErrorInfo(resultCode).c_str());
    }
    return result;
}

bool CCHikCameraImp::setImageBuffer(unsigned int imageBuffer)
{
    if (!m_isOpened) {
        return false;
    }

    // 设置图像缓冲区数量，以解决偶发性的图片丢帧、错位的问题
    int ret = MV_CC_SetImageNodeNum(m_handle, imageBuffer);
    if (MV_OK != ret) {
        CCError(u8"%s设置图像缓冲区数量失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    } else {
        CCInfo(u8"%s设置图像缓冲区数量成功%d", cameraInfo().c_str(), imageBuffer);
        return true;
    }
}

bool CCHikCameraImp::setResend(unsigned int maxResendPercent, unsigned int resendTimeout)
{
    if (!m_isOpened) {
        return false;
    }

    int ret = MV_GIGE_SetResend(m_handle, 1, maxResendPercent, resendTimeout);
    if (MV_OK != ret) {
        CCWarn(u8"%s设置重发失败,错误码%s,错误原因：%s", cameraInfo().c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
    } else {
        CCInfo(u8"%s设置重发成功", cameraInfo().c_str());
    }
    return true;
}

bool CCHikCameraImp::setGevSCPD(unsigned int gevSCPD)
{
    return setIntValue("GevSCPD", gevSCPD);
}

bool CCHikCameraImp::setPacketSize(unsigned int packetSize)
{
    return setIntValue("GevSCPSPacketSize", packetSize);
}

bool CCHikCameraImp::setAutoPacketSize()
{
    if (!m_isOpened) {
        return false;
    }
    unsigned int payloadSize = static_cast<unsigned int>(MV_CC_GetOptimalPacketSize(m_handle));

    return setIntValue("GevSCPSPacketSize", payloadSize);
}

bool CCHikCameraImp::setHeartbeatEnable(bool enable)
{
    return setBoolVaule("GevGVCPHeartbeatDisable", !enable);
}

bool CCHikCameraImp::setHeartbeatTime(unsigned int heartbeatTime)
{
    return setIntValue("GevHeartbeatTimeout", heartbeatTime);
}

bool CCHikCameraImp::setAcquisitionMode(HaikangAcquisitionMode acquisitionMode)
{
    return setEnumValue("AcquisitionMode", acquisitionMode);
}

bool CCHikCameraImp::getPayloadSize()
{
    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    int ret = MV_CC_GetIntValue(m_handle, "PayloadSize", &stParam);
    if (MV_OK != ret) {
        CCError(u8"%s获取数据包大小失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    }

    m_payloadSize = stParam.nCurValue;
    CCInfo(u8"%s获取数据包大小 = %d 成功", cameraInfo().c_str(), m_payloadSize);
    return true;
}

bool CCHikCameraImp::setImageCompression(bool isImageCompression)
{
    m_isImageCompression = isImageCompression;
    if (!setEnumValue("ImageCompressionMode", isImageCompression ? 2 : 0)) {
        CCError(u8"%s设置压缩模式失败", u8"请确认MVS版本是否大于等于3.2.2.1，并且相机支持无损压缩模式", cameraInfo().c_str());
        return false;
    }
    return true;
}

bool CCHikCameraImp::setHardwareTrigger(HaikangTriggerSource triggerSource)
{
    bool ret = true;
    ret &= setEnumValue("TriggerMode", 1);
    ret &= setEnumValue("TriggerSource", triggerSource);
    return ret;
}

bool CCHikCameraImp::setSoftwareTrigger()
{
    bool ret = true;
    ret &= setEnumValue("TriggerMode", 1);
    ret &= setEnumValue("TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    return ret;
}

bool CCHikCameraImp::sendSoftTriggerCommand()
{
    if (!m_isOpened) {
        return false;
    }

    if (!m_isGrabbing) {
        CCError(u8"%s发送软触发命令失败", u8"错误原因：该相机已经停止采集", cameraInfo().c_str());
        return false;
    }

    int ret = MV_CC_SetCommandValue(m_handle, "TriggerSoftware");
    if (MV_OK == ret) {
        CCInfo(u8"%s发送软触发命令成功", cameraInfo().c_str());
        return true;
    } else {
        CCError(u8"%s发送软触发命令失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    }
}

bool CCHikCameraImp::setTriggerActivation(HaikangTriggerActivation triggerActivation)
{
    return setEnumValue("TriggerActivation", triggerActivation);
}

bool CCHikCameraImp::setlineDebouncerTime(unsigned int debouncerTime)
{
    //老4K和面阵相机
    bool result = setIntValue("LineDebouncerTime", debouncerTime);
    if (!result) {
        //新4K相机配置
        result = setIntValue("LineDebouncerTimeNs", debouncerTime);
    }
    return result;
}

bool CCHikCameraImp::setTriggerDelayTime(float delayTime)
{
    return setFloatValue("TriggerDelay", delayTime);
}

bool CCHikCameraImp::isDeviceAccessible()
{
    if (!m_isOpened) {
        return false;
    }

    if (MV_OK != MV_CC_IsDeviceAccessible(&m_deviceInfo, 1)) {
        return false;
    }
    return true;
}

bool CCHikCameraImp::isDeviceOpenned()
{
    return m_isOpened;
}

bool CCHikCameraImp::isGrabbing()
{
    return m_isGrabbing;
}

bool CCHikCameraImp::isAreaScan(bool &isAreaScanCamera)
{
    unsigned int iCameraType = 0;
    bool result = getEnumValue("DeviceScanType", iCameraType);

    if(result) {

        if(1 == iCameraType) {
            isAreaScanCamera = false;
        } else {
            isAreaScanCamera = true;
        }
    }
    return result;
}

bool CCHikCameraImp::isColor(bool &isColorCamera)
{
    std::string strDeviceName = "";
    bool result = getStringValue("DeviceModelName", strDeviceName);
    isColorCamera = strDeviceName.find_last_of('M', strDeviceName.size() - 3) == std::string::npos &&
                    strDeviceName.find_last_of('m', strDeviceName.size() - 3) == std::string::npos;
    return result;
}

bool CCHikCameraImp::registerImageCallBack()
{
    if (nullptr == m_handle)
    {
        CCError(u8"%s注册回调函数失败", u8"错误原因：相机句柄未被创建", cameraInfo().c_str());
        return false;
    }

    int ret = MV_CC_RegisterImageCallBackEx(m_handle, photoReceivedCallBack, this);
    if (MV_OK != ret)
    {
        CCError(u8"%s注册回调函数失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    }
    CCInfo(u8"%s注册回调函数成功", cameraInfo().c_str());
    return true;
}

bool CCHikCameraImp::setRoi(int offsetX, int offsetY, int width, int height)
{
    bool result = true;

    result &= setIntValue("OffsetX", 0);
#ifndef HIK_GIGE_LINE
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
#ifndef HIK_GIGE_LINE
    if (offsetY >= 0) {
        result &= setIntValue("OffsetY", static_cast<uint32_t>(offsetY));
    }
#endif
    return result;
}

bool CCHikCameraImp::setOffsetX(unsigned int offsetX)
{
    return setIntValue("OffsetX", offsetX);
}

bool CCHikCameraImp::setImageWidth(unsigned int width)
{
    return setIntValue("Width", width);
}

bool CCHikCameraImp::setImageHeight(unsigned int height)
{
    return setIntValue("Height", height);
}

bool CCHikCameraImp::setTriggerSource(HaikangTriggerSource triggerSource)
{
    return setEnumValue("TriggerSource", triggerSource);
}

bool CCHikCameraImp::setTriggerMode(bool triggerMode)
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

bool CCHikCameraImp::setExposureTime(float exposureTime)
{
    return setFloatValue("ExposureTime", exposureTime);
}

bool CCHikCameraImp::setGammaEnable(bool enable)
{
    return setBoolVaule("GammaEnable", enable);
}

bool CCHikCameraImp::setGamma(float gamma)
{
    return setFloatValue("Gamma", gamma);
}

bool CCHikCameraImp::setGain(float gain)
{
    return setFloatValue("Gain", gain);
}

bool CCHikCameraImp::setPixelFormat(PixelFormat pixelFormat)
{
    int ret = MV_CC_SetPixelFormat(m_handle, pixelFormat);
    if (MV_OK == ret) {
        CCInfo(u8"%s设置图片格式%d成功", cameraInfo().c_str(), pixelFormat);
        return true;
    } else {
        CCError(u8"%s设置图片格式%d失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), pixelFormat, errorNoToHex(ret).c_str(), getErrorInfo(ret).c_str());
        return false;
    }
}

bool CCHikCameraImp::setPreampGain(float preampGain)
{
    return setEnumValue("PreampGain", static_cast<unsigned int>(preampGain * 1000));
}

bool CCHikCameraImp::setLineRate(unsigned int rate)
{
    return setIntValue("AcquisitionLineRate", rate);
}

bool CCHikCameraImp::getRoi(unsigned int &offsetX, unsigned int &offsetY, unsigned int &width, unsigned int &height)
{
    bool result = true;
    result &= getIntValue("OffsetX", offsetX);
    result &= getIntValue("OffsetY", offsetY);
    result &= getIntValue("Width",   width);
    result &= getIntValue("Height",  height);
    return result;
}

bool CCHikCameraImp::getOffsetX(unsigned int &offsetX)
{
    return getIntValue("OffsetX", offsetX);
}

bool CCHikCameraImp::getImageWidth(unsigned int &width)
{
    return getIntValue("Width", width);
}

bool CCHikCameraImp::getImageHeight(unsigned int &height)
{
    return getIntValue("Height", height);
}

bool CCHikCameraImp::getMaxImageWidth(unsigned int &maxWidth)
{
    return getIntValue("WidthMax", maxWidth);
}

bool CCHikCameraImp::getMaxImageHeight(unsigned int &maxHeight)
{
    return getIntValue("HeightMax", maxHeight);
}

bool CCHikCameraImp::getTriggerSource(HaikangTriggerSource &triggerSource)
{
    unsigned int triggerSourceValue = 0;
    bool result = getEnumValue("TriggerSource", triggerSourceValue);
    triggerSource = HaikangTriggerSource(triggerSourceValue);
    return result;
}

bool CCHikCameraImp::getTriggerMode(bool &triggerMode)
{
    unsigned int cameraTriggerModee = 0;
    bool result = getEnumValue("TriggerMode", cameraTriggerModee);

    if(cameraTriggerModee == 1)
    {
        triggerMode = true;
    }
    else
    {
        triggerMode = false;
    }

    return result;
}

float CCHikCameraImp::getExposureTime()
{
    float exposureTime = 0.0;
    getFloatValue("ExposureTime", exposureTime);
    return exposureTime;
}

bool CCHikCameraImp::getGammaEnable(bool &enable)
{
    return getBoolVaule("GammaEnable", enable);
}

float CCHikCameraImp::getGamma()
{
    float gamma = 0.0;
    getFloatValue("Gamma", gamma);
    return gamma;
}

float CCHikCameraImp::getGain()
{
    float gain = 0.0;
    getFloatValue("Gain", gain);
    return gain;
}

bool CCHikCameraImp::isValidPixelFormat(MvGvspPixelType type)
{
    switch(type)
    {
    case PixelType_Gvsp_Mono8:
    case PixelType_Gvsp_BayerGR8:
    case PixelType_Gvsp_BayerRG8:
    case PixelType_Gvsp_BayerGB8:
    case PixelType_Gvsp_BayerBG8:
    case PixelType_Gvsp_RGB8_Packed:
    case PixelType_Gvsp_BGR8_Packed:
    case PixelType_Gvsp_HB_Mono8:
    case PixelType_Gvsp_HB_BayerGR8:
    case PixelType_Gvsp_HB_BayerRG8:
    case PixelType_Gvsp_HB_BayerGB8:
    case PixelType_Gvsp_HB_BayerBG8:
    case PixelType_Gvsp_HB_RGB8_Packed:
    case PixelType_Gvsp_HB_BGR8_Packed:
        return true;
    default:
        return false;
    }
}

bool CCHikCameraImp::isColor(MvGvspPixelType type)
{
    switch(type)
    {
    case PixelType_Gvsp_BayerGR8:
    case PixelType_Gvsp_BayerRG8:
    case PixelType_Gvsp_BayerGB8:
    case PixelType_Gvsp_BayerBG8:
    case PixelType_Gvsp_RGB8_Packed:
    case PixelType_Gvsp_BGR8_Packed:
    case PixelType_Gvsp_HB_BayerGR8:
    case PixelType_Gvsp_HB_BayerRG8:
    case PixelType_Gvsp_HB_BayerGB8:
    case PixelType_Gvsp_HB_BayerBG8:
    case PixelType_Gvsp_HB_RGB8_Packed:
    case PixelType_Gvsp_HB_BGR8_Packed:
        return true;
    default:
        return false;
    }
}

bool CCHikCameraImp::setReverseX(bool enable)
{
    return setBoolVaule("ReverseX", enable);
}

bool CCHikCameraImp::setReverseY(bool enable)
{
    return setBoolVaule("ReverseY", enable);
}

bool CCHikCameraImp::setIntValue(const std::string &featureName, unsigned int value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s设置参数%s=%d开始。", cameraInfo().c_str(), featureName.c_str(), value);
        int ret = MV_CC_SetIntValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value, u8"设置", ret);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::setFloatValue(const std::string &featureName, float value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s设置参数%s=%f开始。", cameraInfo().c_str(), featureName.c_str(), value);
        int ret = MV_CC_SetFloatValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, static_cast<double>(value), u8"设置", ret);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::setEnumValue(const std::string &featureName, unsigned int value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            CCWarn(u8"%s未设置参数%s, 直接返回", cameraInfo().c_str(), featureName.c_str());
            return true;
        }
        CCInfo(u8"%s设置参数%s=%d开始。", cameraInfo().c_str(), featureName.c_str(), value);
        int ret = MV_CC_SetEnumValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value, u8"设置", ret);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::setBoolVaule(const std::string &featureName, bool value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s设置参数%s=%d开始。", cameraInfo().c_str(), featureName.c_str(), value);
        int ret = MV_CC_SetBoolValue(m_handle, featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value ? "true" : "false", u8"设置", ret);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::setStringValue(const std::string &featureName, const std::string &value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, true)) {
            return true;
        }
        CCInfo(u8"%s设置参数%s=%s开始。", cameraInfo().c_str(), featureName.c_str(), value.c_str());
        int ret = MV_CC_SetStringValue(m_handle, featureName.c_str(), value.c_str());
        return printCameraSetParamInfo(featureName, value, u8"设置", ret);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::getIntValue(const std::string &featureName, unsigned int &value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        MVCC_INTVALUE getParam;
        memset(&getParam, 0, sizeof(MVCC_INTVALUE));
        CCInfo(u8"%s获取参数%s开始。", cameraInfo().c_str(), featureName.c_str());
        int result = MV_CC_GetIntValue(m_handle, featureName.c_str(), &getParam);
        CCInfo(u8"%s获取参数%s=%d", cameraInfo().c_str(), featureName.c_str(), value);
        value = 0;
        if (MV_OK == result) {
            value = getParam.nCurValue;
        }
        return printCameraSetParamInfo(featureName, value, u8"获取", result);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::getFloatValue(const std::string &featureName, float &value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        MVCC_FLOATVALUE getParam;
        memset(&getParam, 0, sizeof(MVCC_FLOATVALUE));
        CCInfo(u8"%s获取参数%s开始。", cameraInfo().c_str(), featureName.c_str());
        int result = MV_CC_GetFloatValue(m_handle, featureName.c_str(), &getParam);
        CCInfo(u8"%s获取参数%s=%f", cameraInfo().c_str(), featureName.c_str(), value);
        value = -1;
        if (MV_OK == result)
        {
            value = getParam.fCurValue;
        }
        return printCameraSetParamInfo(featureName, static_cast<double>(value), u8"获取", result);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::getEnumValue(const std::string &featureName, unsigned int &value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        MVCC_ENUMVALUE getParam;
        memset(&getParam, 0, sizeof(MVCC_ENUMVALUE));
        CCInfo(u8"%s获取参数%s开始。", cameraInfo().c_str(), featureName.c_str());
        int result = MV_CC_GetEnumValue(m_handle, featureName.c_str(), &getParam);
        CCInfo(u8"%s获取参数%s=%d", cameraInfo().c_str(), featureName.c_str(), value);
        value = 0;
        if (MV_OK == result) {
            value = getParam.nCurValue;
        }
        return printCameraSetParamInfo(featureName, value, u8"获取", result);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::getBoolVaule(const std::string &featureName, bool &value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        value = false;
        CCInfo(u8"%s获取参数%s开始。", cameraInfo().c_str(), featureName.c_str());
        int result = MV_CC_GetBoolValue(m_handle, featureName.c_str(), &value);
        CCInfo(u8"%s获取参数%s=%d", cameraInfo().c_str(), featureName.c_str(), value);
        return printCameraSetParamInfo(featureName, value ? "true" : "false", u8"获取", result);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

bool CCHikCameraImp::getStringValue(const std::string &featureName, std::string &value)
{
    if (m_isOpened) {
        if (!checkNode(featureName, false)) {
            return true;
        }
        MVCC_STRINGVALUE getParam;
        memset(&getParam, 0, sizeof(MVCC_STRINGVALUE));
        CCInfo(u8"%s获取参数%s开始。", cameraInfo().c_str(), featureName.c_str());
        int result = MV_CC_GetStringValue(m_handle, featureName.c_str(), &getParam);
        CCInfo(u8"%s获取参数%s=%s", cameraInfo().c_str(), featureName.c_str(), value.c_str());
        value = std::to_string(-1);
        if (MV_OK == result) {
            value = std::string(getParam.chCurValue);
        }
        return printCameraSetParamInfo(featureName, value, u8"获取", result);
    } else {
        CCWarn(u8"%s设置参数%s失败，错误原因：该相机未被打开", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
}

const std::string CCHikCameraImp::cameraInfo()
{
    return m_cameraType + u8"为" + m_cameraInfo + u8"的相机[" + std::to_string(m_cameraId) + "]";
}

void CCHikCameraImp::photoReceivedCallBack(unsigned char *data, MV_FRAME_OUT_INFO_EX *frameInfo, void *pUser)
{
    CCHikCameraImp *self = static_cast<CCHikCameraImp*>(pUser);
    CCInfo(u8"%s收到图片，帧号：%d", self->cameraInfo().c_str(), frameInfo->nFrameNum);

    if (self->isValidPixelFormat(frameInfo->enPixelType))
    {
        bool isColourful = self->isColor(frameInfo->enPixelType); // 是否是彩色的

        uint32_t width = 0;
        uint32_t height = 0;
        bool newBuffer = false;
        unsigned char *pDstBuf = nullptr;
        if (self->m_isImageCompression) { // 是否是无损压缩
            MV_CC_HB_DECODE_PARAM stDecodeParam;
            memset(&stDecodeParam, 0, sizeof(MV_CC_HB_DECODE_PARAM));
            newBuffer = true;
            pDstBuf = new unsigned char[self->m_payloadSize];
            if (nullptr == pDstBuf) {
                CCError(u8"%s无损压缩解码失败", u8"错误原因：申请内存失败", self->cameraInfo().c_str());
                return;
            }

            // 无损压缩解码
            stDecodeParam.pSrcBuf = data;
            stDecodeParam.nSrcLen = frameInfo->nFrameLen;
            stDecodeParam.pDstBuf = pDstBuf;
            stDecodeParam.nDstBufSize = self->m_payloadSize;
            stDecodeParam.enDstPixelType = isColourful ? PixelType_Gvsp_BGR8_Packed : PixelType_Gvsp_Mono8;
            int ret = MV_CC_HB_Decode(self->m_handle, &stDecodeParam);
            if (MV_OK != ret)
            {
                CCError(u8"%s无损压缩解码失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", self->cameraInfo().c_str(), self->errorNoToHex(ret).c_str(), self->getErrorInfo(ret).c_str());
                delete [] pDstBuf;
                return;
            }
            width = stDecodeParam.nWidth;
            height = stDecodeParam.nHeight;
        } else {
            width = frameInfo->nWidth;
            height = frameInfo->nHeight;
            if (isColourful && PixelType_Gvsp_BGR8_Packed != frameInfo->enPixelType) {
                unsigned int size = width * height * 3;
                newBuffer = true;
                pDstBuf = new unsigned char[size];
                if (nullptr == pDstBuf) {
                    CCError(u8"%s转化格式失败失败", u8"错误原因：申请内存失败", self->cameraInfo().c_str());
                    return;
                }

                MV_CC_PIXEL_CONVERT_PARAM para;
                para.nWidth =  static_cast<uint16_t>(width);
                para.nHeight = static_cast<uint16_t>(height);
                para.enSrcPixelType = frameInfo->enPixelType;
                para.pSrcData = data;
                para.nSrcDataLen = frameInfo->nFrameLen;
                para.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
                para.pDstBuffer = pDstBuf;
                para.nDstLen = size;
                para.nDstBufferSize = size;
                int ret = MV_CC_ConvertPixelType(self->m_handle, &para);
                if (MV_OK != ret)
                {
                    CCError(u8"%s转化格式为BGR8失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", self->cameraInfo().c_str(),
                            self->errorNoToHex(ret).c_str(), self->getErrorInfo(ret).c_str());
                    delete [] pDstBuf;
                    return;
                } else {
                    CCInfo(u8"%s转化格式为BGR8成功", self->cameraInfo().c_str());
                }
            } else {
                pDstBuf = data;
            }
        }

        self->m_capturedFunc(pDstBuf, width, height, isColourful ? 3 : 1);
        if (pDstBuf && newBuffer) {
            delete [] pDstBuf;
        }
    } else {
        std::shared_ptr<char[]> buffer(new char[frameInfo->nWidth * frameInfo->nHeight], std::default_delete<char[]>());
        self->m_capturedFunc(buffer.get(), frameInfo->nWidth, frameInfo->nHeight, 1);
        CCError(u8"%s接收到无效的相片格式: %d", u8"请修改为常用的图片格式RGB8、BGR8、Mono8",self->cameraInfo().c_str(), frameInfo->enPixelType);
    }
}

bool CCHikCameraImp::checkNode(const std::string &featureName, bool write)
{
    MV_XML_AccessMode penAccessMode;
    MV_XML_GetNodeAccessMode(m_handle, featureName.c_str(), &penAccessMode);
    if (penAccessMode == AM_NI || penAccessMode == AM_NA) {
        CCWarn(u8"%s设置参数%s失败，该相机的该参数不可用。", cameraInfo().c_str(), featureName.c_str());
        return false;
    } else if (write && penAccessMode == AM_RO) {
        CCWarn(u8"%s设置参数%s失败，该相机的该参数只读。", cameraInfo().c_str(), featureName.c_str());
        return false;
    } else if (!write && penAccessMode == AM_WO) {
        CCWarn(u8"%s设置参数%s失败，该相机的该参数只写。", cameraInfo().c_str(), featureName.c_str());
        return false;
    }
    return true;
}

void CCHikCameraImp::setErrorInfoMap()
{
    // 通用错误码定义:范围0x80000000-0x800000FF
    m_mapErrorInfo[MV_E_HANDLE] = u8"错误或无效的句柄";
    m_mapErrorInfo[MV_E_SUPPORT] = u8"不支持的功能";
    m_mapErrorInfo[MV_E_BUFOVER] = u8"缓存已满";
    m_mapErrorInfo[MV_E_CALLORDER] = u8"函数调用顺序错误";
    m_mapErrorInfo[MV_E_PARAMETER] = u8"错误的参数";
    m_mapErrorInfo[MV_E_RESOURCE] = u8"资源申请失败";
    m_mapErrorInfo[MV_E_NODATA] = u8"无数据";
    m_mapErrorInfo[MV_E_PRECONDITION] = u8"前置条件有误，或运行环境已发生变化";
    m_mapErrorInfo[MV_E_VERSION] = u8"版本不匹配";
    m_mapErrorInfo[MV_E_NOENOUGH_BUF] = u8"传入的内存空间不足";
    m_mapErrorInfo[MV_E_ABNORMAL_IMAGE] = u8"异常图像，可能是丢包导致图像不完整";
    m_mapErrorInfo[MV_E_LOAD_LIBRARY] = u8"动态导入DLL失败";
    m_mapErrorInfo[MV_E_NOOUTBUF] = u8"没有可输出的缓存节点";
    m_mapErrorInfo[MV_E_UNKNOW] = u8"通用的未知错误";
    // GenICam系列错误:范围0x80000100-0x800001FF
    m_mapErrorInfo[MV_E_GC_GENERIC] = u8"通用错误";
    m_mapErrorInfo[MV_E_GC_ARGUMENT] = u8"参数非法";
    m_mapErrorInfo[MV_E_GC_RANGE] = u8"值超出范围";
    m_mapErrorInfo[MV_E_GC_PROPERTY] = u8"属性";
    m_mapErrorInfo[MV_E_GC_RUNTIME] = u8"运行环境有问题";
    m_mapErrorInfo[MV_E_GC_LOGICAL] = u8"逻辑错误";
    m_mapErrorInfo[MV_E_GC_ACCESS] = u8"节点访问条件有误";
    m_mapErrorInfo[MV_E_GC_TIMEOUT] = u8"超时";
    m_mapErrorInfo[MV_E_GC_DYNAMICCAST] = u8"转换异常";
    m_mapErrorInfo[MV_E_GC_UNKNOW] = u8"GenICam未知错误";
    // GigE_STATUS对应的错误码:范围0x80000200-0x800002FF
    m_mapErrorInfo[MV_E_NOT_IMPLEMENTED] = u8"命令不被设备支持";
    m_mapErrorInfo[MV_E_INVALID_ADDRESS] = u8"访问的目标地址不存在";
    m_mapErrorInfo[MV_E_WRITE_PROTECT] = u8"目标地址不可写";
    m_mapErrorInfo[MV_E_ACCESS_DENIED] = u8"设备无访问权限";
    m_mapErrorInfo[MV_E_BUSY] = u8"设备忙，或网络断开";
    m_mapErrorInfo[MV_E_PACKET] = u8"网络包数据错误";
    m_mapErrorInfo[MV_E_NETER] = u8"网络相关错误";
    // GigE相机特有的错误码
    m_mapErrorInfo[MV_E_IP_CONFLICT] = u8"设备IP冲突";
    // USB_STATUS对应的错误码:范围0x80000300-0x800003FF
    m_mapErrorInfo[MV_E_USB_READ] = u8"读usb出错";
    m_mapErrorInfo[MV_E_USB_WRITE] = u8"写usb出错";
    m_mapErrorInfo[MV_E_USB_DEVICE] = u8"设备异常";
    m_mapErrorInfo[MV_E_USB_GENICAM] = u8"GenICam相关错误";
    m_mapErrorInfo[MV_E_USB_BANDWIDTH] = u8"带宽不足";
    m_mapErrorInfo[MV_E_USB_UNKNOW] = u8"USB未知错误";
    // 升级时对应的错误码:范围0x80000400-0x800004FF
    m_mapErrorInfo[MV_E_UPG_FILE_MISMATCH] = u8"升级固件不匹配";
    m_mapErrorInfo[MV_E_UPG_LANGUSGE_MISMATCH] = u8"升级固件语言不匹配";
    m_mapErrorInfo[MV_E_UPG_CONFLICT] = u8"升级冲突（设备已经在升级了再次请求升级即返回此错误）";
    m_mapErrorInfo[MV_E_UPG_INNER_ERR] = u8"升级时相机内部出现错误";
    m_mapErrorInfo[MV_E_UPG_UNKNOW] = u8"升级时未知错误";
}

std::string CCHikCameraImp::errorNoToHex(int error)
{
    std::stringstream ss;
    ss << std::hex << error;
    return ss.str();
}

std::string CCHikCameraImp::getErrorInfo(int errorCode)
{
    uint32_t error = static_cast<uint32_t>(errorCode);
    if (m_mapErrorInfo.find(error) != m_mapErrorInfo.end()) {
        return m_mapErrorInfo[error];
    } else {
        return u8"未知错误码";
    }
}

template<typename T>
bool CCHikCameraImp::printCameraSetParamInfo(const std::string &featureName, T t, const std::string &action, int status)
{

    if(MV_OK == status) {
        CCInfo(u8"%s%s参数%s成功。", cameraInfo().c_str(), action.c_str(), featureName.c_str());
        return true;
    } else {
        CCError(u8"%s%s参数%s失败,错误码%s,错误原因：%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), action.c_str(), featureName.c_str(), errorNoToHex(status).c_str(), getErrorInfo(status).c_str());
        return false;
    }
}
