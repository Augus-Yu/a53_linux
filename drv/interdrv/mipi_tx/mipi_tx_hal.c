/******************************************************************************
Copyright (C), 2016, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : mipi_tx_hal.c
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2016/11/15
Last Modified :
Description   :
Function List :
******************************************************************************/

#include "hi_osal.h"
#include "type.h"
#include "hi_mipi_tx.h"
#include "mipi_tx_hal.h"
#include "mipi_tx_reg.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*
 * macro definition
 */
#define MIPI_TX_REGS_ADDR   0x04780000
#define MIPI_TX_REGS_SIZE   0x10000

#define MIPI_TX_IRQ         120

#define MIPI_TX_CRG         0x0451011C

#define MIPI_TX_REF_CLK     27


#define TLPX                60
#define TCLK_PREPARE        60
#define TCLK_ZERO           250
#define TCLK_TRAIL          80
#define TPRE_DELAY          100
#define THS_PREPARE         80
#define THS_ZERO            180
#define THS_TRAIL           110

/* phy addr */
#define PLL_SET0            0x14
#define PLL_SET1            0x15
#define PLL_SET2            0x16
#ifdef HI_FPGA
#define PLL_SET3            0x17
#endif
#define PLL_SET4            0x1E
#define PLL_SET12           0x12
#define PLL_SET13           0x13

#define DATA0_TPRE_DELAY    0x30
#define DATA1_TPRE_DELAY    0x40
#define DATA2_TPRE_DELAY    0x50
#define DATA3_TPRE_DELAY    0x60

#define CLK_TLPX            0x22
#define CLK_TCLK_PREPARE    0x23
#define CLK_TCLK_ZERO       0x24
#define CLK_TCLK_TRAIL      0x25

#define DATA0_TLPX          0x32
#define DATA0_THS_PREPARE   0x33
#define DATA0_THS_ZERO      0x34
#define DATA0_THS_TRAIL     0x35
#define DATA1_TLPX          0x42
#define DATA1_THS_PREPARE   0x43
#define DATA1_THS_ZERO      0x44
#define DATA1_THS_TRAIL     0x45
#define DATA2_TLPX          0x52
#define DATA2_THS_PREPARE   0x53
#define DATA2_THS_ZERO      0x54
#define DATA2_THS_TRAIL     0x55
#define DATA3_TLPX          0x62
#define DATA3_THS_PREPARE   0x63
#define DATA3_THS_ZERO      0x64
#define DATA3_THS_TRAIL     0x65

#define MIPI_TX_READ_TIMEOUT_CNT 1000
#define MIPI_TX_MAX_RESET_PHY_TIMES 10

/*
 * global operation timing parameters
 */
typedef struct {
    unsigned char data_tpre_delay;
    unsigned char clk_tlpx;
    unsigned char clk_tclk_prepare;
    unsigned char clk_tclk_zero;
    unsigned char clk_tclk_trail;
    unsigned char data_tlpx;
    unsigned char data_ths_prepare;
    unsigned char data_ths_zero;
    unsigned char data_ths_trail;
} mipi_tx_phy_timing_parameters;

/*
 * global variables definition
 */
volatile mipi_tx_regs_type_t *mipi_tx_regs_va = NULL;

unsigned int mipi_tx_irq_num = MIPI_TX_IRQ;

unsigned int actual_phy_data_rate;

extern mipi_tx_dev_phy_t mipi_tx_phy_ctx;

/*
 * function definition
 */
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

static int g_phy_reset_count = 0;

static unsigned int regMapFlag = 0;

int mipi_tx_drv_reset_phy(combo_dev_cfg_t *dev_cfg)
{
    g_phy_reset_count++;
#ifdef MIPI_TX_DEBUG
    osal_printk("reset phy count ===================================== %d\n", g_phy_reset_count);
#endif
    /* set phy config */
    mipi_tx_drv_set_phy_cfg(dev_cfg);

    return g_phy_reset_count;
}


static void set_phy_reg(unsigned int addr, unsigned char value)
{
    osal_isb();
    osal_dsb();
    osal_dmb();
    mipi_tx_regs_va->PHY_TST_CTRL1.u32 = (0x10000 + addr);
    osal_isb();
    osal_dsb();
    osal_dmb();
    mipi_tx_regs_va->PHY_TST_CTRL0.u32 = 0x2;
    osal_isb();
    osal_dsb();
    osal_dmb();
    mipi_tx_regs_va->PHY_TST_CTRL0.u32 = 0x0;
    osal_isb();
    osal_dsb();
    osal_dmb();
    mipi_tx_regs_va->PHY_TST_CTRL1.u32 = value;
    osal_isb();
    osal_dsb();
    osal_dmb();
    mipi_tx_regs_va->PHY_TST_CTRL0.u32 = 0x2;
    osal_isb();
    osal_dsb();
    osal_dmb();
    mipi_tx_regs_va->PHY_TST_CTRL0.u32 = 0x0;
    osal_isb();
    osal_dsb();
    osal_dmb();
}

void mipi_tx_pll_status_check(combo_dev_cfg_t *dev_cfg);

/*
 * get phy pll parameter set1.
 */
static unsigned char mipi_tx_drv_get_phy_pll_set1(unsigned int phy_data_rate)
{
    unsigned char pll_set1;

#ifndef HI_FPGA
    if (phy_data_rate >= 320) {
        pll_set1 = (phy_data_rate + MIPI_TX_REF_CLK - 1) / MIPI_TX_REF_CLK;  /* 594/27=22 */
        actual_phy_data_rate = pll_set1 * MIPI_TX_REF_CLK;     /* 22*27=594 */
    } else {
        pll_set1 = (phy_data_rate * 4 + MIPI_TX_REF_CLK - 1) / MIPI_TX_REF_CLK;
        actual_phy_data_rate = pll_set1 * MIPI_TX_REF_CLK / 4;
    }
#else
    if (phy_data_rate >= 320) {
        pll_set1 = phy_data_rate / MIPI_TX_REF_CLK / 2;
        actual_phy_data_rate = phy_data_rate / MIPI_TX_REF_CLK * MIPI_TX_REF_CLK;
    } else {
        pll_set1 = phy_data_rate * 4 / MIPI_TX_REF_CLK / 2;
        actual_phy_data_rate = phy_data_rate * 4 / MIPI_TX_REF_CLK  * MIPI_TX_REF_CLK / 4;
    }
#endif

    return pll_set1;
}

/*
 * get phy pll parameter set4.
 */
static unsigned char mipi_tx_drv_get_phy_pll_set4(unsigned int phy_data_rate)
{
    unsigned char pll_set4;

#ifndef HI_FPGA
    if (phy_data_rate >= 320) {
        pll_set4 = 0x68;
    } else {
        pll_set4 = 0x6c;
    }
#else
    if (phy_data_rate >= 320) {
        if (phy_data_rate >= 750) {
            pll_set4 = 0x68;
        } else {
            pll_set4 = 0x28;
        }
    } else {
        pll_set4 = 0x2c;
    }
#endif

    return pll_set4;
}

/*
 * set phy pll parameters set0, set1, set2, set4.
 */
