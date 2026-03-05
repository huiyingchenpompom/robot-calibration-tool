/**
 * Daheng网络相机驱动主程序
 *
 */
#ifndef CCDAHENGMAINPROCESS_H
#define CCDAHENGMAINPROCESS_H

#include <mutex>
#include <memory>
#include "ros_msg/msg/cc_get_image.hpp"
#include "ros_msg/msg/cc_camera_param.hpp"
#include "ros_msg/msg/cc_image.hpp"
#include <nlohmann/json.hpp>
#include "node/lens/CCAbstractLens.h"

class CCDahengCameraRosNode;
class CCDahengCameraImp;

namespace utility
{
class CCMessageNode;
}
namespace boost {
namespace dll {
class shared_library;
}
namespace process {
class child;
}
}

namespace driver {
class CCAbstractLens;
}
using namespace ros_msg::msg;

class CCDahengMainProcess/* : public ICaptureEventHandler*/
{
public:
    CCDahengMainProcess(const std::string &, const std::string &, const std::string &, int, const std::string &, int transmitType, const nlohmann::json &json);
    ~CCDahengMainProcess();
    bool                          openCamera(int triggerType);
    void                          run();
    std::shared_ptr<utility::CCMessageNode> rosNode();
    std::shared_ptr<CCDahengCameraImp>   cameraIO();

protected:
    void setCameraParam(const CCCameraParam &param);
    void onSubscribeGetImage( const CCGetImage::SharedPtr );
    void onSubscribeSetParam( const CCCameraParam::SharedPtr);
    void onImageCaptured(void *buffer, int width, int height, int channel);
    bool closeLens();
    bool openLens();
    std::shared_ptr<driver::CCAbstractLens> loadLens(const std::string &fileName);
    bool initializeLens(const nlohmann::json &json);

private:
    const std::string                       m_objId;

    int32_t                                 m_imageIndex{0};
    int                                     m_cameraId;
    CCImage::SharedPtr                      m_image;
    std::mutex                              m_mutex;

    std::shared_ptr<CCDahengCameraRosNode> 	m_node;
    std::shared_ptr<CCDahengCameraImp>      m_camera;
    CCGetImage::SharedPtr                   m_getImage;
    float                                   m_cameraParam[CCCameraParam::PARAM_COUNT];
    std::vector<ros_msg::msg::CCCameraParamItem> m_extraCameraParam;

    // 镜头控制相关成员变量
    std::shared_ptr<driver::CCAbstractLens> m_lens;             // 镜头实例指针
    std::list<boost::dll::shared_library *> m_sharedLibs;       // 保存load dll，防止自动释放
    bool                                    m_isEnableLens;      //是否启用镜头
    bool                                    m_lensEnabled;    // 镜头启用状态
    std::string                             m_lensBrand;       // 镜头品牌
    std::string                             m_lensType;       // 镜头型号
    std::string                             m_lensId;         // 镜头识别码
    std::string                             m_interfaceType;         // 镜头识别码
    mutable std::mutex                      m_lensMutex;      // 镜头操作互斥锁
    nlohmann::json                          m_lensJson;
};

#endif // CCDAHENGMAINPROCESS_H
