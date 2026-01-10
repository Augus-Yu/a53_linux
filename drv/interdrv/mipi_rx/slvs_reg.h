/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description: slvs_reg.h
 * Author:
 * Create: 2016-10-07
 */

#ifndef __SLVS_REG_H__
#define __SLVS_REG_H__

/* Define the union U_PHY_EN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_en                : 1   ; /* [0]  */
        unsigned int    reserved_0            : 3   ; /* [3..1]  */
        unsigned int    phy_lane0_en          : 1   ; /* [4]  */
        unsigned int    phy_lane1_en          : 1   ; /* [5]  */
        unsigned int    phy_lane2_en          : 1   ; /* [6]  */
        unsigned int    phy_lane3_en          : 1   ; /* [7]  */
        unsigned int    phy_lane4_en          : 1   ; /* [8]  */
        unsigned int    phy_lane5_en          : 1   ; /* [9]  */
        unsigned int    phy_lane6_en          : 1   ; /* [10]  */
        unsigned int    phy_lane7_en          : 1   ; /* [11]  */
        unsigned int    reserved_1            : 20  ; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_EN;

/* Define the union U_PHY_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_baudsel           : 2   ; /* [1..0]  */
        unsigned int    reserved_0            : 2   ; /* [3..2]  */
        unsigned int    phy_outsel            : 2   ; /* [5..4]  */
        unsigned int    reserved_1            : 26  ; /* [31..6]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_SEL;

/* Define the union U_PHY_TERM_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_term0_ctrl     : 4   ; /* [3..0]  */
        unsigned int    phy_rg_term1_ctrl     : 4   ; /* [7..4]  */
        unsigned int    phy_rg_term2_ctrl     : 4   ; /* [11..8]  */
        unsigned int    phy_rg_term3_ctrl     : 4   ; /* [15..12]  */
        unsigned int    phy_rg_term4_ctrl     : 4   ; /* [19..16]  */
        unsigned int    phy_rg_term5_ctrl     : 4   ; /* [23..20]  */
        unsigned int    phy_rg_term6_ctrl     : 4   ; /* [27..24]  */
        unsigned int    phy_rg_term7_ctrl     : 4   ; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_TERM_CTRL;

/* Define the union U_PHY_TERM_EN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_term0_en       : 1   ; /* [0]  */
        unsigned int    phy_rg_term1_en       : 1   ; /* [1]  */
        unsigned int    phy_rg_term2_en       : 1   ; /* [2]  */
        unsigned int    phy_rg_term3_en       : 1   ; /* [3]  */
        unsigned int    phy_rg_term4_en       : 1   ; /* [4]  */
        unsigned int    phy_rg_term5_en       : 1   ; /* [5]  */
        unsigned int    phy_rg_term6_en       : 1   ; /* [6]  */
        unsigned int    phy_rg_term7_en       : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_TERM_EN;

/* Define the union U_PHY_PCLK_SEL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_pclk0_sel      : 2   ; /* [1..0]  */
        unsigned int    reserved_0            : 2   ; /* [3..2]  */
        unsigned int    phy_rg_pclk1_sel      : 2   ; /* [5..4]  */
        unsigned int    reserved_1            : 2   ; /* [7..6]  */
        unsigned int    phy_rg_pclk2_sel      : 2   ; /* [9..8]  */
        unsigned int    reserved_2            : 2   ; /* [11..10]  */
        unsigned int    phy_rg_pclk3_sel      : 2   ; /* [13..12]  */
        unsigned int    reserved_3            : 2   ; /* [15..14]  */
        unsigned int    phy_rg_pclk4_sel      : 2   ; /* [17..16]  */
        unsigned int    reserved_4            : 2   ; /* [19..18]  */
        unsigned int    phy_rg_pclk5_sel      : 2   ; /* [21..20]  */
        unsigned int    reserved_5            : 2   ; /* [23..22]  */
        unsigned int    phy_rg_pclk6_sel      : 2   ; /* [25..24]  */
        unsigned int    reserved_6            : 2   ; /* [27..26]  */
        unsigned int    phy_rg_pclk7_sel      : 2   ; /* [29..28]  */
        unsigned int    reserved_7            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PCLK_SEL;

/* Define the union U_PHY_BIST_EN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_bist_en        : 8   ; /* [7..0]  */
        unsigned int    reserved_0            : 8   ; /* [15..8]  */
        unsigned int    phy_rg_bist_sel       : 4   ; /* [19..16]  */
        unsigned int    phy_rg_bist_idrv_ctrl : 3   ; /* [22..20]  */
        unsigned int    reserved_1            : 9   ; /* [31..23]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_EN;

/* Define the union U_PHY_PLL_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_pll_en         : 1   ; /* [0]  */
        unsigned int    reserved_0            : 3   ; /* [3..1]  */
        unsigned int    phy_rg_pll_bias_ctrl  : 4   ; /* [7..4]  */
        unsigned int    phy_rg_bw_ctrl        : 4   ; /* [11..8]  */
        unsigned int    phy_rg_icp_ctrl       : 4   ; /* [15..12]  */
        unsigned int    phy_rg_prediv         : 2   ; /* [17..16]  */
        unsigned int    reserved_1            : 2   ; /* [19..18]  */
        unsigned int    phy_rg_pstdiv         : 7   ; /* [26..20]  */
        unsigned int    reserved_2            : 5   ; /* [31..27]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PLL_CTRL;

/* Define the union U_PHY_PLL_VCO_CTL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_vco_cal        : 4   ; /* [3..0]  */
        unsigned int    phy_rg_vco_ctrl       : 4   ; /* [7..4]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PLL_VCO_CTL;

/* Define the union U_PHY_PLL_CTL_REV */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_pll_ctrl       : 8   ; /* [7..0]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PLL_CTL_REV;

/* Define the union U_PHY_CDR_DEBUG0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_sign_p         : 8   ; /* [7..0]  */
        unsigned int    phy_rg_sign_n         : 8   ; /* [15..8]  */
        unsigned int    reserved_0            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_DEBUG0;

/* Define the union U_PHY_CDR_DEBUG1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_sum_ki0        : 6   ; /* [5..0]  */
        unsigned int    reserved_0            : 2   ; /* [7..6]  */
        unsigned int    phy_rg_sum_ki1        : 6   ; /* [13..8]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    phy_rg_sum_ki2        : 6   ; /* [21..16]  */
        unsigned int    reserved_2            : 2   ; /* [23..22]  */
        unsigned int    phy_rg_sum_ki3        : 6   ; /* [29..24]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_DEBUG1;

/* Define the union U_PHY_CDR_DEBUG2 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_sum_ki4        : 6   ; /* [5..0]  */
        unsigned int    reserved_0            : 2   ; /* [7..6]  */
        unsigned int    phy_rg_sum_ki5        : 6   ; /* [13..8]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    phy_rg_sum_ki6        : 6   ; /* [21..16]  */
        unsigned int    reserved_2            : 2   ; /* [23..22]  */
        unsigned int    phy_rg_sum_ki7        : 6   ; /* [29..24]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_DEBUG2;

/* Define the union U_PHY_CDR_EN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_cdr_en         : 1   ; /* [0]  */
        unsigned int    phy_rg_cdr_2nd_en     : 1   ; /* [1]  */
        unsigned int    reserved_0            : 30  ; /* [31..2]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_EN;

/* Define the union U_PHY_CDR_CTRL0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_cdr_ip_cas     : 1   ; /* [0]  */
        unsigned int    reserved_0            : 3   ; /* [3..1]  */
        unsigned int    phy_rg_cdr_freq_mode  : 1   ; /* [4]  */
        unsigned int    reserved_1            : 3   ; /* [7..5]  */
        unsigned int    phy_rg_cdr_lat_ctrl   : 2   ; /* [9..8]  */
        unsigned int    reserved_2            : 2   ; /* [11..10]  */
        unsigned int    phy_rg_cdr_gain_ip    : 2   ; /* [13..12]  */
        unsigned int    reserved_3            : 2   ; /* [15..14]  */
        unsigned int    phy_rg_cdr_gain_pp    : 2   ; /* [17..16]  */
        unsigned int    reserved_4            : 2   ; /* [19..18]  */
        unsigned int    phy_rg_cdr_stur_ctrl  : 6   ; /* [25..20]  */
        unsigned int    reserved_5            : 2   ; /* [27..26]  */
        unsigned int    phy_rg_cdr_int_depth  : 2   ; /* [29..28]  */
        unsigned int    reserved_6            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_CTRL0;

