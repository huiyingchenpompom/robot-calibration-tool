/**
 * OPT相机驱动主程序
 *
 */
#ifndef CCOPTMAINPROCESS_H
#define CCOPTMAINPROCESS_H

#include <mutex>
#include <memory>

#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_image.hpp"

class CCOptCameraRosNode;
class CCOptCameraImp;

namespace utility
{
class CCMessageNode;
}
using namespace ros_msg::msg;

class CCOptMainProcess
{
public:
    CCOptMainProcess(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType);
    bool openCamera(int triggerType);
    void run();
    std::shared_ptr<utility::CCMessageNode> rosNode();
    std::shared_ptr<CCOptCameraImp> cameraIO();

protected:
    void setCameraParam(const CCCameraParam &param);
    void onSubscribeGetImage( const CCGetImage::SharedPtr );
    void onSubscribeSetParam( const CCCameraParam::SharedPtr );
    void onImageCaptured(void *buffer, int width, int height, int channel);

private:
    const std::string                       m_objId;
    int                                     m_cameraId;
    int32_t                                 m_imageIndex{0};
    CCImage::SharedPtr                      m_image;
    std::mutex                              m_mutex;

    std::shared_ptr<CCOptCameraRosNode> 	m_node;
    std::shared_ptr<CCOptCameraImp>         m_camera;
    CCGetImage::SharedPtr                   m_getImage;
    float                                   m_cameraParam[CCCameraParam::PARAM_COUNT];
    std::vector<ros_msg::msg::CCCameraParamItem> m_extraCameraParam;
};

#endif // CCOPTMAINPROCESS_H
