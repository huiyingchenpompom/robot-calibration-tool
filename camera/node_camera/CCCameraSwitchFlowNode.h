/**
* @author wujitao
* @date   2023-11-13
* @brief  此文件为Camera_Base_Info_Node的数据流类的定义
*/
#ifndef CCCAMERASWITCHFLOWNODE_H
#define CCCAMERASWITCHFLOWNODE_H

#include "utility/core/CCFlowNode.hpp"
#include "utility/core/CCUtilityDefine.h"
#include "ros_msg/msg/cc_driver_switch.hpp"
#include "ros_msg/msg/cc_msg_reply.hpp"

namespace driver
{

class CCCameraSwitchFlowNode : public utility::CCFlowNode
{
public:
    explicit CCCameraSwitchFlowNode(const std::string &id, const std::string &name);
    ~CCCameraSwitchFlowNode();

    virtual void run() override;
    virtual utility::CCFlowNodeType type() override;
    virtual int createOutput(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam) override;
    virtual int parseJsonValue(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam) override;
    virtual int updateInput() override {return RET_SUCCESS;}

    const static utility::CCFlowNodeType ms_type;

protected:
    void onSubscribeCameraReply(const ros_msg::msg::CCMsgReply::SharedPtr reply);

private:
    ros_msg::msg::CCDriverSwitch::SharedPtr m_driverSwitch;
};

}
#endif // CCCAMERASWITCHFLOWNODE_H
