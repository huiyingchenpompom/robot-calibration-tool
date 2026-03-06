#ifndef CCOPTCAMERAIMP_H
#define CCOPTCAMERAIMP_H

#include <functional>
#include <string>
#include <map>
#include "SciCam.h"
#include <sstream>
#include <WS2tcpip.h>
#include <iphlpapi.h>

class CCOptCameraImp
{
public:
    explicit CCOptCameraImp(int cameraId, const std::string &cameraInfo);
    virtual ~CCOptCameraImp();

    enum AcquisitionMode                 /* 相机采集模式 */
    {
        AcquisitionModeContinuous     = 0,      ///< Continuous mode
        AcquisitionModeSingleFrame    = 1,      ///< Single frame mode
    };

    enum TriggerActivation
    {
        RisingEdge,
        FallingEdge,
        AnyEdge,
        LevelHigh,
        LevelLow
    };

//    enum PixelFormat
//    {
//        PixelFormatNone     = 0,                            ///< 空图像
//        PixelFormatMono8    = PixelType_Gvsp_Mono8,         ///< 黑白图像
//        PixelFormatBayerBG8 = PixelType_Gvsp_BayerBG8,      ///< 彩色图像
//        PixelFormatRGB8     = PixelType_Gvsp_RGB8_Packed    ///< 彩色图像
//    };

    bool initDevice(const std::string &deviceInfo);

    /* 以下是设备操作接口 */
    bool createHandle();                                /* 创建相机句柄（上下文） */
    bool destroyHandle();                               /* 释放相机句柄 */
    bool openDevice();                                  /* 打开设备 */
    bool closeDevice();                                 /* 关闭设备 */
    bool startGrabbing();                               /* 开始取图 */
    bool stopGrabbing();                                /* 停止取图 */

    /* 以下是设备传输层，稳定性接口 */
    bool setImageBuffer(unsigned int imageBuffer = 16); /* 设置图片缓冲节点，用于提升性能, 解决偶发性的图片丢帧、错位的问题 */
    bool setResend(unsigned int maxResendPercent = 100,
                   unsigned int resendTimeout = 3000);  /* 设置数据丢包后重传比例和重传超时 */
    bool setGevSCPD(unsigned int gevSCPD = 50);         /* 设置延迟发包间隔 */
    bool setPacketSize(unsigned int packetSize = 1500); /* 设置包长(巨型帧) */
    bool setAutoPacketSize();                           /* 自动设置最大值包长(巨型帧) */
    bool setHeartbeatTime(unsigned int heartbeatTime = 3000);   /* 设置心跳时间 */
    bool setAcquisitionMode(AcquisitionMode acquisitionMode = AcquisitionModeContinuous); /* 设置图片采集模式为连续采集模式 */
    bool getPayloadSize();                                      /* 获取数据包大小 */

    /* 以下是设备触发控制接口 */
    bool setHardwareTrigger();                          /* 设置触发模式为硬件(外部)触发 */
    bool setSoftwareTrigger();                          /* 设置触发模式为软(软件)触发 */
    bool sendSoftTriggerCommand();                      /* 通过软件下发命令，触发一次拍照 */
    bool setTriggerActivation(TriggerActivation triggerActivation);  /* 设置触发激活信号 */
    bool setlineDebouncerTime(unsigned int debouncerTime = 50);             /* 设置线路去抖动时间 */
    bool setTriggerDelayTime(float delayTime = 0);      /* 设置触发延迟时间 */

    /* 以下是设备状态获取接口 */
    bool isDeviceAccessible();                          /* 设备是否可达（是否可以已经掉线） */
    bool isDeviceOpenned();                             /* 设备是否已经打开 */
    bool isGrabbing();                                  /* 设备是否正在进行图片取流 */
    bool isAreaScan(bool &isAreaScanCamera);            /* 设备是否为面阵相机（面阵、线阵） */
    bool isColor(bool &isColorCamera, std::string &pixFormat);/* 设备是否为彩色相机（彩色、黑白） */

    /* 以下是设备回调事件接口 */
    bool registerImageCallBack();                       /* 注册收到图片后的回调函数 */
    /** @brief 用于接收海康相机图片流的回调函数 */
    void photoReceivedCallBack(void *payload, void *pUser);

