/**
 * @file daheng camera
 *
 */
#include "CCDahengCameraImp.h"
#include "LogClientCommon.h"

#define PIXEL 3
int CCDahengCameraImp::ms_initCount = 0;

CCDahengCameraImp::CCDahengCameraImp(int cameraId, const std::string &info)
    :m_cameraId(cameraId),
    m_cameraInfo(info)
{
    initLib();
#ifdef DAHENG_USB_AREA
    m_cameraType = "SN";
#else
    m_cameraType = "IP";
#endif
    //初始化图像质量提升对象
    m_pProcess = IGXFactory::GetInstance().CreateImageProcess();
    //初始化图像转换对象
    m_pConvert = IGXFactory::GetInstance().CreateImageFormatConvert();
}

CCDahengCameraImp::~CCDahengCameraImp()
{
    closeDevice();
    releaseLib();
}

bool CCDahengCameraImp::openDevice()
{
    CCInfo("%s enter.", __FUNCTION__);

    bool bIsDeviceOpen = false;         ///< 设备是否打开标志
    bool bIsStreamOpen = false;         ///< 设备流是否打开标志
    try {
        GxIAPICPP::gxdeviceinfo_vector vectorDeviceInfo;
        IGXFactory::GetInstance().UpdateDeviceList(100, vectorDeviceInfo);
        if (vectorDeviceInfo.size() <= 0) {
            CCError(u8"未扫描到%s", u8"请使用使用相机官方软件辅助检查相机是否连接正常，若不正常，请检查相机网线、USB线、电源线两头连接是否正常。",
            cameraInfo().c_str());
            return false;
        }

        for (const auto &device: vectorDeviceInfo) {
            GxIAPICPP::gxstring strInfo = m_cameraType == "IP" ? device.GetIP() : device.GetSN();
            if (!std::strcmp(strInfo.c_str(), m_cameraInfo.c_str())) {
                if (m_cameraType == "IP") {
                    m_objDevicePtr = IGXFactory::GetInstance().OpenDeviceByIP(strInfo, GX_ACCESS_EXCLUSIVE);
                } else {
                    m_objDevicePtr = IGXFactory::GetInstance().OpenDeviceBySN(strInfo, GX_ACCESS_EXCLUSIVE);
                }
                m_objFeatureControlPtr = m_objDevicePtr->GetRemoteFeatureControl();
                //创建图像处理配置参数对象
                m_objImageProcessPtr = m_objDevicePtr->CreateImageProcessConfig();
                // 颜色校正默认不使能
                m_objImageProcessPtr->EnableColorCorrection(false);
                break;
            }
        }

        //判断设备流是否大于零，如果大于零则打开流
        int nStreamCount = m_objDevicePtr->GetStreamCount();

        if (nStreamCount > 0) {
            m_objStreamPtr = m_objDevicePtr->OpenStream(0);
            m_objStreamFeatureControlPtr = m_objStreamPtr->GetFeatureControl();
            bIsStreamOpen = true;
        } else {
            CCError(u8"未发现%s设备流", u8"请使用相机官方软件辅助查看相机是否连接正常.", cameraInfo().c_str());
            return false;
        }

        GX_DEVICE_CLASS_LIST objDeviceClass = m_objDevicePtr->GetDeviceInfo().GetDeviceClass();
        if(GX_DEVICE_CLASS_GEV == objDeviceClass) { // 判断设备是否支持流通道数据包功能
            if(m_objFeatureControlPtr->IsImplemented("GevSCPSPacketSize")) {
                // 获取当前网络环境的最优包长值
                int nPacketSize = m_objStreamPtr->GetOptimalPacketSize();
                // 将最优包长值设置为当前设备的流通道包长值
                m_objFeatureControlPtr->GetIntFeature("GevSCPSPacketSize")->SetValue(nPacketSize);
            }
        }

        //设置采集模式为连续采集模式
        m_objFeatureControlPtr->GetEnumFeature("AcquisitionMode")->SetValue("Continuous");
        if (m_objFeatureControlPtr->IsImplemented("TriggerMode")) { //设置触发模式开
            m_objFeatureControlPtr->GetEnumFeature("TriggerMode")->SetValue("On");
        }

        //注册掉线回调函数
        m_hCB = m_objDevicePtr->RegisterDeviceOfflineCallback(this, nullptr);
        m_isOffline = false;

        // 设置上升沿滤波为100
        //setFloatValue("TriggerFilterRaisingEdge", 100);

        // 设置Gamma模式
        if (m_objFeatureControlPtr->IsImplemented("Gamma")) {
            setEnumValue("GammaMode", "User");
            setBoolValue("GammaEnable", true);
        }

        // 网口相机设置心跳
        if (m_cameraType == "IP") {
            setHearbeat(3000);
        }
        bIsDeviceOpen = true;
    } catch (CGalaxyException& e) {
        if (bIsStreamOpen) {
            m_objStreamPtr->Close();
        }
        if (bIsDeviceOpen) {
           m_objDevicePtr->Close();
        }

        CCError(u8"打开%s失败:%s", u8"1.参考报错信息。2.检查相机官方软件是否占用相机。3.使用相机官方软件检查相机取图是否正常。4.如果不正常，请检查相机网线、USB线、电源线两头连接是否正常。",
        cameraInfo().c_str(), e.what());
        return false;
    }
    m_isOpen = bIsDeviceOpen;
    // 心跳、回调
    CCInfo("%s exit.", __FUNCTION__);
    return true;
}

