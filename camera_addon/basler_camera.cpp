#include "basler_camera.h"
#include <stdexcept>

// 注意：此文件需要 Basler Pylon SDK。
// 未安装 SDK 时编译为存根实现。
#ifdef HAVE_PYLON_SDK
#include <pylon/PylonIncludes.h>
using namespace Pylon;
#endif

BaslerCamera::BaslerCamera() : camera_handle_(nullptr), connected_(false) {
#ifdef HAVE_PYLON_SDK
    PylonInitialize();
#endif
}

BaslerCamera::~BaslerCamera() {
    disconnect();
#ifdef HAVE_PYLON_SDK
    PylonTerminate();
#endif
}

bool BaslerCamera::connect(int deviceIndex) {
#ifdef HAVE_PYLON_SDK
    try {
        CTlFactory& tlFactory = CTlFactory::GetInstance();
        DeviceInfoList_t devices;
        if (tlFactory.EnumerateDevices(devices) == 0) {
            throw std::runtime_error("未找到 Basler 相机");
        }
        if (deviceIndex >= (int)devices.size()) {
            throw std::runtime_error("设备序号超出范围");
        }
        auto* cam = new CInstantCamera(tlFactory.CreateDevice(devices[deviceIndex]));
        cam->Open();
        camera_handle_ = cam;
        connected_ = true;
        return true;
    } catch (const GenericException& e) {
        throw std::runtime_error(std::string("Basler 连接失败: ") + e.GetDescription());
    }
#else
    (void)deviceIndex;
    throw std::runtime_error("Basler Pylon SDK 未编译，无法连接");
#endif
}

void BaslerCamera::disconnect() {
#ifdef HAVE_PYLON_SDK
    if (camera_handle_) {
        auto* cam = static_cast<CInstantCamera*>(camera_handle_);
        cam->Close();
        delete cam;
        camera_handle_ = nullptr;
    }
#endif
    connected_ = false;
}

bool BaslerCamera::isConnected() const { return connected_; }

std::vector<uint8_t> BaslerCamera::captureImage() {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) throw std::runtime_error("相机未连接");
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    CGrabResultPtr grabResult;
    cam->GrabOne(5000, grabResult);
    if (!grabResult->GrabSucceeded()) {
        throw std::runtime_error(std::string("拍摄失败: ") + grabResult->GetErrorDescription().c_str());
    }
    const uint8_t* pData = static_cast<const uint8_t*>(grabResult->GetBuffer());
    return std::vector<uint8_t>(pData, pData + grabResult->GetBufferSize());
#else
    throw std::runtime_error("Basler SDK 未编译");
#endif
}

void BaslerCamera::setExposure(double exposureUs) {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) return;
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    GenApi::INodeMap& nm = cam->GetNodeMap();
    CFloatParameter(nm, "ExposureTime").SetValue(exposureUs);
#else
    (void)exposureUs;
#endif
}

void BaslerCamera::setGain(double gainDb) {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) return;
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    GenApi::INodeMap& nm = cam->GetNodeMap();
    CFloatParameter(nm, "Gain").SetValue(gainDb);
#else
    (void)gainDb;
#endif
}

void BaslerCamera::setGamma(double gamma) {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) return;
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    GenApi::INodeMap& nm = cam->GetNodeMap();
    CFloatParameter(nm, "Gamma").SetValue(gamma);
#else
    (void)gamma;
#endif
}

std::string BaslerCamera::getModelName() const {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) return "Basler (未连接)";
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    return std::string(cam->GetDeviceInfo().GetModelName().c_str());
#else
    return "Basler (SDK未编译)";
#endif
}
