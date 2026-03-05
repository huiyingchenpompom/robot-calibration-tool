/**
 * 流程取图节点
 *
 */
#include "CCCameraGetImageFlowNode.h"
#include "CCCameraGetImage.h"

#include <filesystem>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include "CCCameraGetImageRosNode.h"
#include "utility/core/CCUtilityDefine.h"
#include "utility/utility/CCTime.hpp"
#include "utility/core/device/CCDeviceManage.h"
#include "utility/utility/data/CCTrajectoryData.h"
#include "LogClientCommon.h"
#include "node/camera/CCCameraTopicName.hpp"
#include "utility/core/datetime/CCDateTime.h"
#include "utility/core/CCFileSystem.hpp"
#include "utility/core/CCToolkit.h"
#include "utility/core/event/CCEventCondVar.h"
#include "utility/ros_msg_shm/CCRosMsgShm.h"
#include "utility/core/setting/CCGlobalSettingManager.h"
#include "ros_msg_json_convert.hpp"

#include "cc_data_id_meta.hpp"
#include "ros_msg/msg/cc_memory_type.hpp"

using std::placeholders::_1;
using namespace std::chrono_literals;
using namespace driver;
using namespace utility;
using namespace ros_msg::msg;
using ordered_json = nlohmann::basic_json<nlohmann::ordered_map>;

CCFlowNodeType CCCameraGetImageFlowNode::ms_type = CCFlowNodeType{"camera_get_image", u8"取图"};

CCCameraGetImageFlowNode::CCCameraGetImageFlowNode(const std::string &id, const std::string &name)
    : utility::CCFlowNode(id, name)
{
    CCDebug("CCCameraGetImageFlowNode enter");
    m_pGetImage = std::make_shared<CCGetImage>();
    m_pGetImage->header.set__node_id(m_nodeId);
    auto productCodeSet = CCDeviceManage::instance().productCodeSet();

    if (!productCodeSet.empty()) {
        m_productCode = *(productCodeSet.begin());
    }

    m_transmitType = utility::driverTransmitType();
    m_numberImageCount = std::make_shared<CCModbusData>();
    m_numberImageCount->header.set__node_id(id);
    m_numberImageCount->item.resize(1);
    m_numberImageCount->item[0].set__data_type(CCModbusData::DATA_INT32);
    CCDebug("CCCameraGetImageFlowNode exit");
}

CCCameraGetImageFlowNode::~CCCameraGetImageFlowNode()
{
}

int CCCameraGetImageFlowNode::start()
{
    m_eventCv->reset();
    m_eventCvBack->reset();
    CC_INIT_CC_DATA_ID( m_pGetImage->id );
    CC_SET_CCDATAID_FIELD_VALUE( m_pGetImage->id.task_id, utility::taskId() );

    if ( ! m_productCode.empty() )
    {
        std::vector<uint8_t> code( m_productCode.begin(), m_productCode.end() );
        CC_SET_CCDATAID_FIELD_VALUE( m_pGetImage->id.product_code, code );
    }

    CCDebug( "Current task id=%d", utility::taskId() );
    return RET_SUCCESS;
}

void CCCameraGetImageFlowNode::stop()
{
    m_eventCv->wakeup( true );
    m_eventCvBack->wakeup( true );
}

void CCCameraGetImageFlowNode::run()
{
    CCDebug( "%s run publish get image enter.", m_nodeId.c_str() );

    // 判断是否收到所有输入
    m_eventCv->wait();
    CCDebug( "%s run publish get image input: %d", m_nodeId.c_str(), m_pGetImage->id.plc_cycle_id );

    // publish image count
    m_numberImageCount->header.set__time(utility::ccNow());
    m_numberImageCount->item[0].set__value_int32(static_cast<int32_t>(m_pGetImage->param.size()));
    if (!m_productCode.empty()) {
        m_numberImageCount->item.resize(1 + m_productCode.size());
        for (size_t i = 0; i < m_productCode.size(); ++i) {
            m_numberImageCount->item[i + 1].set__data_type(CCValueItem::DATA_INT8);
            m_numberImageCount->item[i + 1].set__value_int8(m_productCode[i]);
        }
    }
    auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>( m_rosNode );
    rosNode->publishImageCount( m_numberImageCount );

    // 设置camera param
    setCameraParam();

    // publish getImage(dataId)
    m_pGetImage->header.set__time(utility::ccNow());
    m_pGetImage->header.times.emplace_back(CCDateTime::now().toMSecsSinceEpoch());
    CC_SET_CCDATAID_FIELD_VALUE(m_pGetImage->id.task_id, utility::taskId());
    auto transmitType = utility::driverTransmitType();
    auto flagUseShmMem = m_flagUseShmMem && !(transmitType & TransmitCrossComputer);
    m_pGetImage->set__memory_type(
        flagUseShmMem ? CCMemoryType::MEM_SHARE : CCMemoryType::MEM_DATA);

    rosNode->publishGetImage(m_pGetImage);
    if (!m_pGetImage->param.empty()) {
        m_eventCv->sleep();
    }
    CCDebug("%s run publish get image exit: %d, %d", m_nodeId.c_str(), m_pGetImage->id.plc_cycle_id,
            m_numberImageCount->item[0].value_int32);
}