/* Define the union U_PHY_CDR_CTRL1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_cdr_ext_mode   : 1   ; /* [0]  */
        unsigned int    phy_rg_cdr_ext_en     : 1   ; /* [1]  */
        unsigned int    phy_rg_cdr_bbpd_freq  : 1   ; /* [2]  */
        unsigned int    reserved_0            : 1   ; /* [3]  */
        unsigned int    phy_rg_cdr_ctrl       : 8   ; /* [11..4]  */
        unsigned int    reserved_1            : 20  ; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_CTRL1;

/* Define the union U_PHY_CDR_PI_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_cdr_pi_step    : 2   ; /* [1..0]  */
        unsigned int    reserved_0            : 2   ; /* [3..2]  */
        unsigned int    phy_cdr_lsb_ctrl      : 1   ; /* [4]  */
        unsigned int    phy_rg_pi_res_bp      : 1   ; /* [5]  */
        unsigned int    phy_rg_pi_delay_sel   : 2   ; /* [7..6]  */
        unsigned int    phy_cdr_ph_trg        : 1   ; /* [8]  */
        unsigned int    reserved_1            : 3   ; /* [11..9]  */
        unsigned int    phy_cdr_manu_pi       : 8   ; /* [19..12]  */
        unsigned int    phy_rg_pi_inv_ctrl    : 4   ; /* [23..20]  */
        unsigned int    reserved_2            : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_PI_CTRL;

/* Define the union U_PHY_CDR_MANU_CTL0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_cdr_manu_ctrl0    : 6   ; /* [5..0]  */
        unsigned int    reserved_0            : 2   ; /* [7..6]  */
        unsigned int    phy_cdr_manu_ctrl1    : 6   ; /* [13..8]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    phy_cdr_manu_ctrl2    : 6   ; /* [21..16]  */
        unsigned int    reserved_2            : 2   ; /* [23..22]  */
        unsigned int    phy_cdr_manu_ctrl3    : 6   ; /* [29..24]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_MANU_CTL0;

/* Define the union U_PHY_CDR_MANU_CTL1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_cdr_manu_ctrl4    : 6   ; /* [5..0]  */
        unsigned int    reserved_0            : 2   ; /* [7..6]  */
        unsigned int    phy_cdr_manu_ctrl5    : 6   ; /* [13..8]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    phy_cdr_manu_ctrl6    : 6   ; /* [21..16]  */
        unsigned int    reserved_2            : 2   ; /* [23..22]  */
        unsigned int    phy_cdr_manu_ctrl7    : 6   ; /* [29..24]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CDR_MANU_CTL1;

/* Define the union U_PHY_EQ_CTRL0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_eq_ctrl0       : 8   ; /* [7..0]  */
        unsigned int    phy_rg_eq_ctrl1       : 8   ; /* [15..8]  */
        unsigned int    phy_rg_eq_ctrl2       : 8   ; /* [23..16]  */
        unsigned int    phy_rg_eq_ctrl3       : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_EQ_CTRL0;

/* Define the union U_PHY_EQ_CTRL1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_eq_ctrl4       : 8   ; /* [7..0]  */
        unsigned int    phy_rg_eq_ctrl5       : 8   ; /* [15..8]  */
        unsigned int    phy_rg_eq_ctrl6       : 8   ; /* [23..16]  */
        unsigned int    phy_rg_eq_ctrl7       : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_EQ_CTRL1;

/* Define the union U_PHY_EQ_CTRL2 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_ieq_ctl        : 4   ; /* [3..0]  */
        unsigned int    phy_rg_eq_rl_sel      : 6   ; /* [9..4]  */
        unsigned int    phy_rg_vcmin_ctl      : 2   ; /* [11..10]  */
        unsigned int    phy_rg_eq_icurr_sel   : 8   ; /* [19..12]  */
        unsigned int    phy_rg_oscal_rcsel    : 2   ; /* [21..20]  */
        unsigned int    reserved_0            : 2   ; /* [23..22]  */
        unsigned int    phy_rg_oscal_en       : 1   ; /* [24]  */
        unsigned int    reserved_1            : 1   ; /* [25]  */
        unsigned int    phy_rg_eq_preamp_vcmo_ctl : 3   ; /* [28..26]  */
        unsigned int    reserved_2            : 3   ; /* [31..29]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_EQ_CTRL2;

/* Define the union U_PHY_CTRL_TEST */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_zone_ctrl      : 1   ; /* [0]  */
        unsigned int    reserved_0            : 3   ; /* [3..1]  */
        unsigned int    phy_rg_igen_ctrl      : 4   ; /* [7..4]  */
        unsigned int    phy_rg_tst_ana_sel    : 4   ; /* [11..8]  */
        unsigned int    phy_rg_tst_cksel      : 3   ; /* [14..12]  */
        unsigned int    reserved_1            : 1   ; /* [15]  */
        unsigned int    phy_rg_phy_ctl        : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CTRL_TEST;

/* Define the union U_PHY_PDAC_LDO */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_en_ldo         : 1   ; /* [0]  */
        unsigned int    phy_rg_ldo_sw         : 1   ; /* [1]  */
        unsigned int    phy_rg_ldo_ctl        : 2   ; /* [3..2]  */
        unsigned int    reserved_0            : 28  ; /* [31..4]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PDAC_LDO;

/* Define the union U_PHY_LANE_DATA0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_data0_slvs        : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 6   ; /* [15..10]  */
        unsigned int    phy_data1_slvs        : 10  ; /* [25..16]  */
        unsigned int    reserved_1            : 6   ; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_LANE_DATA0;

/* Define the union U_PHY_LANE_DATA1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_data2_slvs        : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 6   ; /* [15..10]  */
        unsigned int    phy_data3_slvs        : 10  ; /* [25..16]  */
        unsigned int    reserved_1            : 6   ; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_LANE_DATA1;

/* Define the union U_PHY_LANE_DATA2 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_data4_slvs        : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 6   ; /* [15..10]  */
        unsigned int    phy_data5_slvs        : 10  ; /* [25..16]  */
        unsigned int    reserved_1            : 6   ; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_LANE_DATA2;

/* Define the union U_PHY_LANE_DATA3 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_data6_slvs        : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 6   ; /* [15..10]  */
        unsigned int    phy_data7_slvs        : 10  ; /* [25..16]  */
        unsigned int    reserved_1            : 6   ; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_LANE_DATA3;

/* Define the union U_PHY_LANE_CTRL0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_lane0_ctl      : 8   ; /* [7..0]  */
        unsigned int    phy_rg_lane1_ctl      : 8   ; /* [15..8]  */
        unsigned int    phy_rg_lane2_ctl      : 8   ; /* [23..16]  */
        unsigned int    phy_rg_lane3_ctl      : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_LANE_CTRL0;

/* Define the union U_PHY_LANE_CTRL1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_lane4_ctl      : 8   ; /* [7..0]  */
        unsigned int    phy_rg_lane5_ctl      : 8   ; /* [15..8]  */
        unsigned int    phy_rg_lane6_ctl      : 8   ; /* [23..16]  */
        unsigned int    phy_rg_lane7_ctl      : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_LANE_CTRL1;

/* Define the union U_PHY_PH_SEL_DEBUG0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_ph_sel0        : 8   ; /* [7..0]  */
        unsigned int    phy_rg_ph_sel1        : 8   ; /* [15..8]  */
        unsigned int    phy_rg_ph_sel2        : 8   ; /* [23..16]  */
        unsigned int    phy_rg_ph_sel3        : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PH_SEL_DEBUG0;

/* Define the union U_PHY_PH_SEL_DEBUG1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_ph_sel4        : 8   ; /* [7..0]  */
        unsigned int    phy_rg_ph_sel5        : 8   ; /* [15..8]  */
        unsigned int    phy_rg_ph_sel6        : 8   ; /* [23..16]  */
        unsigned int    phy_rg_ph_sel7        : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PH_SEL_DEBUG1;

