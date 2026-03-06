/**
 * 流程节点对外接口，主程序加载此动态库时，调用以下接口，获取对应
 * 节点信息和创建方法
 *
 */
#include "node/CCFlowNodeInterface.h"

#include <cassert>
#include "CCCameraGetImageFixedFlowNode.h"

using namespace driver;
using namespace utility;

std::shared_ptr<utility::CCFlowNode> createGetImageFlowNode(const std::string &id, const std::string &name)
{
    return std::make_shared<CCCameraGetImageFixedFlowNode>(id, name);
}

CCFunNode getFlowNodeType(utility::CCFlowNodeType &type)
{
    type = CCCameraGetImageFixedFlowNode::ms_type;
    return createGetImageFlowNode;
}