static void mipi_tx_drv_set_phy_pll_setx(unsigned int phy_data_rate)
{
    unsigned char pll_set0;
    unsigned char pll_set1;
    unsigned char pll_set2;
#ifdef HI_FPGA
    unsigned char pll_set3;
#endif
    unsigned char pll_set4;


    /* step2 : */
    /* pll_set0 */
    pll_set0 = 0x0;
    set_phy_reg(PLL_SET0, pll_set0);

    /* pll_set1 */
    pll_set1 = 0xf;
    set_phy_reg(PLL_SET1, pll_set1);
    /* pll_set2 */
    pll_set2 = 0x61;
    set_phy_reg(PLL_SET2, pll_set2);
    /* pll_set4 */
    pll_set4 = 0x68;
    set_phy_reg(PLL_SET4, pll_set4);

    /* step3 :phy power on */
    mipi_tx_regs_va->PHY_RSTZ.u32 = 0xf;
    /* step4 */
    osal_udelay(100);

    /* step5:999M start pll  */
    /* pll_set1 */
    pll_set1 = 0x25;
    set_phy_reg(PLL_SET1, pll_set1);

    /* pll_set4 */
    pll_set4 = 0x68;
    set_phy_reg(PLL_SET4, pll_set4);

    /* pll_set0 */
    pll_set0 = 0x02;
    set_phy_reg(PLL_SET0, pll_set0);

    /* pll_set2 */
    pll_set2 = 0xe1;
    set_phy_reg(PLL_SET2, pll_set2);

    osal_udelay(100);

    /* pll_set0 */
    pll_set0 = 0x0;
    set_phy_reg(PLL_SET0, pll_set0);

    /* pll_set2 */
    pll_set2 = 0x61;
    set_phy_reg(PLL_SET2, pll_set2);

    /* pll_set1 */
    pll_set1 = mipi_tx_drv_get_phy_pll_set1(phy_data_rate);
    set_phy_reg(PLL_SET1, pll_set1);

    /* pll_set4 */
    pll_set4 = mipi_tx_drv_get_phy_pll_set4(phy_data_rate);
    set_phy_reg(PLL_SET4, pll_set4);

#ifdef HI_FPGA
    pll_set3 = 0x1;
    set_phy_reg(PLL_SET3, pll_set3);
#endif

#ifdef MIPI_TX_DEBUG
    osal_printk("\n==========phy pll info=======\n");
    osal_printk("pll_set0(0x14): 0x%x\n", pll_set0);
    osal_printk("pll_set1(0x15): 0x%x\n", pll_set1);
    osal_printk("pll_set2(0x16): 0x%x\n", pll_set2);
#ifdef HI_FPGA
    osal_printk("pll_set3(0x17): 0x%x\n", pll_set3);
#endif
    osal_printk("pll_set4(0x1e): 0x%x\n", pll_set4);
    osal_printk("=========================\n");
#endif
}

/*
 * get global operation timing parameters.
 */
static void mipi_tx_drv_get_phy_timing_parameters(mipi_tx_phy_timing_parameters *tp)
{
    unsigned int temp0;
    unsigned int temp1;

    /* DATA0~3 TPRE-DELAY */
    tp->data_tpre_delay = (actual_phy_data_rate * TPRE_DELAY + 7999) / 8000 - 1; /* 9 */

    /* CLK_TLPX */
    tp->clk_tlpx = (actual_phy_data_rate * TLPX + 7999) / 8000 - 1; /* 6 */

    /* CLK_TCLK_PREPARE */
    temp0 = ((actual_phy_data_rate * TCLK_PREPARE + 7999) / 8000 - 1 + ((actual_phy_data_rate + 799) / 800)
             - ((((actual_phy_data_rate * TCLK_PREPARE + 7999) / 8000 - 1 + ((actual_phy_data_rate + 799) / 800) + 1) *
                         8000 - 10 * actual_phy_data_rate - TCLK_PREPARE * actual_phy_data_rate) / 8000));
    if (temp0 > 0) {
        temp1 = temp0;
    } else {
        temp1 = 0;
    }

    if (((temp1 + 1) * 8000 - 10 * actual_phy_data_rate) > 94 * actual_phy_data_rate) {
        if (temp0 > 0) {
            tp->clk_tclk_prepare = temp0 - 1;
        } else {
            tp->clk_tclk_prepare = -1;
            HI_MIPI_TX_ERR("err when calc phy timing \n");
        }
    } else {
        if (temp0 > 0) {
            tp->clk_tclk_prepare = temp0;
        } else {
            tp->clk_tclk_prepare = 0;
        }
    }

    /* CLK_TCLK_ZERO */
    if ((actual_phy_data_rate * TCLK_ZERO + 7999) / 8000 > 5) {
        tp->clk_tclk_zero = (actual_phy_data_rate * TCLK_ZERO + 7999) / 8000 - 5; /* 19 = 0x13 */
    } else {
        tp->clk_tclk_zero = 0;
    }

    /* CLK_TCLK_TRAIL */
    tp->clk_tclk_trail = (actual_phy_data_rate * TCLK_TRAIL + 7999) / 8000 + 3;      /* 6 */

    /* DATA_TLPX */
    tp->data_tlpx = (actual_phy_data_rate * TLPX + 7999) / 8000 - 1;                  /* 6 */

    /* DATA_THS_PREPARE */
    temp0 = ((actual_phy_data_rate * THS_PREPARE + 7999) / 8000 - 1 + ((actual_phy_data_rate + 799) / 800)
             - ((((actual_phy_data_rate * THS_PREPARE + 7999) / 8000 - 1 + ((actual_phy_data_rate + 799) / 800) + 1) *
                         8000 - 10 * actual_phy_data_rate - THS_PREPARE * actual_phy_data_rate) / 8000));
    if (temp0 > 0) {
        temp1 = temp0;
    } else {
        temp1 = 0;
    }

    if ((actual_phy_data_rate > 105)&&
    (((temp1 + 1) * 8000-10 * actual_phy_data_rate) > (85 * actual_phy_data_rate + 6 * 1000))) {
    if (temp0 > 0) {
        tp->data_ths_prepare = temp0 - 1;
    } else {
            tp->data_ths_prepare = -1;
            HI_MIPI_TX_ERR("err when calc phy timing \n");
    }
    } else {
        if (temp0 > 0) {
            tp->data_ths_prepare = temp0;
        } else {
            tp->data_ths_prepare = 0;
        }
    }
    /* DATA_THS_ZERO */
    if ((actual_phy_data_rate * THS_ZERO + 7999) / 8000 > 5) {
    tp->data_ths_zero = (actual_phy_data_rate * THS_ZERO + 7999) / 8000 - 5; /* 7 */
    } else {
        tp->data_ths_zero = 0;
    }

    /* DATA_THS_TRAIL */
    tp->data_ths_trail = (actual_phy_data_rate * THS_TRAIL + 7999) / 8000 + 3;        /* 6 */
}


/*
 * set global operation timing parameters.
 */
