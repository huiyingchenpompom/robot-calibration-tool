/**
 * 流程取图节点
 *
 */
#ifndef CCCAMERAGETIMAGEFASTFLOWNODE_H
#define CCCAMERAGETIMAGEFASTFLOWNODE_H

#include "utility/core/CCFlowNode.hpp"

#include "ros_msg/msg/cc_image.hpp"
#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_modbus_data.hpp"
#include "ros_msg/msg/cc_string.hpp"
#include <opencv2/opencv.hpp>

namespace utility
{
class CCDataIdErrorHandle;
}

namespace driver
{

class CCCameraGetImageRosNode;

class CCCameraGetImageFastFlowNode : public utility::CCFlowNode
{
public:
    CCCameraGetImageFastFlowNode(const std::string &, const std::string &);
    ~CCCameraGetImageFastFlowNode();
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
    void onSubscribeParam(const ros_msg::msg::CCCameraParam::SharedPtr);
    void onSubscribeImage(const ros_msg::msg::CCImage::SharedPtr);
    void onSubscribeCycleId(const ros_msg::msg::CCModbusData::SharedPtr);
    void onSubscribeImageId(const ros_msg::msg::CCModbusData::SharedPtr);
    void onSubscribeImageCount(const ros_msg::msg::CCModbusData::SharedPtr);
    void onSubscribeProductCode(const ros_msg::msg::CCString::SharedPtr);

protected:
    int m_imageCount = 1;
    ros_msg::msg::CCGetImage::SharedPtr m_pGetImage;
    bool m_bDropImageNumberZero {};
    std::map<int, cv::Mat> m_replaceImageCache;
    ros_msg::msg::CCModbusData::SharedPtr m_numberImageCount;

    // 相机参数, 从flow_node.json读取
    std::map<int64_t, ros_msg::msg::CCCameraParam> m_cameraParamFromJson;

    // 相机参数表, 从数据库读取<product_code, <camera_image_id, param>>
    std::map<std::string, std::map<int64_t, ros_msg::msg::CCCameraParam>> m_cameraParamFromDb;
    std::string m_productCode{""};

    // 如果节点需要访问DataId,必须创建错误操作句柄对象
    utility::CCDataIdErrorHandle    *m_pDataIdErrorHandle{nullptr};
};


}

#endif // CCCAMERAGETIMAGEFASTFLOWNODE_H