int CCCameraGetImageFlowNode::createOutput(const ordered_json &, const ordered_json &)
{
    const auto topicName = driver::ccRosTopicNameNodeImageData(ms_type.type, m_nodeId);
    CCFlowNodeOutputData data;
    data.topic = topicName;
    data.valueType = CCFlowNodeInOutValueType::ImageType;
    m_outputData.emplace_back(data);

    data.topic = ccRosTopicMerge( ms_type.type, m_nodeId ) + "/image_count";
    data.valueType = CCFlowNodeInOutValueType::ModbusNodeType + std::string(".image_count");
    m_outputData.emplace_back(data);

    return RET_SUCCESS;
}

int CCCameraGetImageFlowNode::parseJsonValue(const ordered_json &jsonDevice,
        const ordered_json &jsonNodeParam)
{
    CCInfo( "node(%s) parseJsonValue enter.", m_nodeId.c_str() );

    // parse json
    utility::CCDeviceType devType;
    utility::CCDeviceObject devObject;

    try
    {
        m_deviceId = jsonDevice.at( "id" );
        CCDeviceManage::instance().getDeviceObj( m_deviceId, devType, devObject );

        if ( devType.mainType.empty() )
        {
            CCError(u8"节点%s设备[%s]不存在", u8"请确定节点设备配置是否正确", m_nodeId.c_str(), m_deviceId.data());
            return RET_FAILED;
        }

        //获取标记
        m_flagTtrajSource = false;

        if ( jsonDevice.contains( "source_flag" ) )
        {
            m_flagTtrajSource = jsonDevice.at( "source_flag" );
        }

        if ( jsonNodeParam.contains( "trajectory_name" ) ) // 飞拍
        {
            m_getImageType = 0;
            const std::string trajectoryName = jsonNodeParam.at( "trajectory_name" );

            if ( ! trajectoryName.empty() )
            {
                m_trajectoryName.push_back( trajectoryName );
                int ret = this->updateImageIdFromTrajectory();

                if ( ret != RET_SUCCESS )
                {
                    return ret;
                }
            }
        }
        else
        {
            m_getImageType = 1;

            // param
            if ( jsonNodeParam.contains( "camera_param" ) )
            {
                const auto &jsonParam = jsonNodeParam.at( "camera_param" );

                for ( const auto &item : jsonParam )
                {
                    if ( item.empty() )
                    {
                        continue;
                    }

                    CCCameraParam param;

                    if ( item.contains( "exposure" ) )
                    {
                        param.params.push_back( CCCameraParam::PARAM_EXPOSURE );
                        param.values.push_back( item.at( "exposure" ) );
                    }

                    if ( item.contains( "gamma" ) )
                    {
                        param.params.push_back( CCCameraParam::PARAM_GAMMA );
                        param.values.push_back( item.at( "gamma" ) );
                    }

                    if ( item.contains( "gain" ) )
                    {
                        param.params.push_back( CCCameraParam::PARAM_GAIN );
                        param.values.push_back( item.at( "gain" ) );
                    }

                    if ( item.contains( "preamp_gain" ) )
                    {
                        param.params.push_back( CCCameraParam::PARAM_PREAMP_GAIN );
                        param.values.push_back( item.at( "preamp_gain" ) );
                    }

                    if ( item.contains( "line_rate" ) )
                    {
                        param.params.push_back( CCCameraParam::PARAM_LINERATE );
                        param.values.push_back( item.at( "line_rate" ) );
                    }

                    if ( item.contains( "focal_length_step" ) && ! item.at( "focal_length_step" ).is_null() )
                    {
                        param.params.push_back( CCCameraParam::PARAM_FOCUS );
                        param.values.push_back( item.at( "focal_length_step" ) );
                    }

                    if ( item.contains( "aperture_step" ) && ! item.at( "aperture_step" ).is_null() )
                    {
                        param.params.push_back( CCCameraParam::PARAM_IRIS );
                        param.values.push_back( item.at( "aperture_step" ) );
                    }

                    int validCount = std::count_if( param.values.cbegin(), param.values.cend(),
                                                    []( const auto value )
                    {
                        return value > 0;
                    } );

                    if ( validCount == param.values.size() )
                    {
                        m_cameraParamFromJson.emplace_back( param );
                    }
                }

                // 附加参数
                if ( jsonNodeParam.contains( "camera_param_feature" ) )
                {
                    std::string firstFeatureName;
                    size_t indexParam{0};
                    const auto &jsonCameraParams = jsonNodeParam.at( "camera_param_feature" );

                    for ( const auto &jsonParamItem : jsonCameraParams )
                    {
                        if ( jsonParamItem.empty() )
                        {
                            continue;
                        }

                        // parse
                        const std::string featureName = jsonParamItem.at( "feature_name" );

                        if ( featureName.empty() )
                        {
                            CCError( u8"%s参数解析失败: feature_name为空", u8"请检查对应节点参数配置",
                                     m_nodeId.c_str() );
                            return RET_FAILED;
                        }

                        uint8_t valueType = CCCameraParamItem::TYPE_INT;
                        const std::string valueTypeStr = jsonParamItem.at( "value_type" );

                        if ( valueTypeStr == "int" )
                        {
                            valueType = CCCameraParamItem::TYPE_INT;
                        }
                        else if ( valueTypeStr == "float" )
                        {
                            valueType = CCCameraParamItem::TYPE_FLOAT;
                        }
                        else if ( valueTypeStr == "enum" )
                        {
                            valueType = CCCameraParamItem::TYPE_ENUM;
                        }
                        else if ( valueTypeStr == "bool" )
                        {
                            valueType = CCCameraParamItem::TYPE_BOOL;
                        }
                        else if ( valueTypeStr == "string" )
                        {
                            valueType = CCCameraParamItem::TYPE_STRING;
                        }
                        else
                        {
                            CCError(u8"%s参数解析失败: value_type不支持类型%s", u8"目前仅支持int/float/enum/bool/string，请检查对应节点参数配置", m_nodeId.c_str(), valueTypeStr.c_str());
                            return RET_FAILED;
                        }

                        // assign
                        CCCameraParamItem item;
                        item.set__feature_name( featureName );
                        item.set__value_type( valueType );

                        if ( jsonParamItem.contains( "value_int" ) )
                        {
                            item.set__value_int( jsonParamItem.at( "value_int" ) );
                        }

                        if ( jsonParamItem.contains( "value_float" ) )
                        {
                            item.set__value_float( jsonParamItem.at( "value_float" ) );
                        }

                        if ( jsonParamItem.contains( "value_str" ) )
                        {
                            item.set__value_str( jsonParamItem.at( "value_str" ) );
                        }

                        m_cameraParamFromJson[ indexParam ].item.push_back( item );

                        // 确定附加参数对应的m_cameraParamFromJson位置
                        if ( firstFeatureName == featureName )
                        {
                            ++ indexParam;
                        }

                        if ( firstFeatureName.empty() )
                        {
                            firstFeatureName = featureName;
                        }
                    }
                }
            }

            // 如果flow_node.json未定义camera_param，从数据库读取
            if ( m_cameraParamFromJson.empty() )
            {
                m_cameraParamFromDb = getCameraParamFromDb();
            }

            // camera id
            if ( jsonNodeParam.contains( "image_id" ) )
            {
                const auto &jsonCameraId = jsonNodeParam.at( "image_id" );
                jsonCameraId.get_to( m_pGetImage->camera_image_id );
            }

            // delay time
            if ( jsonNodeParam.contains( "delay_time" ) )
            {
                const auto &jsonDelayTime = jsonNodeParam.at( "delay_time" );
                jsonDelayTime.get_to( m_pGetImage->delay_time );
            }
        }

        // trigger type
        if ( jsonNodeParam.contains( "trigger_type" ) )
        {
            const auto &jsonTriggerType = jsonNodeParam.at( "trigger_type" );
            jsonTriggerType.get_to( m_pGetImage->trigger_type );
        }
        else
        {
            m_pGetImage->trigger_type = -1;
        }
    }
    catch ( ordered_json::exception &e )
    {
        CCError( u8"流程节点%s参数解析失败: %s", u8"请检查对应节点参数配置",
                 this->id().data(), e.what() );
        return RET_FAILED;
    }

    // create node
    std::string nodeName = utility::ccRosNodeName(ms_type.type, m_nodeId);
//    nodeName = "node_" + nodeName + "_" + this->id() + "_image";
    std::shared_ptr<CCCameraGetImageRosNode> rosNode = std::make_shared<CCCameraGetImageRosNode>
        ( nodeName, m_transmitType );
    rosNode->setTopicTransmitType( m_topicTransmitType );
    m_rosNode = rosNode;

    // create publish getImage & subscribe getImageBack
    std::string topicName = driver::ccRosTopicNameGetImage( devType, devObject.deviceObjId );
    rosNode->createPublishGetImage( topicName, 1024 );
    topicName = driver::ccRosTopicNameDriverImageData( devType, devObject.deviceObjId );
    rosNode->createSubscribeGetImage(topicName,  m_nodeId, std::bind(&CCCameraGetImageFlowNode::onSubscribeImage, this, _1), 128);
    topicName = driver::ccRosTopicNameSetParamBack(devType, devObject.deviceObjId);
    rosNode->createSubscribeParam(topicName, m_nodeId, std::bind(&CCCameraGetImageFlowNode::onSubscribeParam, this, _1), 1024);

    for ( const auto &outputData : m_outputData )
    {
        topicName = outputData.topic;
        const auto valueType = outputData.valueType;

        if ( valueType == CCFlowNodeInOutValueType::ImageType )
        {
            rosNode->createPublishImage( topicName, 1024 );
        }
        else if ( valueType == CCFlowNodeInOutValueType::ModbusNodeType + std::string( ".image_count" ) )
        {
            rosNode->createPublishImageCount( topicName, 1024 );
        }
    }

    CCDebug( "node(%s) parseJsonValue exit.", m_nodeId.c_str() );
    return RET_SUCCESS;
}

