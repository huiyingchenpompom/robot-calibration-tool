#include "daheng_camera.h"
#include <stdexcept>

// 大恒相机存根实现
// TODO: 集成大恒 Galaxy SDK 实现真实功能

DahengCamera::DahengCamera() : connected_(false) {}
DahengCamera::~DahengCamera() { disconnect(); }

bool DahengCamera::connect(int deviceIndex) {
    (void)deviceIndex;
    // TODO: 集成大恒 Galaxy SDK
    throw std::runtime_error("大恒相机 SDK 尚未集成");
}

void DahengCamera::disconnect() {
    connected_ = false;
}

bool DahengCamera::isConnected() const { return connected_; }

std::vector<uint8_t> DahengCamera::captureImage() {
    throw std::runtime_error("大恒相机 SDK 尚未集成");
}

void DahengCamera::setExposure(double exposureUs) { (void)exposureUs; }
void DahengCamera::setGain(double gainDb) { (void)gainDb; }
void DahengCamera::setGamma(double gamma) { (void)gamma; }

std::string DahengCamera::getModelName() const {
    return "Daheng (SDK未集成)";
}
