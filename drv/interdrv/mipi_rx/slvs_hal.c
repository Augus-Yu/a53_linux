/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description: slvs_hal.c
 * Author:
 * Create: 2016-10-07
 */

#include "hi_osal.h"
#include "type.h"
#include "hi_mipi.h"
#include "slvs_hal.h"
#include "slvs_reg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/****************************************************************************
 * macro definition                                                         *
 ***************************************************************************/
#define SLVS_REGS_ADDR   0x04A80000
#define SLVS_REGS_SIZE   0x10000

#define SLVS_CRG_ADDR    0x045100F8
#define SLVS_WORK_MODE_ADDR 0x04528098

#define SLVS_IRQ         161
static unsigned int     slvsregMapFlag = 0;

/****************************************************************************
 * global variables definition                                              *
 ****************************************************************************/
slvs_regs_type_t *g_slvs_regs_va = NULL;

unsigned int g_slvs_irq_num = SLVS_IRQ;

slvs_link_err_int_cnt_t g_slvs_link_err_int_cnt[SLVS_MAX_DEV_NUM];
slvs_phy_err_int_cnt_t g_slvs_phy_err_int_cnt[SLVS_MAX_PHY_NUM];

/****************************************************************************
 * function definition                                                      *
 ****************************************************************************/
static void set_bit(unsigned long value, unsigned long offset,
    unsigned long addr)
{
    unsigned long t, mask;

    mask = 1 << offset;
    t = osal_readl((const volatile void *)addr);
    t &= ~mask;
    t |= (value << offset) & mask;
    osal_writel(t, (volatile void *)addr);
}

static void write_reg32(unsigned long addr,
                        unsigned int value,
                        unsigned int mask)
{
    unsigned int t;

    t = osal_readl((void*)addr);
    t &= ~mask;
    t |= value & mask;
    osal_writel(t, (void*)addr);
}

int slvs_drv_is_lane_valid(combo_dev_t devno, short lane_id)
{
    int  lane_valid;

    if (0 <= lane_id && lane_id <= (SLVS_LANE_NUM - 1)) {
        lane_valid = 1;
    } else {
        lane_valid = 0;
    }

    return lane_valid;
}

void slvs_drv_set_lane_num(combo_dev_t devno, unsigned int lane_num)
{
    U_LINK_CTRL link_ctrl;

    link_ctrl.u32 = g_slvs_regs_va->LINK_CTRL.u32;
    link_ctrl.bits.link_lane_num = lane_num - 1;
    g_slvs_regs_va->LINK_CTRL.u32 = link_ctrl.u32;
}

void slvs_drv_set_phy_en(combo_dev_t devno, int enable)
{
    U_PHY_EN phy_en;

    phy_en.u32 = g_slvs_regs_va->PHY_EN.u32;
    phy_en.bits.phy_en = enable;
    g_slvs_regs_va->PHY_EN.u32 = phy_en.u32;
}

static void slvs_drv_set_lane_rx_cken(combo_dev_t devno, short lane_id, int enable)
{
    U_PHY_RSTN_REQ phy_rstn_req;

    phy_rstn_req.u32 = g_slvs_regs_va->PHY_RSTN_REQ.u32;

    if (enable == TRUE) {
        phy_rstn_req.bits.phy_lane_rx_cken |= (0x1 << lane_id % 8);
    } else {
        phy_rstn_req.bits.phy_lane_rx_cken &= !(0x1 << lane_id % 8);
    }

    g_slvs_regs_va->PHY_RSTN_REQ.u32 = phy_rstn_req.u32;
}

static void slvs_drv_set_phy_pcs_cken(combo_dev_t devno, short lane_id, int enable)
{
    U_PHY_PCS_EN phy_pcs_en;

    phy_pcs_en.u32 = g_slvs_regs_va->PHY_PCS_EN.u32;

    switch (lane_id % 8) {
        case 0:
            phy_pcs_en.bits.pcs_lane0_en = enable;
            break;

        case 1:
            phy_pcs_en.bits.pcs_lane1_en = enable;
            break;

        case 2:
            phy_pcs_en.bits.pcs_lane2_en = enable;
            break;

        case 3:
            phy_pcs_en.bits.pcs_lane3_en = enable;
            break;

        case 4:
            phy_pcs_en.bits.pcs_lane4_en = enable;
            break;

        case 5:
            phy_pcs_en.bits.pcs_lane5_en = enable;
            break;

        case 6:
            phy_pcs_en.bits.pcs_lane6_en = enable;
            break;

        case 7:
            phy_pcs_en.bits.pcs_lane7_en = enable;
            break;

        default:
            break;
    }

    g_slvs_regs_va->PHY_PCS_EN.u32 = phy_pcs_en.u32;
}