int CCCameraGetImageFlowNode::updateInput()
{
    auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>( m_rosNode );

    for ( const auto &inputData : m_inputData )
    {
        CCDebug( "%s topic=%s, selfType=%s, preValueType=%s", id().c_str(), inputData.topic.c_str(),
                 inputData.selfValueType.c_str(), inputData.preValueType.c_str() );

        if ( ! checkInputData( {inputData.preValueType} ) )
        {
            return RET_FAILED;
        }

        const auto &topic = inputData.topic;
        auto pos = inputData.selfValueType.find( "." );
        const std::string valueType = inputData.selfValueType.substr( 0, pos );
        const std::string extraType = pos == std::string::npos ? "" : inputData.selfValueType.substr(
                pos + 1 );

        if ( valueType == CCFlowNodeInOutValueType::ModbusNodeType )
        {
            if ( extraType == "cycle" )
            {
                m_inputNodeId[ INPUT_CYCLE ] = inputData.nodeId;
                rosNode->createSubscribeCycleId(
                    topic,
                    std::bind(&CCCameraGetImageFlowNode::onSubscribeCycleId, this, _1 ), 1024 );
                m_eventCv->setCount( inputData.nodeId, 1 );
            }
            else if ( extraType == "image_id" )
            {
                m_inputNodeId[ INPUT_IMAGE_ID ] = inputData.nodeId;
                rosNode->createSubscribeImageId(
                    topic,
                    std::bind(&CCCameraGetImageFlowNode::onSubscribeImageId, this, _1 ), 1024 );
                m_eventCv->setCount( inputData.nodeId, 1 );
            }
            else
            {
                CCError( u8"流程节点%s配置不正确", u8"请检查参数input_data/self_type配置,value_array后仅支持cycle和image_id",
                         m_nodeId.c_str() );
                return RET_FAILED;
            }
        }
        else if ( valueType == CCFlowNodeInOutValueType::CCString )
        {

            if ( extraType == "product_code" )
            {
                rosNode->createSubscribeProductCode(
                    topic,
                    std::bind(&CCCameraGetImageFlowNode::onSubscribeProductCode, this, _1 ), 1024 );
                m_eventCv->setCount( inputData.nodeId, 1 );
                CCDebug( "%s subscribed on topic: %s", id().c_str(), topic.c_str() );
            }
            else
            {
                CCError( u8"流程节点%s配置不正确", u8"请检查参数input_data/self_type配置,string后仅支持product_code",
                         m_nodeId.c_str() );
                return RET_FAILED;
            }
        }
        else if ( valueType == CCFlowNodeInOutValueType::StringArray )
        {
            rosNode->createSubscribeTrajectoryName(
                topic, std::bind(&CCCameraGetImageFlowNode::onSubscribeTrajectoryName, this, _1 ), 1024 );
            m_eventCv->setCount( inputData.nodeId, 1 );
            CCDebug( "%s subscribed on topic: %s", id().c_str(), topic.c_str() );
        }
        else
        {
            CCError( u8"流程节点%s配置不正确", u8"请检查参数input_data/self_type配置，仅支持value_array/string/string_array类型",
                     m_nodeId.c_str() );
            return RET_FAILED;
        }
    }

    return RET_SUCCESS;
}

