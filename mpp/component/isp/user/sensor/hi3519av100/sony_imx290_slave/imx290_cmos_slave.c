/*
* Copyright (C) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
* Description:
* Author: Hisilicon multimedia software group
* Create: 2011/06/28
*/

#if !defined(__IMX290_CMOS_SLAVE_H_)
#define __IMX290_CMOS_SLAVE_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"

#include "imx290_cmos_slave_ex.h"
#include "imx290_slave_priv.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define IMX290_SLAVE_ID              292
#define IMX290_VS_TIME_MAX           0xFFFFFFFF
#define IMX290_SLAVE_INCREASE_LINES  (1) /* make real fps less than stand fps because NVR require */

/****************************************************************************
 * global variables                                                            *
 ****************************************************************************/
const IMX290_VIDEO_MODE_TBL_S g_astImx290ModeTbl[IMX290_MODE_BUTT] = {
    { 74500000, 1103, 1241667, 1125 + IMX290_SLAVE_INCREASE_LINES, 60, "1080P_60FPS_RAW10_SLAVE_2LANE" }, /* Actually 59.94fps */
};

ISP_SLAVE_SNS_SYNC_S gstImx290Sync[ISP_MAX_PIPE_NUM];

/* Sensor slave mode, default binding setting : Slave[0]->Pipe[0]&[1]; Slave[1]->Pipe[2]&[3]; Slave[2]->Pipe[4]&[5] */
HI_S32 g_Imx290SlaveBindDev[ISP_MAX_PIPE_NUM] = { 0, 0, 1, 1, 2, 2 };
HI_U32 g_Imx290SalveSensorModeTime[ISP_MAX_PIPE_NUM] = { 0, 0, 0, 0, 0, 0 };

ISP_SNS_STATE_S *g_pastImx290Slave[ISP_MAX_PIPE_NUM] = { HI_NULL };

#define IMX290SLAVE_SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_pastImx290Slave[dev])
#define IMX290SLAVE_SENSOR_SET_CTX(dev, pstCtx) (g_pastImx290Slave[dev] = pstCtx)
#define IMX290SLAVE_SENSOR_RESET_CTX(dev) (g_pastImx290Slave[dev] = HI_NULL)

ISP_SNS_COMMBUS_U g_aunImx290SlaveBusInfo[ISP_MAX_PIPE_NUM] = {
    [0] = { .s8I2cDev = 0 },
    [1 ... ISP_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1 }
};

static HI_U32 g_au32InitExposure[ISP_MAX_PIPE_NUM] = { 0 };
static HI_U32 g_au32LinesPer500ms[ISP_MAX_PIPE_NUM] = { 0 };

static HI_U16 g_au16InitWBGain[ISP_MAX_PIPE_NUM][3] = { { 0 } };
static HI_U16 g_au16SampleRgain[ISP_MAX_PIPE_NUM] = { 0 };
static HI_U16 g_au16SampleBgain[ISP_MAX_PIPE_NUM] = { 0 };

extern const unsigned int imx290slave_i2c_addr;
extern unsigned int imx290slave_addr_byte;
extern unsigned int imx290slave_data_byte;

typedef struct hiIMX290_SLAVE_STATE_S {
    HI_U8 u8Hcg;
    HI_U32 u32BRL;
    HI_U32 u32RHS1_MAX;
    HI_U32 u32RHS2_MAX;
} IMX290_SLAVE_STATE_S;

IMX290_SLAVE_STATE_S g_astimx290slaveState[ISP_MAX_PIPE_NUM] = { { 0 } };

extern void imx290slave_init(VI_PIPE ViPipe);
extern void imx290slave_exit(VI_PIPE ViPipe);
extern void imx290slave_standby(VI_PIPE ViPipe);
extern void imx290slave_restart(VI_PIPE ViPipe);
extern int imx290slave_write_register(VI_PIPE ViPipe, int addr, int data);
extern int imx290slave_read_register(VI_PIPE ViPipe, int addr);

/****************************************************************************
 * local variables                                                            *
 ****************************************************************************/
#define IMX290_SLAVE_FULL_LINES_MAX  (0x3FFFF)

/* Imx290Slave Register Address */
#define IMX290_SLAVE_SHS1_ADDR       (0x3020)
#define IMX290_SLAVE_SHS2_ADDR       (0x3024)
#define IMX290_SLAVE_SHS3_ADDR       (0x3028)
#define IMX290_SLAVE_GAIN_ADDR       (0x3014)
#define IMX290_SLAVE_HCG_ADDR        (0x3009)
#define IMX290_SLAVE_VMAX_ADDR       (0x3018)
#define IMX290_SLAVE_HMAX_ADDR       (0x301c)
#define IMX290_SLAVE_Y_OUT_SIZE_ADDR (0x3418)

