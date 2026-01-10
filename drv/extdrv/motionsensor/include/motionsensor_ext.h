/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description : motionsensor_ext.h
 * Author : ISP SW
 * Create : 2018-7-30
 * Version : Initial Draft
 */

#ifndef __MOTIONSENSOR_EXT_H__
#define __MOTIONSENSOR_EXT_H__

#include "hi_comm_motionsensor.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef struct {
    hi_s32  (*pfn_get_config_from_chip) (hi_msensor_param *param);
    hi_s32  (*pfn_write_data_to_buf) (hi_void);
} msensor_mng_callback;

typedef hi_s32 fn_msensor_mng_add_user(hi_s32 *id);
typedef hi_s32 fn_msensor_mng_delete_user(hi_s32 *id);
typedef hi_s32 fn_msensor_mng_get_data(hi_void *data);
typedef hi_s32 fn_msensor_mng_release_data(hi_void *data);
typedef hi_s32 fn_msensor_mng_get_m_sensor_config(hi_msensor_param *param);

typedef hi_s32 fn_msensor_mng_write_data_to_buff(hi_msensor_data *chip_data);
typedef hi_s32 fn_msensor_mng_init(hi_msensor_attr *attr, hi_msensor_buf_attr *buf_attr, hi_msensor_config *config);

typedef hi_s32 fn_msensor_mng_deinit(hi_void);

typedef hi_s32 fn_msensor_mng_register_call_back(msensor_mng_callback *callback);
typedef hi_void fn_msensor_mng_un_register_call_back(hi_void);

typedef struct {
    /*call_back to dis/avs*/
    fn_msensor_mng_add_user              *pfn_add_msensor_user;
    fn_msensor_mng_delete_user           *pfn_delete_msensor_user;
    fn_msensor_mng_get_data              *pfn_get_data;
    fn_msensor_mng_release_data          *pfn_release_data;
    fn_msensor_mng_get_m_sensor_config   *pfn_get_msensor_config;

    /*call_back to motionsensor_chip*/
    fn_msensor_mng_write_data_to_buff    *pfn_chip_write_data_to_mng_buff;
    fn_msensor_mng_init                  *pfn_init;
    fn_msensor_mng_deinit                *pfn_deinit;

    /*register to motionsensor_chip*/
    fn_msensor_mng_register_call_back    *pfn_register_call_back;
    fn_msensor_mng_un_register_call_back *pfn_unregister_call_back;
} msensor_mng_export_func;

#define ckfn_msensor_mng_register_call_back()\
    (NULL != FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR)->pfn_register_call_back)
#define call_msensor_mng_register_call_back(m_sensor_mng_callback)\
    FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR)->pfn_register_call_back(m_sensor_mng_callback)

#define ckfn_msensor_mng_unregister_call_back()\
    (NULL != FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR)->pfn_unregister_call_back)
#define call_msensor_mng_unregister_call_back(hi_void)\
    FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR)->pfn_unregister_call_back(hi_void)

#define HI_TRACE_MSENSOR(level, fmt, ...)\
    do{ \
        HI_TRACE(level, HI_ID_MOTIONSENSOR,"[func]:%s [line]:%d [info]:"fmt,__FUNCTION__, __LINE__,##__VA_ARGS__);\
    }while(0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* end of #ifdef __cplusplus */

#endif

