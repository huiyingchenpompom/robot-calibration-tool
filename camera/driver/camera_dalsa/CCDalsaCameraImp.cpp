#include "CCDalsaCameraImp.h"
#include "LogClientCommon.h"
#include <filesystem>
#include <fstream>
#include <sstream>

CCDalsaCameraImp::CCDalsaCameraImp(int cameraId, const std::string &cameraInfo)
    : m_sapAcquistion(nullptr),
      m_buffers(nullptr),
      m_sapTransfer(nullptr)
{
    m_grabEnabled               = false;
    m_opened                    = false;
    m_isColor                   = false;
    initDalsaDevInfo(cameraInfo, cameraId, 1, 1);
}

CCDalsaCameraImp::~CCDalsaCameraImp()
{
    destroyObjects();
}

bool CCDalsaCameraImp::init()
{
    if(m_opened)
    {
        close();
    }
    m_opened = false;

    //配置相机ccf文件
    std::string configFile;
    if (m_profileProductId <= 0)
    {
        CCError(u8"初始化Dalsa相机, 需要相机产品码, 当前的相机产品码是: %d", "", m_profileProductId);
        return false;
    }
    else
    {
        std::ostringstream oss;
        oss << "/conf/dalsa/camera_" << std::setfill('0') << std::setw(2) << m_cameraId
            << "/profile_product_" << std::setfill('0') << std::setw(2) << m_profileProductId
            << ".ccf";
        configFile = oss.str();
        CCInfo(u8"初始化Dalsa相机, 使用的配置文件:%s", configFile.c_str());
    }

    std::string path = std::filesystem::current_path().string();
    configFile = path + configFile;
    if(!parseConfigFile(configFile, m_isColor, m_cameraPortId))
    {
        return false;
    }

    char *serverName = m_frameGrabberName.data();

    char *filePath = configFile.data();

    CCInfo(u8"当前的采集卡 :%s, 配置文件 :%s, 设备号 :%d", serverName, configFile.c_str(), m_cameraPortId);
    SapLocation loc(serverName, m_cameraPortId);
    m_sapAcquistion = new SapAcquisition(loc, filePath, filePath);
    m_buffers       = new SapBufferWithTrash(2, m_sapAcquistion);
    m_sapTransfer   = new SapAcqToBuf(m_sapAcquistion, m_buffers, photoReceivedCallBack, this);
    //Q_ASSERT(m_sapAcquistion && m_buffers && m_sapTransfer);

    if (createObjects())
    {
        m_opened = true;
        bool started = start();
        CCInfo(u8"初始化结果: %d", started);
        return started;
    }
    else
    {
        CCInfo(u8"初始化失败");
        return false;
    }
}

bool CCDalsaCameraImp::createObjects()
{
    if (m_sapAcquistion && !*m_sapAcquistion && !m_sapAcquistion->Create())
    {
        CCWarn(u8"创建SapAcquistion对象失败");
        destroyObjects();
        return false;
    }

    if (m_buffers && !*m_buffers)
    {
        if( !m_buffers->Create())
        {
            CCWarn(u8"Create SapBuffer object failed");
            destroyObjects();
            return false;
        }
        // Clear all buffers
        m_buffers->Clear();
    }
    if (m_sapTransfer && !*m_sapTransfer && !m_sapTransfer->Create())
    {
        CCWarn(u8"Create SapTransfer object failed");
        destroyObjects();
        return false;
    }

    return true;
}
bool CCDalsaCameraImp::destroyObjects()
{
    if (m_sapTransfer && *m_sapTransfer)
        m_sapTransfer->Destroy();

    if (m_buffers && *m_buffers)
        m_buffers->Destroy();

    if (m_sapAcquistion && *m_sapAcquistion)
        m_sapAcquistion->Destroy();

    if (m_sapTransfer)
    {
        delete m_sapTransfer;
        m_sapTransfer = nullptr;
    }

    if (m_buffers)
    {
        delete m_buffers;
        m_buffers = nullptr;
    }

    if (m_sapAcquistion)
    {
        delete m_sapAcquistion;
        m_sapAcquistion = nullptr;
    }

    return true;
}