// sensor fps mode
#define IMX290_SLAVE_RES_IS_1080P(w, h) ((w) <= 1920 && (h) <= 1080)

static HI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstAeSnsDft);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(&pstAeSnsDft->stAERouteAttr, 0, sizeof(ISP_AE_ROUTE_S));

    pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FlickerFreq = 50 * 256;
    pstAeSnsDft->u32FullLinesMax = IMX290_SLAVE_FULL_LINES_MAX;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->stIntTimeAccu.f32Offset = 0;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 1;

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 1;

    pstAeSnsDft->u32ISPDgainShift = 8;
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
    pstAeSnsDft->u32MaxISPDgainTarget = 2 << pstAeSnsDft->u32ISPDgainShift;

    if (g_au32LinesPer500ms[ViPipe] == 0) {
        pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * 30 / 2;
    } else {
        pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
    }

    pstAeSnsDft->enMaxIrisFNO = ISP_IRIS_F_NO_1_0;
    pstAeSnsDft->enMinIrisFNO = ISP_IRIS_F_NO_32_0;

    pstAeSnsDft->bAERouteExValid = HI_FALSE;
    pstAeSnsDft->stAERouteAttr.u32TotalNum = 0;
    pstAeSnsDft->stAERouteAttrEx.u32TotalNum = 0;

    switch (pstSnsState->enWDRMode) {
        default:
        case WDR_MODE_NONE: /* linear mode */
            pstAeSnsDft->au8HistThresh[0] = 0xd;
            pstAeSnsDft->au8HistThresh[1] = 0x28;
            pstAeSnsDft->au8HistThresh[2] = 0x60;
            pstAeSnsDft->au8HistThresh[3] = 0x80;

            pstAeSnsDft->u32MaxAgain = 62564;
            pstAeSnsDft->u32MinAgain = 1024;
            pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
            pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

            pstAeSnsDft->u32MaxDgain = 38577;
            pstAeSnsDft->u32MinDgain = 1024;
            pstAeSnsDft->u32MaxDgainTarget = 20013;
            pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

            pstAeSnsDft->u8AeCompensation = 0x38;
            pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;

            pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] : 148859;

            pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 1;
            pstAeSnsDft->u32MaxIntTimeTarget = 65535;
            pstAeSnsDft->u32MinIntTimeTarget = 1;
            break;
    }

    return HI_SUCCESS;
}

/* the function of sensor set fps */
static HI_VOID cmos_fps_set(VI_PIPE ViPipe, HI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    HI_FLOAT f32maxFps = 0;
    HI_U32 u32Lines = 0;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    f32maxFps = g_astImx290ModeTbl[pstSnsState->u8ImgMode].f32MaxFps;

    switch (pstSnsState->u8ImgMode) {
        case IMX290_2M60FPS_LINER_MODE:
            if ((f32Fps <= f32maxFps) && (f32Fps >= 0.5)) {
                u32Lines = (g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32VertiLines) * f32maxFps / DIV_0_TO_1_FLOAT(f32Fps);
                gstImx290Sync[ViPipe].u32VsTime = (g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32InckPerVs) * (f32maxFps / DIV_0_TO_1_FLOAT(f32Fps));
                pstAeSnsDft->u32LinesPer500ms = (HI_U32)(g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32VertiLines*f32maxFps / 2);
            } else {
                ISP_ERR_TRACE("Not support Fps: %f\n", f32Fps);
                return;
            }
            u32Lines = (u32Lines > IMX290_SLAVE_FULL_LINES_MAX) ? IMX290_SLAVE_FULL_LINES_MAX : u32Lines;
            break;

        default:
            return;
            break;
    }

    if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
        pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = (u32Lines & 0xFF);
        pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = ((u32Lines & 0xFF00) >> 8);
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = ((u32Lines & 0xF0000) >> 16);
    }

    pstSnsState->u32FLStd = u32Lines;
    pstSnsState->astRegsInfo[0].stSlvSync.u32SlaveVsTime = gstImx290Sync[ViPipe].u32VsTime;

    pstAeSnsDft->f32Fps = f32Fps;
    pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
    pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
    pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
    pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];

    return;
}