static void mipi_tx_drv_set_phy_timing_parameters(mipi_tx_phy_timing_parameters *tp)
{
    unsigned char data_tpre_delay = tp->data_tpre_delay;
    unsigned char clk_tlpx = tp->clk_tlpx;
    unsigned char clk_tclk_prepare = tp->clk_tclk_prepare;
    unsigned char clk_tclk_zero = tp->clk_tclk_zero;
    unsigned char clk_tclk_trail = tp->clk_tclk_trail;
    unsigned char data_tlpx = tp->data_tlpx;
    unsigned char data_ths_prepare = tp->data_ths_prepare;
    unsigned char data_ths_zero = tp->data_ths_zero;
    unsigned char data_ths_trail = tp->data_ths_trail;

    /* DATA0~3 TPRE-DELAY */
    set_phy_reg(DATA0_TPRE_DELAY, data_tpre_delay);
    set_phy_reg(DATA1_TPRE_DELAY, data_tpre_delay);
    set_phy_reg(DATA2_TPRE_DELAY, data_tpre_delay);
    set_phy_reg(DATA3_TPRE_DELAY, data_tpre_delay);

    /* CLK_TLPX */
    set_phy_reg(CLK_TLPX, clk_tlpx);

    /* CLK_TCLK_PREPARE */
    set_phy_reg(CLK_TCLK_PREPARE, clk_tclk_prepare);

    /* CLK_TCLK_ZERO */
    set_phy_reg(CLK_TCLK_ZERO, clk_tclk_zero);

    /* CLK_TCLK_TRAIL */
    set_phy_reg(CLK_TCLK_TRAIL, clk_tclk_trail);

    /*
     * DATA_TLPX
     * DATA_THS_PREPARE
     * DATA_THS_ZERO
     * DATA_THS_TRAIL
     */
    set_phy_reg(DATA0_TLPX, data_tlpx);
    set_phy_reg(DATA0_THS_PREPARE, data_ths_prepare);
    set_phy_reg(DATA0_THS_ZERO, data_ths_zero);
    set_phy_reg(DATA0_THS_TRAIL, data_ths_trail);
    set_phy_reg(DATA1_TLPX, data_tlpx);
    set_phy_reg(DATA1_THS_PREPARE, data_ths_prepare);
    set_phy_reg(DATA1_THS_ZERO, data_ths_zero);
    set_phy_reg(DATA1_THS_TRAIL, data_ths_trail);
    set_phy_reg(DATA2_TLPX, data_tlpx);
    set_phy_reg(DATA2_THS_PREPARE, data_ths_prepare);
    set_phy_reg(DATA2_THS_ZERO, data_ths_zero);
    set_phy_reg(DATA2_THS_TRAIL, data_ths_trail);
    set_phy_reg(DATA3_TLPX, data_tlpx);
    set_phy_reg(DATA3_THS_PREPARE, data_ths_prepare);
    set_phy_reg(DATA3_THS_ZERO, data_ths_zero);
    set_phy_reg(DATA3_THS_TRAIL, data_ths_trail);

#ifdef MIPI_TX_DEBUG
    osal_printk("\n==========phy timing parameters=======\n");
    osal_printk("data_tpre_delay(0x30/40/50/60): 0x%x\n", data_tpre_delay);
    osal_printk("clk_tlpx(0x22): 0x%x\n", clk_tlpx);
    osal_printk("clk_tclk_prepare(0x23): 0x%x\n", clk_tclk_prepare);
    osal_printk("clk_tclk_zero(0x24): 0x%x\n", clk_tclk_zero);
    osal_printk("clk_tclk_trail(0x25): 0x%x\n", clk_tclk_trail);
    osal_printk("data_tlpx(0x32/42/52/62): 0x%x\n", data_tlpx);
    osal_printk("data_ths_prepare(0x33/43/53/63): 0x%x\n", data_ths_prepare);
    osal_printk("data_ths_zero(0x34/44/54/64): 0x%x\n", data_ths_zero);
    osal_printk("data_ths_trail(0x35/45/55/65): 0x%x\n", data_ths_trail);
    osal_printk("=========================\n");
#endif
}

/*
 * set data lp2hs,hs2lp time
 * set clk lp2hs,hs2lp time
 * unit: hsclk
 */
static void mipi_tx_drv_set_phy_hs_lp_switch_time(mipi_tx_phy_timing_parameters *tp)
{
    unsigned char data_tpre_delay = tp->data_tpre_delay;
    unsigned char clk_tlpx = tp->clk_tlpx;
    unsigned char clk_tclk_prepare = tp->clk_tclk_prepare;
    unsigned char clk_tclk_zero = tp->clk_tclk_zero;
    unsigned char data_tlpx = tp->data_tlpx;
    unsigned char data_ths_prepare = tp->data_ths_prepare;
    unsigned char data_ths_zero = tp->data_ths_zero;
    unsigned char data_ths_trail = tp->data_ths_trail;

    /* data lp2hs,hs2lp time */
    mipi_tx_regs_va->PHY_TMR_CFG.u32 = ((data_ths_trail - 2) << 16) +
        data_tpre_delay + data_tlpx + data_ths_prepare + data_ths_zero + 8;
    /* clk lp2hs,hs2lp time */
    mipi_tx_regs_va->PHY_TMR_LPCLK_CFG.u32 = ((30 + data_ths_trail) << 16) +
        clk_tlpx + clk_tclk_prepare + clk_tclk_zero + 7;

#ifdef MIPI_TX_DEBUG
    osal_printk("PHY_TMR_CFG(0x9C): 0x%x\n", mipi_tx_regs_va->PHY_TMR_CFG.u32);
    osal_printk("PHY_TMR_LPCLK_CFG(0x98): 0x%x\n", mipi_tx_regs_va->PHY_TMR_LPCLK_CFG.u32);
#endif
}

void mipi_tx_drv_set_phy_cfg(combo_dev_cfg_t *dev_cfg)
{
    unsigned char pll_set12;

    mipi_tx_phy_timing_parameters tp = {0};

    /* disable input */
    mipi_tx_regs_va->OPERATION_MODE.u32 = 0x0;

    /* pwr_up unreset */
    mipi_tx_regs_va->PWR_UP.u32 = 0x0;
    osal_udelay(1);
    mipi_tx_regs_va->PWR_UP.u32 = 0x1;


    /* stop phy driver output */
    pll_set12 = 0x20;
    set_phy_reg(PLL_SET12, pll_set12);
    pll_set12 = 0x30;
    set_phy_reg(PLL_SET12, pll_set12);

    /* step1: phy exit ulps mode */
    mipi_tx_regs_va->PHY_ULPS_CTRL.u32 = 0x0;

    /* phy power down */
    mipi_tx_regs_va->PHY_RSTZ.u32 = 0x0;
    osal_udelay(5);

    /* set phy pll parameters setx */
    mipi_tx_drv_set_phy_pll_setx(dev_cfg->phy_data_rate);

    /* get global operation timing parameters */
    mipi_tx_drv_get_phy_timing_parameters(&tp);

    /* set global operation timing parameters */
    mipi_tx_drv_set_phy_timing_parameters(&tp);

    /* set hs switch to lp and lp switch to hs time  */
    mipi_tx_drv_set_phy_hs_lp_switch_time(&tp);

    osal_msleep(1);
}

static void set_output_format(combo_dev_cfg_t *dev_cfg)
{
    int color_coding = 0;

    if (dev_cfg->output_mode == OUTPUT_MODE_CSI) {
        if (dev_cfg->output_format == OUT_FORMAT_YUV420_8_BIT_NORMAL) {
            color_coding = 0xd;
        } else if (dev_cfg->output_format == OUT_FORMAT_YUV422_8_BIT) {
            color_coding = 0x1;
        }
    } else {
        if (dev_cfg->output_format == OUT_FORMAT_RGB_16_BIT) {
            color_coding = 0x0;
        } else if (dev_cfg->output_format == OUT_FORMAT_RGB_18_BIT) {
            color_coding = 0x3;
        } else if (dev_cfg->output_format == OUT_FORMAT_RGB_24_BIT) {
            color_coding = 0x5;
        }
    }

    mipi_tx_regs_va->COLOR_CODING.u32 = color_coding;
#ifdef MIPI_TX_DEBUG
    osal_printk("set_output_format: 0x%x\n", color_coding);
#endif
}

static void set_video_mode_cfg(combo_dev_cfg_t *dev_cfg)
{
    int video_mode;

    if (dev_cfg->video_mode == NON_BURST_MODE_SYNC_PULSES) {
        video_mode = 0;
    } else if (dev_cfg->video_mode == NON_BURST_MODE_SYNC_EVENTS) {
        video_mode = 1;
    } else {
        video_mode = 2;
    }

    if ((dev_cfg->output_mode == OUTPUT_MODE_CSI) || (dev_cfg->output_mode == OUTPUT_MODE_DSI_CMD)) {
        video_mode = 2;
    }

    mipi_tx_regs_va->VID_MODE_CFG.u32 = 0x3f00 + video_mode;
}

