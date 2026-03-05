#ifndef INFERBOXTOPICNAME_HPP
#define INFERBOXTOPICNAME_HPP

#include "utility/core/CCRosName.hpp"
#include "utility/core/CCDevice.hpp"

using namespace utility;

namespace driver
{


inline std::string ccRosTopicNameGetImage(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("get_image");

    return name;
}

inline std::string ccRosTopicNameGetImageBack(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("get_image_back");

    return name;
}

inline std::string ccRosTopicNameDriverImageData(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("image_data");

    return name;
}

inline std::string ccRosTopicNameNodeImageData(const std::string &nodeType, const std::string &nodeId)
{
    std::string name = ccRosTopicMerge(nodeType, nodeId);
    name.append("/");
    name.append("image_data");
    return name;
}

inline std::string ccRosTopicNameSetParam(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("set_param");

    return name;
}

inline std::string ccRosTopicNameSetParamBack(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("set_param_back");

    return name;
}

inline std::string ccRosTopicNameParamData(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("param_data");

    return name;
}

inline std::string ccRosTopicNameGetParam(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("get_param");

    return name;
}

inline std::string ccRosTopicNameGetParamData(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("get_param_data");

    return name;
}

inline std::string ccRosTopicNameCameraSwitch(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("camera_switch");

    return name;
}

inline std::string ccRosTopicNameCameraSwitchReply(const CCDeviceType &devType, const std::string &objId)
{
    std::string name = ccRosTopicMerge(devType, objId);
    name.append("/");
    name.append("camera_switch_reply");

    return name;
}





}

#endif // INFERBOXTOPICNAME_HPP
