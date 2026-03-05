/**
 * Daheng网络相机采集图像
 *
 */
#ifndef CCDAHENGCAMERAIMP_H
#define CCDAHENGCAMERAIMP_H

#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include "GalaxyIncludes.h"

class CCDahengCameraImp : public ICaptureEventHandler, public IDeviceOfflineEventHandler
{
public:
    enum TriggerMode{
        SoftwareTriggerMode,
        HardwareTriggerMode,
        ContinuousTriggerMode
    };
    explicit CCDahengCameraImp(int cameraId, const std::string &info);
    virtual ~CCDahengCameraImp();
    bool  openDevice();
    bool  closeDevice();         /* 关闭 */
    bool  startGrabbing();       /* 开始采集 */
    bool  stopGrabbing();        /* 停止采集 */

    static void  initLib();
    static void  releaseLib();
    bool  setTriggerMode(TriggerMode mode);
    bool  sendSoftwareTriggerCommand();
    bool  setTriggerDelay(float delayTime);/* 设置触发延时 */
    bool  setGevSCPSPacketSize(int size); /* 设置包长大小 */
    bool  setImageBuffer(int imageBuffer = 16); /* 设置图片缓冲节点，用于提升性能, 解决偶发性的图片丢帧、错位的问题 */
    bool  isDeviceAccessible();  /* 是否掉线 */

    float getExposureTime();
    bool  setExposureTime(float dbVal);

    float getGamma();
    bool  setGamma(float dbVal);

    float getGain();
    bool setGain(float dbVal);
    void setHearbeat(int hb);
    bool openCamera(int);
    bool setRoi(int offsetX, int offsetY, int width, int height);
    void setImageCapturedCallback(std::function<void(void*, int, int, int)> func) {m_capturedFunc = func;}
    void DoOnImageCaptured(CImageDataPointer& objImageDataPointer, void*) override;
    void DoOnDeviceOfflineEvent(void* pUserParam) override;

private:
    void isColor(CGXDevicePointer& objCGXDevicePointer, bool &bIsColorFilter) const;
    int64_t convertPixelFormatToInt(std::string PixelFormat) const;
    void getBasicAttribute();
    void setConvertHandle();

public:
    bool setIntValue(const char *featureName, int64_t value);
    bool setFloatValue(const char *featureName, float value);
    bool setEnumValue(const char *featureName, const std::string &value);
    bool setBoolValue(const char *featureName, bool value);
    bool setStringValue(const char *featureName, const std::string &value);

private:
    const std::string cameraInfo();
    bool setCommandValue(const char *featureName);
    bool getIntValue(const char *featureName, int64_t &value);
    bool getFloatValue(const char *featureName, float &value);
    bool getEnumValue(const char *featureName, std::string &value);
    bool getBoolValue(const char *featureName, bool &value);
    bool getStringValue(const char *featureName, std::string &value);
    bool checkFeature(const char *featureName, bool read);
    bool isSupportColor(CGXDevicePointer& objCGXDevicePointer);
    GX_VALID_BIT_LIST getBestValudBit(GX_PIXEL_FORMAT_ENTRY emPixelFormatEntry);
    int64_t  Adjust(int64_t srcVal, int64_t minVal, int64_t maxVal);

private:
    int                             m_cameraId;
    bool                            m_isOffline = true;                 //< 记录当前相机是否掉线
    bool                            m_gammaEnabled = false;
    bool                            m_isOpen = false;                   //< 设备打开标识
    bool                            m_bGrabbing = false;                //< 设备采集标识
    static int                      ms_initCount;                       //< 初始化库的计数
    bool                            m_bIsColor ;                        //< 是否是彩色像素
    int64_t                         m_i64ImageHeight;                   //< 原始图像高
    int64_t                         m_i64ImageWidth;                    //< 原始图像宽
    int64_t                         m_i64ConvertSize;                   //< 格式转化图片大小
    int64_t                         m_i64PixelFormat;                   //< 当前的像素格式
    std::unique_ptr<BYTE[]>         m_pImageBuffer = nullptr;           //< 保存翻转后的图像用于显示
    std::string                     m_cameraType;                       //< 设备类型
    std::string                     m_cameraInfo;                       //< 设备信息
    CGXDevicePointer                m_objDevicePtr;                     //< 设备句柄
    CGXStreamPointer                m_objStreamPtr;                     //< 设备流
    GX_DEVICE_OFFLINE_CALLBACK_HANDLE m_hCB = nullptr;                  //< 掉线回调句柄
    CGXFeatureControlPointer        m_objFeatureControlPtr;             //< 属性控制器
    CGXFeatureControlPointer        m_objStreamFeatureControlPtr;       //< 流层控制器对象
    CImageProcessConfigPointer      m_objImageProcessPtr;               //< 图像处理对象
    CGXImageProcessPointer          m_pProcess;                         //< 图像质量提升指针
    CGXImageFormatConvertPointer    m_pConvert;                         //< 像素格式转换指针
    std::function<void(void*, int, int, int)> m_capturedFunc = nullptr;
};

#endif // CCDAHENGCAMERAIMP_H
