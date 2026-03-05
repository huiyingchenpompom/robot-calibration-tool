/**
 * Daheng网络相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#include "CCOrbbecDevice.h"
#include "CCOrbbecCamera.h"
#include "CCOrbbecCameraImp.h"
#include "LogClientCommon.h"

using namespace utility;

int getDevice(CCDeviceInfo &devInfo)
{
    // 获取设备信息
    CCInfo("orbbec gige camera getDevice enter");
    getDeviceType(devInfo.first);
    return 0;
}

CCOrbbecCameraImp* getCameraHandle(std::shared_ptr<utility::CCDeviceImpl> orbbecCamera)
{
    std::shared_ptr<CCOrbbecCamera> orbbec = std::dynamic_pointer_cast<CCOrbbecCamera>(orbbecCamera);
    return orbbec->getCameraHandle().get();
}
