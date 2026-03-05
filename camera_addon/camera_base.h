#pragma once
#include <string>
#include <vector>
#include <cstdint>

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

    // 拍摄图像，返回 JPEG/PNG 编码的字节数据
    virtual std::vector<uint8_t> captureImage() = 0;

    // 设置曝光时间 (微秒)
    virtual void setExposure(double exposureUs) = 0;

    // 设置增益 (dB)
    virtual void setGain(double gainDb) = 0;

    // 设置 Gamma 值
    virtual void setGamma(double gamma) = 0;

    // 获取相机型号描述
    virtual std::string getModelName() const = 0;
};
