#include "hik_camera.h"
#include <stdexcept>
#include <string>

// 注意：此文件需要海康威视 MVS SDK（MvCameraControl.h）。
// 未安装 SDK 时编译为存根实现。
#ifdef HAVE_HIK_SDK
#include "MvCameraControl.h"
#endif

HikCamera::HikCamera()
    : handle_(nullptr), connected_(false), is_opened_(false), is_grabbing_(false) {}

HikCamera::~HikCamera() { disconnect(); }

bool HikCamera::connect(int deviceIndex) {
#ifdef HAVE_HIK_SDK
    // Step 1: 枚举相机设备
    // 参考 CCHikCameraImp::enumDevices：同时枚举 GigE 和 USB 相机
    MV_CC_DEVICE_INFO_LIST deviceList = {};
    if (MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &deviceList) != MV_OK) {
        throw std::runtime_error("枚举海康相机失败，请检查 MVS 驱动是否安装");
    }
    if (deviceIndex >= (int)deviceList.nDeviceNum) {
        throw std::runtime_error("设备序号超出范围，共扫描到 "
                                 + std::to_string(deviceList.nDeviceNum) + " 台相机");
    }

    // Step 2: 创建设备句柄
    // 参考 CCHikCameraImp::createHandle
    void* h = nullptr;
    if (MV_CC_CreateHandle(&h, deviceList.pDeviceInfo[deviceIndex]) != MV_OK) {
        throw std::runtime_error("创建海康相机句柄失败");
    }

    // Step 3: 打开设备
    // 参考 CCHikCameraImp::openDevice
    if (MV_CC_OpenDevice(h) != MV_OK) {
        MV_CC_DestroyHandle(h);
        throw std::runtime_error(
            "打开海康相机失败，请检查：\n"
            "1. 相机网线/USB 连接是否正常\n"
            "2. MVS 软件是否占用了相机");
    }
    is_opened_ = true;

    // Step 4: 相机初始化配置
    // 参考 CCHikCameraImp::openDevice + setAcquisitionMode + setSoftwareTrigger：
    // ① 连续采集模式 ——防止相机处于单帧模式
    MV_CC_SetEnumValue(h, "AcquisitionMode", MV_ACQ_MODE_CONTINUOUS);
    // ② 关闭触发模式 ——防止相机等待外部硬触发信号导致 GetImageBuffer 超时
    MV_CC_SetEnumValue(h, "TriggerMode", 0); // 0 = Off
    // ③ GigE 相机：设置心跳超时为 3 s（USB 相机无此参数，失败忽略）
    // 参考 CCHikCameraImp::setHeartbeatTime
    MV_CC_SetIntValue(h, "GevHeartbeatTimeout", 3000);

    handle_    = h;
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
        // 参考 CCHikCameraImp::~CCHikCameraImp 的清理顺序：
        // 先停止采集 → 再关闭设备 → 最后销毁句柄
        if (is_grabbing_) {
            MV_CC_StopGrabbing(handle_);
            is_grabbing_ = false;
        }
        if (is_opened_) {
            MV_CC_CloseDevice(handle_);
            is_opened_ = false;
        }
        MV_CC_DestroyHandle(handle_);
        handle_ = nullptr;
    }
#endif
    connected_ = false;
}

bool HikCamera::isConnected() const { return connected_; }