/* Define the union U_PHY_PI_CTRL_DEBUG0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_pi_ctrl0       : 8   ; /* [7..0]  */
        unsigned int    phy_rg_pi_ctrl1       : 8   ; /* [15..8]  */
        unsigned int    phy_rg_pi_ctrl2       : 8   ; /* [23..16]  */
        unsigned int    phy_rg_pi_ctrl3       : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PI_CTRL_DEBUG0;

/* Define the union U_PHY_PI_CTRL_DEBUG1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_rg_pi_ctrl4       : 8   ; /* [7..0]  */
        unsigned int    phy_rg_pi_ctrl5       : 8   ; /* [15..8]  */
        unsigned int    phy_rg_pi_ctrl6       : 8   ; /* [23..16]  */
        unsigned int    phy_rg_pi_ctrl7       : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PI_CTRL_DEBUG1;

/* Define the union U_PHY_SYMBOL_ORDER */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    symbol_order_0        : 1   ; /* [0]  */
        unsigned int    symbol_order_1        : 1   ; /* [1]  */
        unsigned int    symbol_order_2        : 1   ; /* [2]  */
        unsigned int    symbol_order_3        : 1   ; /* [3]  */
        unsigned int    symbol_order_4        : 1   ; /* [4]  */
        unsigned int    symbol_order_5        : 1   ; /* [5]  */
        unsigned int    symbol_order_6        : 1   ; /* [6]  */
        unsigned int    symbol_order_7        : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_SYMBOL_ORDER;

/* Define the union U_PHY_POLARITY_REVERSE */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    polarity_reverse_0    : 1   ; /* [0]  */
        unsigned int    polarity_reverse_1    : 1   ; /* [1]  */
        unsigned int    polarity_reverse_2    : 1   ; /* [2]  */
        unsigned int    polarity_reverse_3    : 1   ; /* [3]  */
        unsigned int    polarity_reverse_4    : 1   ; /* [4]  */
        unsigned int    polarity_reverse_5    : 1   ; /* [5]  */
        unsigned int    polarity_reverse_6    : 1   ; /* [6]  */
        unsigned int    polarity_reverse_7    : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_POLARITY_REVERSE;

/* Define the union U_PHY_PCS_EN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    pcs_lane0_en          : 1   ; /* [0]  */
        unsigned int    pcs_lane1_en          : 1   ; /* [1]  */
        unsigned int    pcs_lane2_en          : 1   ; /* [2]  */
        unsigned int    pcs_lane3_en          : 1   ; /* [3]  */
        unsigned int    pcs_lane4_en          : 1   ; /* [4]  */
        unsigned int    pcs_lane5_en          : 1   ; /* [5]  */
        unsigned int    pcs_lane6_en          : 1   ; /* [6]  */
        unsigned int    pcs_lane7_en          : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PCS_EN;

/* Define the union U_PHY_SYMAL_DATA0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_symal_lane0_d   : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 2   ; /* [11..10]  */
        unsigned int    physt_symal_lane0_pchanged : 1   ; /* [12]  */
        unsigned int    physt_symal_lane0_disp_init : 1   ; /* [13]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    physt_symal_lane1_d   : 10  ; /* [25..16]  */
        unsigned int    reserved_2            : 2   ; /* [27..26]  */
        unsigned int    physt_symal_lane1_pchanged : 1   ; /* [28]  */
        unsigned int    physt_symal_lane1_disp_init : 1   ; /* [29]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_SYMAL_DATA0;

/* Define the union U_PHY_SYMAL_DATA1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_symal_lane2_d   : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 2   ; /* [11..10]  */
        unsigned int    physt_symal_lane2_pchanged : 1   ; /* [12]  */
        unsigned int    physt_symal_lane2_disp_init : 1   ; /* [13]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    physt_symal_lane3_d   : 10  ; /* [25..16]  */
        unsigned int    reserved_2            : 2   ; /* [27..26]  */
        unsigned int    physt_symal_lane3_pchanged : 1   ; /* [28]  */
        unsigned int    physt_symal_lane3_disp_init : 1   ; /* [29]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_SYMAL_DATA1;

/* Define the union U_PHY_SYMAL_DATA2 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_symal_lane4_d   : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 2   ; /* [11..10]  */
        unsigned int    physt_symal_lane4_pchanged : 1   ; /* [12]  */
        unsigned int    physt_symal_lane4_disp_init : 1   ; /* [13]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    physt_symal_lane5_d   : 10  ; /* [25..16]  */
        unsigned int    reserved_2            : 2   ; /* [27..26]  */
        unsigned int    physt_symal_lane5_pchanged : 1   ; /* [28]  */
        unsigned int    physt_symal_lane5_disp_init : 1   ; /* [29]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_SYMAL_DATA2;

/* Define the union U_PHY_SYMAL_DATA3 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_symal_lane6_d   : 10  ; /* [9..0]  */
        unsigned int    reserved_0            : 2   ; /* [11..10]  */
        unsigned int    physt_symal_lane6_pchanged : 1   ; /* [12]  */
        unsigned int    physt_symal_lane6_disp_init : 1   ; /* [13]  */
        unsigned int    reserved_1            : 2   ; /* [15..14]  */
        unsigned int    physt_symal_lane7_d   : 10  ; /* [25..16]  */
        unsigned int    reserved_2            : 2   ; /* [27..26]  */
        unsigned int    physt_symal_lane7_pchanged : 1   ; /* [28]  */
        unsigned int    physt_symal_lane7_disp_init : 1   ; /* [29]  */
        unsigned int    reserved_3            : 2   ; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_SYMAL_DATA3;

/* Define the union U_PHY_SYMAL_VALID */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_symal_valid     : 8   ; /* [7..0]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_SYMAL_VALID;

/* Define the union U_PHY_10B8B_VALID */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_10b8b_valid     : 8   ; /* [7..0]  */
        unsigned int    physt_10b8b_kcode     : 8   ; /* [15..8]  */
        unsigned int    reserved_0            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_10B8B_VALID;

/* Define the union U_PHY_10B8B_DATA0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_10b8b_lane0_d   : 8   ; /* [7..0]  */
        unsigned int    physt_10b8b_lane1_d   : 8   ; /* [15..8]  */
        unsigned int    physt_10b8b_lane2_d   : 8   ; /* [23..16]  */
        unsigned int    physt_10b8b_lane3_d   : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_10B8B_DATA0;

/* Define the union U_PHY_10B8B_DATA1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_10b8b_lane4_d   : 8   ; /* [7..0]  */
        unsigned int    physt_10b8b_lane5_d   : 8   ; /* [15..8]  */
        unsigned int    physt_10b8b_lane6_d   : 8   ; /* [23..16]  */
        unsigned int    physt_10b8b_lane7_d   : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_10B8B_DATA1;

/* Define the union U_PHY_10B8B_DEC_ERR */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_10b8b_disp_err_0 : 1   ; /* [0]  */
        unsigned int    physt_10b8b_code_err_0 : 1   ; /* [1]  */
        unsigned int    physt_10b8b_disp_err_1 : 1   ; /* [2]  */
        unsigned int    physt_10b8b_code_err_1 : 1   ; /* [3]  */
        unsigned int    physt_10b8b_disp_err_2 : 1   ; /* [4]  */
        unsigned int    physt_10b8b_code_err_2 : 1   ; /* [5]  */
        unsigned int    physt_10b8b_disp_err_3 : 1   ; /* [6]  */
        unsigned int    physt_10b8b_code_err_3 : 1   ; /* [7]  */
        unsigned int    physt_10b8b_disp_err_4 : 1   ; /* [8]  */
        unsigned int    physt_10b8b_code_err_4 : 1   ; /* [9]  */
        unsigned int    physt_10b8b_disp_err_5 : 1   ; /* [10]  */
        unsigned int    physt_10b8b_code_err_5 : 1   ; /* [11]  */
        unsigned int    physt_10b8b_disp_err_6 : 1   ; /* [12]  */
        unsigned int    physt_10b8b_code_err_6 : 1   ; /* [13]  */
        unsigned int    physt_10b8b_disp_err_7 : 1   ; /* [14]  */
        unsigned int    physt_10b8b_code_err_7 : 1   ; /* [15]  */
        unsigned int    reserved_0            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_10B8B_DEC_ERR;

