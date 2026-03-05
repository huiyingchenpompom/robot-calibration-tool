/**
 * Daheng 相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#ifndef CCDAHENGDEVICE_H
#define CCDAHENGDEVICE_H

#include "node/CCDriverGlobal.h"
#include "utility/core/CCDevice.hpp"

class CCDahengCameraImp;

#ifdef __cplusplus
extern "C" {
#endif

DRIVER_EXPORT int getDevice(utility::CCDeviceInfo &);


DRIVER_EXPORT CCDahengCameraImp*  getCameraHandle( std::shared_ptr<utility::CCDeviceImpl> dahengCamera );

#ifdef __cplusplus
}
#endif

#endif // CCDAHENGDEVICE_H
