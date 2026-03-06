/**
 * orbbec相机驱动基本信息
 *
 */
#ifndef CCORBBECCAMERA_H
#define CCORBBECCAMERA_H

#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"
#include "CCOrbbecMainProcess.h"

class CCOrbbecMainProcess;

class CCOrbbecCamera : public utility::CCDeviceImpl
{
public:
    explicit CCOrbbecCamera(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType);
    virtual std::shared_ptr<utility::CCMessageNode> rosNode() override;
    bool                                  openCamera(int triggerType);
    std::shared_ptr<CCOrbbecCameraImp>           getCameraHandle();

    void run() override;
    void registerDataIdValidator(void *dataIdMeta) override;

private:
    std::shared_ptr<CCOrbbecMainProcess>        m_process;
};


int getDeviceType(utility::CCDeviceType &type);

#endif // CCORBBECCAMERA_H