static void set_timing_config(combo_dev_cfg_t *dev_cfg)
{
    unsigned int hsa_time;
    unsigned int hbp_time;
    unsigned int hline_time;

    if (dev_cfg->pixel_clk == 0) {
        osal_printk("dev_cfg->pixel_clk is 0, illegal.\n");
        return;
    }

    hsa_time = actual_phy_data_rate * dev_cfg->sync_info.vid_hsa_pixels * 125 / dev_cfg->pixel_clk;
    hbp_time = actual_phy_data_rate * dev_cfg->sync_info.vid_hbp_pixels  * 125 / dev_cfg->pixel_clk;
    hline_time = actual_phy_data_rate * dev_cfg->sync_info.vid_hline_pixels  * 125 / dev_cfg->pixel_clk;

    mipi_tx_regs_va->VID_HSA_TIME.u32 = hsa_time;
    mipi_tx_regs_va->VID_HBP_TIME.u32 = hbp_time;
    mipi_tx_regs_va->VID_HLINE_TIME.u32 = hline_time;

    mipi_tx_regs_va->VID_VSA_LINES.u32 = dev_cfg->sync_info.vid_vsa_lines;
    mipi_tx_regs_va->VID_VBP_LINES.u32 = dev_cfg->sync_info.vid_vbp_lines;
    mipi_tx_regs_va->VID_VFP_LINES.u32 = dev_cfg->sync_info.vid_vfp_lines;
    mipi_tx_regs_va->VID_VACTIVE_LINES.u32 = dev_cfg->sync_info.vid_active_lines;

#ifdef MIPI_TX_DEBUG
    osal_printk("VID_HSA_TIME(0x48): 0x%x\n", hsa_time);
    osal_printk("VID_HBP_TIME(0x4c): 0x%x\n", hbp_time);
    osal_printk("VID_HLINE_TIME(0x50): 0x%x\n", hline_time);
    osal_printk("VID_VSA_LINES(0x54): 0x%x\n", dev_cfg->sync_info.vid_vsa_lines);
    osal_printk("VID_VBP_LINES(0x58): 0x%x\n", dev_cfg->sync_info.vid_vbp_lines);
    osal_printk("VID_VFP_LINES(0x5c): 0x%x\n", dev_cfg->sync_info.vid_vfp_lines);
    osal_printk("VID_VACTIVE_LINES(0x60): 0x%x\n", dev_cfg->sync_info.vid_active_lines);
#endif
}

void set_lane_config(short lane_id[])
{
    int lane_num = 0;
    int i;
    U_LANE_ID lane_id_reg;

    for (i = 0; i < LANE_MAX_NUM; i++) {
        if (-1 != lane_id[i]) {
            lane_num++;
        }
    }

    mipi_tx_regs_va->PHY_IF_CFG.u32 = lane_num - 1;

    lane_id_reg.u32 = mipi_tx_regs_va->LANE_ID.u32;
    if (lane_id[0] >= 0) {
        lane_id_reg.bits.lane0_id = lane_id[0];
    }
    if (lane_id[1] >= 0) {
        lane_id_reg.bits.lane1_id = lane_id[1];
    }
    if (lane_id[2] >= 0) {
        lane_id_reg.bits.lane2_id = lane_id[2];
    }
    if (lane_id[3] >= 0) {
        lane_id_reg.bits.lane3_id = lane_id[3];
    }
    mipi_tx_regs_va->LANE_ID.u32 = lane_id_reg.u32;

#ifdef MIPI_TX_DEBUG
    osal_printk("LANE_ID(0x250): 0x%x\n", lane_id_reg);
#endif
}

int mipi_tx_drv_get_phy_status_phy_stopstateclk(void);
int mipi_tx_drv_get_phy_status_phy_hsstateclk(void);

void mipi_tx_drv_phy_reset(void)
{
    mipi_tx_regs_va->PHY_RSTZ.bits.phy_rstz = 0x0;
}

void mipi_tx_drv_phy_unreset(void)
{
    mipi_tx_regs_va->PHY_RSTZ.bits.phy_rstz = 0x1;
}

void mipi_tx_check_clkstopstate(void)
{
    int i = 0;
    unsigned int trytimes = 4;
    while (i < trytimes) {
        if (mipi_tx_drv_get_phy_status_phy_stopstateclk() == 1) {
            /* check clock stop */
            break;
        } else {
            osal_udelay(5);
        }
        i++;
    }
    if (i == trytimes) {
        osal_printk("%s,%d check clkstop state failed.\n", __FUNCTION__, __LINE__);
    }
}

void mipi_tx_check_clkhsstate(void)
{
    int i = 0;
    unsigned int trytimes = 4;
    while (i < trytimes) {
        if (mipi_tx_drv_get_phy_status_phy_hsstateclk() == 0) {
            /* check clock hs */
            break;
        } else {
            mipi_tx_regs_va->LPCLK_CTRL.u32 = 0x0;
            osal_udelay(1);
            mipi_tx_regs_va->LPCLK_CTRL.u32 = 0x1;
            osal_udelay(5);
        }
        i++;
    }
    if (i == trytimes) {
        osal_printk("%s,%d check clk hs state failed.\n", __FUNCTION__, __LINE__);
    }
}

void mipi_tx_drv_set_clkmgr_cfg(void)
{
    if (actual_phy_data_rate / 160 < 2) {
        mipi_tx_regs_va->CLKMGR_CFG.u32 = 0x102;
    } else {
        mipi_tx_regs_va->CLKMGR_CFG.u32 = 0x100 + (actual_phy_data_rate + 159) / 160;
    }
}

void mipi_tx_drv_set_controller_cfg(combo_dev_cfg_t *dev_cfg)
{
    /* vc_id */
    mipi_tx_regs_va->VCID.u32 = 0x0;

    /* output format */
    set_output_format(dev_cfg);

    /* txescclk,timeout */
    mipi_tx_drv_set_clkmgr_cfg();

    /* cmd transmission mode */
    mipi_tx_regs_va->CMD_MODE_CFG.u32 = 0xffffff00;

    /* crc,ecc,eotp tran */
    mipi_tx_regs_va->PCKHDL_CFG.u32 = 0x1e;
    /* gen_vcid_rx */
    mipi_tx_regs_va->GEN_VCID.u32 = 0x0;

    /* mode config */
    mipi_tx_regs_va->MODE_CFG.u32 = 0x1;

    /* video mode cfg */
    set_video_mode_cfg(dev_cfg);
    if ((dev_cfg->output_mode == OUTPUT_MODE_DSI_VIDEO) || (dev_cfg->output_mode == OUTPUT_MODE_CSI)) {
        mipi_tx_regs_va->VID_PKT_SIZE.u32 = dev_cfg->sync_info.vid_pkt_size;
    } else {
        mipi_tx_regs_va->EDPI_CMD_SIZE.u32 = dev_cfg->sync_info.edpi_cmd_size;
    }

    /* num_chunks/null_size */
    mipi_tx_regs_va->VID_NUM_CHUNKS.u32 = 0x0;
    mipi_tx_regs_va->VID_NULL_SIZE.u32 = 0x0;

    /* timing config */
    set_timing_config(dev_cfg);

    /* invact,outvact time */
    mipi_tx_regs_va->LP_CMD_TIM.u32 = 0x0;

    /* lp_wr_to_cnt */
    mipi_tx_regs_va->LP_WR_TO_CNT.u32 = 0x0;
    /* bta_to_cnt */
    mipi_tx_regs_va->BTA_TO_CNT.u32 = 0x0;

    /* phy_txrequlpsclk */
    mipi_tx_regs_va->PHY_ULPS_CTRL.u32 = 0x0;

    if (dev_cfg->output_mode == OUTPUT_MODE_CSI) {
        if (dev_cfg->output_format == OUT_FORMAT_YUV420_8_BIT_NORMAL) {
            mipi_tx_regs_va->DATATYPE0.u32 = 0x10218;
            mipi_tx_regs_va->DATATYPE1 = 0x31081909;
            mipi_tx_regs_va->CSI_CTRL.u32 = 0x1111;
        } else if (dev_cfg->output_format == OUT_FORMAT_YUV422_8_BIT) {
            mipi_tx_regs_va->DATATYPE0.u32 = 0x1021E;
            mipi_tx_regs_va->DATATYPE1 = 0x31081909;
            mipi_tx_regs_va->CSI_CTRL.u32 = 0x1111;
        }
    }

    /* pll status check program */
    osal_udelay(100);
    g_phy_reset_count = 0;
    mipi_tx_pll_status_check(dev_cfg);

    /* lanes */
    set_lane_config(dev_cfg->lane_id);

    mipi_tx_regs_va->LPCLK_CTRL.u32 = 0x0;
}

