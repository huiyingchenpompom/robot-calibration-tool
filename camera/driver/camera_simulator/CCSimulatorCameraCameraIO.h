/**
 * usb相机采集图像
 *
 */
#ifndef CCSIMULATORCAMERACAMERAIO_H
#define CCSIMULATORCAMERACAMERAIO_H

#include <memory>
#include <thread>
#include <mutex>

#include <opencv2/opencv.hpp>

class CCSimulatorCameraCameraIO
{
public:
    explicit CCSimulatorCameraCameraIO(int, const std::string &);
    void getFrame(cv::Mat &);

protected:
    void read(const std::string &);

private:
    int m_imgIndex{0};
    std::vector<cv::Mat> m_images;
};

#endif // CCSIMULATORCAMERACAMERAIO_H
