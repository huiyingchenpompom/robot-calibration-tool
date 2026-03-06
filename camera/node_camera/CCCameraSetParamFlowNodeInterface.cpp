/**
 * 流程节点对外接口，主程序加载此动态库时，调用以下接口，获取对应
 * 节点信息和创建方法
 *
 */
#include "node/CCFlowNodeInterface.h"
#include "CCCameraSetParamFlowNode.h"

using namespace driver;
using namespace utility;

std::shared_ptr<utility::CCFlowNode> createSetParamFlowNode(const std::string &id, const std::string &name)
{
    return std::make_shared<CCCameraSetParamFlowNode>(id, name);
}

CCFunNode getFlowNodeType(utility::CCFlowNodeType &type)
{
    type = CCCameraSetParamFlowNode::ms_type;
    return createSetParamFlowNode;
}
