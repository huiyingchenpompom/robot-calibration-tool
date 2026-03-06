#include "CCOrbbecCamera.h"
#include <nlohmann/json.hpp>
#include "CCOrbbecMainProcess.h"
#include "LogClientCommon.h"
#include "CCDataIdValidator.hpp"

#include "utility/core/CCUtilityTypeDefine.h"
#include "utility/core/setting/CCGlobalSettingManager.h"

using json = nlohmann::json;

std::shared_ptr<utility::CCDeviceImpl> startDevice(const std::list<std::string> &listString, int transmitType, bool &ret)
{
    CCInfo("orbbec %s enter", __FUNCTION__ );
    if (listString.size() < 4) {
        CCInfo("failed: %s exit", __FUNCTION__ );
        return nullptr;
    }

    auto it = listString.begin();
    std::string mainType(*it);
    ++ it;
    std::string subType(*it);
    ++ it;
    std::string deviceId(*it);

    ++ it;
    json jsonObj = json::parse(*it);
    int cameraId = 0;
    int triggerType = 1;
    std::string info;
    if (jsonObj.contains("camera_id"))
    {
        cameraId = jsonObj["camera_id"];
    }

    if (jsonObj.contains("device_ip"))
    {
        info = jsonObj.at("device_ip");
    }

    if (jsonObj.contains("trigger_type"))
    {
        triggerType = jsonObj.at("trigger_type");
    }

    auto camera = std::make_shared<CCOrbbecCamera>( mainType
                                                    , subType
                                                    , deviceId
                                                    , cameraId
                                                    , info
                                                    , transmitType);
    camera->openCamera(triggerType);

    CCInfo("orbbec %s exit", __FUNCTION__ );
    return camera;
}



int getDeviceType(utility::CCDeviceType &type)
{
    CCInfo("orbbec camera %s enter", __FUNCTION__ );
    std::string text(MAIN_TYPE);
    auto pos = text.find_last_of('/');
    std::string mainType = text.substr(pos + 1);
    type.mainType = mainType;
    type.mainTypeName = MAIN_TYPE_NAME;

    type.subType = "camera_orbbec";
    type.subTypeName = "Orbbec Camera";

    type.deviceExeName = "orbbec";
    type.startDevice = startDevice;

    CCInfo("orbbec camera %s exit", __FUNCTION__ );
    return 0;
}

CCOrbbecCamera::CCOrbbecCamera(const std::string &mainType,
                         const std::string &subType,
                         const std::string &objId,
                         int index,
                         const std::string &info,
                         int transmitType)
    : utility::CCDeviceImpl()
{
    CCInfo("orbbec camera %s enter", __FUNCTION__ );
    try {
        m_process = std::make_shared<CCOrbbecMainProcess>(mainType, subType, objId, index, info, transmitType);
    } catch (std::exception e) {
        CCInfo("create CCOrbbecMainProcess exception : %s", e.what());
    }

    CCInfo("orbbec camera %s exit", __FUNCTION__ );
}

std::shared_ptr<utility::CCMessageNode> CCOrbbecCamera::rosNode()
{
    CCInfo("orbbec camera %s enter", __FUNCTION__ );
    return m_process->rosNode();
}

bool CCOrbbecCamera::openCamera(int triggerType)
{
    CCInfo(" orbbec camera %s enter", __FUNCTION__ );
    return m_process->openCamera(triggerType);
}

std::shared_ptr<CCOrbbecCameraImp> CCOrbbecCamera::getCameraHandle()
{
    CCInfo("orbbec camera %s enter", __FUNCTION__ );
    return m_process->cameraIO();
}

void CCOrbbecCamera::run()
{
    m_process->run();
}

void CCOrbbecCamera::registerDataIdValidator(void *dataIdMeta)
{
    CC_DATA_ID_META_REGISTER(dataIdMeta);
}