static HI_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, HI_U32 u32FullLines,
                                       AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER_VOID(pstAeSnsDft);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32FullLines = (u32FullLines > IMX290_SLAVE_FULL_LINES_MAX) ? IMX290_SLAVE_FULL_LINES_MAX : u32FullLines;
    gstImx290Sync[ViPipe].u32VsTime = MIN((HI_U64)g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32InckPerHs * u32FullLines, IMX290_VS_TIME_MAX);
    pstSnsState->au32FL[0] = u32FullLines;
    pstSnsState->astRegsInfo[0].stSlvSync.u32SlaveVsTime = gstImx290Sync[ViPipe].u32VsTime;

    if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
        pstSnsState->astRegsInfo[0].astI2cData[5].u32Data = (pstSnsState->au32FL[0] & 0xFF);
        pstSnsState->astRegsInfo[0].astI2cData[6].u32Data = ((pstSnsState->au32FL[0] & 0xFF00) >> 8);
        pstSnsState->astRegsInfo[0].astI2cData[7].u32Data = ((pstSnsState->au32FL[0] & 0xF0000) >> 16);
    }

    pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];
    pstAeSnsDft->u32MaxIntTime = pstSnsState->au32FL[0] - 2;

    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static HI_VOID cmos_inttime_update(VI_PIPE ViPipe, HI_U32 u32IntTime)
{
    HI_U32 u32Value = 0;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    u32Value = pstSnsState->au32FL[0] - u32IntTime - 1;

    pstSnsState->astRegsInfo[0].astI2cData[0].u32Data = (u32Value & 0xFF);
    pstSnsState->astRegsInfo[0].astI2cData[1].u32Data = ((u32Value & 0xFF00) >> 8);
    pstSnsState->astRegsInfo[0].astI2cData[2].u32Data = ((u32Value & 0x30000) >> 16);

    return;
}

static HI_U32 gain_table[262] = {
    1024, 1059, 1097, 1135, 1175, 1217, 1259, 1304, 1349, 1397, 1446, 1497, 1549, 1604, 1660, 1719, 1779, 1842, 1906,
    1973, 2043, 2048, 2119, 2194, 2271, 2351, 2434, 2519, 2608, 2699, 2794, 2892, 2994, 3099, 3208, 3321, 3438, 3559,
    3684, 3813, 3947, 4086, 4229, 4378, 4532, 4691, 4856, 5027, 5203, 5386, 5576, 5772, 5974, 6184, 6402, 6627, 6860,
    7101, 7350, 7609, 7876, 8153, 8439, 8736, 9043, 9361, 9690, 10030, 10383, 10748, 11125, 11516, 11921, 12340, 12774,
    13222, 13687, 14168, 14666, 15182, 15715, 16267, 16839, 17431, 18043, 18677, 19334, 20013, 20717, 21445, 22198,
    22978, 23786, 24622, 25487, 26383, 27310, 28270, 29263, 30292, 31356, 32458, 33599, 34780, 36002, 37267, 38577,
    39932, 41336, 42788, 44292, 45849, 47460, 49128, 50854, 52641, 54491, 56406, 58388, 60440, 62564, 64763, 67039,
    69395, 71833, 74358, 76971, 79676, 82476, 85374, 88375, 91480, 94695, 98023, 101468, 105034, 108725, 112545,
    116501, 120595, 124833, 129220, 133761, 138461, 143327, 148364, 153578, 158975, 164562, 170345, 176331, 182528,
    188942, 195582, 202455, 209570, 216935, 224558, 232450, 240619, 249074, 257827, 266888, 276267, 285976, 296026,
    306429, 317197, 328344, 339883, 351827, 364191, 376990, 390238, 403952, 418147, 432842, 448053, 463799, 480098,
    496969, 514434, 532512, 551226, 570597, 590649, 611406, 632892, 655133, 678156, 701988, 726657, 752194, 778627,
    805990, 834314, 863634, 893984, 925400, 957921, 991585, 1026431, 1062502, 1099841, 1138491, 1178500, 1219916,
    1262786, 1307163, 1353100, 1400651, 1449872, 1500824, 1553566, 1608162, 1664676, 1723177, 1783733, 1846417,
    1911304, 1978472, 2048000, 2119971, 2194471, 2271590, 2351418, 2434052, 2519590, 2608134, 2699789, 2794666,
    2892876, 2994538, 3099773, 3208706, 3321467, 3438190, 3559016, 3684087, 3813554, 3947571, 4086297, 4229898,
    4378546, 4532417, 4691696, 4856573, 5027243, 5203912, 5386788, 5576092, 5772048, 5974890, 6184861, 6402210,
    6627198, 6860092, 7101170, 7350721, 7609041, 7876439, 8153234
};

