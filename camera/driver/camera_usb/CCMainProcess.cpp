/**
 * usb相机驱动主程序
 *
 */
#include "CCMainProcess.h"

#include <rclcpp/rclcpp.hpp>

#include "CCUsbCameraRosNode.h"
#include "CCCameraIO.h"

#include "node/camera/CCCameraTopicName.hpp"
#include "utility/core/CCTime.hpp"
#include "ros_msg/msg/cc_memory_type.hpp"

using std::placeholders::_1;

CCMainProcess::CCMainProcess(const std::string &mainType, const std::string &subType,
                         const std::string &objId, int index)
    : m_objId(objId)
    , m_camera(std::make_shared<CCCameraIO>(index))
{
    m_camera->start();
    m_image = std::make_shared<CCImage>();

    utility::CCDeviceType type;
    type.mainType = mainType;
    type.subType = subType;

    std::string nodeName = utility::ccRosNodeName(type, objId);
    auto keyName = "sharedmemory_" + nodeName;
    m_sharedMemory = shared_memory_object(
        open_or_create, keyName.data(), read_write);
    m_image->set__sharedmem_key(keyName);
    m_sharedMemory.truncate(5120*5120*3);
    m_mappedRegion = mapped_region(m_sharedMemory, read_write);

    nodeName = "driver_" + nodeName;
    m_node = std::make_shared<CCUsbCameraRosNode>(nodeName);

    std::string topicName = driver::ccRosTopicNameImageData(type, objId);
    m_node->createPublishImage(topicName, 1024);
    topicName = driver::ccRosTopicNameGetImage(type, objId);
    m_node->createSubscribeGetImage(topicName,
                            std::bind(&CCMainProcess::onSubscribeGetImage, this, _1), 1024);

    topicName = driver::ccRosTopicNameParamData(type, objId);
    m_node->createPublishParam(topicName, 1024);
    topicName = driver::ccRosTopicNameSetParam(type, objId);
    m_node->createSubscribeSetParam(topicName,
                                    std::bind(&CCMainProcess::onSubscribeSetParam, this, _1), 1024);
}

void CCMainProcess::run()
{
    rclcpp::spin(m_node);
}

std::shared_ptr<rclcpp::Node> CCMainProcess::rosNode()
{
    return m_node;
}

void CCMainProcess::onSubscribeGetImage(const CCGetImage::SharedPtr getImage)
{
    // 相机取图
    m_camera->getFrame(m_frame);

    m_image->header.set__time(utility::ccNow());
    m_image->header.set__device_id(m_objId);
    m_image->set__id(getImage->id);
    m_image->set__memory_type(getImage->memory_type);

    m_image->img_width = m_frame.cols;
    m_image->img_height = m_frame.rows;
    m_image->img_channel = m_frame.channels();
    m_image->img_format = m_frame.type();

    const int Size = m_image->img_width * m_image->img_height * m_image->img_channel;

    if (m_image->memory_type == CCMemoryType::MEM_DATA) {
        if (m_image->data.size() != Size) {
            m_image->data.resize(Size);
        }
        memcpy(m_image->data.data(), m_frame.data, Size);
    }
    else if (m_image->memory_type == CCMemoryType::MEM_SHARE) {
        m_image->data.clear();
        memcpy(m_mappedRegion.get_address(), m_frame.data, Size);
    }
    else {
        if (m_image->data.size() != Size) {
            m_image->data.resize(Size);
        }
        memcpy(m_image->data.data(), m_frame.data, Size);
        memcpy(m_mappedRegion.get_address(), m_frame.data, Size);
    }

    // 发布图片
    m_node->publishImage(m_image);
}

void CCMainProcess::onSubscribeSetParam(const CCCameraParam::SharedPtr)
{
    // 设置参数
    CCCameraParam::SharedPtr paramBack = std::make_shared<CCCameraParam>();

    // 返回参数
    m_node->publishParam(paramBack);
}