void slvs_drv_set_lane_cken(combo_dev_t devno, short* p_lane_id, int enable)
{
    int i;
    int lane_id_size;

    lane_id_size = SLVS_LANE_NUM;

    for (i = 0; i < lane_id_size; i++) {
        if (IS_VALID_ID(p_lane_id[i])) {
            slvs_drv_set_lane_rx_cken(devno, p_lane_id[i], enable);
            slvs_drv_set_phy_pcs_cken(devno, p_lane_id[i], enable);
        }
    }
}

static void slvs_drv_set_phy_lane_en(combo_dev_t devno, int lane_id, int enable)
{
    U_PHY_EN phy_en;

    phy_en.u32 = g_slvs_regs_va->PHY_EN.u32;

    switch (lane_id % 8) {
        case 0:
            phy_en.bits.phy_lane0_en = enable;
            break;

        case 1:
            phy_en.bits.phy_lane1_en = enable;
            break;

        case 2:
            phy_en.bits.phy_lane2_en = enable;
            break;

        case 3:
            phy_en.bits.phy_lane3_en = enable;
            break;

        case 4:
            phy_en.bits.phy_lane4_en = enable;
            break;

        case 5:
            phy_en.bits.phy_lane5_en = enable;
            break;

        case 6:
            phy_en.bits.phy_lane6_en = enable;
            break;

        case 7:
            phy_en.bits.phy_lane7_en = enable;
            break;

        default:
            break;
    }

    g_slvs_regs_va->PHY_EN.u32 = phy_en.u32;
}

static void slvs_drv_set_phy_term_en(combo_dev_t devno, int lane_id, int enable)
{
    U_PHY_TERM_EN phy_term_en;

    phy_term_en.u32 = g_slvs_regs_va->PHY_TERM_EN.u32;

    switch (lane_id % 8) {
        case 0:
            phy_term_en.bits.phy_rg_term0_en = enable;
            break;

        case 1:
            phy_term_en.bits.phy_rg_term1_en = enable;
            break;

        case 2:
            phy_term_en.bits.phy_rg_term2_en = enable;
            break;

        case 3:
            phy_term_en.bits.phy_rg_term3_en = enable;
            break;

        case 4:
            phy_term_en.bits.phy_rg_term4_en = enable;
            break;

        case 5:
            phy_term_en.bits.phy_rg_term5_en = enable;
            break;

        case 6:
            phy_term_en.bits.phy_rg_term6_en = enable;
            break;

        case 7:
            phy_term_en.bits.phy_rg_term7_en = enable;
            break;

        default:
            break;
    }

    g_slvs_regs_va->PHY_TERM_EN.u32 = phy_term_en.u32;
}

static void slvs_drv_set_phy_align_en(combo_dev_t devno, int lane_id, int enable)
{
    U_PHY_ALIGN_EN_LINK phy_align_en_link;

    phy_align_en_link.u32 = g_slvs_regs_va->PHY_ALIGN_EN_LINK.u32;

    phy_align_en_link.bits.link_phy_align_en = enable;

    switch (lane_id % 8) {
        case 0:
            phy_align_en_link.bits.link_phy_align_lane0_en = enable;
            break;

        case 1:
            phy_align_en_link.bits.link_phy_align_lane1_en = enable;
            break;

        case 2:
            phy_align_en_link.bits.link_phy_align_lane2_en = enable;
            break;

        case 3:
            phy_align_en_link.bits.link_phy_align_lane3_en = enable;
            break;

        case 4:
            phy_align_en_link.bits.link_phy_align_lane4_en = enable;
            break;

        case 5:
            phy_align_en_link.bits.link_phy_align_lane5_en = enable;
            break;

        case 6:
            phy_align_en_link.bits.link_phy_align_lane6_en = enable;
            break;

        case 7:
            phy_align_en_link.bits.link_phy_align_lane7_en = enable;
            break;

        default:
            break;
    }

    g_slvs_regs_va->PHY_ALIGN_EN_LINK.u32 = phy_align_en_link.u32;
}