static HI_VOID cmos_again_calc_table(VI_PIPE ViPipe, HI_U32 *pu32AgainLin, HI_U32 *pu32AgainDb)
{
    int i;

    CMOS_CHECK_POINTER_VOID(pu32AgainLin);
    CMOS_CHECK_POINTER_VOID(pu32AgainDb);

    if (*pu32AgainLin >= gain_table[120]) {
        *pu32AgainLin = gain_table[120];
        *pu32AgainDb = 120;
        return;
    }

    for (i = 1; i < 121; i++) {
        if (*pu32AgainLin < gain_table[i]) {
            *pu32AgainLin = gain_table[i - 1];
            *pu32AgainDb = i - 1;
            break;
        }
    }
    return;
}

static HI_VOID cmos_dgain_calc_table(VI_PIPE ViPipe, HI_U32 *pu32DgainLin, HI_U32 *pu32DgainDb)
{
    int i;

    CMOS_CHECK_POINTER_VOID(pu32DgainLin);
    CMOS_CHECK_POINTER_VOID(pu32DgainDb);

    if (*pu32DgainLin >= gain_table[106]) {
        *pu32DgainLin = gain_table[106];
        *pu32DgainDb = 106;
        return;
    }

    for (i = 1; i < 106; i++) {
        if (*pu32DgainLin < gain_table[i]) {
            *pu32DgainLin = gain_table[i - 1];
            *pu32DgainDb = i - 1;
            break;
        }
    }

    return;
}

static HI_VOID cmos_gains_update(VI_PIPE ViPipe, HI_U32 u32Again, HI_U32 u32Dgain)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;
    HI_U32 u32HCG = g_astimx290slaveState[ViPipe].u8Hcg;
    HI_U32 u32Tmp;

    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    if (u32Again >= 21) {
        u32HCG = u32HCG | 0x10;  // bit[4] HCG  .Reg0x3009[7:0]
        u32Again = u32Again - 21;
    }

    u32Tmp = u32Again + u32Dgain;

    pstSnsState->astRegsInfo[0].astI2cData[3].u32Data = (u32Tmp & 0xFF);
    pstSnsState->astRegsInfo[0].astI2cData[4].u32Data = (u32HCG & 0xFF);

    return;
}

static HI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table = cmos_dgain_calc_table;

    return HI_SUCCESS;
}

/* Rgain and Bgain of the golden sample */
#define GOLDEN_RGAIN                 0
#define GOLDEN_BGAIN                 0
static HI_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstAwbSnsDft);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));
    pstAwbSnsDft->u16WbRefTemp = 4900;

    pstAwbSnsDft->au16GainOffset[0] = 0x1C3;
    pstAwbSnsDft->au16GainOffset[1] = 0x100;
    pstAwbSnsDft->au16GainOffset[2] = 0x100;
    pstAwbSnsDft->au16GainOffset[3] = 0x1D4;

    pstAwbSnsDft->as32WbPara[0] = -37;
    pstAwbSnsDft->as32WbPara[1] = 293;
    pstAwbSnsDft->as32WbPara[2] = 0;
    pstAwbSnsDft->as32WbPara[3] = 179537;
    pstAwbSnsDft->as32WbPara[4] = 128;
    pstAwbSnsDft->as32WbPara[5] = -123691;

    pstAwbSnsDft->u16GoldenRgain = GOLDEN_RGAIN;
    pstAwbSnsDft->u16GoldenBgain = GOLDEN_BGAIN;

    switch (pstSnsState->enWDRMode) {
        default:
        case WDR_MODE_NONE:
            memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm, sizeof(AWB_CCM_S));
            memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));
            break;
    }

    pstAwbSnsDft->u16InitRgain = g_au16InitWBGain[ViPipe][0];
    pstAwbSnsDft->u16InitGgain = g_au16InitWBGain[ViPipe][1];
    pstAwbSnsDft->u16InitBgain = g_au16InitWBGain[ViPipe][2];
    pstAwbSnsDft->u16SampleRgain = g_au16SampleRgain[ViPipe];
    pstAwbSnsDft->u16SampleBgain = g_au16SampleBgain[ViPipe];

    return HI_SUCCESS;
}

static HI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    CMOS_CHECK_POINTER(pstExpFuncs);

    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

    return HI_SUCCESS;
}

