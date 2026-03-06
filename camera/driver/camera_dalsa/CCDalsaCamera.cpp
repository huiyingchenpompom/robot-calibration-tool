#include "CCDalsaCamera.h"
#include <nlohmann/json.hpp>
#include "CCDalsaMainProcess.h"
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
#ifdef DALSA_CL_LINE
    if (jsonObj.contains("device_board"))
    {
        info = jsonObj.at("device_board");
    }
#elif DALSA_USB_AREA
    if (jsonObj.contains("device_sn"))
    {
        info = jsonObj.at("device_sn");
    }
#endif

    if (jsonObj.contains("trigger_type"))
    {
        triggerType = jsonObj.at("trigger_type");
    }

    auto camera = std::make_shared<CCDalsaCamera>( mainType
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
    CCInfo("dalsa camera %s enter", __FUNCTION__ );
    std::string text(MAIN_TYPE);
    auto pos = text.find_last_of('/');
    std::string mainType = text.substr(pos + 1);
    type.mainType = mainType;
    type.mainTypeName = MAIN_TYPE_NAME;
#ifdef DALSA_CL_LINE
    type.subType = "camera_dalsa_cl_line";
    type.subTypeName = "Dalsa CL Camera";
#elif
    type.subType = "camera_dalsa_usb_area";
    type.subTypeName = "Dalsa USB Camera";
#endif
    type.deviceExeName = "dalsa";
    type.startDevice = startDevice;

    CCInfo("dalsa camera %s exit", __FUNCTION__ );
    return 0;
}

CCDalsaCamera::CCDalsaCamera(const std::string &mainType,
                                     const std::string &subType,
                                     const std::string &objId,
                                     int index,
                                     const std::string &info,
                                     int transmitType)
    : utility::CCDeviceImpl()
{
    CCInfo("dalsa camera %s enter", __FUNCTION__ );
    m_process = std::make_shared<CCDalsaMainProcess>(mainType, subType, objId, index, info, transmitType);
    CCInfo("dalsa camera %s exit", __FUNCTION__ );
}

std::shared_ptr<utility::CCMessageNode> CCDalsaCamera::rosNode()
{
    CCInfo("dalsa camera %s enter", __FUNCTION__ );
    return m_process->rosNode();
}

bool CCDalsaCamera::openCamera(int triggerType)
{
    CCInfo(" dalsa camera %s enter", __FUNCTION__ );
    return m_process->openCamera(triggerType);
}

std::shared_ptr<CCDalsaCameraImp> CCDalsaCamera::getCameraHandle()
{
    CCInfo("dalsa camera %s enter", __FUNCTION__ );
    return m_process->cameraIO();
}

void CCDalsaCamera::run()
{
    m_process->run();
}


void CCDalsaCamera::registerDataIdValidator(void *dataIdMeta)
{
    CC_DATA_ID_META_REGISTER(dataIdMeta);
}
