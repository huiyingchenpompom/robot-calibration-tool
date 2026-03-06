/**
 * Basler相机采集图像
 *
 */
#include "CCBaslerCameraImp.h"
#include "LogClientCommon.h"
#include "utility/core/CCFileSystem.hpp"

int CCBaslerCameraImp::ms_initCount = 0;

CCBaslerCameraImp::CCBaslerCameraImp(int cameraId, const std::string &info)
    :m_cameraId(cameraId),
    m_cameraInfo(info)
{
    initLib();
#ifdef BASLER_USB_AREA
    m_cameraType = "SN";
#else
    m_cameraType = "IP";
#endif
}

CCBaslerCameraImp::~CCBaslerCameraImp()
{
    closeDevice();
    releaseLib();
}

bool CCBaslerCameraImp::openDevice(const std::string &info)
{
    try
    {
        CDeviceInfo cInfo;
        String_t str = String_t(info.c_str());
        if (m_cameraType == "IP") {
            cInfo.SetIpAddress(str);
        } else {
            cInfo.SetSerialNumber(str);
        }
        auto device = CTlFactory::GetInstance().CreateDevice(cInfo);
        m_instanceCamera.Attach(device);
        m_instanceCamera.RegisterImageEventHandler(this, RegistrationMode_Append, Cleanup_None);
        m_instanceCamera.RegisterConfiguration(this, RegistrationMode_Append, Cleanup_None);
        m_instanceCamera.GrabCameraEvents = true;
        m_instanceCamera.Open();
        m_instanceCamera.MaxNumBuffer = 16;
        m_isOffline = false;
        if (m_cameraType == "IP") {
            GenApi::INodeMap& tlNodeMap = m_instanceCamera.GetTLNodeMap();
            CIntegerParameter hbParam( tlNodeMap, "HeartbeatTimeout" );
            CCInfo("Current heartbeat: %lld", hbParam.GetValue() );
            hbParam.SetValue(3000);
            CCInfo("set heartbeat to %lld\n",hbParam.GetValue());
        }
        // 设置防抖滤波100
        setDoubleValue("LineDebouncerTimeAbs", 100);
        // 设置Gamma模式
        setEnumValue("GammaSelector", "User");
        setBoolValue("GammaEnable", true);
    }
    catch (GenICam::GenericException &e)
    {
        CCError(u8"%s打开失败: %s", u8"1.参考报错信息。2.检查相机官方软件是否占用相机。3.使用相机官方软件检查相机取图是否正常。4.如果不正常，请检查相机网线、USB线、电源线两头连接是否正常。",
                cameraInfo().c_str(), e.GetDescription());
        return false;
    }
    CCInfo("basler camera connect success");
    return true;
}

bool CCBaslerCameraImp::closeDevice()
{
    CCInfo("basler camera close enter");
    try
    {
        if(m_instanceCamera.IsOpen())
        {
            m_instanceCamera.DetachDevice();
            m_instanceCamera.Close();
            m_isOffline = false;
        }
    }
    catch (GenICam::GenericException &e)
    {
        CCError(u8"%s关闭错误: %s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。",
                cameraInfo().c_str(), e.GetDescription());
        return false;
    }
    CCInfo("basler camera close exit");

    return true;
}

bool CCBaslerCameraImp::startGrabbing()
{
    try {
        // Configure single frame acquisition on the camera
        setEnumValue("AcquisitionMode", "Continuous");
        m_instanceCamera.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    }
    catch (GenICam::GenericException &e)
    {
        CCError(u8"%s开始采集失败: %s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。",
                cameraInfo().c_str(), e.GetDescription());
        return false;
    }
    return true;
}

bool CCBaslerCameraImp::stopGrabbing()
{
    try
    {
        if (m_instanceCamera.IsGrabbing())
        {
            m_instanceCamera.StopGrabbing();
        }
    }
    catch (GenICam::GenericException &e)
    {
        CCError(u8"%s停止采集失败: %s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。",
                cameraInfo().c_str(), e.GetDescription());
        return false;
    }
    return true;
}