bool CCDahengCameraImp::closeDevice()
{
    CCInfo("%s enter.", __FUNCTION__);
    if (!stopGrabbing()) {
        return false;
    }

    try {
        //关闭流对象
        m_objStreamPtr->Close();
        //关闭设备
        m_objDevicePtr->Close();
    } catch(CGalaxyException& e) {
        CCError(u8"%s关闭失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str(), e.what());
        return false;
    }
    m_isOpen = false;
    m_bGrabbing = false;

    CCInfo("%s exit.", __FUNCTION__);
    return true;
}

bool CCDahengCameraImp::startGrabbing()
{
    CCInfo("%s enter.", __FUNCTION__);
    try {
        if (m_bGrabbing) {
           CCWarn(u8"%s已经开始采集", cameraInfo().c_str());
           return true;
        }
        try {
           //设置Buffer处理模式
           m_objStreamFeatureControlPtr->GetEnumFeature("StreamBufferHandlingMode")->SetValue("OldestFirst");
        } catch (CGalaxyException& e) {
           CCError(u8"%s设置Buffer处理模式失败:%s", u8"参考报错信息处理", cameraInfo().c_str(), e.what());
           return false;
        }
        //注册回调函数
        m_objStreamPtr->RegisterCaptureCallback(this, this);
        //开启流层通道
        m_objStreamPtr->StartGrab();
        // 收图准备
        getBasicAttribute();
        // 设置图像转换句柄
        setConvertHandle();
        m_pImageBuffer = std::make_unique<BYTE[]>(m_i64ImageWidth * m_i64ImageHeight * (m_bIsColor ? 3 : 1));
        //发送开采命令
        m_objFeatureControlPtr->GetCommandFeature("AcquisitionStart")->Execute();
        m_bGrabbing = true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s开始采集失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str(), e.what());
        return false;
    } catch (std::exception& e) {
        CCError(u8"%s开始采集失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str(), e.what());
        return false;
    }
    CCInfo("%s exit.", __FUNCTION__);
    return true;
}

bool CCDahengCameraImp::stopGrabbing()
{
    CCInfo("%s enter.", __FUNCTION__);
    try {
        if (m_bGrabbing) {
           //发送停采命令
           m_objFeatureControlPtr->GetCommandFeature("AcquisitionStop")->Execute();
           //关闭流层通道
           m_objStreamPtr->StopGrab();
           //注销采集回调
           m_objStreamPtr->UnregisterCaptureCallback();
           m_bGrabbing = false;
        } else {
           CCWarn(u8"%s停止采集失败，该相机已停止采集", cameraInfo().c_str());
           return true;
        }
    } catch (CGalaxyException& e) {
        CCError(u8"%s停止采集失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str(), e.what());
        return false;
    } catch (std::exception& e) {
        CCError(u8"%s停止采集失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str(), e.what());
        return false;
    }
    CCInfo("%s exit.", __FUNCTION__);
    return true;
}

void CCDahengCameraImp::initLib()
{
    CCInfo("initLib:null interface");
    try {
        if (1 != ++ms_initCount) {
           CCInfo(u8"相机加载SDK动态库，当前相机计数:%d", ms_initCount);
           return;
        }
        IGXFactory::GetInstance().Init(); //初始化库
    } catch (CGalaxyException& e) {
        CCError(u8"相机加载SDK动态库失败:%s", u8"推荐安装相机软件Galaxy Viewer 2.2.2407.9181及以上版本", e.what());
    }
}

void CCDahengCameraImp::releaseLib()
{
    CCInfo("releaseLib:null interface");
    try {
        if (0 == ms_initCount) {
           return;
        }

        if (--ms_initCount > 0) {
           CCInfo(u8"相机卸载SDK动态库，当前相机计数:%d", ms_initCount);
           return;
        }
        IGXFactory::GetInstance().Uninit(); //初始化库
    } catch (CGalaxyException& e) {
        CCError(u8"卸载SDK动态库失败:%s", u8"请重新卸载相机库", e.what());
    }
}

bool CCDahengCameraImp::setTriggerMode(TriggerMode mode)
{
    CCInfo("%s enter.", __FUNCTION__);
    bool ret = true;
    if (mode == ContinuousTriggerMode) {
        ret &= setEnumValue("TriggerMode", "Off");
    } else {
        ret &= setEnumValue("TriggerMode", "On");
        if (mode == HardwareTriggerMode) {
            ret &= setEnumValue("TriggerSource", "Line0");
        } else {
            ret &= setEnumValue("TriggerSource", "Software");
        }
    }
    return ret;
}

bool CCDahengCameraImp::sendSoftwareTriggerCommand()
{
    CCInfo(u8"%s执行软触发", cameraInfo().c_str());
    return setCommandValue("TriggerSoftware");
}

bool CCDahengCameraImp::setTriggerDelay(float delayTime)
{
    CCInfo("%s enter.", __FUNCTION__);
    return setFloatValue("TriggerDelay", delayTime);
}

bool CCDahengCameraImp::setGevSCPSPacketSize(int size)
{
    CCInfo("%s enter.", __FUNCTION__);
    return setIntValue("GevSCPSPacketSize", size);
}

bool CCDahengCameraImp::setImageBuffer(int imageBuffer)
{
    CCInfo("%s enter.", __FUNCTION__);
    return setIntValue("MaxNumBuffer", imageBuffer);
}

bool CCDahengCameraImp::isDeviceAccessible()
{
    CCInfo("%s online status:%s", cameraInfo().c_str(), m_isOffline ? "offline" : "online");
    return !m_isOffline;
}

float CCDahengCameraImp::getExposureTime()
{
    CCInfo("%s enter.", __FUNCTION__);
    float value = 0.0f;
    getFloatValue("ExposureTime", value);
    return value;
}

float CCDahengCameraImp::getGamma()
{
    CCInfo("%s enter.", __FUNCTION__);
    if (m_objFeatureControlPtr->IsImplemented("Gamma")) {
        float value = 0.0f;
        getFloatValue("Gamma", value);
        return value;
    } else {
        return m_objImageProcessPtr->GetGammaParam();
    }
}

float CCDahengCameraImp::getGain()
{
    CCInfo("%s enter.", __FUNCTION__);
    float gain = 0.0f;
    getFloatValue("Gain", gain);
    return gain;
}

bool CCDahengCameraImp::setExposureTime(float dbVal)
{
    CCInfo("%s enter.", __FUNCTION__);
    return setFloatValue("ExposureTime", dbVal);
}

bool CCDahengCameraImp::setGamma(float dbVal)
{
    CCInfo("%s enter.", __FUNCTION__);
    if (m_objFeatureControlPtr->IsImplemented("Gamma")) {
        return setFloatValue("Gamma", dbVal);;
    } else {
        m_gammaEnabled = true;
        m_objImageProcessPtr->SetGammaParam(dbVal);
        return true;
    }
}

bool CCDahengCameraImp::setGain(float dbVal)
{
    CCInfo("%s enter.", __FUNCTION__);
    return setFloatValue("Gain", dbVal);
}

void CCDahengCameraImp::setHearbeat(int hb)
{
    CCInfo("%s enter.", __FUNCTION__);
    setIntValue("GevHeartbeatTimeout", hb);
}

bool CCDahengCameraImp::setRoi(int offsetX, int offsetY, int width, int height)
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

bool CCDahengCameraImp::openCamera(int triggerType)
{
    CCInfo("%s enter.", __FUNCTION__);
    bool result = true;
    result &= openDevice();
    if (triggerType == 0) {
        result &= setTriggerMode(SoftwareTriggerMode);
    } else if (triggerType == 1) {
        result &= setTriggerMode(HardwareTriggerMode);
    } else {
        result &= setTriggerMode(ContinuousTriggerMode);
    }
    result &= startGrabbing();
    CCInfo("%s exit.", __FUNCTION__);
    return result;
}

void CCDahengCameraImp::DoOnImageCaptured(CImageDataPointer &objImageDataPointer, void *)
{
    CCInfo(u8"%s enter.%s收到图片，帧号：%d", __FUNCTION__, cameraInfo().c_str(), objImageDataPointer->GetFrameID());

    if (!objImageDataPointer.IsNull()) {
        if (m_capturedFunc) {
            auto emValidBits = getBestValudBit(objImageDataPointer->GetPixelFormat());
            if (m_gammaEnabled) {
                //首先配置图像处理参数以免造成图像处理耗时
                m_objImageProcessPtr->SetValidBit(emValidBits);
            }
            // 根据图片格式转化
            if (objImageDataPointer->GetStatus() == GX_FRAME_STATUS_SUCCESS) {
                if (m_gammaEnabled) {
                    //使用图像处理接口ImageImprovment，获取buffer
                    if (m_bIsColor) {
                        GX_IMAGE_INFO ori_Image;//图像数据组
                        ori_Image.pBuffer = static_cast<BYTE*>(objImageDataPointer->GetBuffer());
                        ori_Image.nHeight = m_i64ImageHeight;
                        ori_Image.nWidth = m_i64ImageWidth;
                        ori_Image.emSrcFormat = static_cast<GX_PIXEL_FORMAT_ENTRY>(m_i64PixelFormat);
                        m_objImageProcessPtr->EnableConvertFlip(false);
                        m_pProcess->ImageImprovment(ori_Image, m_pImageBuffer.get(), m_objImageProcessPtr);
                    } else {
                        m_objImageProcessPtr->EnableConvertFlip(false);
                        m_pProcess->ImageImprovment(objImageDataPointer, m_pImageBuffer.get(), m_objImageProcessPtr);
                    }
                } else {
                    try {
                        if ((m_bIsColor && objImageDataPointer->GetPixelFormat() == GX_PIXEL_FORMAT_BGR8) || (!m_bIsColor && objImageDataPointer->GetPixelFormat() == GX_PIXEL_FORMAT_MONO8)) {
                            // 设置的格式与需要的一致，就不需要转化了
                            memcpy(m_pImageBuffer.get(), objImageDataPointer->GetBuffer(), m_i64ConvertSize);
                        } else {
                            m_pConvert->Convert(objImageDataPointer, m_pImageBuffer.get(), static_cast<size_t>(m_i64ConvertSize), false);
                        }
                    } catch (CGalaxyException& e) {
                        CCWarn(u8"%s图片格式转化失败：%s,帧号：%d, 格式:%d", cameraInfo().c_str(), e.what(), objImageDataPointer->GetFrameID(), objImageDataPointer->GetPixelFormat());
                    }
                }
                m_capturedFunc(m_pImageBuffer.get(), m_i64ImageWidth, m_i64ImageHeight, m_bIsColor ? 3 : 1);
            } else {
                CCWarn(u8"%s收到残帧图,宽:%d, 高:%d, 通道数:%d", cameraInfo().c_str(), m_i64ImageWidth, m_i64ImageHeight, m_bIsColor ? 3 : 1);
            }
        } else {
            CCError(u8"%s收图错误:未设置取图回调函数", u8"请联系SC开发人员排查问题", cameraInfo().c_str());
        }
    } else {
        CCError(u8"%s收图失败，丢帧", u8"请通过相机官方软件查看相机连接状态，若不正常请检查相机网线、USB线、电源线两头连接是否正常。", cameraInfo().c_str());
    }
    CCInfo("%s exit.", __FUNCTION__);
}

void CCDahengCameraImp::DoOnDeviceOfflineEvent(void *)
{
    m_isOffline = true;
    CCError(u8"%s掉线", u8"请检查相机网线、USB线、电源线两头连接是否正常，并重新插拔相机网线或USB口", cameraInfo().c_str());
}

void CCDahengCameraImp::isColor(CGXDevicePointer &objCGXDevicePointer, bool &bIsColorFilter) const
{
    const std::string  strPixelFormat = objCGXDevicePointer->GetRemoteFeatureControl()
                                           ->GetEnumFeature("PixelFormat")->GetValue().c_str();

    GX_PIXEL_FORMAT_ENTRY i32Pixel = static_cast<GX_PIXEL_FORMAT_ENTRY>(convertPixelFormatToInt(strPixelFormat));

    //将图像格式和下述宏定义做按位与（&）运算，可判断像素格式是mono还是RGB
    const int32_t i32PixelMono = 0x01000000;               //判断是否为MONO格式的掩码
    const int32_t i32PixelRgb = 0x20000000;                //判断是否为RGB格式的掩码
    const int32_t i32PixelColorMask = 0xFF000000;          //判断是否为彩色格式的掩码

    //将图像格式与下述宏定义做按位与（&）运算，可得到像素格式的ID
    int32_t i32PixelIdMask = 0x0000FFFF;

    bool bIsMono = ((i32PixelColorMask & i32Pixel) == i32PixelMono); // 是否为mono格式

    bool bIsRgb = ((i32PixelColorMask & i32Pixel) == i32PixelRgb);  // 是否为RGB格式
    bool bIsBayer = (((i32PixelIdMask & GX_PIXEL_FORMAT_BAYER_GR8) <= (i32PixelIdMask & i32Pixel)
                      && (i32PixelIdMask & i32Pixel) <= (i32PixelIdMask & GX_PIXEL_FORMAT_BAYER_BG12))
                     || ((i32PixelIdMask & GX_PIXEL_FORMAT_BAYER_GR14) <= (i32PixelIdMask & i32Pixel)
                         && (i32PixelIdMask & i32Pixel) <= (i32PixelIdMask & GX_PIXEL_FORMAT_BAYER_BG14))
                     || ((i32PixelIdMask & GX_PIXEL_FORMAT_BAYER_GR16) <= (i32PixelIdMask & i32Pixel)
                         && (i32PixelIdMask & i32Pixel) <= (i32PixelIdMask & GX_PIXEL_FORMAT_BAYER_BG16)));   // 是否为Bayer格式

    bIsColorFilter = !(bIsMono && (!bIsBayer) && (!bIsRgb));  // 用于判断是否为黑白相机
}

int64_t CCDahengCameraImp::convertPixelFormatToInt(std::string PixelFormat) const
{
    if("Mono8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_MONO8;
    }
    else if("BayerRG8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_RG8;
    }
    else if("BayerGB8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GB8;
    }
    else if("BayerGR8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GR8;
    }
    else if("BayerBG8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_BG8;
    }
    else if("RGB8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_RGB8;
    }
    else if("BGR8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BGR8;
    }
    else if("Mono10" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_MONO10;
    }
    else if("BayerRG10" == PixelFormat)
    {
        return  GX_PIXEL_FORMAT_BAYER_RG10;
    }
    else if("BayerGB10" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GB10;
    }
    else if("BayerGR10" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GR10;
    }
    else if("BayerBG10" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_BG10;
    }
    else if("Mono10Packed" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_MONO10_PACKED;
    }
    else if("Mono12" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_MONO12;
    }
    else if("BayerRG12" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_RG12;
    }
    else if("BayerGB12" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GB12;
    }
    else if("BayerGR12" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GR12;
    }
    else if("BayerBG12" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_BG12;
    }
    else if("Mono12Packed" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_MONO12_PACKED;
    }
    else if("Mono14" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_MONO14;
    }
    else if("BayerRG14" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_RG14;
    }
    else if("BayerGB14" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GB14;
    }
    else if("BayerGR14" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GR14;
    }
    else if("BayerBG14" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_BG14;
    }
    else if("Mono16" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_MONO16;
    }
    else if("BayerRG16" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_RG16;
    }
    else if("BayerGB16" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GB16;
    }
    else if("BayerGR16" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_GR16;
    }
    else if("BayerBG16" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_BAYER_BG16;
    }
    else if("R8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_R8;
    }
    else if("B8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_B8;
    }
    else if("G8" == PixelFormat)
    {
        return GX_PIXEL_FORMAT_G8;
    }else
    {
        throw std::runtime_error("Format Undefined");
    }
}

void CCDahengCameraImp::getBasicAttribute()
{
    //获得图像宽度、高度等
    m_i64ImageWidth = (int64_t)m_objDevicePtr->GetRemoteFeatureControl()->GetIntFeature("Width")->GetValue();
    m_i64ImageHeight = (int64_t)m_objDevicePtr->GetRemoteFeatureControl()->GetIntFeature("Height")->GetValue();

    //获取当前像素格式
    bool bIsLocal = m_objDevicePtr->GetFeatureControl()->IsImplemented("OutPixelFormat");
    if(bIsLocal)
    {
        std::string  strPixelFormat = m_objDevicePtr->GetFeatureControl()->GetEnumFeature("OutPixelFormat")->GetValue().c_str();
        m_i64PixelFormat = convertPixelFormatToInt(strPixelFormat);
    }else
    {
        std::string  strPixelFormat = m_objDevicePtr->GetRemoteFeatureControl()->GetEnumFeature("PixelFormat")->GetValue().c_str();
        m_i64PixelFormat = convertPixelFormatToInt(strPixelFormat);
    }
    CCInfo(u8"%s图片格式为%d", cameraInfo().c_str(), m_i64PixelFormat);
    //获取当前像素格式是否为彩色
    isColor(m_objDevicePtr,m_bIsColor);
}

void CCDahengCameraImp::setConvertHandle()
{
    // 设置插值方式
    m_pConvert->SetInterpolationType(GX_RAW2RGB_NEIGHBOUR);

    // 获取有效位数
    GX_VALID_BIT_LIST emValidBits = getBestValudBit(static_cast<GX_PIXEL_FORMAT_ENTRY>(m_i64PixelFormat));

    // 设置有效位数
    m_pConvert->SetValidBits(emValidBits);

    if (m_bIsColor)
    {
        // 设置图像格式转换句柄，转换为RGB8格式
        m_pConvert->SetDstFormat(GX_PIXEL_FORMAT_BGR8);
        m_i64ConvertSize = m_pConvert->GetBufferSizeForConversion(m_i64ImageWidth, m_i64ImageHeight, GX_PIXEL_FORMAT_BGR8);
    }
    else
    {
        // 设置图像格式转换句柄，转换为Mono8格式
        m_pConvert->SetDstFormat(GX_PIXEL_FORMAT_MONO8);
        m_i64ConvertSize = m_pConvert->GetBufferSizeForConversion(m_i64ImageWidth, m_i64ImageHeight, GX_PIXEL_FORMAT_MONO8);
    }
}

int64_t CCDahengCameraImp::Adjust(  int64_t srcVal
                           , int64_t minVal
                           , int64_t maxVal)
{
    CCInfo("%s enter.", __FUNCTION__);

    /*Check the lower bound.*/
    if (srcVal < minVal)
    {
        CCWarn("change value to minimum: %d %d", srcVal, minVal);
        srcVal = minVal;
    }

    /*Check the upper bound.*/
    if (srcVal > maxVal)
    {
        CCWarn("change value to maxmum: %d %d", srcVal, maxVal);
        srcVal = maxVal;
    }

    CCInfo("%s exit: %d", __FUNCTION__, srcVal);
    return srcVal;
}

bool CCDahengCameraImp::setIntValue(const char *featureName, int64_t value)
{
    if (!checkFeature(featureName, false)) {
        return true;
    }
    try {
        m_objFeatureControlPtr->GetIntFeature(featureName)->SetValue(value);
        CCInfo(u8"%s设置%s: %f", cameraInfo().c_str(), featureName, value);
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s设置%s = %d失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, value, e.what());
        return false;
    }
}

bool CCDahengCameraImp::setFloatValue(const char *featureName, float value)
{
    if (!checkFeature(featureName, false)) {
        return true;
    }
    try {
        m_objFeatureControlPtr->GetFloatFeature(featureName)->SetValue(value);
        CCInfo(u8"%s设置%s: %f", cameraInfo().c_str(), featureName, value);
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s设置%s = %f失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, value, e.what());
        return false;
    }
}

bool CCDahengCameraImp::setEnumValue(const char *featureName, const std::string &value)
{
    if (!checkFeature(featureName, false)) {
        return true;
    }
    try {
        m_objFeatureControlPtr->GetEnumFeature(featureName)->SetValue(value.c_str());
        CCInfo(u8"%s设置%s: %s", cameraInfo().c_str(), featureName, value.c_str());
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s设置%s = %s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, value.c_str(), e.what());
        return false;
    }
}