void slvs_drv_set_lane_en(combo_dev_t devno, short* p_lane_id, int enable)
{
    int i;
    int lane_id_size;

    lane_id_size = SLVS_LANE_NUM;

    for (i = 0; i < lane_id_size; i++) {
        if (IS_VALID_ID(p_lane_id[i])) {
            slvs_drv_set_phy_lane_en(devno, p_lane_id[i], enable);
            slvs_drv_set_phy_term_en(devno, p_lane_id[i], enable);
            slvs_drv_set_phy_align_en(devno, p_lane_id[i], enable);
        }
    }
}

void slvs_drv_phy_ctrl_test(combo_dev_t devno, int value)
{
    U_PHY_CTRL_TEST phy_ctrl_test;

    phy_ctrl_test.u32 = value;
    g_slvs_regs_va->PHY_CTRL_TEST.u32 = phy_ctrl_test.u32;
}

void slvs_drv_set_raw_type(combo_dev_t devno, data_type_t input_data_type)
{
    U_LINK_CTRL link_ctrl;
    unsigned int temp_data_type = 0;

    if (DATA_TYPE_RAW_8BIT == input_data_type) {
        temp_data_type = 0x0;
    }
    if (DATA_TYPE_RAW_10BIT == input_data_type) {
        temp_data_type = 0x1;
    } else if (DATA_TYPE_RAW_12BIT == input_data_type) {
        temp_data_type = 0x2;
    } else if (DATA_TYPE_RAW_14BIT == input_data_type) {
        temp_data_type = 0x3;
    } else if (DATA_TYPE_RAW_16BIT == input_data_type) {
        temp_data_type = 0x4;
    }

    link_ctrl.u32 = g_slvs_regs_va->LINK_CTRL.u32;
    link_ctrl.bits.link_raw_type = temp_data_type;
    g_slvs_regs_va->LINK_CTRL.u32 = link_ctrl.u32;
}

void slvs_drv_set_data_rate(combo_dev_t devno, mipi_data_rate_t data_rate)
{
    U_LINK_CTRL link_ctrl;
    unsigned int mipi_double_pix_en = 0;

    if (MIPI_DATA_RATE_X1 == data_rate) {
        mipi_double_pix_en = 0x0;
    } else if (MIPI_DATA_RATE_X2 == data_rate) {
        mipi_double_pix_en = 0x1;
    } else {
        HI_ERR("unsupported data_rate:%d devno %d\n", data_rate, devno);
        return;
    }


    link_ctrl.u32 = g_slvs_regs_va->LINK_CTRL.u32;
    link_ctrl.bits.link_double_pix_en = mipi_double_pix_en;
    g_slvs_regs_va->LINK_CTRL.u32 = link_ctrl.u32;
}

void slvs_drv_set_lane_rate(combo_dev_t devno, slvs_lane_rate_t lane_rate)
{
    U_PHY_SEL phy_sel;
    unsigned int phy_baudsel = 0;

    if (lane_rate == SLVS_LANE_RATE_LOW) {
        phy_baudsel = 1;
    } else if (lane_rate == SLVS_LANE_RATE_HIGH) {
        phy_baudsel = 0x0;
    } else {
        HI_ERR("unsupported lane_rate:%d devno %d\n", lane_rate, devno);
        return;
    }

    phy_sel.u32 = g_slvs_regs_va->PHY_SEL.u32;
    phy_sel.bits.phy_baudsel = phy_baudsel;
    g_slvs_regs_va->PHY_SEL.u32 = phy_sel.u32;
}

void slvs_drv_set_wdr_mode(combo_dev_t devno, wdr_mode_t wdr_mode)
{
    U_LINK_CTRL link_ctrl;
    U_LINK_DOL_FR_PAR Link_dol_fr_par;
    unsigned int wdr_num = 0;
    unsigned int dol_fr0_par = 0x0;
    unsigned int dol_fr1_par = 0x0;

    if (wdr_mode == HI_WDR_MODE_NONE) {
        wdr_num = 0;
        dol_fr0_par = 0;
        dol_fr1_par = 0;
    } else if (HI_WDR_MODE_DOL_2F == wdr_mode) {
        wdr_num = 0x1;
        dol_fr0_par = 0x0;
        dol_fr1_par = 0x4;
    } else {
        HI_ERR("unsupported wdr_mode:%d devno %d\n", wdr_mode, devno);
        return;
    }

    link_ctrl.u32 = g_slvs_regs_va->LINK_CTRL.u32;
    link_ctrl.bits.link_wdr_num = wdr_num;
    g_slvs_regs_va->LINK_CTRL.u32 = link_ctrl.u32;

    Link_dol_fr_par.u32 = g_slvs_regs_va->LINK_DOL_FR_PAR.u32;
    Link_dol_fr_par.bits.link_dol_fr0_par = dol_fr0_par;
    Link_dol_fr_par.bits.link_dol_fr1_par = dol_fr1_par;
    g_slvs_regs_va->LINK_DOL_FR_PAR.u32 = Link_dol_fr_par.u32;
}

