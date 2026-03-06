/**
 * Basler网络相机驱动主程序
 *
 */
#ifndef CCORBBECMAINPROCESS_H
#define CCORBBECMAINPROCESS_H

#include <mutex>
#include <memory>

#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_driver_switch.hpp"
#include "ros_msg/msg/cc_image.hpp"

class CCOrbbecCameraRosNode;
class CCOrbbecCameraImp;

namespace utility
{
class CCMessageNode;
}
using namespace ros_msg::msg;

class CCOrbbecMainProcess
{
public:
    CCOrbbecMainProcess(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType);
    bool openCamera(int triggerType);
    void run();
    std::shared_ptr<utility::CCMessageNode> rosNode();
    std::shared_ptr<CCOrbbecCameraImp> cameraIO();

protected:
    void setCameraParam(const CCCameraParam &param);
    void onSubscribeGetImage( const CCGetImage::SharedPtr );
    void onSubscribeSetParam( const CCCameraParam::SharedPtr );
    void onSubscribeCameraSwitch( const CCDriverSwitch::SharedPtr );
    void onImageCaptured(void *buffer, int width, int height, int channel);

private:
    const std::string                       m_objId;
    int                                     m_cameraId;
    int32_t                                 m_imageIndex{0};
    CCImage::SharedPtr                      m_image;
    std::mutex                              m_mutex;

    std::shared_ptr<CCOrbbecCameraRosNode> 	m_node;
    std::shared_ptr<CCOrbbecCameraImp>         m_camera;
    CCGetImage::SharedPtr                   m_getImage;
    float                                   m_cameraParam[CCCameraParam::PARAM_COUNT];
    std::vector<ros_msg::msg::CCCameraParamItem> m_extraCameraParam;
};

#endif // CCORBBECMAINPROCESS_H
