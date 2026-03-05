#include "CCHikCamera.h"
#include <nlohmann/json.hpp>
#include "CCHikMainProcess.h"
#include "LogClientCommon.h"
#include "CCDataIdValidator.hpp"

#include "utility/core/CCUtilityTypeDefine.h"
#include "utility/core/setting/CCGlobalSettingManager.h"

using json = nlohmann::json;

std::shared_ptr<utility::CCDeviceImpl> startDevice(const std::list<std::string> &listString, int transmitType, bool &ret)
{
    CCInfo(" %s enter", __FUNCTION__ );
    if (listString.size() < 4) {
        CCError(u8"海康相机startDevice失败", u8"驱动参数有错误，请联系SC开发人员排查问题");
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
#ifdef HIK_USB_AREA
    if (jsonObj.contains("device_sn"))
    {
        info = jsonObj.at("device_sn");
    }
#else // HIK_GIGE_LINE || HIK_GIGE_AREA
    if (jsonObj.contains("device_ip"))
    {
        info = jsonObj.at("device_ip");
    }
#endif
    if (jsonObj.contains("trigger_type"))
    {
        triggerType = jsonObj.at("trigger_type");
    }

    auto camera = std::make_shared<CCHikCamera>( mainType
                                                    , subType
                                                    , deviceId
                                                    , cameraId
                                                    , info
                                                    , transmitType
                                                    , jsonObj);
    camera->openCamera(triggerType);

    CCInfo(" %s exit", __FUNCTION__ );
    return camera;
}



int getDeviceType(utility::CCDeviceType &type)
{
    CCInfo("hik camera %s enter", __FUNCTION__ );
    std::string text(MAIN_TYPE);
    auto pos = text.find_last_of('/');
    std::string mainType = text.substr(pos + 1);
    type.mainType = mainType;
    type.mainTypeName = MAIN_TYPE_NAME;
#if defined(HIK_USB_AREA)
    type.subType = "camera_hik_usb_area";
    type.subTypeName = "Hik Usb Camera";
#elif defined(HIK_GIGE_AREA)
    type.subType = "camera_hik_gige_area";
    type.subTypeName = "Hik GigE Camera";
#else // HIK_GIGE_LINE
    type.subType = "camera_hik_gige_line";
    type.subTypeName = "Hik GigE Camera";
#endif
    type.deviceExeName = "hik";
    type.startDevice = startDevice;

    CCInfo("hik camera %s exit", __FUNCTION__ );
    return 0;
}

CCHikCamera::CCHikCamera(const std::string &mainType,
                         const std::string &subType,
                         const std::string &objId,
                         int index,
                         const std::string &info,
                         int transmitType,
                         const nlohmann::json &json)
    : utility::CCDeviceImpl()
{
    CCInfo("hik camera %s enter", __FUNCTION__ );
    m_process = std::make_shared<CCHikMainProcess>(mainType, subType, objId, index, info, transmitType, json);
    CCInfo("hik camera %s exit", __FUNCTION__ );
}

std::shared_ptr<utility::CCMessageNode> CCHikCamera::rosNode()
{
    CCInfo("hik camera %s enter", __FUNCTION__ );
    return m_process->rosNode();
}

bool CCHikCamera::openCamera(int triggerType)
{
    CCInfo(" hik camera %s enter", __FUNCTION__ );
    return m_process->openCamera(triggerType);
}

std::shared_ptr<CCHikCameraImp> CCHikCamera::getCameraHandle()
{
    CCInfo("hik camera %s enter", __FUNCTION__ );
    return m_process->cameraIO();
}

void CCHikCamera::run()
{
    m_process->run();
}

void CCHikCamera::registerDataIdValidator(void *dataIdMeta)
{
    CC_DATA_ID_META_REGISTER(dataIdMeta);
}
