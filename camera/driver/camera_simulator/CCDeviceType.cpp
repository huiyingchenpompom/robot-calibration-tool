
#include "CCDeviceType.h"

#include <nlohmann/json.hpp>
#include "CCSimulatorCameraMainProcess.h"
#include "CCDataIdValidator.hpp"

#include "utility/core/CCUtilityTypeDefine.h"
#include "utility/core/setting/CCGlobalSettingManager.h"

using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

std::shared_ptr<utility::CCDeviceImpl> startDevice(const std::list<std::string> &listString, int transmitType, bool &ret)
{
    if (listString.size() < 4) {
        return nullptr;
    }

    auto it = listString.begin();
    std::string mainType(*it);
    ++ it;
    std::string subType(*it);
    ++ it;
    std::string deviceId(*it);

    int cameraId = 0;
    int triggerType = 0;
    std::string imagePath = "image";
    ++ it;
    try {
        ordered_json jsonObj = ordered_json::parse(*it);
        cameraId = jsonObj.at("camera_id");
        triggerType = jsonObj.at("trigger_type");
        if (jsonObj.contains("image_path")) {
            imagePath = jsonObj.at("image_path");
        }
    } catch (const ordered_json::exception &e) {
        CCError(u8"设备配置解析错误: %s", u8"请检查设备配置文件", e.what());
        return nullptr;
    }

    auto camera = std::make_shared<CCSimulatorCamera>(mainType, subType, deviceId, cameraId, triggerType, transmitType, imagePath);
    return camera;
}

int getDeviceType(utility::CCDeviceType &type)
{
    std::string text(MAIN_TYPE);
    auto pos = text.find_last_of('/');
    type.mainType = text.substr(pos + 1);
    type.mainTypeName = MAIN_TYPE_NAME;

    text = std::string(SUB_TYPE);
    pos = text.find_last_of('/');
    type.subType = text.substr(pos + 1);
    type.subTypeName = SUB_TYPE_NAME;

    type.deviceExeName = DEVICE_NODE;
    type.startDevice = startDevice;

    return 0;
}

CCSimulatorCamera::CCSimulatorCamera(const std::string &mainType,
                                     const std::string &subType,
                                     const std::string &objId,
                                     int index, int triggerType,
                                     int transmitType,
                                     const std::string &imagePath)
    : utility::CCDeviceImpl()
{
    m_process = std::make_shared<CCSimulatorCameraMainProcess>(mainType, subType, objId, index, triggerType, transmitType, imagePath);
}

CCSimulatorCamera::~CCSimulatorCamera()
{
}

std::shared_ptr<utility::CCMessageNode> CCSimulatorCamera::rosNode()
{
    return m_process->rosNode();
}

void CCSimulatorCamera::run()
{
    m_process->run();
}

void CCSimulatorCamera::registerDataIdValidator(void *dataIdMeta)
{
    CC_DATA_ID_META_REGISTER(dataIdMeta);
    CC_DATA_ID_NODE_REGISTER("simulator camera.");
}

