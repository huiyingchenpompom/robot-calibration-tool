/**
 * usb相机采集图像
 *
 */
#include "CCSimulatorCameraCameraIO.h"

#include <opencv2/opencv.hpp>
#include "utility/core/CCFileSystem.hpp"
#include "LogClientCommon.h"

CCSimulatorCameraCameraIO::CCSimulatorCameraCameraIO(int index, const std::string &imgPath)
    : m_imgIndex(index)
{
    CCDebug("CCSimulatorCameraCameraIO enter.");
    read(imgPath);
    CCDebug("CCSimulatorCameraCameraIO exit.");
}

void CCSimulatorCameraCameraIO::getFrame(cv::Mat &frame)
{
    CCDebug("getFrame enter: %d", m_imgIndex);
    frame = m_images[m_imgIndex];
    ++ m_imgIndex %= m_images.size();
    CCDebug("getFrame exit.");
}

void CCSimulatorCameraCameraIO::read(const std::string &imgPath)
{
    CCDebug("read enter: %s", imgPath.c_str());
    const auto &imageFiles = utility::ccFilesOfPath(imgPath);
    if (imageFiles.empty()) {
        m_images.resize(3);
        m_images[0] = cv::Mat(5120, 4368, CV_8UC3, cv::Scalar(200, 0, 0));
        m_images[1] = cv::Mat(5120, 4368, CV_8UC3, cv::Scalar(0, 200, 0));
        m_images[2] = cv::Mat(5120, 4368, CV_8UC3, cv::Scalar(0, 0, 200));
    }
    else {
        int index = 0;
        m_images.resize(imageFiles.size());
        for (const auto &fileName : imageFiles) {
            auto mat = cv::imread(fileName, cv::IMREAD_ANYCOLOR);
            m_images[index ++] = mat;
        }
    }
    m_imgIndex = 0;
    CCDebug("read exit.");
}