static int mipi_tx_wait_cmd_fifo_empty(void)
{
    U_CMD_PKT_STATUS cmd_pkt_status;
    unsigned int wait_count;

    wait_count = 0;
    do {
        cmd_pkt_status.u32 = mipi_tx_regs_va->CMD_PKT_STATUS.u32;

        wait_count++;

        osal_udelay(1);

        if (wait_count >  MIPI_TX_READ_TIMEOUT_CNT) {
            HI_MIPI_TX_ERR("timeout when send cmd buffer \n");
            return -1;
        }
    } while (cmd_pkt_status.bits.gen_cmd_empty == 0);

    return 0;
}

static int mipi_tx_wait_write_fifo_empty(void)
{
    U_CMD_PKT_STATUS cmd_pkt_status;
    unsigned int wait_count;

    wait_count = 0;
    do {
        cmd_pkt_status.u32 = mipi_tx_regs_va->CMD_PKT_STATUS.u32;

        wait_count++;

        osal_udelay(1);

        if (wait_count >  MIPI_TX_READ_TIMEOUT_CNT) {
            HI_MIPI_TX_ERR("timeout when send data buffer \n");
            return -1;
        }
    } while (cmd_pkt_status.bits.gen_pld_w_empty == 0);

    return 0;
}

static int mipi_tx_wait_write_fifo_not_full(void)
{
    U_CMD_PKT_STATUS cmd_pkt_status;
    unsigned int wait_count;

    wait_count = 0;
    do {
        cmd_pkt_status.u32 = mipi_tx_regs_va->CMD_PKT_STATUS.u32;
        if(wait_count > 0) {
            osal_udelay(1);
            HI_MIPI_TX_ERR("write fifo full happened wait count = %d\n",wait_count);
        }
        if (wait_count >  MIPI_TX_READ_TIMEOUT_CNT) {
            HI_MIPI_TX_ERR("timeout when wait write fifo not full buffer \n");
            return -1;
        }
        wait_count++;
    } while (cmd_pkt_status.bits.gen_pld_w_full == 1);

    return 0;
}

/*
 * set payloads data by writing register
 * each 4 bytes in cmd corresponds to one register
 */
static void mipi_tx_drv_set_payload_data(unsigned char *cmd, unsigned short cmd_size)
{
    U_GEN_PLD_DATA gen_pld_data;
    int i, j;

    gen_pld_data.u32 = mipi_tx_regs_va->GEN_PLD_DATA.u32;

    for (i = 0; i < (cmd_size / 4); i++) {
        gen_pld_data.bits.gen_pld_b1 = cmd[i * 4];
        gen_pld_data.bits.gen_pld_b2 = cmd[i * 4 + 1];
        gen_pld_data.bits.gen_pld_b3 = cmd[i * 4 + 2];
        gen_pld_data.bits.gen_pld_b4 = cmd[i * 4 + 3];

        mipi_tx_wait_write_fifo_not_full();
        mipi_tx_regs_va->GEN_PLD_DATA.u32 = gen_pld_data.u32;
    }

    j = cmd_size % 4;
    if (j != 0) {
        if (j > 0) {
            gen_pld_data.bits.gen_pld_b1 = cmd[i * 4];
        }
        if (j > 1) {
            gen_pld_data.bits.gen_pld_b2 = cmd[i * 4 + 1];
        }
        if (j > 2) {
            gen_pld_data.bits.gen_pld_b3 = cmd[i * 4 + 2];
        }

        mipi_tx_wait_write_fifo_not_full();
        mipi_tx_regs_va->GEN_PLD_DATA.u32 = gen_pld_data.u32;
    }

#ifdef MIPI_TX_DEBUG
        osal_printk("\n=====set cmd=======\n");
        osal_printk("GEN_PLD_DATA(0x70): 0x%x\n", gen_pld_data);
#endif

    return;
}

int mipi_tx_drv_set_cmd_info(cmd_info_t *cmd_info)
{
    U_GEN_HDR gen_hdr;
    unsigned char *cmd;

    gen_hdr.u32 = mipi_tx_regs_va->GEN_HDR.u32;

    if (cmd_info->cmd != NULL) {
        if (cmd_info->cmd_size > 200 ||cmd_info->cmd_size == 0) {
            HI_MIPI_TX_ERR("set cmd size illegal, size =%d\n",cmd_info->cmd_size);
            return  -1;
        }

        cmd = (unsigned char *)osal_kmalloc(cmd_info->cmd_size, osal_gfp_kernel);
        if (cmd == NULL) {
            HI_MIPI_TX_ERR("kmalloc fail,please check,need %d bytes\n",cmd_info->cmd_size);
            return  -1;
        }

        if (osal_copy_from_user(cmd, cmd_info->cmd, cmd_info->cmd_size)) {
            osal_kfree(cmd);
            return  -1;
        }

        mipi_tx_drv_set_payload_data(cmd, cmd_info->cmd_size);

        osal_kfree(cmd);
    }

    gen_hdr.bits.gen_dt = cmd_info->data_type;
    gen_hdr.bits.gen_wc_lsbyte = cmd_info->cmd_size & 0xff;
    gen_hdr.bits.gen_wc_msbyte = (cmd_info->cmd_size & 0xff00) >> 8;
    mipi_tx_regs_va->GEN_HDR.u32 = gen_hdr.u32;

    /* wait transfer end */
    osal_udelay(350);

    mipi_tx_wait_cmd_fifo_empty();
    mipi_tx_wait_write_fifo_empty();

#ifdef MIPI_TX_DEBUG
    osal_printk("\n=====set cmd=======\n");
    osal_printk("cmd_info->cmd_size: 0x%x\n", cmd_info->cmd_size);
    osal_printk("cmd_info->data_type: 0x%x\n", cmd_info->data_type);
    osal_printk("GEN_HDR(0x6C): 0x%x\n", gen_hdr);
#endif

    return 0;
}

static int mipi_tx_wait_read_fifo_not_empty(void)
{
    U_INT_ST0 int_st0;
    U_INT_ST1 int_st1;
    unsigned int wait_count;
    U_CMD_PKT_STATUS cmd_pkt_status;

    wait_count = 0;
    do {
        int_st1.u32 =  mipi_tx_regs_va->INT_ST1.u32;
        int_st0.u32 =  mipi_tx_regs_va->INT_ST0.u32;

        if ((int_st1.u32 & 0x3e) != 0) {
            HI_MIPI_TX_ERR("err happened when read data, int_st1 = 0x%x,int_st0 = %x\n", int_st1.u32, int_st0.u32);
            return -1;
        }

        if (wait_count >  MIPI_TX_READ_TIMEOUT_CNT) {
            HI_MIPI_TX_ERR("timeout when read data\n");
            return -1;
        }

        wait_count++;

        osal_udelay(1);

        cmd_pkt_status.u32 = mipi_tx_regs_va->CMD_PKT_STATUS.u32;
    } while (cmd_pkt_status.bits.gen_pld_r_empty == 0x1);

    return 0;
}

