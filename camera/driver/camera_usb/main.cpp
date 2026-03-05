/**
 * usb相机驱动
 * 实现功能：相机参数设置、取图
 *
 */
#include <rclcpp/rclcpp.hpp>
#include <nlohmann/json.hpp>

#include "CCMainProcess.h"
#include "utility/core/CCLogClient.hpp"

using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

int main(int argc, char **argv)
{
    // 日志
    if (utility::ccLogClientStart("usb_camera") == RET_FAILED)
    {
        std::cerr << "Failed to start log client." << std::endl;
        exit(-1);
    }
    if (argc < 5) {
        CCError("the number of arguments is error", "");
        exit(-1);
    }
    rclcpp::init(argc, argv);

    std::string mainType(argv[1]);
    std::string subType(argv[2]);
    std::string objId(argv[3]);

    ordered_json jsonObj = ordered_json::parse(argv[4]);
    int index = 0;
    if (jsonObj.contains("obj_index")) {
        index = jsonObj.at("obj_index");
    }
    CCMainProcess process(mainType, subType, objId, index);
    process.run();

    rclcpp::shutdown();
    wrapperLogStop();
    return 0;
}