void slvs_drv_set_deskew_symbol(combo_dev_t devno, int symbol)
{
    U_PHY_DESKEW_SYMBOL_LINK pht_deskew_symbol_link;

    pht_deskew_symbol_link.u32 = g_slvs_regs_va->PHY_DESKEW_SYMBOL_LINK.u32;
    pht_deskew_symbol_link.bits.link_phy_deskew_symbol = symbol;
    g_slvs_regs_va->PHY_DESKEW_SYMBOL_LINK.u32 = pht_deskew_symbol_link.u32;
}

void slvs_drv_set_clear_en(combo_dev_t devno, int enable)
{
    U_LINK_CLEAR_EN link_clear_en;

    link_clear_en.u32 = g_slvs_regs_va->LINK_CLEAR_EN.u32;
    link_clear_en.bits.link_skew_clear_en = enable;
    g_slvs_regs_va->LINK_CLEAR_EN.u32 = link_clear_en.u32;
}

void slvs_drv_set_mem_ck_en(combo_dev_t devno, int enable)
{
    U_LINK_MEMORY_CTRL link_memory_ctrl;

    link_memory_ctrl.u32 = g_slvs_regs_va->LINK_MEMORY_CTRL.u32;
    link_memory_ctrl.bits.link_mem_ck_en = enable;
    g_slvs_regs_va->LINK_MEMORY_CTRL.u32 = link_memory_ctrl.u32;
}

void slvs_drv_set_sensor_avalid_width(combo_dev_t devno, int width)
{
    U_LINK_PAYLOAD_SIZE link_payload_size;

    link_payload_size.u32 = g_slvs_regs_va->LINK_PAYLOAD_SIZE.u32;
    link_payload_size.bits.link_paylsize = width;
    g_slvs_regs_va->LINK_PAYLOAD_SIZE.u32 = link_payload_size.u32;
}

void slvs_drv_set_image_rect(combo_dev_t devno, img_rect_t *p_img_rect)
{
    U_LINK_HEIGHT link_height;
    U_LINK_WIDTH link_width;

    link_width.u32 = g_slvs_regs_va->LINK_WIDTH.u32;
    link_width.bits.link_start_x = p_img_rect->x;
    link_width.bits.link_imgwidth = p_img_rect->width - 1;
    g_slvs_regs_va->LINK_WIDTH.u32 = link_width.u32;

    link_height.u32 = g_slvs_regs_va->LINK_HEIGHT.u32;
    link_height.bits.link_line_start = p_img_rect->y;
    link_height.bits.link_line_end = p_img_rect->y + p_img_rect->height - 1;
    g_slvs_regs_va->LINK_HEIGHT.u32 = link_height.u32;
}

void slvs_drv_set_crop_en(combo_dev_t devno, int enable)
{
    U_LINK_CTRL link_ctrl;

    link_ctrl.u32 = g_slvs_regs_va->LINK_CTRL.u32;
    link_ctrl.bits.link_hcrop_en = enable;
    link_ctrl.bits.link_vcrop_en = enable;
    g_slvs_regs_va->LINK_CTRL.u32 = link_ctrl.u32;
}

void slvs_drv_set_crc_enable(combo_dev_t devno, int enable)
{
    U_LINK_CTRL link_ctrl;

    link_ctrl.u32 = g_slvs_regs_va->LINK_CTRL.u32;
    link_ctrl.bits.link_crc_en = enable;
    g_slvs_regs_va->LINK_CTRL.u32 = link_ctrl.u32;
}

