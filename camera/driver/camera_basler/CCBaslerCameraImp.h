 /**
 * Basler网络相机采集图像
 *
 */
#ifndef CCBASLERCAMERAIMP_H
#define CCBASLERCAMERAIMP_H

#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include "pylon/PylonIncludes.h"

using namespace Pylon;
using namespace GenApi;

class CCBaslerCameraImp : public CImageEventHandler, public CConfigurationEventHandler
{
public:
    explicit CCBaslerCameraImp(int cameraId, const std::string &);
    ~CCBaslerCameraImp();
    void getFrame();

    bool openDevice(const std::string &info);
    bool closeDevice();         /* 关闭 */
    bool startGrabbing();       /* 开始采集 */
    bool stopGrabbing();        /* 停止采集 */

    void initLib();              /* 装载库 */
    void releaseLib();           /* 释放库 */
    bool sendSoftTriggerCommand();      /* 软触发 */
    bool setTriggerDelay(double delayTime);/* 设置触发延时 */
    void setDataChunk(const char *featureName);
    bool setGevSCPSPacketSize(int size); /* 设置包长大小 */
    bool setImageBuffer(int imageBuffer = 16); /* 设置图片缓冲节点，用于提升性能, 解决偶发性的图片丢帧、错位的问题 */
    bool isDeviceAccessible();  /* 是否掉线 */

    double getExposureTime();
    bool setExposureTime(double dbVal);

    double getGamma();
    bool setGamma(double dbVal);

    double getGain();
    bool setGain(double dbVal);

    bool openCamera(int);
    bool setRoi(int offsetX, int offsetY, int width, int height);
    void OnImageGrabbed(CInstantCamera&, const CGrabResultPtr& ptrGrabResult ) override;
    void OnCameraDeviceRemoved(CInstantCamera& camera) override;
    void setImageCapturedCallback(std::function<void(void*, int, int, int)> func) {m_capturedFunc = func;}
    bool setSoftTrigger();
    bool setHardwareTrigger();

private:
    enum InfoLevel {
        Info,
        Warning,
        Error
    };
    static int  ms_initCount;           //< 初始化库的计数
    int m_cameraId;
    bool m_isOffline = true;            //< 记录当前相机是否掉线
    std::string m_cameraType;           //< 设备类型
    std::string m_cameraInfo;           //< 设备信息
    PylonAutoInitTerm autoInitTerm;
    CInstantCamera  m_instanceCamera;
    std::function<void(void*, int, int, int)> m_capturedFunc = nullptr;

    const std::string cameraInfo();
    int64_t getIntValue(const char *featureName);
    double getDoubleValue(const char *featureName);
    bool getEnumValue(const char *featureName, int &dbValue);
    bool getEnumValue(const char *featureName, std::string &dbValue);

public:
    bool setIntValue(const char *featureName, int64_t dbValue);
    bool setDoubleValue(const char *featureName, double dbValue);
    bool setEnumValue(const char *featureName, unsigned int dbValue);
    bool setEnumValue(const char *featureName, const char *dbValue);
    bool setBoolValue(const char *featureName, bool dbValue);

private:
    template<typename T>
    bool printCameraParamInfo(const char *featureName, T value, InfoLevel level, std::string errorString = "");

    int64_t Adjust(int64_t srcVal, int64_t minVal, int64_t maxVal, int64_t incVal);
};

#endif // CCBASLERCAMERAIMP_H
