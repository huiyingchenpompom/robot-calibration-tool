#ifndef CCDALSACAMERAIMP_H
#define CCDALSACAMERAIMP_H

#include <vector>
#include <functional>
#include "Basic/SapClassBasic.h"

class CCDalsaCameraImp
{
public:
    explicit CCDalsaCameraImp(int cameraId, const std::string &cameraInfo);
    virtual ~CCDalsaCameraImp();

public:
    /******相机设备的相关操作******/
    bool init();
    bool close();
    bool start();
    bool stop();

    /******相机设备的前置操作********/
    bool initDalsaDevInfo(const std::string frameGrabberName, int cameraId, int cameraPortId, int profileProductId = -1);
    bool setSoftwareTrigger();
    bool setHardwareTrigger();
    bool sendSoftTriggerCommand();
    bool enumDalsaServer(); //枚举采集卡设备列表
    void setImageCapturedCallback(std::function<void(void*, int, int, int)> func) {m_capturedFunc = func;}

private:
    /***用于接收相机图片流的回调函数******/
    static void photoReceivedCallBack(SapXferCallbackInfo *pInfo);  /* 相机取图回调函数 */
    bool createObjects();
    bool destroyObjects();
    bool parseConfigFile(const std::string &filePath, bool &isColr, int &index);

private:
    SapAcquisition        *m_sapAcquistion = nullptr;
    SapBufferWithTrash    *m_buffers = nullptr;
    SapTransfer           *m_sapTransfer = nullptr;
    std::vector<std::string>        m_cameraLinkFrameGrabberName;
    std::string m_frameGrabberName  = "";          /* 设备号 */
    bool    m_grabEnabled           = false;       /* 记录当前相机是否已启用抓图 */
    bool    m_opened                = false;       /* 记录当前相机是否已打开（已连接到实体相机） */
    bool    m_isColor               = false;
    int     m_cameraId              = 1;
    int     m_cameraPortId          = 0;
    int     m_profileProductId      = -1;
    std::function<void(void*, int, int, int)> m_capturedFunc = nullptr;
};

#endif // CCDALSACAMERAIMP_H
