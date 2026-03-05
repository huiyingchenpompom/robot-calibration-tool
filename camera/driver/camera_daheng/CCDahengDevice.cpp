/**
 * Daheng网络相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#include "CCDahengDevice.h"
#include "CCDahengCamera.h"
#include "CCDahengCameraImp.h"
#include "LogClientCommon.h"

using namespace utility;

int getDevice(CCDeviceInfo &devInfo)
{
    // 获取设备信息
    CCInfo("daheng usb camera getDevice enter");
    getDeviceType(devInfo.first);
    return 0;
}

CCDahengCameraImp* getCameraHandle(std::shared_ptr<utility::CCDeviceImpl> dahengCamera)
{
    std::shared_ptr<CCDahengCamera> daheng = std::dynamic_pointer_cast<CCDahengCamera>(dahengCamera);
    return daheng->getCameraHandle().get();
}
