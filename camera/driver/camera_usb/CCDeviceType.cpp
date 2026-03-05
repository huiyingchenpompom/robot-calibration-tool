
#include "CCDeviceType.h"

#include <nlohmann/json.hpp>
#include "CCMainProcess.h"

using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

std::shared_ptr<utility::CCDeviceImpl> startDevice(const std::list<std::string> &listString, bool &ret)
{
    if (listString.size() < 4) {
        return nullptr;
    }

    auto it = listString.begin();
    std::string mainType(*it);
    ++ it;
    std::string subType(*it);
    ++ it;
    std::string objId(*it);

    ++ it;
    ordered_json jsonObj = ordered_json::parse(*it);
    int index = 0;
    if (jsonObj.contains("index")) {
        index = jsonObj.at("index");
    }

    return std::make_shared<CCUsbCamera>(mainType, subType, objId, index);
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

CCUsbCamera::CCUsbCamera(const std::string &mainType,
                                     const std::string &subType,
                                     const std::string &objId,
                                     int index)
    : utility::CCDeviceImpl()
{
    m_process = std::make_shared<CCMainProcess>(mainType, subType, objId, index);
}

std::shared_ptr<rclcpp::Node> CCUsbCamera::rosNode()
{
    return m_process->rosNode();
}
