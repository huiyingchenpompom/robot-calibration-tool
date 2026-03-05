#include "camera_manager.h"
#include "basler_camera.h"
#include "hik_camera.h"
#include "daheng_camera.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>

// Base64 编码表
static const char BASE64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string CameraManager::base64Encode(const std::vector<uint8_t>& data) {
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t val = (uint32_t)data[i] << 16;
        if (i + 1 < data.size()) val |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < data.size()) val |= (uint32_t)data[i + 2];
        result += BASE64_CHARS[(val >> 18) & 0x3F];
        result += BASE64_CHARS[(val >> 12) & 0x3F];
        result += (i + 1 < data.size()) ? BASE64_CHARS[(val >> 6) & 0x3F] : '=';
        result += (i + 2 < data.size()) ? BASE64_CHARS[val & 0x3F] : '=';
    }
    return result;
}

CameraBrand CameraManager::parseBrand(const std::string& brandStr) {
    std::string lower = brandStr;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "basler") return CameraBrand::BASLER;
    if (lower == "hik") return CameraBrand::HIK;
    if (lower == "daheng") return CameraBrand::DAHENG;
    return CameraBrand::UNKNOWN;
}

CameraManager::CameraManager() = default;
CameraManager::~CameraManager() { disconnect(); }

bool CameraManager::connect(const std::string& brand, int deviceIndex) {
    disconnect();
    CameraBrand b = parseBrand(brand);
    switch (b) {
        case CameraBrand::BASLER:
            camera_ = std::make_unique<BaslerCamera>();
            break;
        case CameraBrand::HIK:
            camera_ = std::make_unique<HikCamera>();
            break;
        case CameraBrand::DAHENG:
            camera_ = std::make_unique<DahengCamera>();
            break;
        default:
            throw std::runtime_error("不支持的相机品牌: " + brand);
    }
    return camera_->connect(deviceIndex);
}

void CameraManager::disconnect() {
    if (camera_) {
        camera_->disconnect();
        camera_.reset();
    }
}

bool CameraManager::isConnected() const {
    return camera_ && camera_->isConnected();
}

std::string CameraManager::captureImageBase64() {
    if (!camera_ || !camera_->isConnected()) {
        throw std::runtime_error("相机未连接");
    }
    auto data = camera_->captureImage();
    return "data:image/jpeg;base64," + base64Encode(data);
}

void CameraManager::setExposure(double exposureUs) {
    if (camera_) camera_->setExposure(exposureUs);
}

void CameraManager::setGain(double gainDb) {
    if (camera_) camera_->setGain(gainDb);
}

void CameraManager::setGamma(double gamma) {
    if (camera_) camera_->setGamma(gamma);
}

std::string CameraManager::getModelName() const {
    if (camera_) return camera_->getModelName();
    return "未连接";
}
