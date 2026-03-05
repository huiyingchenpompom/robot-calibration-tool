#include "daheng_camera.h"
#include <stdexcept>
#include <cstring>

// 注意：此文件需要大恒 Galaxy SDK。
// 未安装 SDK 时编译为存根实现。
#ifdef HAVE_DAHENG_SDK
#include "GxIAPI.h"
#endif

DahengCamera::DahengCamera() : handle_(nullptr), connected_(false) {
#ifdef HAVE_DAHENG_SDK
    if (GXInitLib() != GX_STATUS_SUCCESS) {
        throw std::runtime_error("大恒相机 Galaxy SDK 初始化失败");
    }
#endif
}

DahengCamera::~DahengCamera() {
    disconnect();
#ifdef HAVE_DAHENG_SDK
    GXCloseLib();
#endif
}

bool DahengCamera::connect(int deviceIndex) {
#ifdef HAVE_DAHENG_SDK
    uint32_t nDeviceNum = 0;
    if (GXUpdateAllDeviceList(&nDeviceNum, 1000) != GX_STATUS_SUCCESS) {
        throw std::runtime_error("枚举大恒相机失败");
    }
    if (nDeviceNum == 0 || deviceIndex >= (int)nDeviceNum) {
        throw std::runtime_error("未找到大恒相机或设备序号超出范围");
    }
    GX_DEV_HANDLE hDevice = nullptr;
    // GXOpenDeviceByIndex 使用 1-based 索引
    if (GXOpenDeviceByIndex((uint32_t)deviceIndex + 1, &hDevice) != GX_STATUS_SUCCESS) {
        throw std::runtime_error("打开大恒相机失败");
    }
    handle_ = hDevice;
    connected_ = true;
    return true;
#else
    (void)deviceIndex;
    throw std::runtime_error("大恒相机 Galaxy SDK 未编译，无法连接");
#endif
}

void DahengCamera::disconnect() {
#ifdef HAVE_DAHENG_SDK
    if (handle_) {
        GX_DEV_HANDLE hDevice = static_cast<GX_DEV_HANDLE>(handle_);
        GXStreamOff(hDevice);
        GXCloseDevice(hDevice);
        handle_ = nullptr;
    }
#endif
    connected_ = false;
}

bool DahengCamera::isConnected() const { return connected_; }

std::vector<uint8_t> DahengCamera::captureImage() {
#ifdef HAVE_DAHENG_SDK
    if (!handle_) throw std::runtime_error("相机未连接");
    GX_DEV_HANDLE hDevice = static_cast<GX_DEV_HANDLE>(handle_);
    GXStreamOn(hDevice);
    PGX_FRAME_BUFFER pFrameBuffer = nullptr;
    GX_STATUS status = GXDQBuf(hDevice, &pFrameBuffer, 1000);
    if (status != GX_STATUS_SUCCESS || !pFrameBuffer) {
        GXStreamOff(hDevice);
        throw std::runtime_error("获取图像失败");
    }
    std::vector<uint8_t> data(
        static_cast<uint8_t*>(pFrameBuffer->pImgBuf),
        static_cast<uint8_t*>(pFrameBuffer->pImgBuf) + pFrameBuffer->nImgSize);
    GXQBuf(hDevice, pFrameBuffer);
    GXStreamOff(hDevice);
    return data;
#else
    throw std::runtime_error("大恒相机 Galaxy SDK 未编译");
#endif
}

void DahengCamera::setExposure(double exposureUs) {
#ifdef HAVE_DAHENG_SDK
    if (handle_)
        GXSetFloat(static_cast<GX_DEV_HANDLE>(handle_), GX_FLOAT_EXPOSURE_TIME, exposureUs);
#else
    (void)exposureUs;
#endif
}

void DahengCamera::setGain(double gainDb) {
#ifdef HAVE_DAHENG_SDK
    if (handle_)
        GXSetFloat(static_cast<GX_DEV_HANDLE>(handle_), GX_FLOAT_GAIN, gainDb);
#else
    (void)gainDb;
#endif
}

void DahengCamera::setGamma(double gamma) {
#ifdef HAVE_DAHENG_SDK
    if (handle_)
        GXSetFloat(static_cast<GX_DEV_HANDLE>(handle_), GX_FLOAT_GAMMA, gamma);
#else
    (void)gamma;
#endif
}

std::string DahengCamera::getModelName() const {
#ifdef HAVE_DAHENG_SDK
    if (!handle_) return "Daheng (未连接)";
    char modelName[128] = {};
    size_t len = sizeof(modelName);
    if (GXGetString(static_cast<GX_DEV_HANDLE>(handle_),
                    GX_STRING_DEVICE_MODEL_NAME, modelName, &len) != GX_STATUS_SUCCESS) {
        return "Daheng (读取型号失败)";
    }
    return std::string(modelName);
#else
    return "Daheng (SDK未编译)";
#endif
}
