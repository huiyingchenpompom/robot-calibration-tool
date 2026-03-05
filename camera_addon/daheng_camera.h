#pragma once
#include "camera_base.h"

// 大恒相机实现
// 依赖 Galaxy SDK: https://www.daheng-imaging.com/
class DahengCamera : public CameraBase {
public:
    DahengCamera();
    ~DahengCamera() override;

    bool connect(int deviceIndex) override;
    void disconnect() override;
    bool isConnected() const override;
    std::vector<uint8_t> captureImage() override;
    void setExposure(double exposureUs) override;
    void setGain(double gainDb) override;
    void setGamma(double gamma) override;
    std::string getModelName() const override;

private:
    void* handle_;
    bool connected_;
};
