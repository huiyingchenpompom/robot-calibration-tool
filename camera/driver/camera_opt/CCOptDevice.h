/**
 * OPT相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#ifndef CCOPTDEVICE_H
#define CCOPTDEVICE_H

#include "node/CCDriverGlobal.h"
#include "utility/core/CCDevice.hpp"

class CCOptCameraImp;

#ifdef __cplusplus
extern "C" {
#endif

DRIVER_EXPORT int getDevice(utility::CCDeviceInfo &);


DRIVER_EXPORT CCOptCameraImp*  getCameraHandle( std::shared_ptr<utility::CCDeviceImpl> optCamera );

#ifdef __cplusplus
}
#endif

#endif // CCDEVICELIST_H
