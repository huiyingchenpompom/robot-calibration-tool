/**
 * 图片保存，启动线程保存采集到的图片到文件
 *
 */
#ifndef CCIMAGESAVED_H
#define CCIMAGESAVED_H

#include "utility/core/CCThreadList.hpp"
#include "ros_msg/msg/cc_image.hpp"

#include <thread>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <opencv2/opencv.hpp>

using namespace ros_msg::msg;
using namespace utility;
using namespace boost::interprocess;

class CCImageSaved
{
public:
    CCImageSaved();
    ~CCImageSaved();

    void addImage(const CCImage::SharedPtr &);
    size_t rawSize() const;
    void stopSave();

protected:
    void onSaveImage();

private:
    utility::CCThreadList<CCImage::SharedPtr> m_images;
    utility::CCThreadList<CCImage::SharedPtr> m_imagesRaw;
    std::unique_ptr<std::thread> m_thread;
    bool m_flagSave;

    shared_memory_object m_sharedMemory;
    mapped_region m_mappedRegion;
};

#endif // CCIMAGESAVED_H
