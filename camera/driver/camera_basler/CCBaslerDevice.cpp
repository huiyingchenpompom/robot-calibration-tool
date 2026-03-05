/**
 * Basler网络相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#include "CCBaslerDevice.h"
#include "CCBaslerCamera.h"
#include "CCBaslerCameraImp.h"
#include "LogClientCommon.h"

using namespace utility;

int getDevice(CCDeviceInfo &devInfo)
{
    // 获取设备信息
    CCInfo("basler camera getDevice enter");
    getDeviceType(devInfo.first);
    return 0;
}

CCBaslerCameraImp* getCameraHandle(std::shared_ptr<utility::CCDeviceImpl> baslerCamera)
{
    std::shared_ptr<CCBaslerCamera> basler = std::dynamic_pointer_cast<CCBaslerCamera>(baslerCamera);
    return basler->getCameraHandle().get();
}