static ISP_CMOS_DNG_COLORPARAM_S g_stDngColorParam = {
    { 378, 256, 430 },
    { 439, 256, 439 }
};

static HI_S32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstDef);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));

    pstDef->unKey.bit1Lsc = 0;
    pstDef->pstLsc = &g_stCmosLsc;
    pstDef->unKey.bit1RLsc = 0;
    pstDef->pstRLsc = &g_stCmosRLsc;
    pstDef->unKey.bit1Ca = 1;
    pstDef->pstCa = &g_stIspCA;
    pstDef->unKey.bit1Clut = 1;
    pstDef->pstClut = &g_stIspCLUT;
    pstDef->unKey.bit1EdgeMark = 0;
    pstDef->pstEdgeMark = &g_stIspEdgeMark;
    pstDef->unKey.bit1Wdr = 1;
    pstDef->pstWdr = &g_stIspWDR;

    switch (pstSnsState->enWDRMode) {
        default:
        case WDR_MODE_NONE:
            pstDef->unKey.bit1Demosaic = 1;
            pstDef->pstDemosaic = &g_stIspDemosaic;
            pstDef->unKey.bit1Sharpen = 1;
            pstDef->pstSharpen = &g_stIspYuvSharpen;
            pstDef->unKey.bit1Drc = 1;
            pstDef->pstDrc = &g_stIspDRC;
            pstDef->unKey.bit1Gamma = 1;
            pstDef->pstGamma = &g_stIspGamma;
            pstDef->unKey.bit1BayerNr = 1;
            pstDef->pstBayerNr = &g_stIspBayerNr;
            pstDef->unKey.bit1Ge = 1;
            pstDef->pstGe = &g_stIspGe;
            pstDef->unKey.bit1AntiFalseColor = 1;
            pstDef->pstAntiFalseColor = &g_stIspAntiFalseColor;
            pstDef->unKey.bit1Dpc = 1;
            pstDef->pstDpc = &g_stCmosDpc;
            pstDef->unKey.bit1Ldci = 1;
            pstDef->pstLdci = &g_stIspLdci;
            memcpy(&pstDef->stNoiseCalibration, &g_stIspNoiseCalibration, sizeof(ISP_CMOS_NOISE_CALIBRATION_S));

            break;
    }
    pstDef->stSensorMode.u32SensorID = IMX290_SLAVE_ID;
    pstDef->stSensorMode.u8SensorMode = pstSnsState->u8ImgMode;

    memcpy(&pstDef->stDngColorParam, &g_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));
    switch (pstSnsState->u8ImgMode) {
        default:
        case IMX290_2M60FPS_LINER_MODE:
            pstDef->stSensorMode.stDngRawFormat.u8BitsPerSample = 10;
            pstDef->stSensorMode.stDngRawFormat.u32WhiteLevel = 1023;
            break;
    }
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Numerator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Denominator = 1;
    pstDef->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Numerator = 1;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimCols = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatRows = 2;
    pstDef->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatCols = 2;
    pstDef->stSensorMode.stDngRawFormat.enCfaLayout = CFALAYOUT_TYPE_RECTANGULAR;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPlaneColor[2] = 2;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[0] = 0;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[1] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[2] = 1;
    pstDef->stSensorMode.stDngRawFormat.au8CfaPattern[3] = 2;
    pstDef->stSensorMode.bValidDngRawFormat = HI_TRUE;

    return HI_SUCCESS;
}

static HI_S32 cmos_get_isp_black_level(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
    HI_S32 i;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstBlackLevel);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_FALSE;

    /* black level of linear mode */
    if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
        for (i = 0; i < 4; i++) {
            pstBlackLevel->au16BlackLevel[i] = 0xF0;  // 240
        }
    } else {
    /* black level of DOL mode */
        pstBlackLevel->au16BlackLevel[0] = 0xF0;
        pstBlackLevel->au16BlackLevel[1] = 0xF0;
        pstBlackLevel->au16BlackLevel[2] = 0xF0;
        pstBlackLevel->au16BlackLevel[3] = 0xF0;
    }

    return HI_SUCCESS;
}

