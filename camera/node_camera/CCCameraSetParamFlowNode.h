/**
 * 流程设置相机参数节点
 *
 */
#ifndef CCCAMERASETPARAMFLOWNODE_H
#define CCCAMERASETPARAMFLOWNODE_H

#include "utility/core/CCUtilityDefine.h"
#include "utility/core/CCFlowNode.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_modbus_data.hpp"

using namespace ros_msg::msg;

namespace driver
{

class CCCameraSetParamRosNode;

class CCCameraSetParamFlowNode : public utility::CCFlowNode
{
public:
    CCCameraSetParamFlowNode(const std::string &, const std::string &);
    ~CCCameraSetParamFlowNode();

    virtual int start() override;
    virtual void stop() override;
    virtual void run() override;
    virtual int createOutput(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam) override {return RET_SUCCESS;}
    virtual int parseJsonValue(const ordered_json &, const ordered_json &) override;
    virtual int updateInput() override;

    virtual utility::CCFlowNodeType type() override;
    static utility::CCFlowNodeType ms_type;

protected:
    void onSubscribeParam(const CCCameraParam::SharedPtr);
    void onSubscribeImageId(const CCModbusData::SharedPtr);

protected:
    int m_imageId = 0;
    std::map<int, CCCameraParam> m_param;
};


}

#endif // CCCAMERASETPARAMFLOWNODE_H