static int mipi_tx_wait_read_fifo_empty(void)
{
    U_GEN_PLD_DATA pld_data;
    U_INT_ST1 int_st1;
    unsigned int wait_count;

    wait_count = 0;
    do {
        int_st1.u32 = mipi_tx_regs_va->INT_ST1.u32;
        if ((int_st1.bits.gen_pld_rd_err) == 0x0) {
            pld_data.u32 = mipi_tx_regs_va->GEN_PLD_DATA.u32;
        }
        wait_count++;
        osal_udelay(1);
        if (wait_count >  MIPI_TX_READ_TIMEOUT_CNT) {
            HI_MIPI_TX_ERR("timeout when clear data buffer, the last read data is 0x%x \n", pld_data.u32);
            return -1;
        }
    } while ((int_st1.bits.gen_pld_rd_err) == 0x0);

    return 0;
}

static int mipi_tx_send_short_packet(unsigned char virtual_channel, short unsigned data_type, unsigned short  data_param)
{
    U_GEN_HDR gen_hdr;

    gen_hdr.bits.gen_vc = virtual_channel;
    gen_hdr.bits.gen_dt = data_type;
    gen_hdr.bits.gen_wc_lsbyte = (data_param & 0xff);
    gen_hdr.bits.gen_wc_msbyte = (data_param & 0xff00) >> 8;
    mipi_tx_regs_va->GEN_HDR.u32 = gen_hdr.u32;

    if(mipi_tx_wait_cmd_fifo_empty() != 0) {
        return -1;
    }

    return 0;
}

static int mipi_tx_get_read_fifo_data(unsigned int get_data_size, unsigned char *data_buf)
{
    U_GEN_PLD_DATA pld_data;
    unsigned int i, j;

    for (i = 0; i < get_data_size / 4; i++) {
        if (mipi_tx_wait_read_fifo_not_empty() != 0) {
            return -1;
        }
        pld_data.u32 = mipi_tx_regs_va->GEN_PLD_DATA.u32;
        data_buf[i * 4] = pld_data.bits.gen_pld_b1;
        data_buf[i * 4 + 1] = pld_data.bits.gen_pld_b2;
        data_buf[i * 4 + 2] = pld_data.bits.gen_pld_b3;
        data_buf[i * 4 + 3] = pld_data.bits.gen_pld_b4;
    }

    j = get_data_size % 4;

    if (j != 0) {
        if (mipi_tx_wait_read_fifo_not_empty() != 0) {
            return -1;
        }
        pld_data.u32 = mipi_tx_regs_va->GEN_PLD_DATA.u32;
        if (j > 0) {
            data_buf[i * 4] = pld_data.bits.gen_pld_b1;
        }
        if (j > 1) {
            data_buf[i * 4 + 1] = pld_data.bits.gen_pld_b2;
        }
        if (j > 2) {
            data_buf[i * 4 + 2] = pld_data.bits.gen_pld_b3;
        }
    }

    return 0;
}

void mipi_tx_reset(void)
{
    mipi_tx_regs_va->PWR_UP.u32 = 0x0;
    mipi_tx_regs_va->PHY_RSTZ.u32 = 0xd;
    osal_udelay(1);
    mipi_tx_regs_va->PWR_UP.u32 = 0x1;
    mipi_tx_regs_va->PHY_RSTZ.u32 = 0xf;
    osal_udelay(1);
    return;
}

int mipi_tx_drv_get_cmd_info(get_cmd_info_t *get_cmd_info)
{
    unsigned char* data_buf = NULL;

    if (get_cmd_info->get_data_size > 200 || get_cmd_info->get_data_size == 0) {
        HI_MIPI_TX_ERR("get_data_size illegal, size =%d\n",get_cmd_info->get_data_size);
        return -1;
    }

    if (get_cmd_info->get_data == NULL) {
        HI_MIPI_TX_ERR("get_data is NULL,illegal\n");
        return -1;
    }

    data_buf = (unsigned char*)osal_kmalloc(get_cmd_info->get_data_size, osal_gfp_kernel);
    if (data_buf == NULL) {
        return -1;
    }

    if (mipi_tx_wait_read_fifo_empty() != 0) {
        goto fail0;
    }

    if (mipi_tx_send_short_packet(0, get_cmd_info->data_type, get_cmd_info->data_param) != 0) {
        goto fail0;
    }

    if (mipi_tx_get_read_fifo_data(get_cmd_info->get_data_size, data_buf) != 0) {
        /* fail will block mipi data lane ,so need reset  */
        mipi_tx_reset();
        goto fail0;
    }

    osal_copy_to_user(get_cmd_info->get_data, data_buf, get_cmd_info->get_data_size);

    osal_kfree(data_buf);

    return 0;

fail0:
    osal_kfree(data_buf);
    return -1;
}

void mipi_tx_drv_enable_input(output_mode_t output_mode)
{
    if ((output_mode == OUTPUT_MODE_DSI_VIDEO) || (output_mode == OUTPUT_MODE_CSI)) {
        mipi_tx_regs_va->MODE_CFG.u32 = 0x0;
    }

    if (output_mode == OUTPUT_MODE_DSI_CMD) {
        mipi_tx_regs_va->CMD_MODE_CFG.u32 = 0x0;
    }

    osal_udelay(1);
    /* B0 [2] = 1 */
    mipi_tx_check_clkstopstate();
    mipi_tx_regs_va->LPCLK_CTRL.u32 = 0x1;
    osal_udelay(1);
    /* B0 [2] = 0 */
    mipi_tx_check_clkhsstate();

    /* enable input */
    mipi_tx_regs_va->OPERATION_MODE.u32 = 0x80150000;
    /* reset mipi_tx */
    mipi_tx_regs_va->PWR_UP.u32 = 0x0;
    mipi_tx_drv_phy_reset();
    osal_udelay(1);
    mipi_tx_drv_phy_unreset();
    mipi_tx_regs_va->PWR_UP.u32 = 0x1;
}

void mipi_tx_drv_disable_input(void)
{
    /* disable input */
    mipi_tx_regs_va->OPERATION_MODE.u32 = 0x0;

    mipi_tx_regs_va->CMD_MODE_CFG.u32 = 0xffffff00;

    /* command mode */
    mipi_tx_regs_va->MODE_CFG.u32 = 0x1;

    mipi_tx_regs_va->LPCLK_CTRL.u32 = 0x0;

    mipi_tx_reset();
}

int mipi_tx_drv_get_vall_det(unsigned int *vall_det)
{
    *vall_det = mipi_tx_regs_va->VERT_DET.bits.vall_det;
    return 0;
}

int mipi_tx_drv_get_vact_det(unsigned int *vact_det)
{
    *vact_det = mipi_tx_regs_va->VERT_DET.bits.vact_det;
    return 0;
}
int mipi_tx_drv_get_hline_det(unsigned int *hall_det)
{
    *hall_det = mipi_tx_regs_va->HORI0_DET.bits.hline_det;
    return 0;
}

int mipi_tx_drv_get_hact_det(unsigned int *hact_det)
{
    *hact_det = mipi_tx_regs_va->HORI0_DET.bits.hact_det;
    return 0;
}

int mipi_tx_drv_get_hbp_det(unsigned int *hbp_det)
{
    *hbp_det = mipi_tx_regs_va->HORI1_DET.bits.hbp_det;
    return 0;
}

int mipi_tx_drv_get_hsa_det(unsigned int *hsa_det)
{
    *hsa_det = mipi_tx_regs_va->HORI1_DET.bits.hsa_det;
    return 0;
}

