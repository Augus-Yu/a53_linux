/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description : hi_comm_motionsensor.h
 * Author : ISP SW
 * Create : 2018-7-30
 * Version : Initial Draft
 */

#ifndef _HI_COMM_MOTIONSENSOR_H__
#define _HI_COMM_MOTIONSENSOR_H__

#include "hi_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define MAX_DATA_NUM        128
#define GRADIENT            (0x1<<10)

#define MSENSOR_TEMP_GYRO   0x1
#define MSENSOR_TEMP_ACC    0x2
#define MSENSOR_TEMP_MAGN   0x4
#define MSENSOR_TEMP_ALL    0x7

#define MSENSOR_DEVICE_GYRO 0x1
#define MSENSOR_DEVICE_ACC  0x2
#define MSENSOR_DEVICE_MAGN 0x4
#define MSENSOR_DEVICE_ALL  0x7

typedef struct {
    hi_u32  device_mask;
    hi_u32  temperature_mask;
} hi_msensor_attr;

typedef struct {
    hi_u64 phy_addr;
    hi_u64 vir_addr;
    hi_u32 buflen;
} hi_msensor_buf_attr;

typedef struct {
    hi_s32 x;
    hi_s32 y;
    hi_s32 z;
    hi_s32 temp;
    hi_u64 pts;
} hi_msensor_sample_data;

typedef struct {
    hi_msensor_sample_data gyro_data[MAX_DATA_NUM];
    hi_u32                 data_num;
} hi_msensor_gyro_buffer;

typedef struct {
    hi_msensor_sample_data acc_data[MAX_DATA_NUM];
    hi_u32                 data_num;
} hi_msensor_acc_buffer;

typedef struct {
    hi_msensor_sample_data magn_data[MAX_DATA_NUM];
    hi_u32                 data_num;
} hi_msensor_magn_buffer;

typedef struct {
    hi_u64 odr;
    hi_u64 fsr;
    hi_u8  data_width;
    hi_s32 temp_max;
    hi_s32 temp_min;
} hi_msensor_gyro_config;

typedef struct {
    hi_u64 odr;
    hi_u64 fsr;
    hi_u8  data_width;
    hi_s32 temp_max;
    hi_s32 temp_min;
} hi_msensor_acc_config;

typedef struct {
    hi_u64 odr;
    hi_u64 fsr;
    hi_u8  data_width;
    hi_s32 temp_max;
    hi_s32 temp_min;
} hi_msensor_magn_config;

typedef struct {
    hi_msensor_gyro_config  gyro_config;
    hi_msensor_acc_config   acc_config;
    hi_msensor_magn_config  magn_config;
} hi_msensor_config;

typedef struct {
    hi_msensor_buf_attr buf_attr;
    hi_msensor_config   config;
    hi_msensor_attr     attr;
} hi_msensor_param;

typedef enum {
    MSENSOR_DATA_GYRO = 0,
    MSENSOR_DATA_ACC,
    MSENSOR_DATA_MAGN,
    MSENSOR_DATA_BUTT
} hi_msensor_data_type;

typedef struct {
    hi_s32 *x_phy_addr;
    hi_s32 *y_phy_addr;
    hi_s32 *z_phy_addr;
    hi_s32 *temp_phy_addr;
    hi_u64 *pts_phy_addr;
    hi_u32  num;      /* number of valid data */
} hi_msensor_data_addr;

typedef struct {
    hi_s32                id;
    hi_msensor_data_type  data_type;
    hi_msensor_data_addr  data[2];
    hi_u64                begin_pts;
    hi_u64                end_pts;
    hi_s64                addr_offset;
} hi_msensor_data_info;

typedef struct {
    hi_msensor_attr        attr;
    hi_msensor_gyro_buffer gyro_buffer;
    hi_msensor_acc_buffer  acc_buffer;
    hi_msensor_magn_buffer magn_buffer;
} hi_msensor_data;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* end of #ifdef __cplusplus */

#endif