/* Define the union U_PHY_DESKEW_VALID */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_deskew_valid    : 8   ; /* [7..0]  */
        unsigned int    physt_deskew_kcode    : 8   ; /* [15..8]  */
        unsigned int    reserved_0            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_DESKEW_VALID;

/* Define the union U_PHY_DESKEW_DATA0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_deskew_lane0_d  : 8   ; /* [7..0]  */
        unsigned int    physt_deskew_lane1_d  : 8   ; /* [15..8]  */
        unsigned int    physt_deskew_lane2_d  : 8   ; /* [23..16]  */
        unsigned int    physt_deskew_lane3_d  : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_DESKEW_DATA0;

/* Define the union U_PHY_DESKEW_DATA1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_deskew_lane4_d  : 8   ; /* [7..0]  */
        unsigned int    physt_deskew_lane5_d  : 8   ; /* [15..8]  */
        unsigned int    physt_deskew_lane6_d  : 8   ; /* [23..16]  */
        unsigned int    physt_deskew_lane7_d  : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_DESKEW_DATA1;

/* Define the union U_PHY_AFIFO_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    physt_afifo0_full     : 1   ; /* [0]  */
        unsigned int    physt_afifo1_full     : 1   ; /* [1]  */
        unsigned int    physt_afifo2_full     : 1   ; /* [2]  */
        unsigned int    physt_afifo3_full     : 1   ; /* [3]  */
        unsigned int    physt_afifo4_full     : 1   ; /* [4]  */
        unsigned int    physt_afifo5_full     : 1   ; /* [5]  */
        unsigned int    physt_afifo6_full     : 1   ; /* [6]  */
        unsigned int    physt_afifo7_full     : 1   ; /* [7]  */
        unsigned int    physt_afifo0_empty    : 1   ; /* [8]  */
        unsigned int    physt_afifo1_empty    : 1   ; /* [9]  */
        unsigned int    physt_afifo2_empty    : 1   ; /* [10]  */
        unsigned int    physt_afifo3_empty    : 1   ; /* [11]  */
        unsigned int    physt_afifo4_empty    : 1   ; /* [12]  */
        unsigned int    physt_afifo5_empty    : 1   ; /* [13]  */
        unsigned int    physt_afifo6_empty    : 1   ; /* [14]  */
        unsigned int    physt_afifo7_empty    : 1   ; /* [15]  */
        unsigned int    physt_afifo0_rden     : 1   ; /* [16]  */
        unsigned int    physt_afifo1_rden     : 1   ; /* [17]  */
        unsigned int    physt_afifo2_rden     : 1   ; /* [18]  */
        unsigned int    physt_afifo3_rden     : 1   ; /* [19]  */
        unsigned int    physt_afifo4_rden     : 1   ; /* [20]  */
        unsigned int    physt_afifo5_rden     : 1   ; /* [21]  */
        unsigned int    physt_afifo6_rden     : 1   ; /* [22]  */
        unsigned int    physt_afifo7_rden     : 1   ; /* [23]  */
        unsigned int    reserved_0            : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_AFIFO_STATUS;

/* Define the union U_PHY_PCS_INT_RAW */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    int_disp_err0_raw     : 1   ; /* [0]  */
        unsigned int    int_code_err0_raw     : 1   ; /* [1]  */
        unsigned int    int_disp_err1_raw     : 1   ; /* [2]  */
        unsigned int    int_code_err1_raw     : 1   ; /* [3]  */
        unsigned int    int_disp_err2_raw     : 1   ; /* [4]  */
        unsigned int    int_code_err2_raw     : 1   ; /* [5]  */
        unsigned int    int_disp_err3_raw     : 1   ; /* [6]  */
        unsigned int    int_code_err3_raw     : 1   ; /* [7]  */
        unsigned int    int_disp_err4_raw     : 1   ; /* [8]  */
        unsigned int    int_code_err4_raw     : 1   ; /* [9]  */
        unsigned int    int_disp_err5_raw     : 1   ; /* [10]  */
        unsigned int    int_code_err5_raw     : 1   ; /* [11]  */
        unsigned int    int_disp_err6_raw     : 1   ; /* [12]  */
        unsigned int    int_code_err6_raw     : 1   ; /* [13]  */
        unsigned int    int_disp_err7_raw     : 1   ; /* [14]  */
        unsigned int    int_code_err7_raw     : 1   ; /* [15]  */
        unsigned int    int_align_afifo0_raw  : 1   ; /* [16]  */
        unsigned int    int_align_afifo1_raw  : 1   ; /* [17]  */
        unsigned int    int_align_afifo2_raw  : 1   ; /* [18]  */
        unsigned int    int_align_afifo3_raw  : 1   ; /* [19]  */
        unsigned int    int_align_afifo4_raw  : 1   ; /* [20]  */
        unsigned int    int_align_afifo5_raw  : 1   ; /* [21]  */
        unsigned int    int_align_afifo6_raw  : 1   ; /* [22]  */
        unsigned int    int_align_afifo7_raw  : 1   ; /* [23]  */
        unsigned int    reserved_0            : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PCS_INT_RAW;

/* Define the union U_PHY_PCS_INT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    int_disp_err0_st      : 1   ; /* [0]  */
        unsigned int    int_code_err0_st      : 1   ; /* [1]  */
        unsigned int    int_disp_err1_st      : 1   ; /* [2]  */
        unsigned int    int_code_err1_st      : 1   ; /* [3]  */
        unsigned int    int_disp_err2_st      : 1   ; /* [4]  */
        unsigned int    int_code_err2_st      : 1   ; /* [5]  */
        unsigned int    int_disp_err3_st      : 1   ; /* [6]  */
        unsigned int    int_code_err3_st      : 1   ; /* [7]  */
        unsigned int    int_disp_err4_st      : 1   ; /* [8]  */
        unsigned int    int_code_err4_st      : 1   ; /* [9]  */
        unsigned int    int_disp_err5_st      : 1   ; /* [10]  */
        unsigned int    int_code_err5_st      : 1   ; /* [11]  */
        unsigned int    int_disp_err6_st      : 1   ; /* [12]  */
        unsigned int    int_code_err6_st      : 1   ; /* [13]  */
        unsigned int    int_disp_err7_st      : 1   ; /* [14]  */
        unsigned int    int_code_err7_st      : 1   ; /* [15]  */
        unsigned int    int_align_afifo0_st   : 1   ; /* [16]  */
        unsigned int    int_align_afifo1_st   : 1   ; /* [17]  */
        unsigned int    int_align_afifo2_st   : 1   ; /* [18]  */
        unsigned int    int_align_afifo3_st   : 1   ; /* [19]  */
        unsigned int    int_align_afifo4_st   : 1   ; /* [20]  */
        unsigned int    int_align_afifo5_st   : 1   ; /* [21]  */
        unsigned int    int_align_afifo6_st   : 1   ; /* [22]  */
        unsigned int    int_align_afifo7_st   : 1   ; /* [23]  */
        unsigned int    reserved_0            : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PCS_INT;

