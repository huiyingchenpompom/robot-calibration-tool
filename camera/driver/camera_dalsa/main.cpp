/**
 * usb相机驱动
 * 实现功能：相机参数设置、取图
 *
 */

#include <chrono>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include <rclcpp/rclcpp.hpp>
#include <nlohmann/json.hpp>
#include <boost/dll/shared_library.hpp>

#include "CCDalsaMainProcess.h"
#include "utility/CCUtilityDefine.h"
#include "LogClientCommon.h"
#include <fstream>
#include "cc_toolkit.h"
#include "cc_ros_msg_interface.h"

using json = nlohmann::json;
using namespace utility;
using namespace std::chrono_literals;

static void readConfig(const std::string &file, std::string &jsonStr)
{
    std::ifstream in(file, std::ios::in | std::ios::binary);
    if (!in.is_open())
    {
        return;
    }
    std::istreambuf_iterator<char> beg(in), end;
    jsonStr = std::string(beg, end);
    in.seekg(0, std::ios::end);
    in.close();
}

int main(int argc, char **argv)
{
    std::string modName;
    std::string ipAddr;
    std::string dumpDir;
    mi_uint16   port;
    mi_uint32   interval;
    mi_uint32   logLevel;
    std::string camperaIp;
    std::string jsonStr;
    readConfig(argv[1], jsonStr);

    auto cfgRoot = nlohmann::json::parse(jsonStr);
    if(cfgRoot.empty())
    {
        exit(1);
    }
    cfgRoot["ipAddr"].get_to(ipAddr);
    cfgRoot["dumpDir"].get_to(dumpDir);
    cfgRoot["port"].get_to(port);
    cfgRoot["interval"].get_to(interval);
    cfgRoot["logLevel"].get_to(logLevel);
    cfgRoot["modName"].get_to(modName);
    cfgRoot["cameraIpAddr"].get_to(camperaIp);
    if( !wrapperLogStart(  modName.c_str()
                         , ipAddr.c_str()
                         , dumpDir.c_str()
                         , port, logLevel ))
    {
        fprintf(stderr, "Failed to start log client.\n");
        exit(1);
    }
    CCInfo("MI CC system start.");
    rclcpp::init(argc, argv);

    std::string mainType("dadd");
    std::string subType("34ffd");
    std::string deviceId("fdad");

    int cameraId = 0;
    CCDalsaMainProcess process(mainType, subType, deviceId, cameraId, camperaIp);
    process.run();

    rclcpp::shutdown();
    CCInfo("MI CC system exit.");
    wrapperLogStop();
    return 0;
}
