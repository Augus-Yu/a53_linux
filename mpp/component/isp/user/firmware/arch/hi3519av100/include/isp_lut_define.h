// ******************************************************************************
// Copyright     :  Copyright (C) 2017, Hisilicon Technologies Co., Ltd.
// File name     :   isp_lut_define.h
// Author        :
// Version       :  1.0
// Date          :  2017-02-23
// Description   :  Define all registers/tables
// History       :   2017-02-23 Create file
// ******************************************************************************

#ifndef __ISP_LUT_DEFINE_H__
#define __ISP_LUT_DEFINE_H__

/* Define the union U_ISP_AE_WEIGHT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ae_weight0 : 4; /* [3..0]  */
        unsigned int reserved_0 : 4; /* [7..4]  */
        unsigned int isp_ae_weight1 : 4; /* [11..8]  */
        unsigned int reserved_1 : 4; /* [15..12]  */
        unsigned int isp_ae_weight2 : 4; /* [19..16]  */
        unsigned int reserved_2 : 4; /* [23..20]  */
        unsigned int isp_ae_weight3 : 4; /* [27..24]  */
        unsigned int reserved_3 : 4; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_AE_WEIGHT;

/* Define the union U_ISP_DIS_REFINFO_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_dis_refinfo_wlut : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_DIS_REFINFO_WLUT;

/* Define the union U_ISP_LSC_RGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_lsc_rgain0 : 10; /* [9..0]  */
        unsigned int isp_lsc_rgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LSC_RGAIN;

/* Define the union U_ISP_LSC_GRGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_lsc_grgain0 : 10; /* [9..0]  */
        unsigned int isp_lsc_grgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LSC_GRGAIN;

/* Define the union U_ISP_LSC_BGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_lsc_bgain0 : 10; /* [9..0]  */
        unsigned int isp_lsc_bgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LSC_BGAIN;

/* Define the union U_ISP_LSC_GBGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_lsc_gbgain0 : 10; /* [9..0]  */
        unsigned int isp_lsc_gbgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LSC_GBGAIN;