void CCBaslerCameraImp::initLib()
{
    try {
        if (1 != ++ms_initCount) {
            CCInfo(u8"%s加载SDK动态库，当前相机计数:%d", cameraInfo().c_str(), ms_initCount);
            return;
        }
        PylonInitialize(); //初始化库
    } catch (GenICam::GenericException &e) {
        CCError(u8"%s加载SDK动态库失败:%s", u8"推荐安装相机软件pylon Viewer 7.4.0", cameraInfo().c_str(), e.what());
    }
}

void CCBaslerCameraImp::releaseLib()
{
    try {
        if (0 == ms_initCount) {
            return;
        }

        if (--ms_initCount > 0) {
            CCInfo(u8"%s卸载SDK动态库，当前相机计数:%d", cameraInfo().c_str(), ms_initCount);
            return;
        }
        PylonTerminate(); //初始化库
    } catch (GenICam::GenericException& e) {
        CCError(u8"%s卸载SDK动态库失败:%s", u8"推荐安装相机软件pylon Viewer 7.4.0", cameraInfo().c_str(), e.what());
    }
}

bool CCBaslerCameraImp::sendSoftTriggerCommand()
{
    if(!m_instanceCamera.IsOpen() || !m_instanceCamera.IsGrabbing())
    {
        return false;
    }
    CCInfo(u8"%s执行软触发", cameraInfo().c_str());
    if (m_instanceCamera.WaitForFrameTriggerReady(1000, TimeoutHandling_ThrowException))
    {
        m_instanceCamera.ExecuteSoftwareTrigger();
        return true;
    }
    return true;
}

bool CCBaslerCameraImp::setTriggerDelay(double delayTime)
{
    return setDoubleValue("TriggerDelay", delayTime);
}

void CCBaslerCameraImp::setDataChunk(const char *featureName)
{
    // setBoolValue("ChunkModeActive", true);
    setEnumValue("ChunkSelector", featureName);
    // setBoolValue("ChunkEnable", true);
}

bool CCBaslerCameraImp::setGevSCPSPacketSize(int size)
{
    return setIntValue("GevSCPSPacketSize", size);
}

bool CCBaslerCameraImp::setImageBuffer(int imageBuffer)
{
    return setIntValue("MaxNumBuffer", imageBuffer);
}

bool CCBaslerCameraImp::isDeviceAccessible()
{
    CCInfo(u8"%s online status:%s", cameraInfo().c_str(), m_isOffline ? "offline" : "online");
    return !m_isOffline;
}

void CCBaslerCameraImp::getFrame()
{
   try {
        if(m_instanceCamera.IsOpen() && m_instanceCamera.IsGrabbing()) {
            if (m_instanceCamera.WaitForFrameTriggerReady(1000, TimeoutHandling_ThrowException)) {
                m_instanceCamera.ExecuteSoftwareTrigger();
                CCInfo(u8"%s软触发成功", cameraInfo().c_str());
                return;
            }
        }
   }
   catch (GenICam::GenericException &e) {
        CCError(("%s软触发失败: " + std::string(e.GetDescription())).c_str(),
                u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str());
   }

   CCError("Camera status error when get frame", u8"请联系开发人员");
}

double CCBaslerCameraImp::getExposureTime()
{
#ifdef BASLER_USB_AREA
    return getDoubleValue("ExposureTime");
#else
    return getIntValue("ExposureTimeRaw");
#endif
}

double CCBaslerCameraImp::getGamma()
{
    return getDoubleValue("Gamma");
}

double CCBaslerCameraImp::getGain()
{
#ifdef BASLER_USB_AREA
    return getDoubleValue("Gain");
#else
    return getIntValue("GainRaw");
#endif
}

bool CCBaslerCameraImp::setExposureTime(double dbVal)
{
    CCDebug("basler set exposure: %lf", dbVal);
#ifdef BASLER_USB_AREA
    return setDoubleValue("ExposureTime", dbVal);
#else
    return setIntValue("ExposureTimeRaw", static_cast<int>(dbVal));
#endif
}

