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
};
