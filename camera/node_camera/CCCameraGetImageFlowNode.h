/**
 * 流程取图节点
 *
 */
#ifndef CCCAMERAGETIMAGEFLOWNODE_H
#define CCCAMERAGETIMAGEFLOWNODE_H

#include "utility/core/CCFlowNode.hpp"

#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_modbus_data.hpp"
#include "ros_msg/msg/cc_string.hpp"
#include "ros_msg/msg/cc_string_array.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"

namespace utility
{
class CCDataIdErrorHandle;
}

namespace driver
{

class CCCameraGetImageRosNode;

class CCCameraGetImageFlowNode : public utility::CCFlowNode
{
    enum {
        INPUT_CYCLE,
        INPUT_IMAGE_ID,
        INPUT_COUNT
    };

public:
    CCCameraGetImageFlowNode(const std::string &, const std::string &);
    ~CCCameraGetImageFlowNode();
    virtual int start() override;
    virtual void stop() override;
    virtual void run() override;
    virtual int createOutput(const ordered_json &jsonDevice, const ordered_json &jsonNodeParam) override;
    virtual int parseJsonValue(const ordered_json &, const ordered_json &) override;
    virtual int updateInput() override;

    //extra setting
    int getDefaultExtraSetting(std::vector<utility::CCExtraSetting> &nodeExtraSetting) override;     //获取默认本地参数列表
    int setExtraSettingValue(const std::vector<utility::CCExtraSetting> &nodeExtraSetting) override; //设置本地参数列表数值

    virtual utility::CCFlowNodeType type() override;
    static utility::CCFlowNodeType ms_type;

    virtual void registerDataIdMeta(void *) override;

    // 如果节点需要访问DataId,必须实现该错误操作接口
    virtual const utility::CCDataIdErrorHandle *getDataIdErrorObj() const;

protected:
    void onSubscribeParam(const ros_msg::msg::CCCameraParam::SharedPtr param);
    void onSubscribeImage(const ros_msg::msg::CCImage::SharedPtr);
    void onSubscribeCycleId(const ros_msg::msg::CCModbusData::SharedPtr);
    void onSubscribeImageId(const ros_msg::msg::CCModbusData::SharedPtr);
    void onSubscribeProductCode(const ros_msg::msg::CCString::SharedPtr);
    void onSubscribeTrajectoryName(const ros_msg::msg::CCStringArray::SharedPtr data);

    void setCameraParam();
    int32_t updateImageIdFromTrajectory();

protected:
    int m_receivedImageCount = 0;
    ros_msg::msg::CCGetImage::SharedPtr m_pGetImage;
    std::array<std::string, INPUT_COUNT> m_inputNodeId;

    // 取图数量
    ros_msg::msg::CCModbusData::SharedPtr m_numberImageCount;

    // 取图类型
    int m_getImageType{1};   // 0: 飞拍or转拍; 1: 其它
    std::vector<std::string> m_trajectoryName; //轨迹名称
    bool m_flagTtrajSource{false};    // 轨迹文件路径标识
    std::string m_deviceId;           // 飞拍or转拍时设备id

    // 相机参数, 从flow_node.json读取
    std::vector<ros_msg::msg::CCCameraParam> m_cameraParamFromJson;

    // 相机参数表, 从数据库读取<product_code, <camera_image_id, param>>
    std::map<std::string, std::map<int64_t, ros_msg::msg::CCCameraParam>> m_cameraParamFromDb;
    std::string m_productCode{""};

    // 如果节点需要访问DataId,必须创建错误操作句柄对象
    utility::CCDataIdErrorHandle    *m_pDataIdErrorHandle{nullptr};
};


}

#endif // CCCAMERAGETIMAGEFLOWNODE_H
