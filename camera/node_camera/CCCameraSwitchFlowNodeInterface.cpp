#include <assert.h>
#include "node/CCFlowNodeInterface.h"
#include "CCCameraSwitchFlowNode.h"

using namespace driver;

std::shared_ptr<utility::CCFlowNode> createCameraSwitchFlowNode(const std::string &id, const std::string &name)
{
    return std::make_shared<CCCameraSwitchFlowNode>(id, name);
}

CCFunNode getFlowNodeType(utility::CCFlowNodeType &type)
{
    type = CCCameraSwitchFlowNode::ms_type;
    return createCameraSwitchFlowNode;
}