static HI_VOID cmos_set_pixel_detect(VI_PIPE ViPipe, HI_BOOL bEnable)
{
    HI_U32 u32FullLines_5Fps, u32MaxIntTime_5Fps;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    /* Detect set 5fps */
    CHECK_RET(HI_MPI_ISP_GetSnsSlaveAttr(ViPipe, &gstImx290Sync[ViPipe]));

    if (pstSnsState->enWDRMode == WDR_MODE_2To1_LINE || pstSnsState->enWDRMode == WDR_MODE_3To1_LINE) {
        return;
    } else {

        if (pstSnsState->u8ImgMode == IMX290_2M60FPS_LINER_MODE) {
            gstImx290Sync[ViPipe].u32VsTime = (g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32InckPerVs) * g_astImx290ModeTbl[pstSnsState->u8ImgMode].f32MaxFps / 5;
            u32FullLines_5Fps = (g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32VertiLines) * g_astImx290ModeTbl[pstSnsState->u8ImgMode].f32MaxFps / 5;
        } else {

            return;
        }
    }

    u32MaxIntTime_5Fps = 4;

    if (bEnable) { /* setup for ISP pixel calibration mode */
        imx290slave_write_register(ViPipe, IMX290_SLAVE_GAIN_ADDR, 0x00);

        imx290slave_write_register(ViPipe, IMX290_SLAVE_VMAX_ADDR, u32FullLines_5Fps & 0xFF);
        imx290slave_write_register(ViPipe, IMX290_SLAVE_VMAX_ADDR + 1, (u32FullLines_5Fps & 0xFF00) >> 8);
        imx290slave_write_register(ViPipe, IMX290_SLAVE_VMAX_ADDR + 2, (u32FullLines_5Fps & 0xF0000) >> 16);

        imx290slave_write_register(ViPipe, IMX290_SLAVE_SHS1_ADDR, u32MaxIntTime_5Fps & 0xFF);
        imx290slave_write_register(ViPipe, IMX290_SLAVE_SHS1_ADDR + 1, (u32MaxIntTime_5Fps & 0xFF00) >> 8);
        imx290slave_write_register(ViPipe, IMX290_SLAVE_SHS1_ADDR + 2, (u32MaxIntTime_5Fps & 0xF0000) >> 16);

    } else { /* setup for ISP 'normal mode' */
        pstSnsState->u32FLStd = (pstSnsState->u32FLStd > 0x1FFFF) ? 0x1FFFF : pstSnsState->u32FLStd;
        gstImx290Sync[ViPipe].u32VsTime = (g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32InckPerVs);
        imx290slave_write_register(ViPipe, IMX290_SLAVE_VMAX_ADDR, pstSnsState->u32FLStd & 0xFF);
        imx290slave_write_register(ViPipe, IMX290_SLAVE_VMAX_ADDR + 1, (pstSnsState->u32FLStd & 0xFF00) >> 8);
        imx290slave_write_register(ViPipe, IMX290_SLAVE_VMAX_ADDR + 2, (pstSnsState->u32FLStd & 0xF0000) >> 16);
        pstSnsState->bSyncInit = HI_FALSE;
    }
    CHECK_RET(HI_MPI_ISP_SetSnsSlaveAttr(ViPipe, &gstImx290Sync[ViPipe]));

    return;
}

static HI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, HI_U8 u8Mode)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    pstSnsState->bSyncInit = HI_FALSE;

    switch (u8Mode & 0x3f) {
        case WDR_MODE_NONE:
            pstSnsState->enWDRMode = WDR_MODE_NONE;
            printf("linear mode\n");
            break;

        default:
            ISP_ERR_TRACE("NOT support this mode!\n");
            return HI_FAILURE;
    }

    memset(pstSnsState->au32WDRIntTime, 0, sizeof(pstSnsState->au32WDRIntTime));

    return HI_SUCCESS;
}

