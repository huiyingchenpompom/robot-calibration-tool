/**
 * usb相机驱动基本信息
 *
 */
#ifndef CCDEVICETYPE_H
#define CCDEVICETYPE_H

#include "node/camera/driver/camera_usb/config.h"

#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"

class CCMainProcess;

class CCUsbCamera : public utility::CCDeviceImpl
{
public:
    explicit CCUsbCamera(const std::string &, const std::string &, const std::string &, int);
    virtual std::shared_ptr<rclcpp::Node> rosNode() override;

private:
    std::shared_ptr<CCMainProcess> m_process;
};

int getDeviceType(utility::CCDeviceType &type);

#endif // DEVICETYPE_H
