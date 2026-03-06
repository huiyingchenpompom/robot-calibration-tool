/**
 * usb相机采集图像
 *
 */
#ifndef CCCAMERAIO_H
#define CCCAMERAIO_H

#include <memory>
#include <thread>
#include <mutex>

#include <opencv2/opencv.hpp>

#include "ros_msg/msg/cc_image.hpp"

class CCCameraIO
{
public:
    explicit CCCameraIO(int);
    explicit CCCameraIO(const std::string &);

    void setFrameRate(int rate) {m_frameRate = rate;}
    void setFrameSize(int width, int height) {
        m_frameWidth = width;
        m_frameHeight = height;
    }
    void start();
    void stop();

    void setParam(int, double);
    double getParam(int);

    void getFrame(cv::Mat &);

protected:
    void read();

private:
    int m_devIndex{-1};      // 摄像头设备序号
    std::string m_devName;   // 摄像头设备名称
    int m_frameRate{8};      // 摄像头帧率
    int m_frameWidth{640};   //
    int m_frameHeight{480};  //
    std::unique_ptr<std::thread> m_readThread;

    bool m_flagRun{false};
    std::atomic_bool m_flagRead;
    cv::VideoCapture m_capture;
    cv::Mat m_frame;

    std::mutex m_mutex;
};

#endif // CCMICAMERAIO_H
