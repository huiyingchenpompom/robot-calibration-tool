/**
 * Basler相机驱动基本信息
 *
 */
#ifndef CCBASLERCAMERA_H
#define CCBASLERCAMERA_H

#include "node/camera/config.h"
#include "utility/core/CCDevice.hpp"
#include "CCBaslerMainProcess.h"
#include <nlohmann/json.hpp>

class CCBaslerCamera : public utility::CCDeviceImpl
{
public:
    explicit CCBaslerCamera(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType, const nlohmann::json &);
    virtual std::shared_ptr<utility::CCMessageNode> rosNode() override;
    bool                                  openCamera(int triggerType);
    std::shared_ptr<CCBaslerCameraImp>           getCameraHandle();

    void run() override;
    void registerDataIdValidator(void *dataIdMeta) override;

private:
    std::shared_ptr<CCBaslerMainProcess>        m_process;
};


int getDeviceType(utility::CCDeviceType &type);

#endif // CCBASLERCAMERA_H
