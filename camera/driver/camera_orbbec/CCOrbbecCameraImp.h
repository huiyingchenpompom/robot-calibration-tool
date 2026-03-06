#ifndef CCORBBECCAMERAIMP_H
#define CCORBBECCAMERAIMP_H

#include <functional>
#include <string>
#include <map>
#include "libobsensor/ObSensor.hpp"

class CCOrbbecCameraImp
{
public:
    explicit CCOrbbecCameraImp(int cameraId, const std::string &cameraIp);
    virtual ~CCOrbbecCameraImp();

    /* 以下是相机设备枚举参数  */
    enum OrbbecTriggerMode		/* 相机触发模式 */
    {
        // from enum ob_multi_device_sync_mode
        TriggerModeSoftWare          = OB_MULTI_DEVICE_SYNC_MODE_SOFTWARE_TRIGGERING,            ///< Soft Ware Trigger
        TriggerModeHardWare          = OB_MULTI_DEVICE_SYNC_MODE_HARDWARE_TRIGGERING,            ///< Hard Ware Trigger
    };

//    bool enumDevices(std::map<std::string, std::shared_ptr<ob::Device>> &cameraList);
    bool initDevice(const std::string &deviceInfo);

    /* 以下是设备操作接口 */
    bool openDevice();                                  /* 打开设备 */
    bool closeDevice();                                 /* 关闭设备 */
    bool startGrabbing();                               /* 开始取图 */
    bool stopGrabbing();                                /* 停止取图 */

    /* 以下是设备传输层，稳定性接口 */
    bool setHeartbeatEnable(bool enable);               /* 设置心跳使能 */
    bool getPayloadSize();                                      /* 获取数据包大小 */
    bool setImageCompression(bool isImageCompression);          /* 设置图片是否是无损压缩 */

    /* 以下是设备触发控制接口 */
    bool setHardwareTrigger();      /* 设置触发模式为硬件(外部)触发 */
    bool setSoftwareTrigger();                          /* 设置触发模式为软(软件)触发 */
    bool sendSoftTriggerCommand();                      /* 通过软件下发命令，触发一次拍照 */
    bool setlineDebouncerTime(unsigned int debouncerTime = 50);             /* 设置线路去抖动时间 */
    bool setTriggerDelayTime(float delayTime = 0);      /* 设置触发延迟时间 */

    /* 以下是设备状态获取接口 */
//    bool isDeviceOpenned();                             /* 设备是否已经打开 */
    bool isGrabbing();                                  /* 设备是否正在进行图片取流 */

    /* 以下是设备回调事件接口 */
    bool registerImageCallBack();                       /* 注册收到图片后的回调函数 */

    /* 以下是设备参数设置接口 */
    bool setExposureTime(int exposureTime);           /* 设置曝光 */
    bool setGammaEnable(bool enable);                   /* 设置伽马使能 */
    bool setGamma(int gamma);                         /* 设置伽马值 */
    bool setGain(int gain);                           /* 设置增益值 */
    bool setPreampGain(int preGain);				    /* 设置前置增益 */
    bool setLineRate(int rate);                /* 设置线扫行频 */

    /* 以下是设备参数读取接口 */
    int getExposureTime();                            /* 读取曝光值 */
    bool getGammaEnable();                            /* 读取伽马使能 */
    int getGamma();                                   /* 读取伽马值 */
    int getGain();                                    /* 读取增益值 */
    int getPreampGain();                              /* 读取前置增益 */
    int getLineRate();								  /* 读取线扫行频 */
    void setImageCapturedCallback(std::function<void(void*, int, int, int)> func);

public:
    /* 以下是设备通用参数设置接口 */
    /* 一般情况下不建议使用      */
    bool setIntValue(const std::string &featureName, unsigned int value);
    bool setFloatValue(const std::string &featureName, float value);
    bool setBoolVaule(const std::string &featureName, bool value);
    bool setEnumValue(const std::string &featureName, unsigned int value){return true;}
    bool setStringValue(const std::string &featureName, const std::string &value){return true;}

private:
    int getIntValueById(OBPropertyID id);
    float getFloatValueById(OBPropertyID id);
    bool getBoolValueById(OBPropertyID id);

    bool setIntValueById(OBPropertyID id, int value);
    bool setFloatValueById(OBPropertyID id, float value);
    bool setBoolValueById(OBPropertyID id, bool value);

private:
    /* 以下是设备通用参数读取接口 */
    /* 一般情况下不建议使用      */
    bool getIntValue(const std::string &featureName, unsigned int &value);
    bool getFloatValue(const std::string &featureName, float &value);
    bool getBoolVaule(const std::string &featureName, bool &value);

protected:
    /** @brief 用于接收orbbec相机图片的回调函数 */
#ifdef Q_OS_LINUX
//    static void photoReceivedCallBack(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
//    static void photoReceivedCallBack(unsigned char *pData, ob::Frame *pFrameInfo, void *pUser);
#else
//    static void __stdcall photoReceivedCallBack(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
//    static void __stdcall photoReceivedCallBack(unsigned char *pData, ob::Frame *pFrameInfo, void *pUser);
#endif
    void handleFrameSetCallback(std::shared_ptr<ob::FrameSet> frameSet);

    bool checkPropertyItem(const std::string &featureName, bool write, OBPropertyID &id);
    void setErrorInfoMap();
    std::string errorNoToHex(int error);
    /* 将错误码转换成错误描述 */
    std::string getErrorInfo(int errorCode);

    std::map<uint32_t, std::string> m_mapErrorInfo;                         /* 相机的错误信息 */
    std::shared_ptr<ob::Context>    m_pContext = nullptr;					/* 相机上下文 */
    std::shared_ptr<ob::DeviceInfo> m_pDeviceInfo = nullptr;                           /* 相机的设备信息 */
    int                             m_cameraId;
    std::shared_ptr<ob::Device>     m_pDevice                 = nullptr;    /* 指向相机实例对象的指针， 相机实例通过Context获取 */
    std::shared_ptr<ob::Config>		m_pDeviceCfg			  = nullptr;	/* 设备配置对象指针 */
    std::shared_ptr<ob::Pipeline>   m_pPipeline				  = nullptr;	/* 操作相机实现拍照等功能的对象 */
    std::map<std::string, OBPropertyItem> 	m_mapOBPropertyItem;			/* 缓存可设置属性的名称到该属性对象的映射关系 */
    bool                            m_isLoselessCompression   = false;      /* 是否是无损压缩模式 */
    bool                            m_isGrabbing              = false;      /* 记录当前相机是否已开启图片取流 */
//    bool                            m_isOpened                = false;      /* 记录当前相机是否已打开（已连接到实体相机） */
    unsigned int                    m_payloadSize             = 0;          /* 数据包大小 */
    std::string                     m_cameraIp;                             /* 相机IP地址 */
    bool                            m_isImageCompression      = 0;          /* 是否是压缩图片 */
    std::function<void(void*, int, int, int)> m_capturedFunc = nullptr;
};

#endif // CCORBBECCAMERAIMP_H
