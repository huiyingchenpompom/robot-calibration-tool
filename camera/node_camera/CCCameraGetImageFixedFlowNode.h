/**
 * 流程取图节点
 *
 */
#ifndef CCCAMERAGETIMAGEFIXEDFLOWNODE_H
#define CCCAMERAGETIMAGEFIXEDFLOWNODE_H

#include "utility/core/CCFlowNode.hpp"

#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_modbus_data.hpp"

namespace utility
{
class CCDataIdErrorHandle;
}

namespace driver
{

class CCCameraGetImageRosNode;

class CCCameraGetImageFixedFlowNode : public utility::CCFlowNode
{
public:
    CCCameraGetImageFixedFlowNode(const std::string &, const std::string &);
    ~CCCameraGetImageFixedFlowNode();
    virtual int start() override;
    virtual void stop() override;
    virtual void run() override;
    virtual int createOutput(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam) override;
    virtual int parseJsonValue(const ordered_json &, const ordered_json &) override;
    virtual int updateInput() override;

    virtual utility::CCFlowNodeType type() override;
    static utility::CCFlowNodeType ms_type;

    virtual void registerDataIdMeta(void *) override;

    // 如果节点需要访问DataId,必须实现该错误操作接口
    virtual const utility::CCDataIdErrorHandle *getDataIdErrorObj() const;

protected:
    void onSubscribeImage(const ros_msg::msg::CCImage::SharedPtr);
    void onSubscribeCycleId(const ros_msg::msg::CCModbusData::SharedPtr);
    void onSubscribeImageId(const ros_msg::msg::CCModbusData::SharedPtr);

protected:
    ros_msg::msg::CCGetImage::SharedPtr m_pGetImage;
    std::map<int64_t, ros_msg::msg::CCCameraParam> m_cameraParam;

    // 如果节点需要访问DataId,必须创建错误操作句柄对象
    utility::CCDataIdErrorHandle    *m_pDataIdErrorHandle{nullptr};
};


}

#endif // CCCAMERAGETIMAGEFIXEDFLOWNODE_H