static void slvs_drv_set_link_align_id(combo_dev_t devno, int lane_idx, short lane_id)
{

    U_PHY_ALIGN_ID_LINK phy_align_id_link;

    phy_align_id_link.u32 = g_slvs_regs_va->PHY_ALIGN_ID_LINK.u32;


    switch (lane_id) {
        case 0:
            phy_align_id_link.bits.link_phy_align_lane0_id = lane_idx;
            break;

        case 1:
            phy_align_id_link.bits.link_phy_align_lane1_id = lane_idx;
            break;

        case 2:
            phy_align_id_link.bits.link_phy_align_lane2_id = lane_idx;
            break;

        case 3:
            phy_align_id_link.bits.link_phy_align_lane3_id = lane_idx;
            break;

        case 4:
            phy_align_id_link.bits.link_phy_align_lane4_id = lane_idx;
            break;

        case 5:
            phy_align_id_link.bits.link_phy_align_lane5_id = lane_idx;
            break;

        case 6:
            phy_align_id_link.bits.link_phy_align_lane6_id = lane_idx;
            break;

        case 7:
            phy_align_id_link.bits.link_phy_align_lane7_id = lane_idx;
            break;

        default:
            break;
    }

    g_slvs_regs_va->PHY_ALIGN_ID_LINK.u32 = phy_align_id_link.u32;
}

void slvs_drv_set_link_lane_order(combo_dev_t devno, short* p_lane_id)
{
    int i;
    int lane_id_size;

    lane_id_size = SLVS_LANE_NUM;

    for (i = 0; i < lane_id_size; i++) {
        if (IS_VALID_ID(p_lane_id[i])) {
            slvs_drv_set_link_align_id(devno, i,  p_lane_id[i]);
        }
    }
}

static void slvs_drv_lane_srst_reset(combo_dev_t devno, short lane_id, int reset)
{
    U_PHY_RSTN_REQ phy_rstn_req;

    phy_rstn_req.u32 = g_slvs_regs_va->PHY_RSTN_REQ.u32;

    if (reset == TRUE) {
        phy_rstn_req.bits.phy_lane_srst_req |= (0x1 << lane_id % 8);
    } else {
        phy_rstn_req.bits.phy_lane_srst_req &= !(0x1 << lane_id % 8);
    }

    g_slvs_regs_va->PHY_RSTN_REQ.u32 = phy_rstn_req.u32;
}

static void slvs_drv_cdr_en(int enable)
{
    U_PHY_CDR_EN phy_cdr_en;
    U_PHY_CDR_CTRL0 phy_cdr_ctrl0;
    U_PHY_CDR_CTRL1 phy_cdr_ctrl1;


    phy_cdr_en.u32 = g_slvs_regs_va->PHY_CDR_EN.u32;
    phy_cdr_en.bits.phy_rg_cdr_en = enable;
    g_slvs_regs_va->PHY_CDR_EN.u32 = phy_cdr_en.u32;


    phy_cdr_ctrl0.u32 = 0x33f00010;
    g_slvs_regs_va->PHY_CDR_CTRL0.u32 = phy_cdr_ctrl0.u32;

    phy_cdr_ctrl1.u32 = 0x0;
    g_slvs_regs_va->PHY_CDR_CTRL1.u32 = phy_cdr_ctrl1.u32;
}

void slvs_drv_phy_eq_ctrl(int eq)
{
    U_PHY_EQ_CTRL0 phy_eq_ctrl0;
    U_PHY_EQ_CTRL1 phy_eq_ctrl1;
    U_PHY_EQ_CTRL2 phy_eq_ctrl2;

    phy_eq_ctrl0.u32 = g_slvs_regs_va->PHY_EQ_CTRL0.u32;
    phy_eq_ctrl0.bits.phy_rg_eq_ctrl0 = eq;
    phy_eq_ctrl0.bits.phy_rg_eq_ctrl1 = eq;
    phy_eq_ctrl0.bits.phy_rg_eq_ctrl2 = eq;
    phy_eq_ctrl0.bits.phy_rg_eq_ctrl3 = eq;
    g_slvs_regs_va->PHY_EQ_CTRL0.u32 = phy_eq_ctrl0.u32;

    phy_eq_ctrl1.u32 = g_slvs_regs_va->PHY_EQ_CTRL1.u32;
    phy_eq_ctrl1.bits.phy_rg_eq_ctrl4 = eq;
    phy_eq_ctrl1.bits.phy_rg_eq_ctrl5 = eq;
    phy_eq_ctrl1.bits.phy_rg_eq_ctrl6 = eq;
    phy_eq_ctrl1.bits.phy_rg_eq_ctrl7 = eq;
    g_slvs_regs_va->PHY_EQ_CTRL1.u32 = phy_eq_ctrl1.u32;

    phy_eq_ctrl2.u32 = 0x040029b2;
    g_slvs_regs_va->PHY_EQ_CTRL2.u32 = phy_eq_ctrl2.u32;

}