bool CCDalsaCameraImp::parseConfigFile(const std::string &filePath, bool &isColor, int &index)
{
    //解释ccf文件
    std::ifstream file(filePath);
    if(!file.good())
    {
        CCWarn(u8"Initilizing dalsa camera, config file not exist, config file:%s", filePath.c_str());
        return false;
    }

    if (!file.is_open())
    {
        CCWarn(u8"Initilizing dalsa camera, config file can not open, config fiele: %s", filePath.c_str());
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string line;
    for (int i = 0; i < 4; i++)
    {
        //逐行获取内容
        std::getline(buffer, line);
        if (2 == i)
        {
            //第三行内容判断黑白还是彩色
            if (line.find("Mono") != std::string::npos)
            {
                isColor = false;
            }
            else
            {
                isColor = true;
            }
        }
    }
    //根据ccf内容第四行，获取当前采集模式的索引号
    index = line.back() - '0';
    file.close();

    return true;
}

bool CCDalsaCameraImp::close()
{
    if (!m_opened)
    {
        m_grabEnabled = false;
        CCWarn(u8"DalsaCameraLinkCamera FrameGrabber name %s by Port %d should not close without opened", m_frameGrabberName.c_str(), m_cameraPortId);
        return false;
    }

    if(m_grabEnabled)
    {
        if(!stop())
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    destroyObjects();
    CCInfo(u8"DalsaCameraLinkCamera with FrameGrabber %s by Port %d closed", m_frameGrabberName.c_str(), m_cameraPortId);
    return true;
}

bool CCDalsaCameraImp::start()
{
    if (!m_opened)
    {
        m_grabEnabled = false;
        CCWarn(u8"DalsaCameraLinkCamera FrameGrabber name %s by Port %d should can not close without opened", m_frameGrabberName.c_str(), m_cameraPortId);
        return false;
    }

    if (TRUE != m_sapTransfer->Grab())
    {
        CCWarn(u8"DalsaCameraLinkCamera with FrameGrabber %s by Port %d start grabbing failed", m_frameGrabberName.c_str(), m_cameraPortId);
        m_grabEnabled = false;
        return m_grabEnabled;
    }

    m_grabEnabled = true;
    CCInfo(u8"DalsaCameraLinkCamera with FrameGrabber %s by Port %d start grabbing successed", m_frameGrabberName.c_str(), m_cameraPortId);
    return m_grabEnabled;
}

bool CCDalsaCameraImp::stop()
{
    if (!m_opened)
    {
        m_grabEnabled = false;
        CCWarn(u8"Close failed, not DalsaCameraLink Camera");
        return false;
    }

    if (TRUE != m_sapTransfer->Freeze())
    {
        CCWarn(u8"DalsaCameraLinkCamera with FrameGrabber %s by Port %d stop grabbing failed", m_frameGrabberName.c_str(), m_cameraPortId);
        return false;
    }

    m_sapTransfer->Abort();
    m_grabEnabled = false;
    CCInfo(u8"DalsaCameraLinkCamera with FrameGrabber %s by Port %d stop grabbing successed", m_frameGrabberName.c_str(), m_cameraPortId);
    return true;
}

bool CCDalsaCameraImp::initDalsaDevInfo(const std::string frameGrabberName, int cameraId, int cameraPortId, int profileProductId)
{
    m_cameraId = cameraId;
    m_cameraPortId = cameraPortId;
    m_profileProductId = profileProductId;
    m_frameGrabberName = frameGrabberName;
    if (!enumDalsaServer())
    {
        return false;
    }

    bool result = false;
    for (uint32_t i = 0; i < m_cameraLinkFrameGrabberName.size(); i++)
    {
        if (m_cameraLinkFrameGrabberName[i] == m_frameGrabberName)
        {
            result = true;
            break;
        }
    }

    if(!result)
    {
        CCError(u8"采集卡匹配失败, 采集卡名称: %s", "", m_frameGrabberName.c_str());
        return false;
    }
    else
    {
        CCInfo(u8"采集卡匹配成功, 采集卡名称: %s", m_frameGrabberName.c_str());
    }

    return true;
}

bool CCDalsaCameraImp::setSoftwareTrigger()
{
    return true;
}

bool CCDalsaCameraImp::setHardwareTrigger()
{
    return true;
}

bool CCDalsaCameraImp::sendSoftTriggerCommand()
{
    return true;
}

bool CCDalsaCameraImp::enumDalsaServer()
{
    m_cameraLinkFrameGrabberName.clear();

    //获取板卡数量
    int frameGrabberCount = SapManager::GetServerCount(SapManager::ResourceAcq);
    if (frameGrabberCount <= 0)
    {
        CCError(u8"找到Dalsa CameraLink数量为%d，请先将Dalsa CameraLink相机和算力机同时断电30秒以上，然后再上电", "", frameGrabberCount);
        return false;
    }

    CCInfo(u8"Dalsa Cameralink 相机采集卡数量: %d", frameGrabberCount);

    for (int ii = 0; ii < frameGrabberCount; ++ii)
    {
        char frameGrabberName[CORSERVER_MAX_STRLEN] = {0};
        bool ret = SapManager::GetServerName(ii, SapManager::ResourceAcq, frameGrabberName);
        if (!ret)
        {
            CCError(u8"检测Dalsa相机采集卡服务下标%d失败, 错误码: %d", "", ii, ret);
            return false;
        }
        else
        {
            CCInfo(u8"检测到Dalsa相机采集卡名称:%s, 下标:%d", frameGrabberName, ii);
            m_cameraLinkFrameGrabberName.push_back(std::string(frameGrabberName));
        }
    }

    if (m_cameraLinkFrameGrabberName.size() <= 0)
    {
        CCError(u8"找不到有效的Dalsa相机采集卡", "");
        return false;
    }
    else
    {
        CCInfo(u8"扫描到有效Dalsa CameraLink相机数量: %d", m_cameraLinkFrameGrabberName.size());
        return true;
    }
}

void CCDalsaCameraImp::photoReceivedCallBack(SapXferCallbackInfo *pInfo)
{
    CCDalsaCameraImp *self = static_cast<CCDalsaCameraImp *>(pInfo->GetContext());
    CCInfo(u8"采集卡%s收到图片", self->m_frameGrabberName.c_str());
    uint8_t *data;
    if (TRUE != self->m_buffers->GetAddress(reinterpret_cast<void **>(&data)))
    {
        CCWarn(u8"Can not get buffer image pointer address");
        return;
    }

    int width = self->m_buffers->GetWidth();
    int height = self->m_buffers->GetHeight();

    CCInfo(u8"Received a photo, dalsa cameralink photo size:(%d, %d)", width, height);
    self->m_capturedFunc(data, width, height, self->m_isColor ? 3 : 1);
}
