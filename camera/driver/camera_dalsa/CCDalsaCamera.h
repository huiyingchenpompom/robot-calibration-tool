/**
 * Basler相机驱动基本信息
 *
 */
#ifndef CCDALSACAMERA_H
#define CCDALSACAMERA_H

#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"
#include "CCDalsaMainProcess.h"

class CCDalsaMainProcess;

class CCDalsaCamera : public utility::CCDeviceImpl
{
public:
    explicit CCDalsaCamera(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType);
    virtual std::shared_ptr<utility::CCMessageNode> rosNode() override;
    bool                                  openCamera(int triggerType);
    std::shared_ptr<CCDalsaCameraImp>           getCameraHandle();
    void run() override;
    void registerDataIdValidator(void *dataIdMeta) override;

private:
    std::shared_ptr<CCDalsaMainProcess>        m_process;
};


int getDeviceType(utility::CCDeviceType &type);

#endif // CCDALSACAMERA_H