int mipi_tx_drv_get_vsa_det(unsigned int *vsa_det)
{
    *vsa_det = mipi_tx_regs_va->VSA_DET.bits.vsa_det;
    return 0;
}

unsigned int mipi_tx_drv_get_phy_status_phy_lock(void)
{
    unsigned int phy_lock;

    phy_lock = mipi_tx_regs_va->PHY_STATUS.bits.phy_lock;
    return phy_lock;
}

static int mipi_tx_drv_reg_init(void)
{
    if (mipi_tx_regs_va == NULL) {
        mipi_tx_regs_va = (volatile mipi_tx_regs_type_t *)osal_ioremap(
            MIPI_TX_REGS_ADDR, (unsigned int)MIPI_TX_REGS_SIZE);
        if (mipi_tx_regs_va == NULL) {
            HI_MIPI_TX_ERR("remap mipi_tx reg addr fail\n");
            return -1;
        }
        regMapFlag = 1;
    }

    return 0;
}

static void mipi_tx_drv_reg_exit(void)
{
    if (regMapFlag == 1) {
        if (mipi_tx_regs_va != NULL) {
            osal_iounmap((void *)mipi_tx_regs_va);
            mipi_tx_regs_va = NULL;
        }
        regMapFlag = 0;
    }
}

static int mipi_tx_enable_int(int enable)
{
    mipi_tx_regs_va->INT_MSK1.bits.mask_vsstart = enable & 0x1;
    return 0;
}
static void mipi_tx_clear_int(void)
{
    int int_stat;

    /* warning: read register to clear interrupt status, do not delete it. */
    int_stat = mipi_tx_regs_va->INT_ST1.u32;
    /* ignore compile warning of variable 'int_stat' set but not used */
    (void)int_stat;
}

static int mipi_tx_interrupt_route(int irq, void* dev_id)
{
    mipi_tx_clear_int();
    mipi_tx_drv_get_vall_det(&mipi_tx_phy_ctx.vall_det);
    mipi_tx_drv_get_vact_det(&mipi_tx_phy_ctx.vact_det);
    mipi_tx_drv_get_hline_det(&mipi_tx_phy_ctx.hall_det);
    mipi_tx_drv_get_hact_det(&mipi_tx_phy_ctx.hact_det);
    mipi_tx_drv_get_hbp_det(&mipi_tx_phy_ctx.hbp_det);
    mipi_tx_drv_get_hsa_det(&mipi_tx_phy_ctx.hsa_det);
    mipi_tx_drv_get_vsa_det(&mipi_tx_phy_ctx.vsa_det);
    return OSAL_IRQ_HANDLED;
}

static int mipi_tx_register_irq(void)
{
    int ret;
    mipi_tx_enable_int(0x1);
    ret = osal_request_irq(mipi_tx_irq_num, mipi_tx_interrupt_route, NULL, "MIPI_TX", mipi_tx_interrupt_route);
    if (ret < 0) {
        HI_MIPI_TX_ERR("mipi_tx: failed to register irq.\n");
        return -1;
    }

    return 0;
}

static void mipi_tx_unregister_irq(void)
{
    osal_free_irq(mipi_tx_irq_num, mipi_tx_interrupt_route);

    mipi_tx_enable_int(0x0);
}

static void mipi_tx_drv_hw_init(void)
{
    unsigned long mipi_tx_crg_addr;

    mipi_tx_crg_addr = (unsigned long)osal_ioremap(MIPI_TX_CRG, (unsigned long)0x4);

    /* mipi_tx gate clk enable */
    write_reg32(mipi_tx_crg_addr, 1, 0x1);

    /* unreset */
    write_reg32(mipi_tx_crg_addr, 0, 0x1 << 2);

    /* ref clk */
    write_reg32(mipi_tx_crg_addr, 1 << 4, 0x1 << 4);

    osal_iounmap((void *)mipi_tx_crg_addr);
}

int hal_phy_getphy_statusphy_stopstate_xlane(unsigned int x, unsigned int *phy_stopstate_xlane)
{
    switch (x) {
        /* 0lane */
        case 0:
            *phy_stopstate_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_stopstate0lane;
            break;
        /* 1lane */
        case 1:
            *phy_stopstate_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_stopstate1lane;
            break;
        /* 2lane */
        case 2:
            *phy_stopstate_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_stopstate2lane;
            break;
        /* 3lane */
        case 3:
            *phy_stopstate_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_stopstate3lane;
            break;
        default:
            /* do nothing. */
            break;
    }
    return 1;
}

int hal_phy_getphy_statusphy_stopstateclk(unsigned int *phy_stopstateclk)
{
    *phy_stopstateclk = mipi_tx_regs_va->PHY_STATUS.bits.phy_stopstateclklane;
    return 1;
}

int hal_phy_getphy_statusphy_hsstateclk(unsigned int *phy_hsstateclk)
{
    *phy_hsstateclk = mipi_tx_regs_va->PHY_STATUS.bits.phy_stopstateclklane;
    return 1;
}

int hal_phy_setphy_ulps_ctrlphy_txrequlpslan(unsigned int uphy_txrequlpslan)
{
    mipi_tx_regs_va->PHY_ULPS_CTRL.bits.phy_txrequlpslan = uphy_txrequlpslan;
    return 1;
}


int hal_phy_getphy_statusphy_ulpsactivenot_xlane(unsigned int x, unsigned int *phy_ulpsactivenot_xlane)
{
    switch (x) {
        /* 0lane */
        case 0:
            *phy_ulpsactivenot_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_ulpsactivenot0lane;
            break;
        /* 1lane */
        case 1:
            *phy_ulpsactivenot_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_ulpsactivenot1lane;
            break;
        /* 2lane */
        case 2:
            *phy_ulpsactivenot_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_ulpsactivenot2lane;
            break;
        /* 3lane */
        case 3:
            *phy_ulpsactivenot_xlane = mipi_tx_regs_va->PHY_STATUS.bits.phy_ulpsactivenot3lane;
            break;
        default:
            /* do nothing. */
            break;
    }
    return 1;
}

int hal_phy_setphy_ulps_ctrlphy_txexitulpslan(unsigned int uphy_txexitulpslan)
{
    mipi_tx_regs_va->PHY_ULPS_CTRL.bits.phy_txexitulpslan = uphy_txexitulpslan;
    return 1;
}


int hal_phy_getphy_statusphy_ulpsactivenotclk(unsigned int *phy_ulpsactivenotclk)
{
    *phy_ulpsactivenotclk = mipi_tx_regs_va->PHY_STATUS.bits.phy_ulpsactivenotclk;
    return 1;
}


int hal_phy_setphy_ulps_ctrlphy_txrequlpsclk(unsigned int uphy_txrequlpsclk)
{
    mipi_tx_regs_va->PHY_ULPS_CTRL.bits.phy_txrequlpsclk = uphy_txrequlpsclk;
    return 1;
}

int hal_phy_setphy_ulps_ctrlphy_txexitulpsclk(unsigned int uphy_txexitulpsclk)
{
    mipi_tx_regs_va->PHY_ULPS_CTRL.bits.phy_txexitulpsclk = uphy_txexitulpsclk;
    return 1;
}

int hal_phy_getphy_status(unsigned int *phy_status)
{
    *phy_status = mipi_tx_regs_va->PHY_STATUS.u32;
    return 1;
}


int mipi_tx_drv_get_phy_status_phy_stopstate_xlane(unsigned int xlane)
{
    unsigned int phy_stopstate_xlane = 0;

    hal_phy_getphy_statusphy_stopstate_xlane(xlane, &phy_stopstate_xlane);
    return phy_stopstate_xlane;
}