bool CCDahengCameraImp::setBoolValue(const char *featureName, bool value)
{
    if (!checkFeature(featureName, false)) {
        return true;
    }
    try {
        m_objFeatureControlPtr->GetBoolFeature(featureName)->SetValue(value);
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s设置%s = %d失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, value, e.what());
        return false;
    }
}

bool CCDahengCameraImp::setStringValue(const char *featureName, const std::string &value)
{
    if (!checkFeature(featureName, false)) {
        return true;
    }
    try {
        m_objFeatureControlPtr->GetStringFeature(featureName)->SetValue(value.c_str());
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s设置%s = %s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常",cameraInfo().c_str(), featureName, value.c_str(), e.what());
        return false;
    }
}

const std::string CCDahengCameraImp::cameraInfo()
{
    return m_cameraType + u8"为" + m_cameraInfo + u8"的相机[" + std::to_string(m_cameraId) + "]";
}

bool CCDahengCameraImp::setCommandValue(const char *featureName)
{
    if (!checkFeature(featureName, false)) {
        return true;
    }
    try {
        m_objFeatureControlPtr->GetCommandFeature(featureName)->Execute();
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s执行%s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, e.what());
        return false;
    }
}

bool CCDahengCameraImp::getIntValue(const char *featureName, int64_t &value)
{
    if (!checkFeature(featureName, true)) {
        return true;
    }
    try {
        value = m_objFeatureControlPtr->GetIntFeature(featureName)->GetValue();
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s获取%s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, e.what());
        return false;
    }
}