void slvs_drv_phy_pdac_ldo(int value)
{
    U_PHY_PDAC_LDO phy_pdac_ldo;

    phy_pdac_ldo.u32 = value;
    g_slvs_regs_va->PHY_PDAC_LDO.u32 = phy_pdac_ldo.u32;
}

unsigned int slvs_drv_get_phy_data(short lane_id)
{
    unsigned int lane_data = 0x0;
    U_PHY_LANE_DATA0 phy_lane_data0;
    U_PHY_LANE_DATA1 phy_lane_data1;
    U_PHY_LANE_DATA2 phy_lane_data2;
    U_PHY_LANE_DATA3 phy_lane_data3;

    switch (lane_id % 8) {
        case 0:
            phy_lane_data0.u32 = g_slvs_regs_va->PHY_LANE_DATA0.u32;
            lane_data = phy_lane_data0.bits.phy_data0_slvs;
            break;
        case 1:
            phy_lane_data0.u32 = g_slvs_regs_va->PHY_LANE_DATA0.u32;
            lane_data = phy_lane_data0.bits.phy_data1_slvs;
            break;
        case 2:
            phy_lane_data1.u32 = g_slvs_regs_va->PHY_LANE_DATA1.u32;
            lane_data = phy_lane_data1.bits.phy_data2_slvs;
            break;
        case 3:
            phy_lane_data1.u32 = g_slvs_regs_va->PHY_LANE_DATA1.u32;
            lane_data = phy_lane_data1.bits.phy_data3_slvs;
            break;
        case 4:
            phy_lane_data2.u32 = g_slvs_regs_va->PHY_LANE_DATA2.u32;
            lane_data = phy_lane_data2.bits.phy_data4_slvs;
            break;
        case 5:
            phy_lane_data2.u32 = g_slvs_regs_va->PHY_LANE_DATA2.u32;
            lane_data = phy_lane_data2.bits.phy_data5_slvs;
            break;
        case 6:
            phy_lane_data3.u32 = g_slvs_regs_va->PHY_LANE_DATA3.u32;
            lane_data = phy_lane_data3.bits.phy_data6_slvs;
            break;
        case 7:
            phy_lane_data3.u32 = g_slvs_regs_va->PHY_LANE_DATA3.u32;
            lane_data = phy_lane_data3.bits.phy_data7_slvs;
            break;
        default:
            lane_data = 0x0;
            break;
    }

    return lane_data;
}

unsigned int slvs_drv_get_phy_aligned_data(short lane_id)
{
    unsigned int lane_data = 0x0;
    U_PHY_SYMAL_DATA0 phy_lane_data0;
    U_PHY_SYMAL_DATA1 phy_lane_data1;
    U_PHY_SYMAL_DATA2 phy_lane_data2;
    U_PHY_SYMAL_DATA3 phy_lane_data3;

    switch (lane_id % 8) {
        case 0:
            phy_lane_data0.u32 = g_slvs_regs_va->PHY_SYMAL_DATA0.u32;
            lane_data = phy_lane_data0.bits.physt_symal_lane0_d;
            break;
        case 1:
            phy_lane_data0.u32 = g_slvs_regs_va->PHY_SYMAL_DATA0.u32;
            lane_data = phy_lane_data0.bits.physt_symal_lane1_d;
            break;
        case 2:
            phy_lane_data1.u32 = g_slvs_regs_va->PHY_SYMAL_DATA1.u32;
            lane_data = phy_lane_data1.bits.physt_symal_lane2_d;
            break;
        case 3:
            phy_lane_data1.u32 = g_slvs_regs_va->PHY_SYMAL_DATA1.u32;
            lane_data = phy_lane_data1.bits.physt_symal_lane3_d;
            break;
        case 4:
            phy_lane_data2.u32 = g_slvs_regs_va->PHY_SYMAL_DATA2.u32;
            lane_data = phy_lane_data2.bits.physt_symal_lane4_d;
            break;
        case 5:
            phy_lane_data2.u32 = g_slvs_regs_va->PHY_SYMAL_DATA2.u32;
            lane_data = phy_lane_data2.bits.physt_symal_lane5_d;
            break;
        case 6:
            phy_lane_data3.u32 = g_slvs_regs_va->PHY_SYMAL_DATA3.u32;
            lane_data = phy_lane_data3.bits.physt_symal_lane6_d;
            break;
        case 7:
            phy_lane_data3.u32 = g_slvs_regs_va->PHY_SYMAL_DATA3.u32;
            lane_data = phy_lane_data3.bits.physt_symal_lane7_d;
            break;
        default:
            lane_data = 0x0;
            break;
    }

    return lane_data;
}

