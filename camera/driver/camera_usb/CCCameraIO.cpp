/**
 * usb相机采集图像
 *
 */
#include "CCCameraIO.h"

CCCameraIO::CCCameraIO(int idx)
    : m_devIndex(idx)
    , m_flagRun(false)
    , m_flagRead(false)
{
    m_capture.open(idx);
}

CCCameraIO::CCCameraIO(const std::string &name)
    : m_devName(name)
    , m_flagRun(false)
    , m_flagRead(false)
{
    m_capture.open(m_devName);
}

void CCCameraIO::start()
{
    m_flagRun = true;
    m_flagRead = false;
    m_readThread.reset(new std::thread(&CCCameraIO::read, this));
}

void CCCameraIO::stop()
{
    m_flagRun = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_readThread.release();
}

void CCCameraIO::getFrame(cv::Mat &frame)
{
    m_flagRead = true;
    while (m_flagRead) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::swap(frame, m_frame);
    }
}

void CCCameraIO::setParam(int param, double value)
{
    m_capture.set(param, value);
}

double CCCameraIO::getParam(int param)
{
    return m_capture.get(param);
}

void CCCameraIO::read()
{
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, m_frameWidth);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, m_frameHeight);
    m_capture.set(cv::CAP_PROP_AUTO_EXPOSURE, 0);
    m_capture.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    m_capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    while (m_flagRun) {
        if (m_flagRead) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_capture.read(m_frame);
            }
            if (m_frame.rows > 0 && m_frame.cols > 0) {
                m_flagRead = false;
            }
        }
        else {
            m_capture.grab();
        }
        std::this_thread::yield();
    }
}