    /* 以下是设备参数设置接口 */
    virtual bool setRoi(int offsetX, int offsetY, int width, int height);/* 设置感兴趣区域（热点区域） */
    bool setOffsetX(unsigned int offsetX);              /* 设置横向偏移值X */
    bool setImageWidth(unsigned int width);             /* 设置图像宽度 */
    bool setImageHeight(unsigned int height);           /* 设置图像高度 */
    bool setTriggerMode(bool triggerMode);              /* 设置触发使能 */
    bool setExposureTime(float exposureTime);           /* 设置曝光 */
    bool setGammaEnable(bool enable);                   /* 设置伽马使能 */
    bool setGamma(float gamma);                         /* 设置伽马值 */
    bool setGain(float gain);                           /* 设置增益值 */
    bool setPixelFormat(const std::string &pixelFormat);       /* 设置相机像素格式 */
    bool setPreampGain(float preampGain);               /* 设置前置放大器增益 */
    bool setLineRate(unsigned int rate);                /* 设置线扫行频 */

    /* 以下是设备参数读取接口 */
    virtual bool getRoi(unsigned int &offsetX,
                        unsigned int &offsetY,
                        unsigned int &width,
                        unsigned int &height);          /* 读取感兴趣区域（热点区域） */
    bool getOffsetX(unsigned int &offsetX);             /* 读取横向偏移值X */
    bool getImageWidth(unsigned int &width);            /* 读取图像宽度 */
    bool getImageHeight(unsigned int &height);          /* 读取图像高度 */
    bool getMaxImageWidth(unsigned int &maxWidth);      /* 读取图片最大宽度 */
    bool getMaxImageHeight(unsigned int &maxHeight);    /* 读取图片最大高度 */
    bool getTriggerMode(bool &triggerMode);             /* 读取触发使能 */
    float getExposureTime();                            /* 读取曝光值 */
    bool getGammaEnable(bool &enable);                  /* 读取伽马使能 */
    float getGamma();                                   /* 读取伽马值 */
    float getGain();                                    /* 读取增益值 */
    void setImageCapturedCallback(std::function<void(void*, int, int, int)> func) {m_capturedFunc = func;}


    /* 以下是设置图像反转接口 */
    bool setReverseX(bool enable = false);             /* 设置图像沿X（垂直）方向反转 */
    bool setReverseY(bool enable = false);             /* 设置图像沿Y（水平）方向反转 */

public:
    /* 以下是设备通用参数设置接口 */
    /* 一般情况下不建议使用      */
    bool setIntValue(const std::string &featureName, unsigned int value, bool warn = false);
    bool setFloatValue(const std::string &featureName, float value, bool warn = false);
    bool setEnumValue(const std::string &featureName, unsigned int value, bool warn = false);
    bool setEnumValue(const std::string &featureName, const std::string &value, bool warn = false);
    bool setBoolVaule(const std::string &featureName, bool value, bool warn = false);
    bool setStringValue(const std::string &featureName, const std::string &value, bool warn = false);

private:
    /* 以下是设备通用参数读取接口 */
    /* 一般情况下不建议使用      */
    bool getIntValue(const std::string &featureName, unsigned int &value, bool warn = false);
    bool getFloatValue(const std::string &featureName, float &value, bool warn = false);
    bool getEnumValue(const std::string &featureName, unsigned int &value, bool warn = false);
    bool getEnumValue(const std::string &featureName, std::string &value, bool warn = false);
    bool getBoolVaule(const std::string &featureName, bool &value, bool warn = false);
    bool getStringValue(const std::string &featureName, std::string &value, bool warn = false);

protected:
    bool checkNode(const std::string &featureName, bool write);
    //std::string errorNoToHex(int error);
    /* 将错误码转换成错误描述 */
    std::string getErrorInfo(int errorCode);

    /* 打印相机参数配置信息 */
    template<typename T>
    bool printCameraSetParamInfo(const std::string &featureName, T value, const std::string &action, int status, bool warn = false);

    std::map<uint32_t, std::string> m_mapErrorInfo;                         /* 相机的错误信息 */
    SCI_DEVICE_INFO                 m_deviceInfo;                           /* 相机的设备信息 */
    int                             m_cameraId;
    void                            *m_handle                 = nullptr;    /* 指向海康相机实例对象的指针， 相机实例通过海康SDK创建 */
    bool                            m_isLoselessCompression   = false;      /* 是否是无损压缩模式 */
    bool                            m_isGrabbing              = false;      /* 记录当前相机是否已开启图片取流 */
    bool                            m_isOpened                = false;      /* 记录当前相机是否已打开（已连接到实体相机） */
    unsigned int                    m_payloadSize             = 0;          /* 数据包大小 */
    std::string                     m_cameraType = "IP";                    /* 相机类型 */
    std::string                     m_cameraInfo;                           /* 相机信息 */
    bool                            m_isImageCompression      = 0;          /* 是否是压缩图片 */
    bool                            m_isColor;
    std::string                     m_pixelFormat;
    std::function<void(void*, int, int, int)> m_capturedFunc = nullptr;
};

#endif // CCOPTCAMERAIMP_H