static HI_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    HI_S32 i;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstSnsRegsInfo);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    if ((pstSnsState->bSyncInit == HI_FALSE) || (pstSnsRegsInfo->bConfig == HI_FALSE)) {
        pstSnsState->astRegsInfo[0].enSnsType = ISP_SNS_I2C_TYPE;
        pstSnsState->astRegsInfo[0].unComBus.s8I2cDev = g_aunImx290SlaveBusInfo[ViPipe].s8I2cDev;
        pstSnsState->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        pstSnsState->astRegsInfo[0].u32RegNum = 8;

        for (i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++) {
            pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            pstSnsState->astRegsInfo[0].astI2cData[i].u8DevAddr = imx290slave_i2c_addr;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32AddrByteNum = imx290slave_addr_byte;
            pstSnsState->astRegsInfo[0].astI2cData[i].u32DataByteNum = imx290slave_data_byte;
        }

        // Linear Mode Regs
        pstSnsState->astRegsInfo[0].astI2cData[0].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[0].u32RegAddr = IMX290_SLAVE_SHS1_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[1].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[1].u32RegAddr = IMX290_SLAVE_SHS1_ADDR + 1;
        pstSnsState->astRegsInfo[0].astI2cData[2].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[2].u32RegAddr = IMX290_SLAVE_SHS1_ADDR + 2;

        pstSnsState->astRegsInfo[0].astI2cData[3].u8DelayFrmNum = 0;  // make shutter and gain effective at the same time
        pstSnsState->astRegsInfo[0].astI2cData[3].u32RegAddr = IMX290_SLAVE_GAIN_ADDR;  // gain
        pstSnsState->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 1;
        pstSnsState->astRegsInfo[0].astI2cData[4].u32RegAddr = IMX290_SLAVE_HCG_ADDR;

        pstSnsState->astRegsInfo[0].astI2cData[5].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[5].u32RegAddr = IMX290_SLAVE_VMAX_ADDR;
        pstSnsState->astRegsInfo[0].astI2cData[6].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[6].u32RegAddr = IMX290_SLAVE_VMAX_ADDR + 1;
        pstSnsState->astRegsInfo[0].astI2cData[7].u8DelayFrmNum = 0;
        pstSnsState->astRegsInfo[0].astI2cData[7].u32RegAddr = IMX290_SLAVE_VMAX_ADDR + 2;

        /* Slave Sensor VsTime cfg */
        pstSnsState->astRegsInfo[0].stSlvSync.bUpdate = HI_TRUE;
        pstSnsState->astRegsInfo[0].stSlvSync.u8DelayFrmNum = 1;
        pstSnsState->astRegsInfo[0].stSlvSync.u32SlaveBindDev = g_Imx290SlaveBindDev[ViPipe];

        pstSnsState->bSyncInit = HI_TRUE;

    } else {

        for (i = 0; i < pstSnsState->astRegsInfo[0].u32RegNum; i++) {
            if (pstSnsState->astRegsInfo[0].astI2cData[i].u32Data == pstSnsState->astRegsInfo[1].astI2cData[i].u32Data) {
                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = HI_FALSE;
            } else {

                pstSnsState->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            }
        }

        if (pstSnsState->astRegsInfo[0].stSlvSync.u32SlaveVsTime == pstSnsState->astRegsInfo[1].stSlvSync.u32SlaveVsTime) {
            pstSnsState->astRegsInfo[0].stSlvSync.bUpdate = HI_FALSE;
        } else {
            pstSnsState->astRegsInfo[0].stSlvSync.bUpdate = HI_TRUE;
        }
    }

    pstSnsRegsInfo->bConfig = HI_FALSE;
    memcpy(pstSnsRegsInfo, &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&pstSnsState->astRegsInfo[1], &pstSnsState->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    return HI_SUCCESS;
}

static HI_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    HI_U8 u8SensorImageMode = 0;
    HI_FLOAT f32maxFps = 0;
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    CMOS_CHECK_POINTER(pstSensorImageMode);
    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER(pstSnsState);

    u8SensorImageMode = pstSnsState->u8ImgMode;
    pstSnsState->bSyncInit = HI_FALSE;
    f32maxFps = g_astImx290ModeTbl[pstSnsState->u8ImgMode].f32MaxFps;

    if (pstSensorImageMode->f32Fps <= f32maxFps) {
        if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
            if (IMX290_SLAVE_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
                u8SensorImageMode = IMX290_2M60FPS_LINER_MODE;
                g_astimx290slaveState[ViPipe].u8Hcg = 0x1;
            } else {
                ISP_ERR_TRACE("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                          pstSensorImageMode->u16Width,
                          pstSensorImageMode->u16Height,
                          pstSensorImageMode->f32Fps,
                          pstSnsState->enWDRMode);
                return HI_FAILURE;
            }
        } else {

            ISP_ERR_TRACE("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
                      pstSensorImageMode->u16Width,
                      pstSensorImageMode->u16Height,
                      pstSensorImageMode->f32Fps,
                      pstSnsState->enWDRMode);
            return HI_FAILURE;
        }
    } else {
    }

    if ((pstSnsState->bInit == HI_TRUE) && (u8SensorImageMode == pstSnsState->u8ImgMode)) {
        /* Don't need to switch SensorImageMode */
        return ISP_DO_NOT_NEED_SWITCH_IMAGEMODE;
    }

    pstSnsState->u8ImgMode = u8SensorImageMode;
    pstSnsState->u32FLStd = g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32VertiLines;
    pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
    pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

    return HI_SUCCESS;
}

