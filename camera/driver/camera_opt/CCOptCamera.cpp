#include "CCOptCamera.h"
#include <nlohmann/json.hpp>
#include "CCOptMainProcess.h"
#include "LogClientCommon.h"
#include "CCDataIdValidator.hpp"

#include "utility/core/CCUtilityTypeDefine.h"
#include "utility/core/setting/CCGlobalSettingManager.h"

using json = nlohmann::json;

std::shared_ptr<utility::CCDeviceImpl> startDevice(const std::list<std::string> &listString, int transmitType, bool &ret)
{
    CCInfo(" %s enter", __FUNCTION__ );
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
#ifdef OPT_USB_AREA
    if (jsonObj.contains("device_sn"))
    {
        info = jsonObj.at("device_sn");
    }
#else // OPT_GIGE_LINE || OPT_GIGE_AREA
    if (jsonObj.contains("device_ip"))
    {
        info = jsonObj.at("device_ip");
    }
#endif
    if (jsonObj.contains("trigger_type"))
    {
        triggerType = jsonObj.at("trigger_type");
    }

    auto camera = std::make_shared<CCOptCamera>( mainType
                                                    , subType
                                                    , deviceId
                                                    , cameraId
                                                    , info
                                                    , transmitType);
    camera->openCamera(triggerType);

    CCInfo(" %s exit", __FUNCTION__ );
    return camera;
}



int getDeviceType(utility::CCDeviceType &type)
{
    CCInfo("opt camera %s enter", __FUNCTION__ );
    std::string text(MAIN_TYPE);
    auto pos = text.find_last_of('/');
    std::string mainType = text.substr(pos + 1);
    type.mainType = mainType;
    type.mainTypeName = MAIN_TYPE_NAME;
#if defined(OPT_USB_AREA)
    type.subType = "camera_opt_usb_area";
    type.subTypeName = "Opt Usb Camera";
#elif defined(OPT_GIGE_AREA)
    type.subType = "camera_opt_gige_area";
    type.subTypeName = "Opt GigE Camera";
#else // OPT_GIGE_LINE
    type.subType = "camera_opt_gige_line";
    type.subTypeName = "Opt GigE Camera";
#endif
    type.deviceExeName = "opt";
    type.startDevice = startDevice;

    CCInfo("opt camera %s exit", __FUNCTION__ );
    return 0;
}

CCOptCamera::CCOptCamera(const std::string &mainType,
                         const std::string &subType,
                         const std::string &objId,
                         int index,
                         const std::string &info,
                         int transmitType)
    : utility::CCDeviceImpl()
{
    CCInfo("opt camera %s enter", __FUNCTION__ );
    m_process = std::make_shared<CCOptMainProcess>(mainType, subType, objId, index, info, transmitType);
    CCInfo("opt camera %s exit", __FUNCTION__ );
}

std::shared_ptr<utility::CCMessageNode> CCOptCamera::rosNode()
{
    CCInfo("opt camera %s enter", __FUNCTION__ );
    return m_process->rosNode();
}

bool CCOptCamera::openCamera(int triggerType)
{
    CCInfo(" opt camera %s enter", __FUNCTION__ );
    return m_process->openCamera(triggerType);
}

std::shared_ptr<CCOptCameraImp> CCOptCamera::getCameraHandle()
{
    CCInfo("opt camera %s enter", __FUNCTION__ );
    return m_process->cameraIO();
}

void CCOptCamera::run()
{
    m_process->run();
}

void CCOptCamera::registerDataIdValidator(void *dataIdMeta)
{
    CC_DATA_ID_META_REGISTER(dataIdMeta);
}