void slvs_drv_get_imgsize_statis(combo_dev_t devno, short vc, img_size_t* p_size)
{
    U_LINK_IMGSIZE0_STATUS MIPI_IMGSIZE0_STATIS;
    U_LINK_IMGSIZE1_STATUS MIPI_IMGSIZE1_STATIS;
    U_LINK_IMGSIZE2_STATUS MIPI_IMGSIZE2_STATIS;
    U_LINK_IMGSIZE3_STATUS MIPI_IMGSIZE3_STATIS;

    if (vc == 0) {
        MIPI_IMGSIZE0_STATIS.u32 = g_slvs_regs_va->LINK_IMGSIZE0_STATUS.u32;
        p_size->width  = MIPI_IMGSIZE0_STATIS.bits.link_imgwidth_statis_vc0;
        p_size->height = MIPI_IMGSIZE0_STATIS.bits.link_imgheight_statis_vc0;
    } else if (vc == 1) {
        MIPI_IMGSIZE1_STATIS.u32 = g_slvs_regs_va->LINK_IMGSIZE1_STATUS.u32;
        p_size->width  = MIPI_IMGSIZE1_STATIS.bits.link_imgwidth_statis_vc1;
        p_size->height = MIPI_IMGSIZE1_STATIS.bits.link_imgheight_statis_vc1;
    } else if (vc == 2) {
        MIPI_IMGSIZE2_STATIS.u32 = g_slvs_regs_va->LINK_IMGSIZE2_STATUS.u32;
        p_size->width  = MIPI_IMGSIZE2_STATIS.bits.link_imgwidth_statis_vc2;
        p_size->height = MIPI_IMGSIZE2_STATIS.bits.link_imgheight_statis_vc2;
    } else if (vc == 3) {
        MIPI_IMGSIZE3_STATIS.u32 = g_slvs_regs_va->LINK_IMGSIZE3_STATUS.u32;
        p_size->width  = MIPI_IMGSIZE3_STATIS.bits.link_imgwidth_statis_vc3;
        p_size->height = MIPI_IMGSIZE3_STATIS.bits.link_imgheight_statis_vc3;
    }
}

void slvs_drv_lane_reset(combo_dev_t devno, short* p_lane_id)
{
    int i;
    int lane_id_size;

    lane_id_size = SLVS_LANE_NUM;

    for (i = 0; i < lane_id_size; i++) {
        if (IS_VALID_ID(p_lane_id[i])) {
            slvs_drv_lane_srst_reset(devno, p_lane_id[i], TRUE);
        }
    }
}

void slvs_drv_lane_unreset(combo_dev_t devno, short* p_lane_id)
{
    int i;
    int lane_id_size;

    lane_id_size = SLVS_LANE_NUM;

    for (i = 0; i < lane_id_size; i++) {
        if (IS_VALID_ID(p_lane_id[i])) {
            slvs_drv_lane_srst_reset(devno, p_lane_id[i], FALSE);
        }
    }
}

static void slvs_enable_disable_pixel_clock(combo_dev_t combo_dev, int enable)
{
    unsigned long slvs_clock_addr;

    slvs_clock_addr  = (unsigned long)osal_ioremap(SLVS_CRG_ADDR, (unsigned long)0x4);
    if (slvs_clock_addr == NULL) {
        HI_ERR("slvs clock ioremap failed!\n");
        return;
    }
    set_bit(enable, 16 + combo_dev, slvs_clock_addr);
    osal_iounmap((void*)slvs_clock_addr);
}

void slvs_drv_enable_pixel_clock(combo_dev_t combo_dev)
{
    slvs_enable_disable_pixel_clock(combo_dev, 1);
}

void slvs_drv_disable_pixel_clock(combo_dev_t combo_dev)
{
    slvs_enable_disable_pixel_clock(combo_dev, 0);
}

static void slvs_core_reset_unreset(combo_dev_t combo_dev, int reset)
{
    unsigned long slvs_core_reset_addr;

    slvs_core_reset_addr  = (unsigned long)osal_ioremap(SLVS_CRG_ADDR, (unsigned long)0x4);
    if (slvs_core_reset_addr == NULL) {
        HI_ERR("slvs reset reg ioremap failed!\n");
        return;
    }
    set_bit(reset, 1 + combo_dev, slvs_core_reset_addr);
    osal_iounmap((void*)slvs_core_reset_addr);
}