/* Define the union U_PHY_PCS_INT_MASK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    int_disp_err0_mask    : 1   ; /* [0]  */
        unsigned int    int_code_err0_mask    : 1   ; /* [1]  */
        unsigned int    int_disp_err1_mask    : 1   ; /* [2]  */
        unsigned int    int_code_err1_mask    : 1   ; /* [3]  */
        unsigned int    int_disp_err2_mask    : 1   ; /* [4]  */
        unsigned int    int_code_err2_mask    : 1   ; /* [5]  */
        unsigned int    int_disp_err3_mask    : 1   ; /* [6]  */
        unsigned int    int_code_err3_mask    : 1   ; /* [7]  */
        unsigned int    int_disp_err4_mask    : 1   ; /* [8]  */
        unsigned int    int_code_err4_mask    : 1   ; /* [9]  */
        unsigned int    int_disp_err5_mask    : 1   ; /* [10]  */
        unsigned int    int_code_err5_mask    : 1   ; /* [11]  */
        unsigned int    int_disp_err6_mask    : 1   ; /* [12]  */
        unsigned int    int_code_err6_mask    : 1   ; /* [13]  */
        unsigned int    int_disp_err7_mask    : 1   ; /* [14]  */
        unsigned int    int_code_err7_mask    : 1   ; /* [15]  */
        unsigned int    int_align_afifo0_mask : 1   ; /* [16]  */
        unsigned int    int_align_afifo1_mask : 1   ; /* [17]  */
        unsigned int    int_align_afifo2_mask : 1   ; /* [18]  */
        unsigned int    int_align_afifo3_mask : 1   ; /* [19]  */
        unsigned int    int_align_afifo4_mask : 1   ; /* [20]  */
        unsigned int    int_align_afifo5_mask : 1   ; /* [21]  */
        unsigned int    int_align_afifo6_mask : 1   ; /* [22]  */
        unsigned int    int_align_afifo7_mask : 1   ; /* [23]  */
        unsigned int    reserved_0            : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_PCS_INT_MASK;

/* Define the union U_PHY_RSTN_REQ */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_lane_srst_req     : 8   ; /* [7..0]  */
        unsigned int    phy_lane_rx_cken      : 8   ; /* [15..8]  */
        unsigned int    reserved_0            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_RSTN_REQ;

/* Define the union U_PHY_BIST_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_initial_code : 8   ; /* [7..0]  */
        unsigned int    phy_bist_compare_length : 16  ; /* [23..8]  */
        unsigned int    phy_bist_length_en    : 1   ; /* [24]  */
        unsigned int    phy_bist_big_endian   : 1   ; /* [25]  */
        unsigned int    phy_test_srst_req     : 1   ; /* [26]  */
        unsigned int    phy_test_en           : 1   ; /* [27]  */
        unsigned int    reserved_0            : 4   ; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_CTRL;

/* Define the union U_PHY_BIST_LANE0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane0_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane0_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane0_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane0_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE0;

/* Define the union U_PHY_BIST_LANE1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane1_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane1_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane1_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane1_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE1;

/* Define the union U_PHY_BIST_LANE2 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane2_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane2_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane2_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane2_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE2;

/* Define the union U_PHY_BIST_LANE3 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane3_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane3_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane3_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane3_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE3;

/* Define the union U_PHY_BIST_LANE4 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane4_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane4_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane4_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane4_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE4;

/* Define the union U_PHY_BIST_LANE5 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane5_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane5_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane5_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane5_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE5;

/* Define the union U_PHY_BIST_LANE6 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane6_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane6_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane6_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane6_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE6;

/* Define the union U_PHY_BIST_LANE7 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    phy_bist_lane7_error_count : 16  ; /* [15..0]  */
        unsigned int    phy_bist_lane7_comparing : 1   ; /* [16]  */
        unsigned int    phy_bist_lane7_result : 1   ; /* [17]  */
        unsigned int    phy_bist_lane7_compare_done : 1   ; /* [18]  */
        unsigned int    reserved_0            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_BIST_LANE7;

/* Define the union U_COLORBAR_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    colorbar_en           : 1   ; /* [0]  */
        unsigned int    colorbar_symbol_order : 1   ; /* [1]  */
        unsigned int    reserved_0            : 14  ; /* [15..2]  */
        unsigned int    deskew_length         : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_COLORBAR_CTRL;

/* Define the union U_COLORBAR_WIDTH */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    x_lane_size           : 16  ; /* [15..0]  */
        unsigned int    hblank_lane_size      : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_COLORBAR_WIDTH;

/* Define the union U_COLORBAR_HEIGHT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    y_size                : 16  ; /* [15..0]  */
        unsigned int    line_size             : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_COLORBAR_HEIGHT;

/* Define the union U_COLORBAR_INIT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    colorbar_data_init    : 16  ; /* [15..0]  */
        unsigned int    colorbar_data_inc     : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_COLORBAR_INIT;

/* Define the union U_COLORBAR_ERR */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    colorbar_disp_err     : 8   ; /* [7..0]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_COLORBAR_ERR;

/* Define the union U_SLVS_INT_RAW */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    int_phy_ctrl_raw      : 1   ; /* [0]  */
        unsigned int    int_link0_raw         : 1   ; /* [1]  */
        unsigned int    int_link1_raw         : 1   ; /* [2]  */
        unsigned int    reserved_0            : 29  ; /* [31..3]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_SLVS_INT_RAW;

/* Define the union U_SLVS_INT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    int_phy_ctrl_st       : 1   ; /* [0]  */
        unsigned int    int_link0_st          : 1   ; /* [1]  */
        unsigned int    int_link1_st          : 1   ; /* [2]  */
        unsigned int    reserved_0            : 29  ; /* [31..3]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_SLVS_INT;

/* Define the union U_SLVS_INT_MASK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    int_phy_ctrl_mask     : 1   ; /* [0]  */
        unsigned int    int_link0_mask        : 1   ; /* [1]  */
        unsigned int    int_link1_mask        : 1   ; /* [2]  */
        unsigned int    reserved_0            : 29  ; /* [31..3]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_SLVS_INT_MASK;

/* Define the union U_PHY_DESKEW_SYMBOL_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_phy_deskew_symbol : 9   ; /* [8..0]  */
        unsigned int    reserved_0            : 23  ; /* [31..9]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_DESKEW_SYMBOL_LINK;

/* Define the union U_PHY_IDLE0_SYMBOL_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_phy_idle_code0   : 9   ; /* [8..0]  */
        unsigned int    reserved_0            : 7   ; /* [15..9]  */
        unsigned int    link_phy_idle_code1   : 9   ; /* [24..16]  */
        unsigned int    reserved_1            : 7   ; /* [31..25]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_IDLE0_SYMBOL_LINK;

/* Define the union U_PHY_IDLE1_SYMBOL_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_phy_idle_code2   : 9   ; /* [8..0]  */
        unsigned int    reserved_0            : 7   ; /* [15..9]  */
        unsigned int    link_phy_idle_code3   : 9   ; /* [24..16]  */
        unsigned int    reserved_1            : 7   ; /* [31..25]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_IDLE1_SYMBOL_LINK;

/* Define the union U_PHY_ALIGN_EN_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_phy_align_en     : 1   ; /* [0]  */
        unsigned int    reserved_0            : 3   ; /* [3..1]  */
        unsigned int    link_phy_align_lane0_en : 1   ; /* [4]  */
        unsigned int    link_phy_align_lane1_en : 1   ; /* [5]  */
        unsigned int    link_phy_align_lane2_en : 1   ; /* [6]  */
        unsigned int    link_phy_align_lane3_en : 1   ; /* [7]  */
        unsigned int    link_phy_align_lane4_en : 1   ; /* [8]  */
        unsigned int    link_phy_align_lane5_en : 1   ; /* [9]  */
        unsigned int    link_phy_align_lane6_en : 1   ; /* [10]  */
        unsigned int    link_phy_align_lane7_en : 1   ; /* [11]  */
        unsigned int    reserved_1            : 20  ; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_ALIGN_EN_LINK;

/* Define the union U_PHY_ALIGN_ID_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_phy_align_lane0_id : 4   ; /* [3..0]  */
        unsigned int    link_phy_align_lane1_id : 4   ; /* [7..4]  */
        unsigned int    link_phy_align_lane2_id : 4   ; /* [11..8]  */
        unsigned int    link_phy_align_lane3_id : 4   ; /* [15..12]  */
        unsigned int    link_phy_align_lane4_id : 4   ; /* [19..16]  */
        unsigned int    link_phy_align_lane5_id : 4   ; /* [23..20]  */
        unsigned int    link_phy_align_lane6_id : 4   ; /* [27..24]  */
        unsigned int    link_phy_align_lane7_id : 4   ; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_ALIGN_ID_LINK;

