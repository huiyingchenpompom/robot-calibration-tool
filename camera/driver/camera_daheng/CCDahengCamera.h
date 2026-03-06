/**
 * Daheng相机驱动基本信息
 *
 */
#ifndef CCDAHENGCAMERA_H
#define CCDAHENGCAMERA_H

#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"
#include "CCDahengMainProcess.h"

class CCDahengMainProcess;

class CCDahengCamera : public utility::CCDeviceImpl
{
public:
    explicit CCDahengCamera(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType, const nlohmann::json &json);
    virtual std::shared_ptr<utility::CCMessageNode> rosNode() override;
    bool                                  openCamera(int triggerType);
    std::shared_ptr<CCDahengCameraImp>           getCameraHandle();

    void run() override;
    void registerDataIdValidator(void *dataIdMeta) override;

private:
    std::shared_ptr<CCDahengMainProcess>        m_process;
};


int getDeviceType(utility::CCDeviceType &type);

#endif // CCDAHENGUSBCAMERA_H
