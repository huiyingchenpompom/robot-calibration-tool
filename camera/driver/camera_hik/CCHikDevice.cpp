/**
 * Daheng网络相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#include "CCHikDevice.h"
#include "CCHikCamera.h"
#include "CCHikCameraImp.h"
#include "LogClientCommon.h"

using namespace utility;

int getDevice(CCDeviceInfo &devInfo)
{
    // 获取设备信息
    CCInfo("hik usb camera getDevice enter");
    getDeviceType(devInfo.first);
    return 0;
}

CCHikCameraImp* getCameraHandle(std::shared_ptr<utility::CCDeviceImpl> hikCamera)
{
    std::shared_ptr<CCHikCamera> hik = std::dynamic_pointer_cast<CCHikCamera>(hikCamera);
    return hik->getCameraHandle().get();
}