/* Define the union U_LINK_CLEAR_EN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_skew_clear_en    : 1   ; /* [0]  */
        unsigned int    link_header_crc_clear_en : 1   ; /* [1]  */
        unsigned int    link_payload_crc_clear_en : 1   ; /* [2]  */
        unsigned int    link_ecc_clear_en     : 1   ; /* [3]  */
        unsigned int    link_header_crc_filter_en : 1   ; /* [4]  */
        unsigned int    link_vsync_clear_en   : 1   ; /* [5]  */
        unsigned int    reserved_0            : 26  ; /* [31..6]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_CLEAR_EN;

/* Define the union U_LINK_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_vcrop_en         : 1   ; /* [0]  */
        unsigned int    link_crc_en           : 1   ; /* [1]  */
        unsigned int    link_hcrop_en         : 1   ; /* [2]  */
        unsigned int    link_double_pix_en    : 1   ; /* [3]  */
        unsigned int    link_wdr_num          : 2   ; /* [5..4]  */
        unsigned int    rx2mode               : 1   ; /* [6]  */
        unsigned int    reserved_0            : 1   ; /* [7]  */
        unsigned int    link_ecc_option       : 2   ; /* [9..8]  */
        unsigned int    reserved_1            : 2   ; /* [11..10]  */
        unsigned int    link_raw_type         : 3   ; /* [14..12]  */
        unsigned int    reserved_2            : 1   ; /* [15]  */
        unsigned int    link_lane_num         : 3   ; /* [18..16]  */
        unsigned int    reserved_3            : 13  ; /* [31..19]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_CTRL;

/* Define the union U_LINK_DOL_FR_PAR */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_dol_fr0_par      : 4   ; /* [3..0]  */
        unsigned int    link_dol_fr1_par      : 4   ; /* [7..4]  */
        unsigned int    link_dol_fr2_par      : 4   ; /* [11..8]  */
        unsigned int    link_dol_fr3_par      : 4   ; /* [15..12]  */
        unsigned int    reserved_0            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_DOL_FR_PAR;

/* Define the union U_LINK_HEIGHT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_line_start       : 16  ; /* [15..0]  */
        unsigned int    link_line_end         : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_HEIGHT;

/* Define the union U_LINK_BLOCK_SIZE */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_bblk_size        : 16  ; /* [15..0]  */
        unsigned int    link_extblk_size      : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_BLOCK_SIZE;

/* Define the union U_LINK_PAYLOAD_SIZE */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_paylsize         : 16  ; /* [15..0]  */
        unsigned int    reserved_0            : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_PAYLOAD_SIZE;

/* Define the union U_LINK_NUM_BLOCK_SIZE */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_number_of_basic_block : 8   ; /* [7..0]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_NUM_BLOCK_SIZE;

/* Define the union U_LINK_WIDTH */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_start_x          : 16  ; /* [15..0]  */
        unsigned int    link_imgwidth         : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_WIDTH;

/* Define the union U_LINK_MEMORY_CTRL */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_mem_ck_en        : 1   ; /* [0]  */
        unsigned int    link_mem_depth_en     : 1   ; /* [1]  */
        unsigned int    reserved_0            : 2   ; /* [3..2]  */
        unsigned int    link_mem_rct          : 2   ; /* [5..4]  */
        unsigned int    link_mem_wct          : 2   ; /* [7..6]  */
        unsigned int    link_mem_kp           : 3   ; /* [10..8]  */
        unsigned int    reserved_1            : 1   ; /* [11]  */
        unsigned int    link_mem_depth        : 16  ; /* [27..12]  */
        unsigned int    reserved_2            : 4   ; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_MEMORY_CTRL;

/* Define the union U_PHY_CODEX_VALID_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_physt_code_ext_valid : 8   ; /* [7..0]  */
        unsigned int    link_physt_end_ext_latch : 8   ; /* [15..8]  */
        unsigned int    link_physt_kcode_ext_err : 8   ; /* [23..16]  */
        unsigned int    reserved_0            : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CODEX_VALID_LINK;

/* Define the union U_PHY_CODEX_DATA0_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_physt_code_ext_data7 : 8   ; /* [7..0]  */
        unsigned int    link_physt_code_ext_data6 : 8   ; /* [15..8]  */
        unsigned int    link_physt_code_ext_data5 : 8   ; /* [23..16]  */
        unsigned int    link_physt_code_ext_data4 : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CODEX_DATA0_LINK;

/* Define the union U_PHY_CODEX_DATA1_LINK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_physt_code_ext_data3 : 8   ; /* [7..0]  */
        unsigned int    link_physt_code_ext_data2 : 8   ; /* [15..8]  */
        unsigned int    link_physt_code_ext_data1 : 8   ; /* [23..16]  */
        unsigned int    link_physt_code_ext_data0 : 8   ; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_PHY_CODEX_DATA1_LINK;

/* Define the union U_LINK_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_hd_crc_check_good : 1   ; /* [0]  */
        unsigned int    link_hd_crc_check_bad : 1   ; /* [1]  */
        unsigned int    link_pld_crc_check_good : 1   ; /* [2]  */
        unsigned int    link_pld_crc_check_bad : 1   ; /* [3]  */
        unsigned int    link_ecc_result       : 2   ; /* [5..4]  */
        unsigned int    link_block_cmob_dvalid0 : 1   ; /* [6]  */
        unsigned int    link_block_cmob_dvalid1 : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_STATUS;

/* Define the union U_LINK_DATA_AFIFO_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_cmd_afifo_full   : 1   ; /* [0]  */
        unsigned int    link_cmd_afifo_empty  : 1   ; /* [1]  */
        unsigned int    link_cmd_afifo_wren   : 1   ; /* [2]  */
        unsigned int    link_cmd_afifo_rden   : 1   ; /* [3]  */
        unsigned int    link_data_afifo_full  : 1   ; /* [4]  */
        unsigned int    link_data_afifo_empty : 1   ; /* [5]  */
        unsigned int    link_data_afifo_wren  : 1   ; /* [6]  */
        unsigned int    link_data_afifo_rden  : 1   ; /* [7]  */
        unsigned int    link_data_afifo_wrerr : 1   ; /* [8]  */
        unsigned int    link_data_afifo_rderr : 1   ; /* [9]  */
        unsigned int    reserved_0            : 22  ; /* [31..10]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_DATA_AFIFO_STATUS;

/* Define the union U_LINK_BYTEAL_DATA0_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_byteal_data0      : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_BYTEAL_DATA0_STATUS;
/* Define the union U_LINK_BYTEAL_DATA1_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_byteal_data1      : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_BYTEAL_DATA1_STATUS;
/* Define the union U_LINK_HDDEC_DATA0_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_hddec_data0       : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_HDDEC_DATA0_STATUS;
/* Define the union U_LINK_HDDEC_DATA1_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_hddec_data1       : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_HDDEC_DATA1_STATUS;
/* Define the union U_LINK_HEADER_DATA0_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_header_data0      : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_HEADER_DATA0_STATUS;
/* Define the union U_LINK_HEADER_DATA1_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_header_data1      : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_HEADER_DATA1_STATUS;
/* Define the union U_LINK_RS_DATA0_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_rs_data0          : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_RS_DATA0_STATUS;
/* Define the union U_LINK_RS_DATA1_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_rs_data1          : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_RS_DATA1_STATUS;
/* Define the union U_LINK_MUX_DATA0_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_mux_data0         : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_MUX_DATA0_STATUS;
/* Define the union U_LINK_MUX_DATA1_STATUS */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int link_mux_data1         : 32  ; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
        unsigned int    u32;

} U_LINK_MUX_DATA1_STATUS;
/* Define the union U_LINK_IMGSIZE0_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_imgwidth_statis_vc0 : 16  ; /* [15..0]  */
        unsigned int    link_imgheight_statis_vc0 : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_IMGSIZE0_STATUS;

/* Define the union U_LINK_IMGSIZE1_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_imgwidth_statis_vc1 : 16  ; /* [15..0]  */
        unsigned int    link_imgheight_statis_vc1 : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_IMGSIZE1_STATUS;

/* Define the union U_LINK_IMGSIZE2_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_imgwidth_statis_vc2 : 16  ; /* [15..0]  */
        unsigned int    link_imgheight_statis_vc2 : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_IMGSIZE2_STATUS;

