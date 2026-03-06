#pragma once
#include <string>
#include <vector>
#include <cstdint>

// 相机原始帧数据（含图像元数据）
struct CameraFrame {
    std::vector<uint8_t> pixels;  // 原始像素字节 (Mono8: 1字节/像素; BGR8: 3字节/像素)
    int width    = 0;
    int height   = 0;
    int channels = 1;  // 1 = 灰度/Mono8; 3 = BGR8packed
};

// 相机基础接口类
class CameraBase {
public:
    virtual ~CameraBase() = default;

    // 连接相机（deviceIndex: 设备序号）
    virtual bool connect(int deviceIndex) = 0;

    // 断开相机连接
    virtual void disconnect() = 0;

    // 是否已连接
    virtual bool isConnected() const = 0;

    // 拍摄一帧，返回含宽高通道的原始像素数据
    virtual CameraFrame captureImage() = 0;

    // 设置曝光时间 (微秒)
    virtual void setExposure(double exposureUs) = 0;

    // 设置增益 (dB)
    virtual void setGain(double gainDb) = 0;

    // 设置 Gamma 值
    virtual void setGamma(double gamma) = 0;

    // 获取相机型号描述
    virtual std::string getModelName() const = 0;
};
