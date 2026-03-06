#include <napi.h>
#include "camera_manager.h"
#include <stdexcept>

// 全局相机管理器实例
static CameraManager g_cameraManager;

// connect(brand: string, deviceIndex: number): void
Napi::Value Connect(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "connect(brand: string, deviceIndex: number)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::string brand = info[0].As<Napi::String>().Utf8Value();
    int deviceIndex = info[1].As<Napi::Number>().Int32Value();
    try {
        bool ok = g_cameraManager.connect(brand, deviceIndex);
        return Napi::Boolean::New(env, ok);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// disconnect(): void
Napi::Value Disconnect(const Napi::CallbackInfo& info) {
    g_cameraManager.disconnect();
    return info.Env().Undefined();
}

// isConnected(): boolean
Napi::Value IsConnected(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), g_cameraManager.isConnected());
}

// captureImage(): string (base64 data URL)
Napi::Value CaptureImage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    try {
        std::string imageBase64 = g_cameraManager.captureImageBase64();
        return Napi::String::New(env, imageBase64);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// setParams({ exposure?, gain?, gamma? }): void
Napi::Value SetParams(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "setParams(params: object)").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object params = info[0].As<Napi::Object>();
    try {
        if (params.Has("exposure") && params.Get("exposure").IsNumber()) {
            g_cameraManager.setExposure(params.Get("exposure").As<Napi::Number>().DoubleValue());
        }
        if (params.Has("gain") && params.Get("gain").IsNumber()) {
            g_cameraManager.setGain(params.Get("gain").As<Napi::Number>().DoubleValue());
        }
        if (params.Has("gamma") && params.Get("gamma").IsNumber()) {
            g_cameraManager.setGamma(params.Get("gamma").As<Napi::Number>().DoubleValue());
        }
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }
    return env.Undefined();
}

// getModelName(): string
Napi::Value GetModelName(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), g_cameraManager.getModelName());
}

// 模块初始化
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("connect", Napi::Function::New(env, Connect));
    exports.Set("disconnect", Napi::Function::New(env, Disconnect));
    exports.Set("isConnected", Napi::Function::New(env, IsConnected));
    exports.Set("captureImage", Napi::Function::New(env, CaptureImage));
    exports.Set("setParams", Napi::Function::New(env, SetParams));
    exports.Set("getModelName", Napi::Function::New(env, GetModelName));
    return exports;
}

NODE_API_MODULE(camera_addon, Init)
