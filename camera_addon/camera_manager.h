#pragma once
#include "camera_base.h"
#include <memory>
#include <string>

// 相机品牌枚举
enum class CameraBrand {
    BASLER,
    HIK,
    DAHENG,
    UNKNOWN
};

// 相机管理器 - 工厂模式，负责创建和管理相机实例
class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    // 连接指定品牌和序号的相机
    bool connect(const std::string& brand, int deviceIndex);

    // 断开当前相机
    void disconnect();

    // 是否已连接
    bool isConnected() const;

    // 拍摄图像，返回 base64 编码字符串
    std::string captureImageBase64();

    // 设置相机参数
    void setExposure(double exposureUs);
    void setGain(double gainDb);
    void setGamma(double gamma);

    // 获取型号信息
    std::string getModelName() const;

private:
    std::unique_ptr<CameraBase> camera_;

    static CameraBrand parseBrand(const std::string& brandStr);
    static std::string base64Encode(const std::vector<uint8_t>& data);
};