bool CCDahengCameraImp::getFloatValue(const char *featureName, float &value)
{
    if (!checkFeature(featureName, true)) {
        return true;
    }
    try {
        value = m_objFeatureControlPtr->GetFloatFeature(featureName)->GetValue();
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s获取%s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, e.what());
        return false;
    }
}

bool CCDahengCameraImp::getEnumValue(const char *featureName, std::string &value)
{
    if (!checkFeature(featureName, true)) {
        return true;
    }
    try {
        value = m_objFeatureControlPtr->GetEnumFeature(featureName)->GetValue();
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s获取%s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, e.what());
        return false;
    }
}

bool CCDahengCameraImp::getBoolValue(const char *featureName, bool &value)
{
    if (!checkFeature(featureName, true)) {
        return true;
    }
    try {
        value = m_objFeatureControlPtr->GetBoolFeature(featureName)->GetValue();
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s获取%s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, e.what());
        return false;
    }
}

bool CCDahengCameraImp::getStringValue(const char *featureName, std::string &value)
{
    if (!checkFeature(featureName, true)) {
        return true;
    }
    try {
        value = m_objFeatureControlPtr->GetStringFeature(featureName)->GetValue();
        return true;
    } catch (CGalaxyException& e) {
        CCError(u8"%s获取%s失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), featureName, e.what());
        return false;
    }
}

bool CCDahengCameraImp::checkFeature(const char *featureName, bool read)
{
    try {
        if (!m_objFeatureControlPtr->IsImplemented(featureName)) {
            CCWarn(u8"%s获取%s值失败：该相机不支持此参数", cameraInfo().c_str(), featureName);
            return false;
        }

        if (read) {
            if (!m_objFeatureControlPtr->IsReadable(featureName)) {
                CCWarn(u8"%s获取%s值失败：该值不可读", cameraInfo().c_str(), featureName);
                return false;
            }
        } else {
            if (!m_objFeatureControlPtr->IsWritable(featureName)) {
                CCWarn(u8"%s设置%s值失败：该值不可写", cameraInfo().c_str(), featureName);
                return false;
            }
        }
    } catch (CGalaxyException& e) {
        CCError(u8"%s%s值失败:%s", u8"1.参考报错信息 2.检查相机网线、USB线、电源线两头连接是否正常", cameraInfo().c_str(), read ? "get" : "set", featureName, e.what());
        return false;
    }

    return true;
}

bool CCDahengCameraImp::isSupportColor(CGXDevicePointer &objCGXDevicePointer)
{
    bool      bIsImplemented    = false;
    bool      bIsMono           = false;
    gxstring  strPixelFormat    = "";

    strPixelFormat = objCGXDevicePointer->GetRemoteFeatureControl()->GetEnumFeature("PixelFormat")->GetValue();
    strPixelFormat.substr(0, 4);

    if(0 == memcmp(strPixelFormat.c_str(),"Mono",4)) {
        bIsMono = true;
    } else {
        bIsMono = false;
    }

    bIsImplemented = objCGXDevicePointer->GetRemoteFeatureControl()->IsImplemented("PixelColorFilter");

    // 若当前为非黑白且支持PixelColorFilter则为彩色
    if((!bIsMono) && (bIsImplemented)) {
        return true;
    } else {
        return false;
    }
}

GX_VALID_BIT_LIST CCDahengCameraImp::getBestValudBit(GX_PIXEL_FORMAT_ENTRY emPixelFormatEntry)
{
    GX_VALID_BIT_LIST emValidBits = GX_BIT_0_7;
    switch (emPixelFormatEntry)
    {
    case GX_PIXEL_FORMAT_MONO8:
    case GX_PIXEL_FORMAT_BAYER_GR8:
    case GX_PIXEL_FORMAT_BAYER_RG8:
    case GX_PIXEL_FORMAT_BAYER_GB8:
    case GX_PIXEL_FORMAT_BAYER_BG8:
    {
        emValidBits = GX_BIT_0_7;
        break;
    }
    case GX_PIXEL_FORMAT_MONO10:
    case GX_PIXEL_FORMAT_BAYER_GR10:
    case GX_PIXEL_FORMAT_BAYER_RG10:
    case GX_PIXEL_FORMAT_BAYER_GB10:
    case GX_PIXEL_FORMAT_BAYER_BG10:
    {
        emValidBits = GX_BIT_2_9;
        break;
    }
    case GX_PIXEL_FORMAT_MONO12:
    case GX_PIXEL_FORMAT_BAYER_GR12:
    case GX_PIXEL_FORMAT_BAYER_RG12:
    case GX_PIXEL_FORMAT_BAYER_GB12:
    case GX_PIXEL_FORMAT_BAYER_BG12:
    {
        emValidBits = GX_BIT_4_11;
        break;
    }
    case GX_PIXEL_FORMAT_MONO14:
    {
        //暂时没有这样的数据格式待升级
        break;
    }
    case GX_PIXEL_FORMAT_MONO16:
    case GX_PIXEL_FORMAT_BAYER_GR16:
    case GX_PIXEL_FORMAT_BAYER_RG16:
    case GX_PIXEL_FORMAT_BAYER_GB16:
    case GX_PIXEL_FORMAT_BAYER_BG16:
    {
        //暂时没有这样的数据格式待升级
        break;
    }
    default:
        break;
    }
    return emValidBits;
}
