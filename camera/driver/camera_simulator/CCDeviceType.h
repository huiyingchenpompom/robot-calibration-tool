/**
 * usb相机驱动基本信息
 *
 */
#ifndef CCDEVICETYPE_H
#define CCDEVICETYPE_H

#include "node/camera/driver/camera_simulator/config.h"
#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"

class CCSimulatorCameraMainProcess;

class CCSimulatorCamera : public utility::CCDeviceImpl
{
public:
    explicit CCSimulatorCamera(const std::string &, const std::string &, const std::string &, int, int, int transmitType, const std::string &imagePath);
    ~CCSimulatorCamera();
    void run() override;

    virtual std::shared_ptr<utility::CCMessageNode> rosNode() override;

    void registerDataIdValidator(void *) override;

private:
    std::shared_ptr<CCSimulatorCameraMainProcess> m_process;
};


int getDeviceType(utility::CCDeviceType &type);

#endif // DEVICETYPE_H