int CCCameraGetImageFlowNode::getDefaultExtraSetting( std::vector<utility::CCExtraSetting> &
        nodeExtraSetting )
{
    nodeExtraSetting.clear();
    utility::CCExtraSetting settingErrorImageCount;
    settingErrorImageCount.configurationKey = "flag_error_image_count";
    settingErrorImageCount.configurationName = "取图数量错误时是否弹出报错框";
    settingErrorImageCount.configurationType = ConfigurationValueType::Switch;
    settingErrorImageCount.configurationValue = "true";
    settingErrorImageCount.configurationOptions = "";
    settingErrorImageCount.configurationTips = "true: 不弹出错误框; false: 弹出错误框";
    settingErrorImageCount.configurationIndex = 1;
    settingErrorImageCount.permission = 0;
    settingErrorImageCount.helpLink = "";
    nodeExtraSetting.push_back( settingErrorImageCount );

    return utility::DefaultExtraSettingResult::Success;
}

int CCCameraGetImageFlowNode::setExtraSettingValue( const std::vector<utility::CCExtraSetting> &
        nodeExtraSetting )
{
    for ( const auto &setting : nodeExtraSetting )
    {
        if ( setting.configurationKey == "flag_error_image_count" )
        {
            m_pGetImage->set__error_image_count( setting.configurationValue == "true" ? 1 : 0 );
        }
    }

    return utility::DefaultExtraSettingResult::Success;
}

