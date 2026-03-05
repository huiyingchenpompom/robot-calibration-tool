/**
* @author wujitao
* @date   2024-2-29
* @brief  此文件获取相机参数流程节点
*/
#ifndef CCCAMERAGETPARAMFLOWNODE_H
#define CCCAMERAGETPARAMFLOWNODE_H

#include "utility/core/CCUtilityDefine.h"
#include "utility/core/CCFlowNode.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"

using namespace ros_msg::msg;

namespace driver {

class CCCameraGetParamFlowNode : public utility::CCFlowNode
{
public:
    CCCameraGetParamFlowNode(const std::string &id, const std::string &name);
    ~CCCameraGetParamFlowNode();
    virtual int start() override;
    virtual void stop() override;
    virtual void run() override;
    virtual int createOutput(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam) override {return RET_SUCCESS;}
    virtual int parseJsonValue(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam) override;
    virtual int updateInput() override {return RET_SUCCESS;}

    virtual utility::CCFlowNodeType type() override;
    static utility::CCFlowNodeType ms_type;

protected:
    void onSubscribeParam(const CCCameraParam::SharedPtr);
private:
    std::shared_ptr<CCCameraParam> m_param;
};

}

#endif // CCCAMERAGETPARAMFLOWNODE_H
