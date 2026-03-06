#pragma once
#include "camera_base.h"

// Basler 相机实现
// 依赖 Pylon SDK: https://www.baslerweb.com/en/sales-support/downloads/software-downloads/
class BaslerCamera : public CameraBase {
public:
    BaslerCamera();
    ~BaslerCamera() override;

    bool connect(int deviceIndex) override;
    void disconnect() override;
    bool isConnected() const override;
    CameraFrame captureImage() override;
    void setExposure(double exposureUs) override;
    void setGain(double gainDb) override;
    void setGamma(double gamma) override;
    std::string getModelName() const override;

private:
    void* camera_handle_;  // Pylon camera handle (opaque ptr)
    bool connected_;
};