CameraFrame HikCamera::captureImage() {
#ifdef HAVE_HIK_SDK
    if (!is_opened_) throw std::runtime_error("相机未打开");

    // 参考 CCHikCameraImp::startGrabbing：出现失败时最多重试 10 次
    // （2000 万像素相机偶发性 StartGrabbing 失败需要重试）
    if (!is_grabbing_) {
        bool started  = false;
        int  maxTry   = 10;
        while (maxTry-- >= 0) {
            if (MV_CC_StartGrabbing(handle_) == MV_OK) {
                is_grabbing_ = true;
                started      = true;
                break;
            }
        }
        if (!started) {
            throw std::runtime_error("开始取流失败，请重新连接相机");
        }
    }

    // 参考 CCHikCameraImp 图像获取方式：GetImageBuffer / FreeImageBuffer
    MV_FRAME_OUT frameOut = {};
    if (MV_CC_GetImageBuffer(handle_, &frameOut, 3000) != MV_OK) {
        MV_CC_StopGrabbing(handle_);
        is_grabbing_ = false;
        throw std::runtime_error("获取图像超时（3 s），请检查相机连接和光源");
    }

    CameraFrame frame;
    frame.width  = static_cast<int>(frameOut.stFrameInfo.nWidth);
    frame.height = static_cast<int>(frameOut.stFrameInfo.nHeight);

    // 参考 CCHikCameraImp::isColor / photoReceivedCallBack 的格式判断
    if (frameOut.stFrameInfo.enPixelType == PixelType_Gvsp_Mono8) {
        // 黑白相机
        frame.channels = 1;
        frame.pixels.assign(frameOut.pBufAddr,
                            frameOut.pBufAddr + frame.width * frame.height);
    } else if (frameOut.stFrameInfo.enPixelType == PixelType_Gvsp_BGR8_Packed) {
        // BGR（直接复制，BMP 需要 BGR）
        frame.channels = 3;
        frame.pixels.assign(frameOut.pBufAddr,
                            frameOut.pBufAddr + frame.width * frame.height * 3);
    } else if (frameOut.stFrameInfo.enPixelType == PixelType_Gvsp_RGB8_Packed) {
        // RGB → BGR
        frame.channels = 3;
        int n = frame.width * frame.height;
        frame.pixels.resize(n * 3);
        const uint8_t* src = frameOut.pBufAddr;
        for (int i = 0; i < n; ++i) {
            frame.pixels[i * 3 + 0] = src[i * 3 + 2]; // B
            frame.pixels[i * 3 + 1] = src[i * 3 + 1]; // G
            frame.pixels[i * 3 + 2] = src[i * 3 + 0]; // R
        }
    } else {
        // Bayer 及其他格式：作为单通道灰度显示（棋盘格特征仍清晰可见）
        frame.channels = 1;
        frame.pixels.assign(frameOut.pBufAddr,
                            frameOut.pBufAddr + frame.width * frame.height);
    }

    MV_CC_FreeImageBuffer(handle_, &frameOut);

    // 单次拍摄后停止采集，保持与 connect() 中关闭触发对称的干净状态
    MV_CC_StopGrabbing(handle_);
    is_grabbing_ = false;
    return frame;
#else
    throw std::runtime_error("HIK SDK 未编译");
#endif
}

void HikCamera::setExposure(double exposureUs) {
#ifdef HAVE_HIK_SDK
    // 参考 CCHikCameraImp::setExposureTime
    if (is_opened_)
        MV_CC_SetFloatValue(handle_, "ExposureTime", static_cast<float>(exposureUs));
#else
    (void)exposureUs;
#endif
}

void HikCamera::setGain(double gainDb) {
#ifdef HAVE_HIK_SDK
    // 参考 CCHikCameraImp::setGain
    if (is_opened_)
        MV_CC_SetFloatValue(handle_, "Gain", static_cast<float>(gainDb));
#else
    (void)gainDb;
#endif
}

void HikCamera::setGamma(double gamma) {
#ifdef HAVE_HIK_SDK
    // 参考 CCHikCameraImp::setGamma
    if (is_opened_)
        MV_CC_SetFloatValue(handle_, "Gamma", static_cast<float>(gamma));
#else
    (void)gamma;
#endif
}

std::string HikCamera::getModelName() const {
#ifdef HAVE_HIK_SDK
    // 参考 CCHikCameraImp::getStringValue("DeviceModelName")
    if (!is_opened_) return "HIK (未连接)";
    MVCC_STRINGVALUE stStringValue = {};
    if (MV_CC_GetStringValue(handle_, "DeviceModelName", &stStringValue) == MV_OK) {
        return std::string(stStringValue.chCurValue);
    }
    return "HIK Camera";
#else
    return "HIK (SDK未编译)";
#endif
}

