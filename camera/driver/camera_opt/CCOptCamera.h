/**
 * OPT相机驱动基本信息
 *
 */
#ifndef CCOPTCAMERA_H
#define CCOPTCAMERA_H

#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"
#include "CCOptMainProcess.h"

class CCOptMainProcess;

class CCOptCamera : public utility::CCDeviceImpl
{
public:
    explicit CCOptCamera(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType);
    virtual std::shared_ptr<utility::CCMessageNode> rosNode() override;
    bool                            openCamera(int triggerType);
    std::shared_ptr<CCOptCameraImp> getCameraHandle();

    void run() override;
    void registerDataIdValidator(void *dataIdMeta) override;

private:
    std::shared_ptr<CCOptMainProcess>        m_process;
};


int getDeviceType(utility::CCDeviceType &type);

#endif // CCOPTCAMERA_H
