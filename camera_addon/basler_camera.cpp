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
            throw std::runtime_error("未找到 Basler 相机，请检查 pylon 驱动是否安装");
        }
        if (deviceIndex >= (int)devices.size()) {
            throw std::runtime_error("设备序号超出范围，共找到 "
                                     + std::to_string(devices.size()) + " 台相机");
        }
        auto* cam = new CInstantCamera(tlFactory.CreateDevice(devices[deviceIndex]));
        cam->Open();

        // 参考 CCBaslerCameraImp::openDevice：
        // ① 启用 Gamma 功能（部分型号不支持 GammaSelector，失败则忽略）
        try {
            GenApi::INodeMap& nm = cam->GetNodeMap();
            CEnumerationPtr gammaSel(nm.GetNode("GammaSelector"));
            if (IsValid(gammaSel) && IsWritable(gammaSel)) {
                gammaSel->FromString("User");
            }
            CBooleanPtr gammaEn(nm.GetNode("GammaEnable"));
            if (IsValid(gammaEn) && IsWritable(gammaEn)) {
                gammaEn->SetValue(true);
            }
        } catch (...) {}

        camera_handle_ = cam;
        connected_     = true;
        return true;
    } catch (const GenericException& e) {
        throw std::runtime_error(
            std::string("Basler 连接失败: ") + e.GetDescription()
            + "\n请检查：\n1. 相机网线/USB 连接\n2. pylon Viewer 是否占用相机");
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

CameraFrame BaslerCamera::captureImage() {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) throw std::runtime_error("相机未连接");
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    CGrabResultPtr grabResult;
    cam->GrabOne(5000, grabResult);
    if (!grabResult->GrabSucceeded()) {
        throw std::runtime_error(std::string("拍摄失败: ") + grabResult->GetErrorDescription().c_str());
    }
    CameraFrame frame;
    frame.width  = static_cast<int>(grabResult->GetWidth());
    frame.height = static_cast<int>(grabResult->GetHeight());

    // 参考 CCBaslerCameraImp::OnImageGrabbed 的像素格式分支
    // targetImage 必须声明在 if/else 外部，保证 pData 在 assign() 期间仍然有效
    CPylonImage targetImage;
    if (grabResult->GetPixelType() == PixelType_Mono8) {
        // 灰度图：直接复制
        frame.channels = 1;
        const uint8_t* pData = static_cast<const uint8_t*>(grabResult->GetBuffer());
        frame.pixels.assign(pData, pData + grabResult->GetImageSize());
    } else {
        // 彩色图：使用 Pylon 格式转换器统一转为 BGR8
        CImageFormatConverter converter;
        converter.OutputPixelFormat = PixelType_BGR8packed;
        converter.Convert(targetImage, grabResult);
        frame.channels = 3;
        const uint8_t* pData = static_cast<const uint8_t*>(targetImage.GetBuffer());
        frame.pixels.assign(pData, pData + frame.width * frame.height * 3);
        // targetImage 在此作用域结束时销毁，但 assign() 已完成数据拷贝
    }
    return frame;
#else
    throw std::runtime_error("Basler SDK 未编译");
#endif
}

void BaslerCamera::setExposure(double exposureUs) {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) return;
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    try {
        GenApi::INodeMap& nm = cam->GetNodeMap();
        // 参考 CCBaslerCameraImp::setExposureTime：
        //   USB / 新型 GigE 相机 → ExposureTime（浮点）
        //   旧型 GigE 相机       → ExposureTimeRaw（整型）
        auto expNode = nm.GetNode("ExposureTime");
        if (IsValid(expNode) && IsWritable(expNode)) {
            CFloatParameter(nm, "ExposureTime").SetValue(exposureUs);
        } else {
            CIntegerParameter(nm, "ExposureTimeRaw").SetValue(
                static_cast<int64_t>(exposureUs));
        }
    } catch (...) {}
#else
    (void)exposureUs;
#endif
}

void BaslerCamera::setGain(double gainDb) {
#ifdef HAVE_PYLON_SDK
    if (!camera_handle_) return;
    auto* cam = static_cast<CInstantCamera*>(camera_handle_);
    try {
        GenApi::INodeMap& nm = cam->GetNodeMap();
        // 参考 CCBaslerCameraImp::setGain：
        //   USB / 新型 GigE 相机 → Gain（浮点）
        //   旧型 GigE 相机       → GainRaw（整型）
        auto gainNode = nm.GetNode("Gain");
        if (IsValid(gainNode) && IsWritable(gainNode)) {
            CFloatParameter(nm, "Gain").SetValue(gainDb);
        } else {
            CIntegerParameter(nm, "GainRaw").SetValue(
                static_cast<int64_t>(gainDb));
        }
    } catch (...) {}
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