utility::CCFlowNodeType CCCameraGetImageFlowNode::type()
{
    return ms_type;
}

void CCCameraGetImageFlowNode::onSubscribeImage( const CCImage::SharedPtr image )
{
    CCRosMsgShm msgShm( image->shm_index, m_nodeId );

    if ( image->header.node_id == m_nodeId )
    {
        auto rosNode = std::dynamic_pointer_cast<CCCameraGetImageRosNode>( m_rosNode );
        bool replaceImage = utility::CCGlobalSettingManager::instance()->getBoolValue( "run_project",
                            "replace_image", false );

        if ( replaceImage )
        {
            auto stringToInt = []( const std::string & str, int & result )
            {
                try
                {
                    result = std::stoi( str );
                    return true;
                }
                catch ( const std::invalid_argument & )
                {
                    // 字符串不是有效的整数表示
                    return false;
                }
                catch ( const std::out_of_range & )
                {
                    // 转换结果超出了 int 范围
                    return false;
                }
            };

            int imageId = image->id.camera_image_id;
            std::string imagePath = utility::getProjectConfigPath() + "replace_image";
            const auto &imageFiles = utility::ccFilesOfPath( imagePath );
            bool findImage = false;

            for ( const auto& fileName : imageFiles )
            {
                std::filesystem::path filePath = fileName;
                const auto &imageIdStr = filePath.stem().string();
                CCInfo( u8"搜索到替换图号文件：%s", imageIdStr.c_str() );
                int imageId2;

                if ( stringToInt( imageIdStr, imageId2 ) && imageId2 == imageId )
                {
                    const auto &cvImage = cv::imread( fileName );

                    if ( cvImage.empty() )
                    {
                        CCWarn( u8"路径%s找到需要替换的图号：%d读取失败", u8"请检查图片",
                                imagePath.c_str(), imageId );
                        break;
                    }

                    CCInfo( u8"取图节点%s已替换文件%s", m_nodeId.c_str(), fileName.c_str() );
                    size_t size = cvImage.cols * cvImage.rows * cvImage.channels();

                    if ( image->memory_type == CCMemoryType::MEM_DATA )
                    {
                        if ( image->data_size < size )
                        {
                            image->data.resize( size );
                        }

                        memcpy( image->data.data(), cvImage.data, size );
                    }
                    else
                    {
                        if ( image->data_size < size )
                        {
                            CCInfo( u8"图片%d大小%d小于替换图大小%d,重新开辟内存后开始拷贝内存",
                                    imageId, image->data_size, size );
                            CCRosMsgShm msgShmNew;
                            auto index = msgShmNew.createShmIndexWithData( cvImage.data, size );
                            image->set__shm_index( index );

                            if ( ! msgShmNew.isValid( index ) )
                            {
                                CCError( u8"图片替换功能：替换图片与原图大小不一致，重新分配共享内存错误", u8"请检查当前系统各进程内存使用量，并确认节点（推理、前处理、环切、MARK图、存图）是否存在图片积压");
                                return;
                            }
                        }
                        else
                        {
                            CCInfo( u8"图片%d大小%d大于等于替换图大小%d,直接拷贝内存", imageId, size );
                            memcpy( msgShm.data( image->shm_index ), cvImage.data, size );
                        }

                        CCInfo( u8"图片%d大小%d拷贝内存完成", imageId, size );
                    }

                    image->set__img_width( cvImage.cols );
                    image->set__img_height( cvImage.rows );
                    image->set__img_channel( cvImage.channels() );
                    image->set__img_format( cvImage.channels() == 1 ? 0 : 16 );
                    image->set__data_size( size );
                    findImage = true;
                    break;
                }
            }

            if ( ! findImage )
            {
                CCWarn( u8"路径%s未找到需要替换的图号：%d", u8"请检查图片", imagePath.c_str(),
                        imageId );
            }
        }

        // 相机图片大小
        CC_SET_CCDATAID_FIELD_VALUE(image->id.camera_image_width, image->img_width);
        CC_SET_CCDATAID_FIELD_VALUE(image->id.camera_image_height, image->img_height);

        rosNode->publishImage(image);
        CCInfo(u8"取图节点%s收到图片, taskid:%d,周期号:%d,相机号：%d 图号：%d ",
               this->id().c_str(), image->id.task_id, image->id.plc_cycle_id, image->id.camera_id,
               image->id.camera_image_id);
        m_receivedImageCount++;
        CCDebug( "CCCameraGetImageFlowNode(%s) onSubscribeImage enter.", this->id().c_str() );

        if ( m_receivedImageCount == m_pGetImage->camera_image_id.size() )
        {
            m_receivedImageCount = 0;
            CCInfo( "CCCameraGetImageFlowNode(%s) received all %d images", this->id().c_str(),
                    m_receivedImageCount );
        }
    }
}

