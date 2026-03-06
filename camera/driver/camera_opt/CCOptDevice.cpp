/**
 * OPT相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#include "CCOptDevice.h"
#include "CCOptCamera.h"
#include "CCOptCameraImp.h"
#include "LogClientCommon.h"

using namespace utility;

int getDevice(CCDeviceInfo &devInfo)
{
    // 获取设备信息
    CCInfo("opt usb camera getDevice enter");
    getDeviceType(devInfo.first);
    return 0;
}

CCOptCameraImp* getCameraHandle(std::shared_ptr<utility::CCDeviceImpl> optCamera)
{
    std::shared_ptr<CCOptCamera> opt = std::dynamic_pointer_cast<CCOptCamera>(optCamera);
    return opt->getCameraHandle().get();
}