/* Define the union U_ISP_FPN_LINE_WLUT */
typedef union {
    /* Define the struct bits  */
    struct {
        unsigned int isp_fpn_line_wlut : 32; /* [31..0]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_FPN_LINE_WLUT;
/* Define the union U_ISP_DPC_BPT_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_dpc_bpt_wlut_x : 13; /* [12..0]  */
        unsigned int reserved_0 : 3; /* [15..13]  */
        unsigned int isp_dpc_bpt_wlut_y : 13; /* [28..16]  */
        unsigned int reserved_1 : 3; /* [31..29]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_DPC_BPT_WLUT;

/* Define the union U_ISP_SHARPEN_MFGAIND */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_sharpen_mfgaind : 12; /* [11..0]  */
        unsigned int reserved_0 : 20; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_SHARPEN_MFGAIND;

/* Define the union U_ISP_SHARPEN_MFGAINUD */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_sharpen_mfgainud : 12; /* [11..0]  */
        unsigned int reserved_0 : 20; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_SHARPEN_MFGAINUD;

/* Define the union U_ISP_SHARPEN_HFGAIND */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_sharpen_hfgaind : 12; /* [11..0]  */
        unsigned int reserved_0 : 20; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_SHARPEN_HFGAIND;

/* Define the union U_ISP_SHARPEN_HFGAINUD */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_sharpen_hfgainud : 12; /* [11..0]  */
        unsigned int reserved_0 : 20; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_SHARPEN_HFGAINUD;

/* Define the union U_ISP_DEMOSAIC_DEPURPLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_demosaic_depurp_lut : 4; /* [3..0]  */
        unsigned int reserved_0 : 28; /* [31..4]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_DEMOSAIC_DEPURPLUT;
/* Define the union U_ISP_NDDM_GF_LUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_nddm_gflut : 12; /* [11..0]  */
        unsigned int reserved_0 : 20; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_NDDM_GF_LUT;

/* Define the union U_ISP_BNR_LMT_EVEN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_lmt_even : 8; /* [7..0]  */
        unsigned int reserved_0 : 24; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_LMT_EVEN;

/* Define the union U_ISP_BNR_LMT_ODD */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_lmt_odd : 8; /* [7..0]  */
        unsigned int reserved_0 : 24; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_LMT_ODD;

/* Define the union U_ISP_BNR_COR_EVEN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_cor_even : 14; /* [13..0]  */
        unsigned int reserved_0 : 18; /* [31..14]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_COR_EVEN;

/* Define the union U_ISP_BNR_COR_ODD */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_cor_odd : 14; /* [13..0]  */
        unsigned int reserved_0 : 18; /* [31..14]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_COR_ODD;

/* Define the union U_ISP_BNR_LSC_RGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_lsc_rgain0 : 10; /* [9..0]  */
        unsigned int isp_bnr_lsc_rgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_LSC_RGAIN;

/* Define the union U_ISP_BNR_LSC_GRGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_lsc_grgain0 : 10; /* [9..0]  */
        unsigned int isp_bnr_lsc_grgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_LSC_GRGAIN;

/* Define the union U_ISP_BNR_LSC_BGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_lsc_bgain0 : 10; /* [9..0]  */
        unsigned int isp_bnr_lsc_bgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_LSC_BGAIN;

/* Define the union U_ISP_BNR_LSC_GBGAIN */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_bnr_lsc_gbgain0 : 10; /* [9..0]  */
        unsigned int isp_bnr_lsc_gbgain1 : 10; /* [19..10]  */
        unsigned int reserved_0 : 12; /* [31..20]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_BNR_LSC_GBGAIN;

/* Define the union U_ISP_WDR_NOSLUT129X8 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_wdr_noslut129x8 : 8; /* [7..0]  */
        unsigned int reserved_0 : 24; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_WDR_NOSLUT129X8;

/* Define the union U_ISP_DEHAZE_PRESTAT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_dehaze_prestat_l : 10; /* [9..0]  */
        unsigned int reserved_0 : 6; /* [15..10]  */
        unsigned int isp_dehaze_prestat_h : 10; /* [25..16]  */
        unsigned int reserved_1 : 6; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_DEHAZE_PRESTAT;

/* Define the union U_ISP_DEHAZE_LUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_dehaze_dehaze_lut : 8; /* [7..0]  */
        unsigned int reserved_0 : 24; /* [31..8]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_DEHAZE_LUT;

/* Define the union U_ISP_PREGAMMA_LUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_pregamma_lut : 21; /* [20..0]  */
        unsigned int reserved_0 : 11; /* [31..21]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_PREGAMMA_LUT;

/* Define the union U_ISP_GAMMA_LUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_gamma_lut : 14; /* [13..0]  */
        unsigned int reserved_0 : 18; /* [31..14]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_GAMMA_LUT;

/* Define the union U_ISP_CA_LUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ca_lut : 24; /* [23..0]  */
        unsigned int reserved_0 : 8; /* [31..24]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CA_LUT;

/* Define the union U_ISP_CLUT_LUT0_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut0 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT0_WLUT;

/* Define the union U_ISP_CLUT_LUT1_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut1 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT1_WLUT;

/* Define the union U_ISP_CLUT_LUT2_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut2 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT2_WLUT;

/* Define the union U_ISP_CLUT_LUT3_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut3 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT3_WLUT;

/* Define the union U_ISP_CLUT_LUT4_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut4 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT4_WLUT;

/* Define the union U_ISP_CLUT_LUT5_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut5 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT5_WLUT;

/* Define the union U_ISP_CLUT_LUT6_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut6 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT6_WLUT;

/* Define the union U_ISP_CLUT_LUT7_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_clut_lut7 : 30; /* [29..0]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_CLUT_LUT7_WLUT;

/* Define the union U_ISP_LDCI_DRC_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ldci_calcdrc_wlut : 10; /* [9..0]  */
        unsigned int reserved_0 : 6; /* [15..10]  */
        unsigned int isp_ldci_statdrc_wlut : 10; /* [25..16]  */
        unsigned int reserved_1 : 6; /* [31..26]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LDCI_DRC_WLUT;

/* Define the union U_ISP_LDCI_HE_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ldci_hepos_wlut : 7; /* [6..0]  */
        unsigned int isp_ldci_heneg_wlut : 7; /* [13..7]  */
        unsigned int reserved_0 : 18; /* [31..14]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LDCI_HE_WLUT;

/* Define the union U_ISP_LDCI_DE_USM_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ldci_usmpos_wlut : 9; /* [8..0]  */
        unsigned int isp_ldci_usmneg_wlut : 9; /* [17..9]  */
        unsigned int isp_ldci_delut_wlut : 9; /* [26..18]  */
        unsigned int reserved_0 : 5; /* [31..27]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LDCI_DE_USM_WLUT;

/* Define the union U_ISP_LDCI_CGAIN_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ldci_cgain_wlut : 12; /* [11..0]  */
        unsigned int reserved_0 : 20; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LDCI_CGAIN_WLUT;

/* Define the union U_ISP_LDCI_POLYP_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ldci_poply1_wlut : 10; /* [9..0]  */
        unsigned int isp_ldci_poply2_wlut : 10; /* [19..10]  */
        unsigned int isp_ldci_poply3_wlut : 10; /* [29..20]  */
        unsigned int reserved_0 : 2; /* [31..30]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LDCI_POLYP_WLUT;

/* Define the union U_ISP_LDCI_POLYQ01_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ldci_plyq0_wlut : 12; /* [11..0]  */
        unsigned int reserved_0 : 4; /* [15..12]  */
        unsigned int isp_ldci_plyq1_wlut : 12; /* [27..16]  */
        unsigned int reserved_1 : 4; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LDCI_POLYQ01_WLUT;

/* Define the union U_ISP_LDCI_POLYQ23_WLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_ldci_plyq2_wlut : 12; /* [11..0]  */
        unsigned int reserved_0 : 4; /* [15..12]  */
        unsigned int isp_ldci_plyq3_wlut : 12; /* [27..16]  */
        unsigned int reserved_1 : 4; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LDCI_POLYQ23_WLUT;

/* Define the union U_ISP_DRC_TMLUT0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_drc_tmlut0_diff : 12; /* [11..0]  */
        unsigned int isp_drc_tmlut0_value : 16; /* [27..12]  */
        unsigned int reserved_0 : 4; /* [31..28]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_DRC_TMLUT0;

/* Define the union U_ISP_DRC_CCLUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_drc_cclut : 12; /* [11..0]  */
        unsigned int reserved_0 : 20; /* [31..12]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_DRC_CCLUT;

/* Define the union U_ISP_WDRSPLIT_LUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_wdrsplit_lut : 16; /* [15..0]  */
        unsigned int reserved_0 : 16; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_WDRSPLIT_LUT;

/* Define the union U_ISP_LOGLUT_LUT */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_loglut_lut : 21; /* [20..0]  */
        unsigned int reserved_0 : 11; /* [31..21]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_LOGLUT_LUT;

/* Define the union U_ISP_RLSC_LUT0 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_rlsc_lut0_ch0 : 16; /* [15..0]  */
        unsigned int isp_rlsc_lut0_ch1 : 16; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_RLSC_LUT0;

/* Define the union U_ISP_RLSC_LUT1 */
typedef union {
    /* Define the struct bits */
    struct {
        unsigned int isp_rlsc_lut1_ch2 : 16; /* [15..0]  */
        unsigned int isp_rlsc_lut1_ch3 : 16; /* [31..16]  */
    } bits;

    /* Define an unsigned member */
    unsigned int u32;

} U_ISP_RLSC_LUT1;

/* Define the global struct */
typedef struct {
    volatile U_ISP_AE_WEIGHT ISP_AE_WEIGHT[64]; /* 0x0~0xfc         */
    volatile U_ISP_DIS_REFINFO_WLUT ISP_DIS_REFINFO_WLUT[3072]; /* 0x100~0x30fc     */
    volatile U_ISP_LSC_RGAIN ISP_LSC_RGAIN[1089]; /* 0x3100~0x4200    */
    volatile unsigned int reserved_0[3]; /* 0x4204~0x420c    */
    volatile U_ISP_LSC_GRGAIN ISP_LSC_GRGAIN[1089]; /* 0x4210~0x5310    */
    volatile unsigned int reserved_1[3]; /* 0x5314~0x531c    */
    volatile U_ISP_LSC_BGAIN ISP_LSC_BGAIN[1089]; /* 0x5320~0x6420    */
    volatile unsigned int reserved_2[3]; /* 0x6424~0x642c    */
    volatile U_ISP_LSC_GBGAIN ISP_LSC_GBGAIN[1089]; /* 0x6430~0x7530    */
    volatile unsigned int reserved_3[3]; /* 0x7534~0x753c    */
    volatile U_ISP_FPN_LINE_WLUT ISP_FPN_LINE_WLUT[4096]; /* 0x7540~0xb53c    */
    volatile U_ISP_DPC_BPT_WLUT ISP_DPC_BPT_WLUT[4096]; /* 0xb540~0xf53c    */
    volatile U_ISP_SHARPEN_MFGAIND ISP_SHARPEN_MFGAIND[64]; /* 0xf540~0xf63c    */
    volatile U_ISP_SHARPEN_MFGAINUD ISP_SHARPEN_MFGAINUD[64]; /* 0xf640~0xf73c    */
    volatile U_ISP_SHARPEN_HFGAIND ISP_SHARPEN_HFGAIND[64]; /* 0xf740~0xf83c    */
    volatile U_ISP_SHARPEN_HFGAINUD ISP_SHARPEN_HFGAINUD[64]; /* 0xf840~0xf93c    */
    volatile U_ISP_DEMOSAIC_DEPURPLUT ISP_DEMOSAIC_DEPURPLUT[16]; /* 0xf940~0xf97c */
    volatile U_ISP_NDDM_GF_LUT ISP_NDDM_GF_LUT[17]; /* 0xf980~0xf9c0 */
    volatile unsigned int reserved_4[3]; /* 0xf9c4~0xf9cc */
    volatile U_ISP_BNR_LMT_EVEN ISP_BNR_LMT_EVEN[65]; /* 0xf9d0~0xfad0 */
    volatile unsigned int reserved_5[3]; /* 0xfad4~0xfadc */
    volatile U_ISP_BNR_LMT_ODD ISP_BNR_LMT_ODD[64]; /* 0xfae0~0xfbdc */
    volatile U_ISP_BNR_COR_EVEN ISP_BNR_COR_EVEN[17]; /* 0xfbe0~0xfc20 */
    volatile unsigned int reserved_6[3]; /* 0xfc24~0xfc2c */
    volatile U_ISP_BNR_COR_ODD ISP_BNR_COR_ODD[16]; /* 0xfc30~0xfc6c */
    volatile U_ISP_BNR_LSC_RGAIN ISP_BNR_LSC_RGAIN[1089]; /* 0xfc70~0x10d70 */
    volatile unsigned int reserved_7[3]; /* 0x10d74~0x10d7c */
    volatile U_ISP_BNR_LSC_GRGAIN ISP_BNR_LSC_GRGAIN[1089]; /* 0x10d80~0x11e80 */
    volatile unsigned int reserved_8[3]; /* 0x11e84~0x11e8c */
    volatile U_ISP_BNR_LSC_BGAIN ISP_BNR_LSC_BGAIN[1089]; /* 0x11e90~0x12f90 */
    volatile unsigned int reserved_9[3]; /* 0x12f94~0x12f9c */
    volatile U_ISP_BNR_LSC_GBGAIN ISP_BNR_LSC_GBGAIN[1089]; /* 0x12fa0~0x140a0 */
    volatile unsigned int reserved_10[3]; /* 0x140a4~0x140ac */
    volatile U_ISP_WDR_NOSLUT129X8 ISP_WDR_NOSLUT129X8[129]; /* 0x140b0~0x142b0 */
    volatile unsigned int reserved_11[3]; /* 0x142b4~0x142bc */
    volatile U_ISP_DEHAZE_PRESTAT ISP_DEHAZE_PRESTAT[512]; /* 0x142c0~0x14abc */
    volatile U_ISP_DEHAZE_LUT ISP_DEHAZE_LUT[256]; /* 0x14ac0~0x14ebc */
    volatile U_ISP_PREGAMMA_LUT ISP_PREGAMMA_LUT[257]; /* 0x14ec0~0x152c0 */
    volatile unsigned int reserved_12[3]; /* 0x152c4~0x152cc */
    volatile U_ISP_GAMMA_LUT ISP_GAMMA_LUT[1025]; /* 0x152d0~0x162d0 */
    volatile unsigned int reserved_13[3]; /* 0x162d4~0x162dc */
    volatile U_ISP_CA_LUT ISP_CA_LUT[256]; /* 0x162e0~0x166dc */
    volatile U_ISP_CLUT_LUT0_WLUT ISP_CLUT_LUT0_WLUT[729]; /* 0x166e0~0x17240 */
    volatile unsigned int reserved_14[3]; /* 0x17244~0x1724c */
    volatile U_ISP_CLUT_LUT1_WLUT ISP_CLUT_LUT1_WLUT[648]; /* 0x17250~0x17c6c */
    volatile U_ISP_CLUT_LUT2_WLUT ISP_CLUT_LUT2_WLUT[648]; /* 0x17c70~0x1868c */
    volatile U_ISP_CLUT_LUT3_WLUT ISP_CLUT_LUT3_WLUT[576]; /* 0x18690~0x18f8c */
    volatile U_ISP_CLUT_LUT4_WLUT ISP_CLUT_LUT4_WLUT[648]; /* 0x18f90~0x199ac */
    volatile U_ISP_CLUT_LUT5_WLUT ISP_CLUT_LUT5_WLUT[576]; /* 0x199b0~0x1a2ac */
    volatile U_ISP_CLUT_LUT6_WLUT ISP_CLUT_LUT6_WLUT[576]; /* 0x1a2b0~0x1abac */
    volatile U_ISP_CLUT_LUT7_WLUT ISP_CLUT_LUT7_WLUT[512]; /* 0x1abb0~0x1b3ac */
    volatile U_ISP_LDCI_DRC_WLUT ISP_LDCI_DRC_WLUT[65]; /* 0x1b3b0~0x1b4b0 */
    volatile unsigned int reserved_15[3]; /* 0x1b4b4~0x1b4bc */
    volatile U_ISP_LDCI_HE_WLUT ISP_LDCI_HE_WLUT[33]; /* 0x1b4c0~0x1b540 */
    volatile unsigned int reserved_16[3]; /* 0x1b544~0x1b54c */
    volatile U_ISP_LDCI_DE_USM_WLUT ISP_LDCI_DE_USM_WLUT[33]; /* 0x1b550~0x1b5d0 */
    volatile unsigned int reserved_17[3]; /* 0x1b5d4~0x1b5dc */
    volatile U_ISP_LDCI_CGAIN_WLUT ISP_LDCI_CGAIN_WLUT[65]; /* 0x1b5e0~0x1b6e0 */
    volatile unsigned int reserved_18[3]; /* 0x1b6e4~0x1b6ec */
    volatile U_ISP_LDCI_POLYP_WLUT ISP_LDCI_POLYP_WLUT[65]; /* 0x1b6f0~0x1b7f0 */
    volatile unsigned int reserved_19[3]; /* 0x1b7f4~0x1b7fc */
    volatile U_ISP_LDCI_POLYQ01_WLUT ISP_LDCI_POLYQ01_WLUT[65]; /* 0x1b800~0x1b900 */
    volatile unsigned int reserved_20[3]; /* 0x1b904~0x1b90c */
    volatile U_ISP_LDCI_POLYQ23_WLUT ISP_LDCI_POLYQ23_WLUT[65]; /* 0x1b910~0x1ba10 */
    volatile unsigned int reserved_21[3]; /* 0x1ba14~0x1ba1c */
    volatile U_ISP_DRC_TMLUT0 ISP_DRC_TMLUT0[200]; /* 0x1ba20~0x1bd3c */
    volatile U_ISP_DRC_CCLUT ISP_DRC_CCLUT[33]; /* 0x1bd40~0x1bdc0 */
    volatile unsigned int reserved_22[3]; /* 0x1bdc4~0x1bdcc */
    volatile U_ISP_WDRSPLIT_LUT ISP_WDRSPLIT_LUT[129]; /* 0x1bdd0~0x1bfd0 */
    volatile unsigned int reserved_23[3]; /* 0x1bfd4~0x1bfdc */
    volatile U_ISP_LOGLUT_LUT ISP_LOGLUT_LUT[1025]; /* 0x1bfe0~0x1cfe0 */
    volatile unsigned int reserved_24[3]; /* 0x1cfe4~0x1cfec */
    volatile U_ISP_RLSC_LUT0 ISP_RLSC_LUT0[130]; /* 0x1cff0~0x1d1f4 */
    volatile unsigned int reserved_25[2]; /* 0x1d1f8~0x1d1fc */
    volatile U_ISP_RLSC_LUT1 ISP_RLSC_LUT1[130]; /* 0x1d200~0x1d404 */

} S_ISP_LUT_REGS_TYPE;

#endif /* __ISP_LUT_DEFINE_H__ */