static HI_VOID sensor_global_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pstSnsState = HI_NULL;

    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pstSnsState);
    CMOS_CHECK_POINTER_VOID(pstSnsState);

    pstSnsState->bInit = HI_FALSE;
    pstSnsState->bSyncInit = HI_FALSE;
    pstSnsState->u8ImgMode = IMX290_2M60FPS_LINER_MODE;
    pstSnsState->enWDRMode = WDR_MODE_NONE;
    pstSnsState->u32FLStd = g_astImx290ModeTbl[pstSnsState->u8ImgMode].u32VertiLines;
    pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
    pstSnsState->au32FL[1] = pstSnsState->u32FLStd;

    memset(&pstSnsState->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&pstSnsState->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));
}

static HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    CMOS_CHECK_POINTER(pstSensorExpFunc);

    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init = imx290slave_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit = imx290slave_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;

    pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    return HI_SUCCESS;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
static HI_S32 imx290slave_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunImx290SlaveBusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return HI_SUCCESS;
}

static HI_S32 sensor_ctx_init(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = HI_NULL;

    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

    if (pastSnsStateCtx == HI_NULL) {
        pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
        if (pastSnsStateCtx == HI_NULL) {
            ISP_ERR_TRACE("Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
            return HI_ERR_ISP_NOMEM;
        }
    }

    memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));

    IMX290SLAVE_SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

    return HI_SUCCESS;
}

static HI_VOID sensor_ctx_exit(VI_PIPE ViPipe)
{
    ISP_SNS_STATE_S *pastSnsStateCtx = HI_NULL;

    IMX290SLAVE_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
    SENSOR_FREE(pastSnsStateCtx);
    IMX290SLAVE_SENSOR_RESET_CTX(ViPipe);
}

static HI_S32 sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S stSnsAttrInfo;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = sensor_ctx_init(ViPipe);

    if (s32Ret != HI_SUCCESS) {
        return HI_FAILURE;
    }

    stSnsAttrInfo.eSensorId = IMX290_SLAVE_ID;

    s32Ret = cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret |= HI_MPI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);

    if (s32Ret != HI_SUCCESS) {
        ISP_ERR_TRACE("sensor register callback function failed!\n");
        return s32Ret;
    }

    s32Ret = cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret |= HI_MPI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);

    if (s32Ret != HI_SUCCESS) {
        ISP_ERR_TRACE("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret |= HI_MPI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);

    if (s32Ret != HI_SUCCESS) {
        ISP_ERR_TRACE("sensor register callback function to awb lib failed!\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 sensor_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;

    CMOS_CHECK_POINTER(pstAeLib);
    CMOS_CHECK_POINTER(pstAwbLib);

    s32Ret = HI_MPI_ISP_SensorUnRegCallBack(ViPipe, IMX290_SLAVE_ID);
    if (s32Ret != HI_SUCCESS) {
        ISP_ERR_TRACE("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, IMX290_SLAVE_ID);
    if (s32Ret != HI_SUCCESS) {
        ISP_ERR_TRACE("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, IMX290_SLAVE_ID);
    if (s32Ret != HI_SUCCESS) {
        ISP_ERR_TRACE("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    sensor_ctx_exit(ViPipe);

    return HI_SUCCESS;
}

static HI_S32 sensor_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
    CMOS_CHECK_POINTER(pstInitAttr);

    g_au32InitExposure[ViPipe] = pstInitAttr->u32Exposure;
    g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
    g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
    g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
    g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
    g_au16SampleRgain[ViPipe] = pstInitAttr->u16SampleRgain;
    g_au16SampleBgain[ViPipe] = pstInitAttr->u16SampleBgain;

    return HI_SUCCESS;
}

ISP_SNS_OBJ_S stSnsImx290SlaveObj = {
    .pfnRegisterCallback = sensor_register_callback,
    .pfnUnRegisterCallback = sensor_unregister_callback,
    .pfnStandby = imx290slave_standby,
    .pfnRestart = imx290slave_restart,
    .pfnMirrorFlip = HI_NULL,
    .pfnWriteReg = imx290slave_write_register,
    .pfnReadReg = imx290slave_read_register,
    .pfnSetBusInfo = imx290slave_set_bus_info,
    .pfnSetInit = sensor_set_init
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __IMX290_CMOS_SLAVE_H_ */
