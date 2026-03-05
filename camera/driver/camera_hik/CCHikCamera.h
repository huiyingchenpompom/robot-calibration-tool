/**
 * Daheng相机驱动基本信息
 *
 */
#ifndef CCHIKCAMERA_H
#define CCHIKCAMERA_H

#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"
#include "CCHikMainProcess.h"

class CCHikMainProcess;

class CCHikCamera : public utility::CCDeviceImpl
{
public:
    explicit CCHikCamera(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType, const nlohmann::json &);
    virtual std::shared_ptr<utility::CCMessageNode> rosNode() override;
    bool                                  openCamera(int triggerType);
    std::shared_ptr<CCHikCameraImp>           getCameraHandle();

    void run() override;
    void registerDataIdValidator(void *dataIdMeta) override;

private:
    std::shared_ptr<CCHikMainProcess>        m_process;
};


int getDeviceType(utility::CCDeviceType &type);

#endif // CCHIKCAMERA_H