void CCCameraGetImageFlowNode::onSubscribeCycleId( const CCModbusData::SharedPtr data )
{
    CCDebug( "%s onSubscribeCycleId enter, modbus data node id : %s, data2Json : %s",
             m_nodeId.c_str(), data->header.node_id.c_str(), msg::ccModbusData2Json(*data ).dump().c_str() );

    if ( data->header.node_id != m_inputNodeId[ INPUT_CYCLE ] || data->item.size() == 0 )
    {
        return;
    }

    const auto &item = data->item[ 0 ];

    switch ( item.data_type )
    {
    case CCModbusData::DATA_INT16:
        CC_SET_CCDATAID_FIELD_VALUE( m_pGetImage->id.plc_cycle_id,
                                     static_cast<int32_t>( item.value_int16 ) );
        CCDebug( "%s receive cycle id: %d", this->id().c_str(),
                 CC_GET_DATA_ID_VALUE( m_pGetImage->id.plc_cycle_id ) );
        m_eventCv->wakeup( data->header.node_id );
        break;

    case CCModbusData::DATA_INT32:
    {
        CC_SET_CCDATAID_FIELD_VALUE( m_pGetImage->id.plc_cycle_id, item.value_int32 );
        CCDebug( "%s receive cycle id: %d", this->id().c_str(),
                 CC_GET_DATA_ID_VALUE( m_pGetImage->id.plc_cycle_id ) );
        m_eventCv->wakeup( data->header.node_id );
        break;
    }

    case CCModbusData::DATA_INT64:
    {
        CC_SET_CCDATAID_FIELD_VALUE( m_pGetImage->id.plc_cycle_id, item.value_int64 );
        CCDebug( "%s receive cycle id: %d", this->id().c_str(),
                 CC_GET_DATA_ID_VALUE( m_pGetImage->id.plc_cycle_id ) );
        m_eventCv->wakeup( data->header.node_id );
        break;
    }

    default:
        break;
    }

    if ( m_pGetImage->id.plc_cycle_id <= 0 )
    {
        CCError( u8"收到的周期号（%d）,不合符预期。", u8"周期号不能小于等于0，请检查流程中周期号输入逻辑", m_pGetImage->id.plc_cycle_id );
    }

    CCDebug( u8"%s 更新周期号 exit.", m_nodeId.c_str() );
}