void slvs_drv_core_reset(combo_dev_t combo_dev)
{
    slvs_core_reset_unreset(combo_dev, 1);
}

void slvs_drv_core_unreset(combo_dev_t combo_dev)
{
    slvs_core_reset_unreset(combo_dev, 0);
}

static int slvs_interrupt_route(int irq, void* dev_id)
{
    return OSAL_IRQ_HANDLED;
}

static int slvs_drv_reg_init(void)
{
    if (!g_slvs_regs_va) {
        g_slvs_regs_va = (slvs_regs_type_t *)osal_ioremap(SLVS_REGS_ADDR, (unsigned int)SLVS_REGS_SIZE);
        if (g_slvs_regs_va == NULL) {
            HI_ERR("remap slvs reg fail\n");
            return -1;
        }
        slvsregMapFlag = 1;
    }

    return 0;
}

static void slvs_drv_reg_exit(void)
{
    if (slvsregMapFlag == 1) {
        if (g_slvs_regs_va != NULL) {
            osal_iounmap((void *)g_slvs_regs_va);
            g_slvs_regs_va = NULL;
        }
        slvsregMapFlag = 0;
    }
}

static int slvs_register_irq(void)
{
    int ret;

    ret = osal_request_irq(g_slvs_irq_num, slvs_interrupt_route, NULL, "SLVS", slvs_interrupt_route);
    if (ret < 0) {
        HI_ERR("slvs: failed to register irq.\n");
        return -1;
    }

    return 0;
}

static void slvs_unregister_irq(void)
{
    osal_free_irq(g_slvs_irq_num, slvs_interrupt_route);
}

static void slvs_drv_hw_init(void)
{
    unsigned long slvs_crg_addr;

#ifndef HI_FPGA
    slvs_crg_addr = (unsigned long)osal_ioremap(SLVS_CRG_ADDR, (unsigned long)0x4);

    /* bus clk & phy clk */
    write_reg32(slvs_crg_addr, 1 << 20, 0x1 << 20);
    write_reg32(slvs_crg_addr, 1 << 22, 0x1 << 22);

    /* bus reset */
    write_reg32(slvs_crg_addr, 1 << 4, 0x1 << 4);
    osal_udelay(10);
    write_reg32(slvs_crg_addr, 0, 0x1 << 4);

    /* phy reset */
    write_reg32(slvs_crg_addr, 1 << 23, 0x1 << 23);
    osal_udelay(10);
    write_reg32(slvs_crg_addr, 0, 0x1 << 23);

    osal_iounmap((void *)slvs_crg_addr);
#endif

    slvs_drv_cdr_en(TRUE);
    slvs_drv_phy_eq_ctrl(0x00000039);
    slvs_drv_phy_pdac_ldo(0x00000006);
}

static void slvs_drv_hw_exit(void)
{
    unsigned long slvs_crg_addr;

#ifndef HI_FPGA
    slvs_crg_addr = (unsigned long)osal_ioremap(SLVS_CRG_ADDR, (unsigned long)0x4);

    /* bus reset */
    write_reg32(slvs_crg_addr, 1 << 4, 0x1 << 4);

    /* phy reset */
    write_reg32(slvs_crg_addr, 1 << 23, 0x1 << 23);

    /* bus clk & phy clk */
    write_reg32(slvs_crg_addr, 0, 0x1 << 20);
    write_reg32(slvs_crg_addr, 0, 0x1 << 22);

    osal_iounmap((void *)slvs_crg_addr);
#endif
}

int slvs_drv_init(void)
{
    int ret;

    ret = slvs_drv_reg_init();
    if (ret < 0) {
        HI_ERR("slvs_drv_reg_init fail!\n");
        goto fail0;
    }

    ret = slvs_register_irq();
    if (ret < 0) {
        HI_ERR("slvs_register_irq fail!\n");
        goto fail1;
    }

    slvs_drv_hw_init();

    return 0;

fail1:
    slvs_drv_reg_exit();
fail0:
    return -1;
}

void slvs_drv_exit(void)
{
    slvs_unregister_irq();
    slvs_drv_reg_exit();
    slvs_drv_hw_exit();
}

#ifdef __cplusplus
#if __cplusplus
}

#endif
#endif /* End of #ifdef __cplusplus */
