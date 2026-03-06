#ifndef CCCAMERAGETIMAGE_H
#define CCCAMERAGETIMAGE_H

#include <map>
#include "utility/database/include/CCDbClientInterface.h"
#include "utility/cc_log_client/include/LogClientCommon.h"

#include "ros_msg/msg/cc_camera_param.hpp"

using CameraParamMap = std::map<std::string, std::map<int64_t, ros_msg::msg::CCCameraParam>>;

inline CameraParamMap getCameraParamFromDb()
{
    CCDebug("get camera param from database enter" );

    CameraParamMap cameraParamMap;

    // 读取数据库
    CCDbClientInterface* db = CCDbClientInterface::instance();
    int rst;
    auto paramConfs = db->getConfig<Db::CCGetImageConf>("", rst);
    if( rst == RET_FAILED )
    {
        CCError( u8"从数据库获取取图配置失败", u8"1.请检查数据库功能是否正常 2.请检查sc_config_.config_base_get_image表数据是否为空" );
        throw std::runtime_error( u8"1.请检查数据库功能是否正常 2.请检查sc_config_.config_base_get_image表数据是否为空" );
    }

    // 附加参数
    auto paramExtraConfs = db->getConfig<Db::CCGetImageExtraConf>("", rst);
    if( rst == RET_FAILED )
    {
        CCError( u8"从数据库获取取图附加配置失败", u8"1.请检查数据库功能是否正常 2.请检查sc_config_.config_base_get_image_extra表数据是否为空" );
        throw std::runtime_error( u8"从数据库获取取图附加配置失败, 1.请检查数据库功能是否正常 2.请检查sc_config_.config_base_get_image_extra表数据是否为空" );
    }

    // 解析数据
    for(const auto &conf : paramConfs)
    {
        cameraParamMap.try_emplace(conf.m_productCode, std::map<int64_t, ros_msg::msg::CCCameraParam>());
        ros_msg::msg::CCCameraParam param;
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_EXPOSURE);
        param.values.push_back(conf.m_exposure);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_GAIN);
        param.values.push_back(conf.m_gain);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_GAMMA);
        param.values.push_back(conf.m_gamma);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_PREAMP_GAIN);
        param.values.push_back(conf.m_preampGain);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_LINERATE);
        param.values.push_back(conf.m_lineRate);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_OFFSETX);
        param.values.push_back(conf.m_offsetX);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_OFFSETY);
        param.values.push_back(conf.m_offsetY);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_WIDTH);
        param.values.push_back(conf.m_width);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_HEIGHT);
        param.values.push_back(conf.m_height);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_FOCUS);
        param.values.push_back(conf.m_focus);
        param.params.push_back(ros_msg::msg::CCCameraParam::PARAM_IRIS);
        param.values.push_back(conf.m_iris);

        // 附加参数
        for (const auto &extrConf : paramExtraConfs) {
            if (conf.m_productCode != extrConf.m_productCode ||
                conf.m_cameraImageId != extrConf.m_cameraImageId) {
                continue;
            }
            ros_msg::msg::CCCameraParamItem item;
            item.set__feature_name(extrConf.m_featureName);
            if (extrConf.m_valueType == "int") {
                item.set__value_type(ros_msg::msg::CCCameraParamItem::TYPE_INT);
                item.set__value_int(extrConf.m_valueInt);
            } else if (extrConf.m_valueType == "float") {
                item.set__value_type(ros_msg::msg::CCCameraParamItem::TYPE_FLOAT);
                item.set__value_float(extrConf.m_valueFloat);
            } else if (extrConf.m_valueType == "enum") {
                item.set__value_type(ros_msg::msg::CCCameraParamItem::TYPE_ENUM);
                if (extrConf.m_valueStr.empty()) {
                    item.set__value_int(extrConf.m_valueInt);
                } else {
                    item.set__value_str(extrConf.m_valueStr);
                }
            } else if (extrConf.m_valueType == "bool") {
                item.set__value_type(ros_msg::msg::CCCameraParamItem::TYPE_BOOL);
                item.set__value_int(extrConf.m_valueInt);
            } else if (extrConf.m_valueType == "string") {
                item.set__value_type(ros_msg::msg::CCCameraParamItem::TYPE_STRING);
                item.set__value_str(extrConf.m_valueStr);
            }
            param.item.push_back(item);
        }

        cameraParamMap[conf.m_productCode][conf.m_cameraImageId] = param;
    }

    CCDebug("get camera param from database exit");

    return cameraParamMap;
}

#endif // CCCAMERAGETIMAGE_H