void CCCameraGetImageFlowNode::onSubscribeImageId( const CCModbusData::SharedPtr data )
{
    CCDebug( "%s onSubscribeImageId enter.", m_nodeId.c_str() );

    if ( data->header.node_id != m_inputNodeId[ INPUT_IMAGE_ID ] || data->item.size() == 0 )
    {
        return;
    }

    m_pGetImage->camera_image_id.resize( data->item.size() );

    for ( uint16_t i = 0; i < data->item.size(); i++ )
    {
        const auto &item = data->item[ i ];

        switch ( item.data_type )
        {
        case CCModbusData::DATA_INT16:
            m_pGetImage->camera_image_id[ i ] = item.value_int16;
            CCDebug( "%s receive image id: %lld", m_nodeId.c_str(), m_pGetImage->camera_image_id[ i ] );
            m_eventCv->wakeup( data->header.node_id );
            break;

        case CCModbusData::DATA_INT32:
            m_pGetImage->camera_image_id[ i ] = item.value_int32;
            CCDebug( "%s receive image id: %lld", m_nodeId.c_str(), m_pGetImage->camera_image_id[ i ] );
            m_eventCv->wakeup( data->header.node_id );
            break;

        case CCModbusData::DATA_INT64:
            m_pGetImage->camera_image_id[ i ] = item.value_int64;
            CCDebug( "%s receive image id: %lld", m_nodeId.c_str(), m_pGetImage->camera_image_id[ i ] );
            m_eventCv->wakeup( data->header.node_id );
            break;

        default:
            break;
        }
    }

    CCDebug( u8"%s 更新图号 exit.", m_nodeId.c_str() );
}

void CCCameraGetImageFlowNode::onSubscribeProductCode( const ros_msg::msg::CCString::SharedPtr
        productCode )
{
    CCDebug( u8"%s 更新产品号: %s", m_nodeId.c_str(), productCode->msg.c_str() );
    m_productCode = productCode->msg;
    std::vector<uint8_t> code( m_productCode.begin(), m_productCode.end() );
    CC_SET_CCDATAID_FIELD_VALUE( m_pGetImage->id.product_code, code );
    m_eventCv->wakeup( productCode->header.node_id );
}

void driver::CCCameraGetImageFlowNode::onSubscribeTrajectoryName( const
        ros_msg::msg::CCStringArray::SharedPtr data )
{
    const auto& inputNodeId = data->header.node_id;
    auto node = std::find_if( m_inputData.begin(), m_inputData.end(),
                              [ &inputNodeId ]( const auto & inputData )
    {
        return inputNodeId == inputData.nodeId;
    } );

    if ( node != m_inputData.end() )
    {
        CCDebug( "%s receive trajectory name: %d, %s", m_nodeId.c_str(), data->str_array.size(),
                 data->str_array.front().c_str() );
        m_trajectoryName = data->str_array;
        this->updateImageIdFromTrajectory();
        m_eventCv->wakeup( data->header.node_id );
    }
}

/**
 * @brief 设置取图相机参数
 */