int mipi_tx_drv_get_phy_status_phy_stopstateclk(void)
{
    unsigned int phy_stopstateclk = 0;

    hal_phy_getphy_statusphy_stopstateclk(&phy_stopstateclk);
    return phy_stopstateclk;
}

int mipi_tx_drv_get_phy_status_phy_hsstateclk(void)
{
    unsigned int phy_hsstateclk = 0;

    hal_phy_getphy_statusphy_hsstateclk(&phy_hsstateclk);
    return phy_hsstateclk;
}

unsigned int mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(unsigned int xlane)
{
    unsigned int phy_ulpsactivenot_xlane = 0;

    hal_phy_getphy_statusphy_ulpsactivenot_xlane(xlane, &phy_ulpsactivenot_xlane);
    return phy_ulpsactivenot_xlane;
}

void mipi_tx_drv_set_ulps_mode(void)
{
    hal_phy_setphy_ulps_ctrlphy_txrequlpslan(0x1);
}

void mipi_tx_drv_exit_ulps_mode(void)
{
    hal_phy_setphy_ulps_ctrlphy_txrequlpslan(0x1);
    hal_phy_setphy_ulps_ctrlphy_txexitulpslan(0x1);
    hal_phy_setphy_ulps_ctrlphy_txrequlpsclk(0x0);
    hal_phy_setphy_ulps_ctrlphy_txexitulpsclk(0x0);
}


unsigned int mipi_tx_drv_get_phy_status_phy_stopstateclklane(void)
{
    unsigned int phy_ulpsactivenotclk = 0;

    hal_phy_getphy_statusphy_ulpsactivenotclk(&phy_ulpsactivenotclk);

    return phy_ulpsactivenotclk;
}

static unsigned int mipi_tx_check_phy_lock(unsigned int try_times)
{
    unsigned int i = 0;

    while (i < try_times) {
        if (mipi_tx_drv_get_phy_status_phy_lock() == 1) {
            /* check phy lock */
            break;
        } else {
            osal_udelay(15);
        }
        i++;
    }
    return i;
}

static unsigned int mipi_tx_check_datalane_stopstatus(unsigned int try_times)
{
    unsigned int i = 0;
    while (i < try_times) {
        if ((mipi_tx_drv_get_phy_status_phy_stopstate_xlane(0) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_stopstate_xlane(1) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_stopstate_xlane(2) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_stopstate_xlane(3) == 1)) {
            /* check data lane stop status */
            break;
        } else {
            osal_udelay(15);
        }
        i++;
    }
    return i;
}

static unsigned int mipi_tx_check_datalane_ulpsstatus(unsigned int try_times)
{
    unsigned int i = 0;
    while (i < try_times) {
        if ((mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(0) == 0) &&
            (mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(1) == 0) &&
            (mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(2) == 0) &&
            (mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(3) == 0)) {
            /* check data lane ulps status */
            break;
        } else {
            osal_udelay(15);
        }
        i++;
    }
    return i;
}

static unsigned int mipi_tx_check_datalane_exitulpsmode(unsigned int try_times)
{
    unsigned int i = 0;
    while (i < try_times) {
        if ((mipi_tx_drv_get_phy_status_phy_stopstateclklane() == 1) &&
            (mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(0) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(1) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(2) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_ulpsactivenot_xlane(3) == 1)) {
            /* check clklane and datalane exit ulps mode. */
            break;
        } else {
            osal_udelay(15);
        }
        i++;
    }
    return i;
}

static unsigned int mipi_tx_check_datalane_stopxlane(unsigned int try_times)
{
    unsigned int i = 0;
    while (i < try_times) {
        if ((mipi_tx_drv_get_phy_status_phy_stopstate_xlane(0) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_stopstate_xlane(1) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_stopstate_xlane(2) == 1) &&
            (mipi_tx_drv_get_phy_status_phy_stopstate_xlane(3) == 1)) {
            /* check phy lock */
            break;
        } else {
            osal_udelay(15);
        }
        i++;
    }
    return i;
}

static void mipi_tx_drv_enter_ulps_mode(void)
{
    hal_phy_setphy_ulps_ctrlphy_txrequlpslan(0x0);
    hal_phy_setphy_ulps_ctrlphy_txexitulpslan(0x0);
    hal_phy_setphy_ulps_ctrlphy_txrequlpsclk(0x0);
    hal_phy_setphy_ulps_ctrlphy_txexitulpsclk(0x0);
}

static void mipi_tx_drv_set_phy_lptx_force_value(void)
{
    unsigned int pll_set12;

    /* step11 */
    pll_set12 = 0x20;
    set_phy_reg(PLL_SET12, pll_set12);

    /* step 12 */
    pll_set12 = 0x0;
    set_phy_reg(PLL_SET12, pll_set12);
}


void mipi_tx_pll_status_check(combo_dev_cfg_t *dev_cfg)
{
    unsigned int i;
    unsigned int trytimes  = 4;
    int          count     = 0;

    while (count < MIPI_TX_MAX_RESET_PHY_TIMES) {
        /* step2 */
        /* step3 */
        i = mipi_tx_check_phy_lock(trytimes);
        if (i == trytimes) {
            /* reset phy */
            count = mipi_tx_drv_reset_phy(dev_cfg);
            /* goto step2 */
            continue;
        }

        /* step 4 */
        i = mipi_tx_check_datalane_stopstatus(trytimes);
        if (i == trytimes) {
            /* reset phy */
            count = mipi_tx_drv_reset_phy(dev_cfg);
            /* goto step2 */
            continue;
        }

        /* step 5 */
        mipi_tx_drv_set_ulps_mode();

        /* step 6 */
        i = mipi_tx_check_datalane_ulpsstatus(trytimes);
        if (i == trytimes) {
            /* reset phy */
            count = mipi_tx_drv_reset_phy(dev_cfg);
            /* goto step2 */
            continue;
        }

        /* step 7 exit ulps mode */
        mipi_tx_drv_exit_ulps_mode();

        /* step 8 */
        i = mipi_tx_check_datalane_exitulpsmode(trytimes);
        if (i == trytimes) {
            /* reset phy */
            count = mipi_tx_drv_reset_phy(dev_cfg);
            /* goto step2 */
            continue;
        }

        /* step 9 */
        mipi_tx_drv_enter_ulps_mode();

        /* step 10 */
        i = mipi_tx_check_datalane_stopxlane(trytimes);
        if (i == trytimes) {
            /* reset phy */
            count = mipi_tx_drv_reset_phy(dev_cfg);
            /* goto step2 */
            continue;
        }

        /* step11, step 12  */
        mipi_tx_drv_set_phy_lptx_force_value();

        break;
    }

    if (count < MIPI_TX_MAX_RESET_PHY_TIMES) {
        osal_printk("[%s][%d]check pll status success.\n", __FUNCTION__, __LINE__);
    } else {
#ifdef MIPI_TX_DEBUG
        osal_printk("[%s][%d]check pll status failed.\n", __FUNCTION__, __LINE__);
#endif
    }
    return;
}

int mipi_tx_drv_init(void)
{
    int ret;

    ret = mipi_tx_drv_reg_init();
    if (ret < 0) {
        HI_MIPI_TX_ERR("mipi_tx_drv_reg_init fail!\n");
        goto fail0;
    }

    ret = mipi_tx_register_irq();
    if (ret < 0) {
        HI_MIPI_TX_ERR("mipi_tx_register_irq fail!\n");
        goto fail1;
    }

    mipi_tx_drv_hw_init();

    return 0;

fail1:
    mipi_tx_drv_reg_exit();
fail0:
    return -1;
}

void mipi_tx_drv_exit(void)
{
    mipi_tx_unregister_irq();
    mipi_tx_drv_reg_exit();
}

#ifdef __cplusplus
#if __cplusplus
}

#endif
#endif