bool CCBaslerCameraImp::setGamma(double dbVal)
{
    CCDebug("basler set gamma: %lf", dbVal);
    return setDoubleValue("Gamma", dbVal);
}

bool CCBaslerCameraImp::setGain(double dbVal)
{
    CCDebug("basler set gain: %lf", dbVal);
#ifdef BASLER_USB_AREA
    return setDoubleValue("Gain", dbVal);
#else
    return setIntValue("GainRaw", static_cast<int>(dbVal));
#endif

}


bool CCBaslerCameraImp::openCamera(int triggerType)
{
    CCInfo("%s enter.", __FUNCTION__);
    if (!openDevice(m_cameraInfo)) {
        return false;
    }
    if (triggerType == 0) {
        setSoftTrigger();
    }
    else {
        setHardwareTrigger();
    }
    startGrabbing();
    CCInfo("basler camera connect success");
    return true;
}

bool CCBaslerCameraImp::setRoi(int offsetX, int offsetY, int width, int height)
{
    CCInfo("%s enter.", __FUNCTION__);
    bool result = true;
    result &= setIntValue("OffsetX", 0);
    result &= setIntValue("OffsetY", 0);

    if (width >= 1) {
        result &= setIntValue("Width", static_cast<uint32_t>(width));
    }
    if (height >= 1) {
        result &= setIntValue("Height", static_cast<uint32_t>(height));
    }
    if (offsetX >= 0) {
        result &= setIntValue("OffsetX", static_cast<uint32_t>(offsetX));
    }
    if (offsetY >= 0) {
        result &= setIntValue("OffsetY", static_cast<uint32_t>(offsetY));
    }
    return result;
}

void CCBaslerCameraImp::OnImageGrabbed(CInstantCamera &, const CGrabResultPtr &ptrGrabResult)
{
    CCInfo(u8"%s接收到图片，帧号：%d", cameraInfo().c_str(), ptrGrabResult->GetID());
    if (ptrGrabResult->GrabSucceeded()) {
        if(ptrGrabResult->GetPixelType() == PixelType_Mono8) {   // 灰度图
            m_capturedFunc(ptrGrabResult->GetBuffer(), ptrGrabResult->GetWidth(), ptrGrabResult->GetHeight(), 1);
        } else {
            CPylonImage targetImage;
            CImageFormatConverter converter;
            converter.OutputPixelFormat = PixelType_BGR8packed;
            converter.Convert(targetImage, ptrGrabResult);
            m_capturedFunc(targetImage.GetBuffer(), ptrGrabResult->GetWidth(), ptrGrabResult->GetHeight(), 3);
        }
    } else {
        std::string errorString = ptrGrabResult->GetErrorDescription().c_str();
        CCError(u8"%s收图错误,错误码：%d，错误原因：%s", u8"参考报错信息，请优化网卡/USB相关驱动属性（缓冲区、巨型帧、双工模式等），优化系统负载，或增加算力硬件",
                cameraInfo().c_str(), ptrGrabResult->GetErrorCode(), errorString.c_str());
        int channel = ptrGrabResult->GetPixelType() == PixelType_Mono8 ? 1 : 3;
        std::unique_ptr<char[]> buffer = std::unique_ptr<char[]>(new char[ptrGrabResult->GetWidth() * ptrGrabResult->GetHeight() * channel], std::default_delete<char[]>());
        m_capturedFunc(buffer.get(), ptrGrabResult->GetWidth(), ptrGrabResult->GetHeight(), channel);
    }
}

