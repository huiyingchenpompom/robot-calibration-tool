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
        throw std::runtime_error("枚举大恒相机失败，请检查 Galaxy 驱动是否安装");
    }
    if (nDeviceNum == 0 || deviceIndex >= (int)nDeviceNum) {
        throw std::runtime_error("未找到大恒相机或设备序号超出范围，共扫描到 "
                                 + std::to_string(nDeviceNum) + " 台相机");
    }

    GX_DEV_HANDLE hDevice = nullptr;
    // GXOpenDeviceByIndex 使用 1-based 索引
    // 参考 CCDahengCameraImp::openDevice 中按 IP/SN 打开设备的等效操作
    if (GXOpenDeviceByIndex((uint32_t)deviceIndex + 1, &hDevice) != GX_STATUS_SUCCESS) {
        throw std::runtime_error(
            "打开大恒相机失败，请检查：\n"
            "1. 相机网线/USB 连接是否正常\n"
            "2. Galaxy Viewer 是否占用了相机");
    }

    // 参考 CCDahengCameraImp::openDevice + setTriggerMode(ContinuousTriggerMode)：
    // ① 设置连续采集模式 ——防止相机处于单帧模式
    GXSetEnum(hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
    // ② 关闭触发模式 ——防止相机等待外部硬触发信号导致 GXDQBuf 超时
    //    对应 CCDahengCameraImp::setTriggerMode(ContinuousTriggerMode) → TriggerMode="Off"
    GXSetEnum(hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);
    // ③ 参考 CCDahengCameraImp::openDevice：启用 Gamma（相机不支持时忽略）
    GXSetBool(hDevice, GX_BOOL_GAMMA_ENABLE, true);

    handle_    = hDevice;
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

CameraFrame DahengCamera::captureImage() {
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

    CameraFrame frame;
    frame.width  = static_cast<int>(pFrameBuffer->nWidth);
    frame.height = static_cast<int>(pFrameBuffer->nHeight);

    // 依据每帧实际字节数推断通道数
    // Mono8 / Bayer8: nImgSize == width * height（1 字节/像素）
    // RGB8:           nImgSize == width * height * 3（3 字节/像素）
    const int monoSize  = frame.width * frame.height;
    const int colorSize = monoSize * 3;
    if (pFrameBuffer->nImgSize >= colorSize) {
        // 3 通道彩色（Galaxy SDK C API 输出 RGB；转为 BGR 以匹配 BMP 格式）
        frame.channels = 3;
        const uint8_t* src = static_cast<const uint8_t*>(pFrameBuffer->pImgBuf);
        frame.pixels.resize(colorSize);
        for (int i = 0; i < monoSize; ++i) {
            frame.pixels[i * 3 + 0] = src[i * 3 + 2]; // B
            frame.pixels[i * 3 + 1] = src[i * 3 + 1]; // G
            frame.pixels[i * 3 + 2] = src[i * 3 + 0]; // R
        }
    } else {
        // 单通道：Mono8 或 Bayer 原始图（Bayer 显示为灰度图，特征依然可见）
        frame.channels = 1;
        frame.pixels.assign(
            static_cast<uint8_t*>(pFrameBuffer->pImgBuf),
            static_cast<uint8_t*>(pFrameBuffer->pImgBuf) + monoSize);
    }

    GXQBuf(hDevice, pFrameBuffer);
    GXStreamOff(hDevice);
    return frame;
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
