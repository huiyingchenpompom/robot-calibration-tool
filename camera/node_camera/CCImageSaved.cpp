/**
 * 图片保存，启动线程保存采集到的图片到文件
 *
 */
#include "CCImageSaved.h"

#include <boost/format.hpp>
#include "utility/CCUtilityDefine.h"
#include "utility/core/CCTime.hpp"
#include "ros_msg/msg/cc_memory_type.hpp"

using namespace utility;

CCImageSaved::CCImageSaved()
    : m_flagSave(true)
{
    for (int i = 0; i < 4; ++i) {
        m_imagesRaw.pushBack(std::make_shared<CCImage>());
    }
    m_thread.reset(new std::thread(&CCImageSaved::onSaveImage, this));
}

CCImageSaved::~CCImageSaved()
{
}

void CCImageSaved::addImage(const CCImage::SharedPtr &image)
{
    auto imgData = m_imagesRaw.front();
    m_imagesRaw.popFront();
    imgData->set__header(image->header);
    imgData->set__id(image->id);
    imgData->width = image->width;
    imgData->height = image->height;
    imgData->channel = image->channel;
    imgData->type = image->type;

    const size_t Size = image->width * image->height * image->channel;
    if (imgData->data.size() < Size) {
        imgData->data.resize(Size);
    }
    if (image->memory_type == ros_msg::msg::CCMemoryType::MEM_DATA) {
        memcpy(imgData->data.data(), image->data.data(), Size);
    }
    else {
        const char *key_name = m_sharedMemory.get_name();
        if (key_name == nullptr || key_name != image->sharedmem_key) {
            m_sharedMemory = shared_memory_object(
                open_only, image->sharedmem_key.data(), read_only);
            m_mappedRegion = mapped_region(m_sharedMemory, read_only);
        }
        memcpy(imgData->data.data(), m_mappedRegion.get_address(), Size);
    }
    m_images.pushBack(imgData);
    return;
}

size_t CCImageSaved::rawSize() const
{
    return m_imagesRaw.size();
}

void CCImageSaved::stopSave()
{
    m_flagSave = false;
    m_thread->join();
}

void CCImageSaved::onSaveImage()
{
    while (m_flagSave) {
        if (m_images.size() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        const auto image = m_images.front();
        m_images.popFront();

        cv::Mat cvImage = cv::Mat(image->height, image->width,
                                  image->channel == 3 ? CV_8UC3 : CV_8UC1, image->data.data());

        boost::format fmtFileName("%simage/%s/%d_%d_%d_%d.bmp");
        fmtFileName % DATA_PATH % PROJECT_NAME
            % image->id.workpiece_id
            % image->id.position_id
            % image->id.camera_id
            % utility::ccEpochMsToString(image->id.epoch_ms, true).data();
//        cv::imwrite(fmtFileName.str(), cvImage);

        m_imagesRaw.pushBack(image);
    }
}
