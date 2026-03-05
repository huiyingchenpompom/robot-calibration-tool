#include "hik_camera.h"
#include <stdexcept>

// 注意：此文件需要海康威视 MVS SDK。
// 未安装 SDK 时编译为存根实现。
#ifdef HAVE_HIK_SDK
#include "MvCameraControl.h"
#endif

HikCamera::HikCamera() : handle_(nullptr), connected_(false) {}

HikCamera::~HikCamera() { disconnect(); }

bool HikCamera::connect(int deviceIndex) {
#ifdef HAVE_HIK_SDK
    MV_CC_DEVICE_INFO_LIST deviceList;
    if (MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &deviceList) != MV_OK) {
        throw std::runtime_error("枚举海康相机失败");
    }
    if (deviceIndex >= (int)deviceList.nDeviceNum) {
        throw std::runtime_error("设备序号超出范围");
    }
    void* h = nullptr;
    if (MV_CC_CreateHandle(&h, deviceList.pDeviceInfo[deviceIndex]) != MV_OK) {
        throw std::runtime_error("创建海康相机句柄失败");
    }
    if (MV_CC_OpenDevice(h) != MV_OK) {
        MV_CC_DestroyHandle(h);
        throw std::runtime_error("打开海康相机失败");
    }
    handle_ = h;
    connected_ = true;
    return true;
#else
    (void)deviceIndex;
    throw std::runtime_error("HIK MVS SDK 未编译，无法连接");
#endif
}

void HikCamera::disconnect() {
#ifdef HAVE_HIK_SDK
    if (handle_) {
        MV_CC_CloseDevice(handle_);
        MV_CC_DestroyHandle(handle_);
        handle_ = nullptr;
    }
#endif
    connected_ = false;
}

bool HikCamera::isConnected() const { return connected_; }

std::vector<uint8_t> HikCamera::captureImage() {
#ifdef HAVE_HIK_SDK
    if (!handle_) throw std::runtime_error("相机未连接");
    MV_CC_StartGrabbing(handle_);
    MV_FRAME_OUT frameOut = {0};
    if (MV_CC_GetImageBuffer(handle_, &frameOut, 1000) != MV_OK) {
        MV_CC_StopGrabbing(handle_);
        throw std::runtime_error("获取图像失败");
    }
    std::vector<uint8_t> data(frameOut.pBufAddr, frameOut.pBufAddr + frameOut.stFrameInfo.nFrameLen);
    MV_CC_FreeImageBuffer(handle_, &frameOut);
    MV_CC_StopGrabbing(handle_);
    return data;
#else
    throw std::runtime_error("HIK SDK 未编译");
#endif
}

void HikCamera::setExposure(double exposureUs) {
#ifdef HAVE_HIK_SDK
    if (handle_) MV_CC_SetFloatValue(handle_, "ExposureTime", (float)exposureUs);
#else
    (void)exposureUs;
#endif
}

void HikCamera::setGain(double gainDb) {
#ifdef HAVE_HIK_SDK
    if (handle_) MV_CC_SetFloatValue(handle_, "Gain", (float)gainDb);
#else
    (void)gainDb;
#endif
}

void HikCamera::setGamma(double gamma) {
#ifdef HAVE_HIK_SDK
    if (handle_) MV_CC_SetFloatValue(handle_, "Gamma", (float)gamma);
#else
    (void)gamma;
#endif
}

std::string HikCamera::getModelName() const {
    return connected_ ? "HIK Camera" : "HIK (未连接)";
}
