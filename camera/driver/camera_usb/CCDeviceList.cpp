/**
 * usb相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#include "CCDeviceList.h"
#include "CCDeviceType.h"

using namespace utility;

int getDevice(CCDeviceInfo &devInfo)
{
    // 获取设备信息
    getDeviceType(devInfo.first);
    return 0;
}