/* Define the union U_LINK_IMGSIZE3_STATUS */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_imgwidth_statis_vc3 : 16  ; /* [15..0]  */
        unsigned int    link_imgheight_statis_vc3 : 16  ; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_IMGSIZE3_STATUS;

/* Define the union U_LINK_CTRL_INT_RAW */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_int_hd_crc_err_raw : 1   ; /* [0]  */
        unsigned int    link_int_pld_crc_err_raw : 1   ; /* [1]  */
        unsigned int    link_int_ecc_err_raw  : 1   ; /* [2]  */
        unsigned int    link_int_dafifo_wrerr_raw : 1   ; /* [3]  */
        unsigned int    link_int_dafifo_rderr_raw : 1   ; /* [4]  */
        unsigned int    link_int_cmdafifo_full_raw : 1   ; /* [5]  */
        unsigned int    link_int_skew_err_raw : 1   ; /* [6]  */
        unsigned int    link_int_vsync_raw    : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_CTRL_INT_RAW;

/* Define the union U_LINK_CTRL_INT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_int_hd_crc_err_st : 1   ; /* [0]  */
        unsigned int    link_int_pld_crc_err_st : 1   ; /* [1]  */
        unsigned int    link_int_ecc_err_st   : 1   ; /* [2]  */
        unsigned int    link_int_dafifo_wrerr_st : 1   ; /* [3]  */
        unsigned int    link_int_dafifo_rderr_st : 1   ; /* [4]  */
        unsigned int    link_int_cmdafifo_full_st : 1   ; /* [5]  */
        unsigned int    link_int_skew_err_st  : 1   ; /* [6]  */
        unsigned int    link_int_vsync_st     : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_CTRL_INT;

/* Define the union U_LINK_CTRL_INT_MASK */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int    link_int_hd_crc_err_mask : 1   ; /* [0]  */
        unsigned int    link_int_pld_crc_err_mask : 1   ; /* [1]  */
        unsigned int    link_int_ecc_err_mask : 1   ; /* [2]  */
        unsigned int    link_int_dafifo_wrerr_mask : 1   ; /* [3]  */
        unsigned int    link_int_dafifo_rderr_mask : 1   ; /* [4]  */
        unsigned int    link_int_cmdafifo_full_mask : 1   ; /* [5]  */
        unsigned int    link_int_skew_err_mask : 1   ; /* [6]  */
        unsigned int    link_int_vsync_mask   : 1   ; /* [7]  */
        unsigned int    reserved_0            : 24  ; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_LINK_CTRL_INT_MASK;

