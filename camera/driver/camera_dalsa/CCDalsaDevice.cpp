/**
 * Daheng网络相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#include "CCDalsaDevice.h"
#include "CCDalsaCamera.h"
#include "CCDalsaCameraImp.h"
#include "LogClientCommon.h"

using namespace utility;

int getDevice(CCDeviceInfo &devInfo)
{
    // 获取设备信息
    CCInfo("dalsa camera getDevice enter");
    getDeviceType(devInfo.first);
    return 0;
}

CCDalsaCameraImp* getCameraHandle(std::shared_ptr<utility::CCDeviceImpl> dalsaCamera)
{
    std::shared_ptr<CCDalsaCamera> dalsa = std::dynamic_pointer_cast<CCDalsaCamera>(dalsaCamera);
    return dalsa->getCameraHandle().get();
}
