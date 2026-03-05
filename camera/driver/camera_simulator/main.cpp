/**
 * usb相机驱动
 * 实现功能：相机参数设置、取图
 *
 */
#include <nlohmann/json.hpp>

#include "CCSimulatorCameraMainProcess.h"
#include "utility/core/CCUtilityDefine.h"
#include "utility/core/CCLogClient.hpp"
#include "utility/core/CCUtilityTypeDefine.h"
#include "utility/cc_message/CCMessageFunction.h"
#include "sc_config.h"

using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

int main(int argc, char **argv)
{
    // 日志
    if (utility::ccLogClientStart("simulator_camera", "simulator_camera", SC_VERSION, SC_GIT_COMMIT_ID) == RET_FAILED)
    {
        std::cerr << "Failed to start log client." << std::endl;
        exit(-1);
    }
    CCInfo("### begin: %s, %s, %s, %s", argv[1], argv[2], argv[3], argv[4]);

    if (argc < 5) {
        CCError("the number of input arguments is error.", "");
        exit(-1);
    }
    utility::cc_message::init(argc, argv);

    std::string mainType(argv[1]);
    std::string subType(argv[2]);
    std::string deviceId(argv[3]);

    ordered_json jsonObj = ordered_json::parse(argv[4]);
    int cameraId = 0;
    int triggerCount = 1;
    if (jsonObj.contains("camera_id")) {
        cameraId = jsonObj.at("camera_id");
    }
    if (jsonObj.contains("trigger_type")) {
        triggerCount = jsonObj.at("trigger_type");
    }
    CCSimulatorCameraMainProcess process(mainType, subType, deviceId, cameraId, triggerCount, utility::TransmitCrossProcess);
    process.run();

    utility::cc_message::shutdownMsg();
    wrapperLogStop();
    return 0;
}