typedef struct {
    volatile U_PHY_EN                        PHY_EN                           ; /* 0x0 */
    volatile U_PHY_SEL                       PHY_SEL                          ; /* 0x4 */
    volatile U_PHY_TERM_CTRL                 PHY_TERM_CTRL                    ; /* 0x8 */
    volatile U_PHY_TERM_EN                   PHY_TERM_EN                      ; /* 0xc */
    volatile U_PHY_PCLK_SEL                  PHY_PCLK_SEL                     ; /* 0x10 */
    volatile U_PHY_BIST_EN                   PHY_BIST_EN                      ; /* 0x14 */
    volatile U_PHY_PLL_CTRL                  PHY_PLL_CTRL                     ; /* 0x18 */
    volatile U_PHY_PLL_VCO_CTL               PHY_PLL_VCO_CTL                  ; /* 0x1c */
    volatile U_PHY_PLL_CTL_REV               PHY_PLL_CTL_REV                  ; /* 0x20 */
    volatile U_PHY_CDR_DEBUG0                PHY_CDR_DEBUG0                   ; /* 0x24 */
    volatile U_PHY_CDR_DEBUG1                PHY_CDR_DEBUG1                   ; /* 0x28 */
    volatile U_PHY_CDR_DEBUG2                PHY_CDR_DEBUG2                   ; /* 0x2c */
    volatile U_PHY_CDR_EN                    PHY_CDR_EN                       ; /* 0x30 */
    volatile U_PHY_CDR_CTRL0                 PHY_CDR_CTRL0                    ; /* 0x34 */
    volatile U_PHY_CDR_CTRL1                 PHY_CDR_CTRL1                    ; /* 0x38 */
    volatile U_PHY_CDR_PI_CTRL               PHY_CDR_PI_CTRL                  ; /* 0x3c */
    volatile U_PHY_CDR_MANU_CTL0             PHY_CDR_MANU_CTL0                ; /* 0x40 */
    volatile U_PHY_CDR_MANU_CTL1             PHY_CDR_MANU_CTL1                ; /* 0x44 */
    volatile unsigned int                    reserved_0                       ; /* 0x48 */
    volatile U_PHY_EQ_CTRL0                  PHY_EQ_CTRL0                     ; /* 0x4c */
    volatile U_PHY_EQ_CTRL1                  PHY_EQ_CTRL1                     ; /* 0x50 */
    volatile U_PHY_EQ_CTRL2                  PHY_EQ_CTRL2                     ; /* 0x54 */
    volatile U_PHY_CTRL_TEST                 PHY_CTRL_TEST                    ; /* 0x58 */
    volatile U_PHY_PDAC_LDO                  PHY_PDAC_LDO                     ; /* 0x5c */
    volatile U_PHY_LANE_DATA0                PHY_LANE_DATA0                   ; /* 0x60 */
    volatile U_PHY_LANE_DATA1                PHY_LANE_DATA1                   ; /* 0x64 */
    volatile U_PHY_LANE_DATA2                PHY_LANE_DATA2                   ; /* 0x68 */
    volatile U_PHY_LANE_DATA3                PHY_LANE_DATA3                   ; /* 0x6c */
    volatile U_PHY_LANE_CTRL0                PHY_LANE_CTRL0                   ; /* 0x70 */
    volatile U_PHY_LANE_CTRL1                PHY_LANE_CTRL1                   ; /* 0x74 */
    volatile U_PHY_PH_SEL_DEBUG0             PHY_PH_SEL_DEBUG0                ; /* 0x78 */
    volatile U_PHY_PH_SEL_DEBUG1             PHY_PH_SEL_DEBUG1                ; /* 0x7c */
    volatile U_PHY_PI_CTRL_DEBUG0            PHY_PI_CTRL_DEBUG0               ; /* 0x80 */
    volatile U_PHY_PI_CTRL_DEBUG1            PHY_PI_CTRL_DEBUG1               ; /* 0x84 */
    volatile unsigned int                    reserved_1[30]                   ; /* 0x88~0xfc */
    volatile U_PHY_SYMBOL_ORDER              PHY_SYMBOL_ORDER                 ; /* 0x100 */
    volatile U_PHY_POLARITY_REVERSE          PHY_POLARITY_REVERSE             ; /* 0x104 */
    volatile U_PHY_PCS_EN                    PHY_PCS_EN                       ; /* 0x108 */
    volatile unsigned int                    reserved_2[5]                    ; /* 0x10c~0x11c */
    volatile U_PHY_SYMAL_DATA0               PHY_SYMAL_DATA0                  ; /* 0x120 */
    volatile U_PHY_SYMAL_DATA1               PHY_SYMAL_DATA1                  ; /* 0x124 */
    volatile U_PHY_SYMAL_DATA2               PHY_SYMAL_DATA2                  ; /* 0x128 */
    volatile U_PHY_SYMAL_DATA3               PHY_SYMAL_DATA3                  ; /* 0x12c */
    volatile U_PHY_SYMAL_VALID               PHY_SYMAL_VALID                  ; /* 0x130 */
    volatile unsigned int                    reserved_3[3]                    ; /* 0x134~0x13c */
    volatile U_PHY_10B8B_VALID               PHY_10B8B_VALID                  ; /* 0x140 */
    volatile U_PHY_10B8B_DATA0               PHY_10B8B_DATA0                  ; /* 0x144 */
    volatile U_PHY_10B8B_DATA1               PHY_10B8B_DATA1                  ; /* 0x148 */
    volatile U_PHY_10B8B_DEC_ERR             PHY_10B8B_DEC_ERR                ; /* 0x14c */
    volatile U_PHY_DESKEW_VALID              PHY_DESKEW_VALID                 ; /* 0x150 */
    volatile U_PHY_DESKEW_DATA0              PHY_DESKEW_DATA0                 ; /* 0x154 */
    volatile U_PHY_DESKEW_DATA1              PHY_DESKEW_DATA1                 ; /* 0x158 */
    volatile U_PHY_AFIFO_STATUS              PHY_AFIFO_STATUS                 ; /* 0x15c */
    volatile unsigned int                    reserved_4[36]                   ; /* 0x160~0x1ec */
    volatile U_PHY_PCS_INT_RAW               PHY_PCS_INT_RAW                  ; /* 0x1f0 */
    volatile U_PHY_PCS_INT                   PHY_PCS_INT                      ; /* 0x1f4 */
    volatile U_PHY_PCS_INT_MASK              PHY_PCS_INT_MASK                 ; /* 0x1f8 */
    volatile unsigned int                    reserved_5[2]                    ; /* 0x1fc~0x200 */
    volatile U_PHY_RSTN_REQ                  PHY_RSTN_REQ                     ; /* 0x204 */
    volatile U_PHY_BIST_CTRL                 PHY_BIST_CTRL                    ; /* 0x208 */
    volatile unsigned int                    reserved_6                       ; /* 0x20c */
    volatile U_PHY_BIST_LANE0                PHY_BIST_LANE0                   ; /* 0x210 */
    volatile U_PHY_BIST_LANE1                PHY_BIST_LANE1                   ; /* 0x214 */
    volatile U_PHY_BIST_LANE2                PHY_BIST_LANE2                   ; /* 0x218 */
    volatile U_PHY_BIST_LANE3                PHY_BIST_LANE3                   ; /* 0x21c */
    volatile U_PHY_BIST_LANE4                PHY_BIST_LANE4                   ; /* 0x220 */
    volatile U_PHY_BIST_LANE5                PHY_BIST_LANE5                   ; /* 0x224 */
    volatile U_PHY_BIST_LANE6                PHY_BIST_LANE6                   ; /* 0x228 */
    volatile U_PHY_BIST_LANE7                PHY_BIST_LANE7                   ; /* 0x22c */
    volatile U_COLORBAR_CTRL                 COLORBAR_CTRL                    ; /* 0x230 */
    volatile U_COLORBAR_WIDTH                COLORBAR_WIDTH                   ; /* 0x234 */
    volatile U_COLORBAR_HEIGHT               COLORBAR_HEIGHT                  ; /* 0x238 */
    volatile U_COLORBAR_INIT                 COLORBAR_INIT                    ; /* 0x23c */
    volatile U_COLORBAR_ERR                  COLORBAR_ERR                     ; /* 0x240 */
    volatile unsigned int                    reserved_7[43]                   ; /* 0x244~0x2ec */
    volatile U_SLVS_INT_RAW                  SLVS_INT_RAW                     ; /* 0x2f0 */
    volatile U_SLVS_INT                      SLVS_INT                         ; /* 0x2f4 */
    volatile U_SLVS_INT_MASK                 SLVS_INT_MASK                    ; /* 0x2f8 */
    volatile unsigned int                    reserved_8[2]                    ; /* 0x2fc~0x300 */
    volatile U_PHY_DESKEW_SYMBOL_LINK        PHY_DESKEW_SYMBOL_LINK           ; /* 0x304 */
    volatile U_PHY_IDLE0_SYMBOL_LINK         PHY_IDLE0_SYMBOL_LINK            ; /* 0x308 */
    volatile U_PHY_IDLE1_SYMBOL_LINK         PHY_IDLE1_SYMBOL_LINK            ; /* 0x30c */
    volatile unsigned int                    reserved_9[4]                    ; /* 0x310~0x31c */
    volatile U_PHY_ALIGN_EN_LINK             PHY_ALIGN_EN_LINK                ; /* 0x320 */
    volatile U_PHY_ALIGN_ID_LINK             PHY_ALIGN_ID_LINK                ; /* 0x324 */
    volatile U_LINK_CLEAR_EN                 LINK_CLEAR_EN                    ; /* 0x328 */
    volatile unsigned int                    reserved_10                      ; /* 0x32c */
    volatile U_LINK_CTRL                     LINK_CTRL                        ; /* 0x330 */
    volatile U_LINK_DOL_FR_PAR               LINK_DOL_FR_PAR                  ; /* 0x334 */
    volatile U_LINK_HEIGHT                   LINK_HEIGHT                      ; /* 0x338 */
    volatile U_LINK_BLOCK_SIZE               LINK_BLOCK_SIZE                  ; /* 0x33c */
    volatile U_LINK_PAYLOAD_SIZE             LINK_PAYLOAD_SIZE                ; /* 0x340 */
    volatile U_LINK_NUM_BLOCK_SIZE           LINK_NUM_BLOCK_SIZE              ; /* 0x344 */
    volatile U_LINK_WIDTH                    LINK_WIDTH                       ; /* 0x348 */
    volatile U_LINK_MEMORY_CTRL              LINK_MEMORY_CTRL                 ; /* 0x34c */
    volatile unsigned int                    reserved_11[4]                   ; /* 0x350~0x35c */
    volatile U_PHY_CODEX_VALID_LINK          PHY_CODEX_VALID_LINK             ; /* 0x360 */
    volatile U_PHY_CODEX_DATA0_LINK          PHY_CODEX_DATA0_LINK             ; /* 0x364 */
    volatile U_PHY_CODEX_DATA1_LINK          PHY_CODEX_DATA1_LINK             ; /* 0x368 */
    volatile U_LINK_STATUS                   LINK_STATUS                      ; /* 0x36c */
    volatile U_LINK_DATA_AFIFO_STATUS        LINK_DATA_AFIFO_STATUS           ; /* 0x370 */
    volatile U_LINK_BYTEAL_DATA0_STATUS      LINK_BYTEAL_DATA0_STATUS         ; /* 0x374 */
    volatile U_LINK_BYTEAL_DATA1_STATUS      LINK_BYTEAL_DATA1_STATUS         ; /* 0x378 */
    volatile unsigned int                    reserved_12                      ; /* 0x37c */
    volatile U_LINK_HDDEC_DATA0_STATUS       LINK_HDDEC_DATA0_STATUS          ; /* 0x380 */
    volatile U_LINK_HDDEC_DATA1_STATUS       LINK_HDDEC_DATA1_STATUS          ; /* 0x384 */
    volatile U_LINK_HEADER_DATA0_STATUS      LINK_HEADER_DATA0_STATUS         ; /* 0x388 */
    volatile U_LINK_HEADER_DATA1_STATUS      LINK_HEADER_DATA1_STATUS         ; /* 0x38c */
    volatile U_LINK_RS_DATA0_STATUS          LINK_RS_DATA0_STATUS             ; /* 0x390 */
    volatile U_LINK_RS_DATA1_STATUS          LINK_RS_DATA1_STATUS             ; /* 0x394 */
    volatile U_LINK_MUX_DATA0_STATUS         LINK_MUX_DATA0_STATUS            ; /* 0x398 */
    volatile U_LINK_MUX_DATA1_STATUS         LINK_MUX_DATA1_STATUS            ; /* 0x39c */
    volatile U_LINK_IMGSIZE0_STATUS          LINK_IMGSIZE0_STATUS             ; /* 0x3a0 */
    volatile U_LINK_IMGSIZE1_STATUS          LINK_IMGSIZE1_STATUS             ; /* 0x3a4 */
    volatile U_LINK_IMGSIZE2_STATUS          LINK_IMGSIZE2_STATUS             ; /* 0x3a8 */
    volatile U_LINK_IMGSIZE3_STATUS          LINK_IMGSIZE3_STATUS             ; /* 0x3ac */
    volatile unsigned int                    reserved_13[16]                  ; /* 0x3b0~0x3ec */
    volatile U_LINK_CTRL_INT_RAW             LINK_CTRL_INT_RAW                ; /* 0x3f0 */
    volatile U_LINK_CTRL_INT                 LINK_CTRL_INT                    ; /* 0x3f4 */
    volatile U_LINK_CTRL_INT_MASK            LINK_CTRL_INT_MASK               ; /* 0x3f8 */
} slvs_regs_type_t;

#endif /* __SLVS_REG_H__ */