void CCBaslerCameraImp::OnCameraDeviceRemoved(CInstantCamera &)
{
    m_isOffline = true;
    CCError(u8"%s掉线", u8"请重新插拔相机网线或USB，并检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str());
}

bool CCBaslerCameraImp::setEnumValue(const char *featureName, unsigned int dbValue)
{
    try
    {
        if(featureName)
        {
            CEnumerationPtr enumPtr = m_instanceCamera.GetNodeMap().GetNode(featureName);

            if (IsWritable(enumPtr) && IsAvailable(enumPtr->GetEntry(dbValue)))
            {
                enumPtr->SetIntValue(dbValue);
                return printCameraParamInfo(featureName, dbValue, Info);
            }
            else
            {
                return printCameraParamInfo(featureName, dbValue, Warning, "参数不可写或节点无效");
            }
        }
        else
        {
            return printCameraParamInfo("未知参数", dbValue, Error, "节点参数为空");
        }
    }
    catch (const GenericException &e)
    {
        return printCameraParamInfo(featureName, dbValue, Error, e.GetDescription());
    }
}

bool CCBaslerCameraImp::setEnumValue(const char *featureName, const char *dbValue)
{
    try
    {
        if(featureName)
        {
            CEnumerationPtr enumPtr = m_instanceCamera.GetNodeMap().GetNode(featureName);

            if (IsWritable(enumPtr) && IsAvailable(enumPtr->GetEntryByName(dbValue)))
            {
                enumPtr->FromString(dbValue);
                return printCameraParamInfo(featureName, dbValue, Info);
            }
            else
            {
                return printCameraParamInfo(featureName, dbValue, Warning, "参数不可写或节点无效");
            }
        }
        else
        {
            return printCameraParamInfo("未知参数", dbValue, Error, "节点参数为空");
        }
    }
    catch (const GenericException &e)
    {
        return printCameraParamInfo(featureName, dbValue, Error, e.GetDescription());
    }
}

bool CCBaslerCameraImp::setBoolValue(const char *featureName, bool dbValue)
{
    try
    {
        if(featureName)
        {
            const CBooleanPtr boolPtr = m_instanceCamera.GetNodeMap().GetNode(featureName);

            if (IsWritable(boolPtr))
            {
                boolPtr->SetValue(dbValue);
                return printCameraParamInfo(featureName, dbValue, Info);
            }
            else
            {
                return printCameraParamInfo(featureName, dbValue, Warning, "参数不可写或节点无效");
            }
        }
        else
        {
            return printCameraParamInfo("未知参数", dbValue, Error, "节点参数为空");
        }
    }
    catch (const GenericException &e)
    {
        return printCameraParamInfo(featureName, dbValue, Error, e.GetDescription());
    }
}

int64_t CCBaslerCameraImp::getIntValue(const char *featureName)
{
    INodeMap &cameraNodeMap = m_instanceCamera.GetNodeMap();
    const CIntegerPtr intPtr = cameraNodeMap.GetNode(featureName);
    return intPtr->GetValue();
}

double CCBaslerCameraImp::getDoubleValue(const char *featureName)
{
    INodeMap &cameraNodeMap = m_instanceCamera.GetNodeMap();
    const CFloatPtr floatPtr = cameraNodeMap.GetNode(featureName);
    return floatPtr->GetValue();
}

bool CCBaslerCameraImp::getEnumValue(const char *featureName, int &dbValue)
{
    INodeMap &cameraNodeMap = m_instanceCamera.GetNodeMap();
    CEnumerationPtr enumPtr = cameraNodeMap.GetNode(featureName);
    dbValue = static_cast<int>(enumPtr->GetIntValue());
    return true;
}

bool CCBaslerCameraImp::getEnumValue(const char *featureName, std::string &dbValue)
{
    INodeMap &cameraNodeMap = m_instanceCamera.GetNodeMap();
    CEnumerationPtr enumPtr = cameraNodeMap.GetNode(featureName);
    dbValue = std::string(enumPtr->ToString());
    return true;
}

bool CCBaslerCameraImp::setIntValue(const char *featureName, int64_t dbValue)
{
    try
    {
        if (featureName)
        {
            const CIntegerPtr intPtr = m_instanceCamera.GetNodeMap().GetNode(featureName);
            if (IsWritable(intPtr))
            {
                dbValue = Adjust(dbValue, intPtr->GetMin(), intPtr->GetMax(), intPtr->GetInc());
                intPtr->SetValue(dbValue);
                return printCameraParamInfo(featureName, dbValue, Info);
            }
            else
            {
                return printCameraParamInfo(featureName, dbValue, Warning, "参数不可写");
            }
        }
        else
        {
            return printCameraParamInfo("未知参数", dbValue, Error, "节点参数为空");
        }
    }
    catch (GenICam::GenericException &e)
    {
        return printCameraParamInfo(featureName, dbValue, Error, e.GetDescription());
    }

}

bool CCBaslerCameraImp::setDoubleValue(const char *featureName, double dbValue)
{
    try
    {
        if(featureName)
        {
            const CFloatPtr floatPtr = m_instanceCamera.GetNodeMap().GetNode(featureName);

            if (IsWritable(floatPtr))
            {
                floatPtr->SetValue(dbValue);
                return printCameraParamInfo(featureName, dbValue, Info);
            }
            else
            {
                return printCameraParamInfo(featureName, dbValue, Warning, "参数不可写");
            }
        }
        else
        {
            return printCameraParamInfo("未知参数", dbValue, Error, "节点参数为空");
        }
    }
    catch (const GenericException &e)
    {
        return printCameraParamInfo(featureName, dbValue, Error, e.GetDescription());
    }
}


int64_t CCBaslerCameraImp::Adjust(int64_t srcVal, int64_t minVal, int64_t maxVal, int64_t incVal)
{
    /* Check the input parameters. */
    if (incVal <= 0)
    {
        /*Negative increments are invalid.*/
        throw LOGICAL_ERROR_EXCEPTION("Unexpected increment %d", incVal);
    }

    if (minVal > maxVal)
    {
        /* Minimum must not be bigger than or equal to the maximum.*/
        throw LOGICAL_ERROR_EXCEPTION("minimum bigger than maximum.");
    }

    /*Check the lower bound.*/
    if (srcVal < minVal)
    {
        return minVal;
    }

    /*Check the upper bound.*/
    if (srcVal > maxVal)
    {
        return maxVal;
    }

    /*Check the increment.*/
    if (incVal == 1)
    {
        /*Special case: all values are valid.*/
        return srcVal;
    }
    else
    {
        /*The value must be min + (n * inc).*/
        /*Due to the integer division, the value will be rounded down.*/
        return minVal + ( ((srcVal - minVal) / incVal) * incVal );
    }
}


template<typename T>
bool CCBaslerCameraImp::printCameraParamInfo(const char *featureName, T value, InfoLevel level, std::string errorString)
{
    if(level == Info)
    {
        CCInfo(u8"%s设置参数 %s 成功", cameraInfo().c_str(), featureName);
        return true;
    } else if (level == Warning) {
        const auto &utf8ErrMsg = utility::gbkToUtf8(errorString);
        CCWarn(u8"%s设置参数 %s 失败: %s",
                cameraInfo().c_str(), featureName, utf8ErrMsg.c_str());
        return true;
    } else {
        const auto &utf8ErrMsg = utility::gbkToUtf8(errorString);
        CCError(u8"%s设置参数 %s 失败: %s",
                u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常",
                cameraInfo().c_str(), featureName, utf8ErrMsg.c_str());
        return false;
    }
}

bool CCBaslerCameraImp::setSoftTrigger()
{
    bool result = true;
    result &= setEnumValue("TriggerSelector", "FrameStart");
    result &= setEnumValue("TriggerMode", 1);

    // 软触发
    result &= setEnumValue("TriggerSource", "Software");

    return result;
}

bool CCBaslerCameraImp::setHardwareTrigger()
{
    bool result = true;
    result &= setEnumValue("TriggerSelector", "FrameStart");
    result &= setEnumValue("TriggerMode", 1);

    // 硬触发
    result &= setEnumValue("TriggerSource", "Line1");

    return result;
}

const std::string CCBaslerCameraImp::cameraInfo()
{
    return m_cameraType + u8"为" + m_cameraInfo + u8"的相机[" + std::to_string(m_cameraId) + "]";
}

