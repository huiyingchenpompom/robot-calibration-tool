#pragma once
#include "camera_base.h"

// 海康威视相机实现
// 依赖 MVS SDK: https://www.hikrobotics.com/
class HikCamera : public CameraBase {
public:
    HikCamera();
    ~HikCamera() override;

    bool connect(int deviceIndex) override;
    void disconnect() override;
    bool isConnected() const override;
    CameraFrame captureImage() override;
    void setExposure(double exposureUs) override;
    void setGain(double gainDb) override;
    void setGamma(double gamma) override;
    std::string getModelName() const override;

private:
    void* handle_;
    bool connected_;
    bool is_opened_;   // MV_CC_OpenDevice 已完成（参考 CCHikCameraImp::m_isOpened）
    bool is_grabbing_; // MV_CC_StartGrabbing 已完成（参考 CCHikCameraImp::m_isGrabbing）
};
