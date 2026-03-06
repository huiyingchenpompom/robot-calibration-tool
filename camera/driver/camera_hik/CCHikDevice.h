/**
 * Daheng 相机驱动信息接口
 * 调试程序、跑料程序加载动态库时，调用以下接口，获取驱动基本信息
 */
#ifndef CCHIKDEVICE_H
#define CCHIKDEVICE_H

#include "node/CCDriverGlobal.h"
#include "utility/core/CCDevice.hpp"

class CCHikCameraImp;

#ifdef __cplusplus
extern "C" {
#endif

DRIVER_EXPORT int getDevice(utility::CCDeviceInfo &);


DRIVER_EXPORT CCHikCameraImp*  getCameraHandle( std::shared_ptr<utility::CCDeviceImpl> hikCamera );

#ifdef __cplusplus
}
#endif

#endif // CCDEVICELIST_H
