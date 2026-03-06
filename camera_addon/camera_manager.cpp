#include "camera_manager.h"
#include "basler_camera.h"
#include "hik_camera.h"
#include "daheng_camera.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>

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

// -----------------------------------------------------------------------
// 24-bit BMP 编码（顶到底存储，负高度避免行翻转，BGR 像素顺序）
// Encode a CameraFrame to a 24-bit BMP byte stream.
// channels == 1: grayscale → replicate to B=G=R=v
// channels == 3: data is BGR → copy directly
// -----------------------------------------------------------------------
static std::vector<uint8_t> encodeFrameToBmp(const CameraFrame& frame) {
    if (frame.width <= 0 || frame.height <= 0 || frame.pixels.empty()) {
        throw std::runtime_error("空图像帧，无法编码");
    }

    // BMP 每行需对齐到 4 字节
    const int bpp       = 3;
    const int rowBytes  = (frame.width * bpp + 3) & ~3;
    const int pixelBytes = rowBytes * frame.height;
    const int fileSize   = 54 + pixelBytes;

    std::vector<uint8_t> bmp(fileSize, 0);

    auto w16 = [&](int off, int16_t v) {
        bmp[off]     = static_cast<uint8_t>(v & 0xFF);
        bmp[off + 1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    };
    auto w32 = [&](int off, int32_t v) {
        bmp[off]     = static_cast<uint8_t>(v & 0xFF);
        bmp[off + 1] = static_cast<uint8_t>((v >> 8)  & 0xFF);
        bmp[off + 2] = static_cast<uint8_t>((v >> 16) & 0xFF);
        bmp[off + 3] = static_cast<uint8_t>((v >> 24) & 0xFF);
    };

    // BITMAPFILEHEADER (14 bytes)
    bmp[0] = 'B'; bmp[1] = 'M';
    w32(2,  fileSize);
    w32(6,  0);   // reserved
    w32(10, 54);  // pixel data offset

    // BITMAPINFOHEADER (40 bytes)
    w32(14, 40);            // header size
    w32(18, frame.width);
    w32(22, -frame.height); // 负高度：行从上到下存储
    w16(26, 1);             // 颜色平面数 = 1
    w16(28, 24);            // 每像素 24 位
    w32(30, 0);             // BI_RGB（无压缩）
    w32(34, pixelBytes);
    w32(38, 2835);          // X pixels/meter (~72 DPI)
    w32(42, 2835);          // Y pixels/meter
    w32(46, 0);
    w32(50, 0);

    // 像素数据
    for (int y = 0; y < frame.height; ++y) {
        uint8_t* dst = bmp.data() + 54 + y * rowBytes;
        if (frame.channels == 1) {
            // 灰度 → 24-bit BGR (R=G=B=灰度值)
            const uint8_t* src = frame.pixels.data() + y * frame.width;
            for (int x = 0; x < frame.width; ++x) {
                uint8_t v = src[x];
                dst[x * 3 + 0] = v;  // B
                dst[x * 3 + 1] = v;  // G
                dst[x * 3 + 2] = v;  // R
            }
        } else {
            // channels == 3，BGR 顺序（直接复制）
            const uint8_t* src = frame.pixels.data() + y * frame.width * 3;
            std::memcpy(dst, src, frame.width * 3);
        }
        // 行末填充字节已在 vector 初始化时清零
    }
    return bmp;
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
    CameraFrame frame = camera_->captureImage();
    auto bmpData = encodeFrameToBmp(frame);
    // 返回 BMP data URL（Electron / Chromium 支持 image/bmp）
    return "data:image/bmp;base64," + base64Encode(bmpData);
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
