/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description: slvs_hal.h
 * Author:
 * Create: 2016-10-07
 */

#ifndef _SLVS_HAL__
#define _SLVS_HAL__

#include "hi_mipi.h"

#define SLVS_MAX_PHY_NUM 1

#define IS_VALID_ID(id)   ((id) != -1)

typedef struct {
    unsigned int header_crc_err_cnt;
    unsigned int payload_crc_err_cnt;
    unsigned int ecc_err_cnt;
    unsigned int data_fifo_w_err_cnt;
    unsigned int data_fifo_r_err_cnt;
    unsigned int cmd_fifo_full_err_cnt;
    unsigned int skew_err_cnt;
    unsigned int vsync_cnt;
} slvs_link_err_int_cnt_t;

typedef struct {
    unsigned int afifo_align_cnt[SLVS_LANE_NUM];
    unsigned int code_err_cnt[SLVS_LANE_NUM];
    unsigned int disp_err_cnt[SLVS_LANE_NUM];
} slvs_phy_err_int_cnt_t;

void slvs_drv_set_lane_num(combo_dev_t devno, unsigned int lane_num);
void slvs_drv_set_phy_en(combo_dev_t devno, int enable);
void slvs_drv_set_lane_cken(combo_dev_t devno, short* p_lane_id, int enable);
void slvs_drv_set_lane_en(combo_dev_t devno, short* p_lane_id, int enable);
void slvs_drv_phy_ctrl_test(combo_dev_t devno, int value);
void slvs_drv_set_raw_type(combo_dev_t devno, data_type_t input_data_type);
void slvs_drv_set_data_rate(combo_dev_t devno, mipi_data_rate_t data_rate);
void slvs_drv_set_lane_rate(combo_dev_t devno, slvs_lane_rate_t lane_rate);
void slvs_drv_set_wdr_mode(combo_dev_t devno, wdr_mode_t wdr_mode);
void slvs_drv_set_deskew_symbol(combo_dev_t devno, int symbol);
void slvs_drv_set_clear_en(combo_dev_t devno, int enable);
void slvs_drv_set_mem_ck_en(combo_dev_t devno, int enable);
void slvs_drv_set_sensor_avalid_width(combo_dev_t devno, int width);
void slvs_drv_set_image_rect(combo_dev_t devno, img_rect_t *p_img_rect);
void slvs_drv_set_crop_en(combo_dev_t devno, int enable);
void slvs_drv_set_crc_enable(combo_dev_t devno, int enable);
void slvs_drv_set_link_lane_order(combo_dev_t devno, short* p_lane_id);
unsigned int slvs_drv_get_phy_data(short lane_id);
unsigned int slvs_drv_get_phy_aligned_data(short lane_id);
void slvs_drv_get_imgsize_statis(combo_dev_t devno, short vc, img_size_t* p_size);

void slvs_drv_lane_reset(combo_dev_t devno, short* p_lane_id);
void slvs_drv_lane_unreset(combo_dev_t devno, short* p_lane_id);

int slvs_drv_is_lane_valid(combo_dev_t devno, short lane_id);

void slvs_drv_enable_pixel_clock(combo_dev_t combo_dev);
void slvs_drv_disable_pixel_clock(combo_dev_t combo_dev);

void slvs_drv_core_reset(combo_dev_t combo_dev);
void slvs_drv_core_unreset(combo_dev_t combo_dev);

int slvs_drv_init(void);
void slvs_drv_exit(void);

#endif