void CCCameraGetImageFlowNode::setCameraParam()
{
    // 飞拍转拍不设置
    if ( m_getImageType == 0 )
    {
        return;
    }

    // flow_node.json配置
    if ( ! m_cameraParamFromJson.empty() )
    {
        m_pGetImage->param.resize(m_cameraParamFromJson.size());
        for (int i = 0; i < m_cameraParamFromJson.size(); i++) {
            m_pGetImage->param[i] = m_cameraParamFromJson[i];
            m_pGetImage->param[i].header.set__node_id(m_nodeId);
        }
        return;
    }

    // 数据库配置
    if ( ! m_productCode.empty() && m_cameraParamFromDb.count( m_productCode ) != 0 )
    {
        auto &cameraParam = m_cameraParamFromDb[ m_productCode ];
        m_pGetImage->param.clear();

        for ( auto imageId : m_pGetImage->camera_image_id )
        {
            if ( cameraParam.count( imageId ) == 0 )
            {
                CCError( u8"产品%s图号%d相机参数未配置", u8"请检查相机参数数据库sc_config_.config_base_get_image配置", m_productCode.c_str(), imageId );
                ros_msg::msg::CCCameraParam param;
                param.header.set__node_id(m_nodeId);
                m_pGetImage->param.push_back(param);
            }
            else
            {
                std::string strParam = std::accumulate(
                                           cameraParam[ imageId ].values.begin(),
                                           cameraParam[ imageId ].values.end(),
                                           std::string(), []( const std::string & acc, double num )
                {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision( 3 ) << num;
                    return acc.empty() ? oss.str() : acc + ", " + oss.str();
                } );

                CCDebug( "%s %s %lld set camera param: %s", m_nodeId.c_str(), m_productCode.c_str(), imageId,
                         strParam.c_str() );
                cameraParam[ imageId ].header.set__node_id(m_nodeId);
                m_pGetImage->param.push_back( cameraParam[ imageId ] );
            }
        }
    }
}

int32_t CCCameraGetImageFlowNode::updateImageIdFromTrajectory()
{
    if (m_trajectoryName.empty()) {
        return RET_FAILED;
    }

    // 加载轨迹文件
    const auto &trajName = m_trajectoryName[0];
    auto pos = trajName.find_first_of('-');
    const std::string driveDeviceId = trajName.substr(0, pos);
    CCTrajectoryData trajData(m_deviceId);
    int ret = trajData.loadFromName(driveDeviceId, trajName, m_flagTtrajSource);
    if (ret != RET_SUCCESS) {
        return ret;
    }

    // 读取数据
    const auto cameraImageId = trajData.cameraImageId();
    const auto cameraParam = trajData.cameraParam();
    if (cameraImageId.empty() || cameraImageId.size() != cameraParam.size()) {
        CCError(u8"轨迹%s图号数量%d与相机参数数量%d定义不正确", u8"请检查对应轨迹文件，图号数量要大于0且与相机参数数量匹配", trajName.c_str(), cameraImageId.size(), cameraParam.size());
        return RET_FAILED;
    }

    // 赋值
    m_productCode = utility::ccProductCodeOfTrajectoryName(trajName);
    m_pGetImage->set__camera_image_id(cameraImageId);
    m_pGetImage->set__param(cameraParam);

    // 输出
    const auto msg = std::accumulate(cameraImageId.begin(), cameraImageId.end(),
                                     std::string(""),
                                     [](const std::string &text, const auto id) {
                                         return text + " " + std::to_string(id);
                                     });
    CCDebug("camera image id:%s ", msg.c_str());
    return RET_SUCCESS;
}

void CCCameraGetImageFlowNode::registerDataIdMeta( void * dataIdMeta )
{
    CC_DATA_ID_META_REGISTER( dataIdMeta );
    m_pDataIdErrorHandle = CC_DATA_ID_NODE_REGISTER( m_nodeId );
}

const utility::CCDataIdErrorHandle * CCCameraGetImageFlowNode::getDataIdErrorObj() const
{
    return m_pDataIdErrorHandle;
}

void CCCameraGetImageFlowNode::onSubscribeParam(const CCCameraParam::SharedPtr param)
{
    CCInfo(u8"取图节点%s收到%s参数修改返回...", m_nodeId.c_str(), param->header.node_id.c_str());
    if (param->header.node_id == m_nodeId) {
        m_eventCv->wakeup(true);
    }
}
