/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "proposal_common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef NEON_SUPPORT
#include <arm_neon.h>
#endif

static const HI_U32 PROPOSAL_COORDINATE_MIN_X_INDEX = 0;
static const HI_U32 PROPOSAL_COORDINATE_MIN_Y_INDEX = 1;
static const HI_U32 PROPOSAL_COORDINATE_MAX_X_INDEX = 2;
static const HI_U32 PROPOSAL_COORDINATE_MAX_Y_INDEX = 3;
static const HI_U32 PROPOSAL_BBOX_SCORE_INDEX = 4;
static const HI_U32 PROPOSAL_RPN_SUPRESS_INDEX = 5;
static const HI_U32 BBOX_DELTA_CENTER_X_INDEX = 0;
static const HI_U32 BBOX_DELTA_CENTER_Y_INDEX = 1;
static const HI_U32 BBOX_DELTA_PRED_W_INDEX = 2;
static const HI_U32 BBOX_DELTA_PRED_H_INDEX = 3;
static const HI_U32 EXP_COEF_BASE_OFFSET = 5;

#define PROPOSAL_LENGTH_HALF_COEFFICIENT 0.5f
/*
* reference QuickExp algorithm
*/
HI_FLOAT g_af32ExpCoef[10][16] = {
    {   (HI_FLOAT)1, (HI_FLOAT)1.00024, (HI_FLOAT)1.00049, (HI_FLOAT)1.00073,
        (HI_FLOAT)1.00098, (HI_FLOAT)1.00122, (HI_FLOAT)1.00147, (HI_FLOAT)1.00171,
        (HI_FLOAT)1.00196, (HI_FLOAT)1.0022, (HI_FLOAT)1.00244, (HI_FLOAT)1.00269,
        (HI_FLOAT)1.00293, (HI_FLOAT)1.00318, (HI_FLOAT)1.00342, (HI_FLOAT)1.00367
    },
    {   (HI_FLOAT)1, (HI_FLOAT)1.00391, (HI_FLOAT)1.00784, (HI_FLOAT)1.01179,
        (HI_FLOAT)1.01575, (HI_FLOAT)1.01972, (HI_FLOAT)1.02371, (HI_FLOAT)1.02772,
        (HI_FLOAT)1.03174, (HI_FLOAT)1.03578, (HI_FLOAT)1.03984, (HI_FLOAT)1.04391,
        (HI_FLOAT)1.04799, (HI_FLOAT)1.05209, (HI_FLOAT)1.05621, (HI_FLOAT)1.06034
    },
    {   (HI_FLOAT)1, (HI_FLOAT)1.06449, (HI_FLOAT)1.13315, (HI_FLOAT)1.20623,
        (HI_FLOAT)1.28403, (HI_FLOAT)1.36684, (HI_FLOAT)1.45499, (HI_FLOAT)1.54883,
        (HI_FLOAT)1.64872, (HI_FLOAT)1.75505, (HI_FLOAT)1.86825, (HI_FLOAT)1.98874,
        (HI_FLOAT)2.117, (HI_FLOAT)2.25353, (HI_FLOAT)2.39888, (HI_FLOAT)2.55359
    },
    {   (HI_FLOAT)1, (HI_FLOAT)2.71828, (HI_FLOAT)7.38906, (HI_FLOAT)20.0855,
        (HI_FLOAT)54.5981, (HI_FLOAT)148.413, (HI_FLOAT)403.429, (HI_FLOAT)1096.63,
        (HI_FLOAT)2980.96, (HI_FLOAT)8103.08, (HI_FLOAT)22026.5, (HI_FLOAT)59874.1,
        (HI_FLOAT)162755, (HI_FLOAT)442413, (HI_FLOAT)1.2026e+006, (HI_FLOAT)3.26902e+006
    },
    {   (HI_FLOAT)1, (HI_FLOAT)8.88611e+006, (HI_FLOAT)7.8963e+013, (HI_FLOAT)7.01674e+020,
        (HI_FLOAT)6.23515e+027, (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034,
        (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034,
        (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034, (HI_FLOAT)5.54062e+034
    },
    {   (HI_FLOAT)1, (HI_FLOAT)0.999756, (HI_FLOAT)0.999512, (HI_FLOAT)0.999268,
        (HI_FLOAT)0.999024, (HI_FLOAT)0.99878, (HI_FLOAT)0.998536, (HI_FLOAT)0.998292,
        (HI_FLOAT)0.998049, (HI_FLOAT)0.997805, (HI_FLOAT)0.997562, (HI_FLOAT)0.997318,
        (HI_FLOAT)0.997075, (HI_FLOAT)0.996831, (HI_FLOAT)0.996588, (HI_FLOAT)0.996345
    },
    {   (HI_FLOAT)1, (HI_FLOAT)0.996101, (HI_FLOAT)0.992218, (HI_FLOAT)0.98835,
        (HI_FLOAT)0.984496, (HI_FLOAT)0.980658, (HI_FLOAT)0.976835, (HI_FLOAT)0.973027,
        (HI_FLOAT)0.969233, (HI_FLOAT)0.965455, (HI_FLOAT)0.961691, (HI_FLOAT)0.957941,
        (HI_FLOAT)0.954207, (HI_FLOAT)0.950487, (HI_FLOAT)0.946781, (HI_FLOAT)0.94309
    },
    {   (HI_FLOAT)1, (HI_FLOAT)0.939413, (HI_FLOAT)0.882497, (HI_FLOAT)0.829029,
        (HI_FLOAT)0.778801, (HI_FLOAT)0.731616, (HI_FLOAT)0.687289, (HI_FLOAT)0.645649,
        (HI_FLOAT)0.606531, (HI_FLOAT)0.569783, (HI_FLOAT)0.535261, (HI_FLOAT)0.502832,
        (HI_FLOAT)0.472367, (HI_FLOAT)0.443747, (HI_FLOAT)0.416862, (HI_FLOAT)0.391606
    },
    {   (HI_FLOAT)1, (HI_FLOAT)0.367879, (HI_FLOAT)0.135335, (HI_FLOAT)0.0497871,
        (HI_FLOAT)0.0183156, (HI_FLOAT)0.00673795, (HI_FLOAT)0.00247875, (HI_FLOAT)0.000911882,
        (HI_FLOAT)0.000335463, (HI_FLOAT)0.00012341, (HI_FLOAT)4.53999e-005, (HI_FLOAT)1.67017e-005,
        (HI_FLOAT)6.14421e-006, (HI_FLOAT)2.26033e-006, (HI_FLOAT)8.31529e-007, (HI_FLOAT)3.05902e-007
    },
    {   (HI_FLOAT)1, (HI_FLOAT)1.12535e-007, (HI_FLOAT)1.26642e-014, (HI_FLOAT)1.42516e-021,
        (HI_FLOAT)1.60381e-028, (HI_FLOAT)1.80485e-035, (HI_FLOAT)2.03048e-042, (HI_FLOAT)0,
        (HI_FLOAT)0, (HI_FLOAT)0, (HI_FLOAT)0, (HI_FLOAT)0,
        (HI_FLOAT)0, (HI_FLOAT)0, (HI_FLOAT)0, (HI_FLOAT)0
    }
};

/*********************************************************
Function: QuickExp
Description: Do QuickExp...
*********************************************************/
#define EXP_CALC_MASK        0x0000000F

HI_FLOAT QuickExp(HI_U32 u32X)
{
    HI_U32 expCoefIdxBase = 0;
    if (u32X & 0x80000000) {
        u32X = ~u32X + 0x00000001;
        expCoefIdxBase += EXP_COEF_BASE_OFFSET;
    }
    return
        g_af32ExpCoef[expCoefIdxBase + 0][u32X & EXP_CALC_MASK] *
        g_af32ExpCoef[expCoefIdxBase + 1][(u32X >> 4) & EXP_CALC_MASK] *
        g_af32ExpCoef[expCoefIdxBase + 2][(u32X >> 8) & EXP_CALC_MASK] *
        g_af32ExpCoef[expCoefIdxBase + 3][(u32X >> 12) & EXP_CALC_MASK] *
        g_af32ExpCoef[expCoefIdxBase + 4][(u32X >> 16) & EXP_CALC_MASK];
}

#if !defined(NEON_SUPPORT) || !defined(NEON_ENABLE)
/*********************************************************
Function: Overlap
Description: Calculate the IOU of two bboxes
*********************************************************/
static HI_S32 Overlap(HI_S32 s32XMin1, HI_S32 s32YMin1, HI_S32 s32XMax1, HI_S32 s32YMax1, HI_S32 s32XMin2,
                      HI_S32 s32YMin2, HI_S32 s32XMax2, HI_S32 s32YMax2, HI_S32 *s32AreaSum, HI_S32 *s32AreaInter,
                      HI_S32 s32Area1, HI_S32 s32Area2)
{
    /* Check the input, and change the Return value  */
    HI_S32 s32XMin = SVP_MAX(s32XMin1, s32XMin2);
    HI_S32 s32YMin = SVP_MAX(s32YMin1, s32YMin2);
    HI_S32 s32XMax = SVP_MIN(s32XMax1, s32XMax2);
    HI_S32 s32YMax = SVP_MIN(s32YMax1, s32YMax2);

    HI_S32 s32InterWidth = s32XMax - s32XMin + 1;
    HI_S32 s32InterHeight = s32YMax - s32YMin + 1;

    s32InterWidth = (s32InterWidth >= 0) ? s32InterWidth : 0;
    s32InterHeight = (s32InterHeight >= 0) ? s32InterHeight : 0;

    HI_S32 s32Inter = s32InterWidth * s32InterHeight;

    HI_S32 s32Total = s32Area1 + s32Area2 - s32Inter;

    *s32AreaSum = s32Total;
    *s32AreaInter = s32Inter;

    return HI_SUCCESS;
}
#endif

/**************************************************
Function: Argswap
Description: used in NonRecursiveQuickSort
***************************************************/
#if !defined(NEON_SUPPORT) || (6 != SVP_WK_PROPOSAL_WIDTH)
HI_S32 Argswap(HI_S32 *ps32Src1, HI_S32 *ps32Src2)
{
    HI_U32 i = 0;
    HI_S32 tmp = 0;
    for (i = 0; i < SVP_WK_PROPOSAL_WIDTH; i++) {
        tmp = ps32Src1[i];
        ps32Src1[i] = ps32Src2[i];
        ps32Src2[i] = tmp;
    }
    return HI_SUCCESS;
}
#else  // NEON acceleration
HI_S32 Argswap(HI_S32 *ps32Src1, HI_S32 *ps32Src2)
{
    int32x4_t s32x4tmp1 = vld1q_s32(ps32Src1);
    int32x4_t s32x4tmp2 = vld1q_s32(ps32Src2);
    vst1q_s32(ps32Src1, s32x4tmp2);
    int32x2_t s32x2tmp1 = vld1_s32(ps32Src1 + 4);
    vst1q_s32(ps32Src2, s32x4tmp1);
    int32x2_t s32x2tmp2 = vld1_s32(ps32Src2 + 4);
    vst1_s32(ps32Src1 + 4, s32x2tmp2);
    vst1_s32(ps32Src2 + 4, s32x2tmp1);
    return HI_SUCCESS;
}
#endif

/**************************************************
Function: NonRecursiveArgQuickSort
Description: sort with NonRecursiveArgQuickSort
***************************************************/
HI_S32 NonRecursiveArgQuickSort(HI_S32 *aResultArray,
                                HI_S32 s32Low, HI_S32 s32High, NNIE_STACK_S *pstStack, HI_U32 u32MaxNum)
{
    HI_S32 i = s32Low;
    HI_S32 j = s32High;
    HI_S32 s32Top = 0;
    pstStack[s32Top].s32Min = s32Low;
    pstStack[s32Top].s32Max = s32High;

    HI_S32 s32KeyConfidence = aResultArray[SVP_WK_PROPOSAL_WIDTH * s32Low + PROPOSAL_BBOX_SCORE_INDEX];

    while (s32Top > -1) {
        s32Low = pstStack[s32Top].s32Min;
        s32High = pstStack[s32Top].s32Max;
        i = s32Low;
        j = s32High;
        s32Top--;

        s32KeyConfidence = aResultArray[SVP_WK_PROPOSAL_WIDTH * s32Low + PROPOSAL_BBOX_SCORE_INDEX];

        while (i < j) {
            while ((i < j) &&
                (s32KeyConfidence > aResultArray[j * SVP_WK_PROPOSAL_WIDTH + PROPOSAL_BBOX_SCORE_INDEX])) {
                j--;
            }
            if (i < j) {
                Argswap(&aResultArray[i * SVP_WK_PROPOSAL_WIDTH], &aResultArray[j * SVP_WK_PROPOSAL_WIDTH]);
                i++;
            }

            while ((i < j) &&
                (s32KeyConfidence < aResultArray[i * SVP_WK_PROPOSAL_WIDTH + PROPOSAL_BBOX_SCORE_INDEX])) {
                i++;
            }
            if (i < j) {
                Argswap(&aResultArray[i * SVP_WK_PROPOSAL_WIDTH], &aResultArray[j * SVP_WK_PROPOSAL_WIDTH]);
                j--;
            }
        }

        if (s32Low <= (HI_S32)u32MaxNum) {
            if (s32Low < i - 1) {
                s32Top++;
                pstStack[s32Top].s32Min = s32Low;
                pstStack[s32Top].s32Max = i - 1;
            }

            if (s32High > i + 1) {
                s32Top++;
                pstStack[s32Top].s32Min = i + 1;
                pstStack[s32Top].s32Max = s32High;
            }
        }
    }
    return HI_SUCCESS;
}

/**************************************************
Function: NonMaxSuppression
Description: proposal NMS u32NmsThresh
***************************************************/
#if !defined(NEON_SUPPORT) || !defined(NEON_ENABLE)
static HI_VOID SetAreaEachAnchor(HI_S32 *ps32Proposals, HI_S32 *ps32AreaEachAnchors, HI_U32 u32NumAnchors)
{
    for (HI_U32 i = 0; i < u32NumAnchors; i++) {
        if (ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_RPN_SUPRESS_INDEX] == RPN_SUPPRESS_FALSE) {
            HI_S32 s32XMin1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MIN_X_INDEX];
            HI_S32 s32YMin1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MIN_Y_INDEX];
            HI_S32 s32XMax1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MAX_X_INDEX];
            HI_S32 s32YMax1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MAX_Y_INDEX];
            ps32AreaEachAnchors[i] = (s32YMax1 - s32YMin1 + 1) * (s32XMax1 - s32XMin1 + 1);
        }
    }
}

HI_S32 NonMaxSuppression(HI_S32 *ps32Proposals, HI_U32 u32NumAnchors, HI_U32 u32NmsThresh, HI_U32 u32MaxRoiNum)
{
    /* define variables */
    HI_S32 s32AreaTotal = 0;
    HI_S32 s32AreaInter = 0;
    HI_U32 u32Num = 0;

    SVP_FALSE_CHECK((u32NumAnchors != 0), HI_FAILURE);

    HI_S32 *ps32AreaEachAnchor = (HI_S32 *)malloc(u32NumAnchors * sizeof(HI_S32));
    SVP_FALSE_CHECK(ps32AreaEachAnchor != HI_NULL, HI_FAILURE);
    memset(ps32AreaEachAnchor, 0x0, u32NumAnchors * sizeof(HI_S32));
    SetAreaEachAnchor(ps32Proposals, ps32AreaEachAnchor, u32NumAnchors);

    for (HI_U32 i = 0; (i < u32NumAnchors) && (u32Num < u32MaxRoiNum); i++) {
        if (ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_RPN_SUPRESS_INDEX] == RPN_SUPPRESS_FALSE) {
            u32Num++;
            HI_S32 s32XMin1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MIN_X_INDEX];
            HI_S32 s32YMin1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MIN_Y_INDEX];
            HI_S32 s32XMax1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MAX_X_INDEX];
            HI_S32 s32YMax1 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MAX_Y_INDEX];
            for (HI_U32 j = i + 1; j < u32NumAnchors; j++) {
                if (ps32Proposals[SVP_WK_PROPOSAL_WIDTH * j + PROPOSAL_RPN_SUPRESS_INDEX] == RPN_SUPPRESS_FALSE) {
                    HI_S32 s32XMin2 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * j + PROPOSAL_COORDINATE_MIN_X_INDEX];
                    HI_S32 s32YMin2 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * j + PROPOSAL_COORDINATE_MIN_Y_INDEX];
                    HI_S32 s32XMax2 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * j + PROPOSAL_COORDINATE_MAX_X_INDEX];
                    HI_S32 s32YMax2 = ps32Proposals[SVP_WK_PROPOSAL_WIDTH * j + PROPOSAL_COORDINATE_MAX_Y_INDEX];

                    HI_BOOL bNoOverlap = (s32XMin2 > s32XMax1) || (s32XMax2 < s32XMin1) || (s32YMin2 > s32YMax1) ||
                                 ((s32YMax2 < s32YMin1) ? HI_TRUE : HI_FALSE);
                    if (bNoOverlap) {
                        continue;
                    }

                    Overlap(s32XMin1, s32YMin1, s32XMax1, s32YMax1, s32XMin2, s32YMin2, s32XMax2, s32YMax2,
                            &s32AreaTotal, &s32AreaInter, ps32AreaEachAnchor[i], ps32AreaEachAnchor[j]);
                    if (s32AreaInter * SVP_WK_QUANT_BASE > (HI_S32)u32NmsThresh * s32AreaTotal) {
                        if (ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_BBOX_SCORE_INDEX]
                            >= ps32Proposals[SVP_WK_PROPOSAL_WIDTH * j + PROPOSAL_BBOX_SCORE_INDEX]) {
                            ps32Proposals[SVP_WK_PROPOSAL_WIDTH * j + PROPOSAL_RPN_SUPRESS_INDEX] = RPN_SUPPRESS_TRUE;
                        } else {
                            ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_RPN_SUPRESS_INDEX] = RPN_SUPPRESS_TRUE;
                        }
                    }
                }
            }
        }
    }
    free(ps32AreaEachAnchor);
    return HI_SUCCESS;
}
#else  // NEON acceleration
HI_S32 NonMaxSuppression(HI_S32 *ps32Proposals, HI_U32 u32NumAnchors, HI_U32 u32NmsThresh, HI_U32 u32MaxRoiNum)
{
    // malloc for NO overlap detection
#define SVP_WK_PROPOSAL_FALSE_LIST_WIDTH SVP_WK_PROPOSAL_WIDTH
    HI_U32 mallocsize = u32NumAnchors * SVP_WK_PROPOSAL_FALSE_LIST_WIDTH;
    HI_S32 *ps32Proposals_False_List = malloc(mallocsize * sizeof(HI_S32));
    SVP_FALSE_CHECK(ps32Proposals_False_List != HI_NULL, HI_FAILURE);

    /* define variables */
    HI_S32 s32XMin1 = 0;
    HI_S32 s32YMin1 = 0;
    HI_S32 s32XMax1 = 0;
    HI_S32 s32YMax1 = 0;
    HI_S32 Area;
    HI_U32 u32Num = 0;

    for (HI_U32 i = 0; i < u32NumAnchors * SVP_WK_PROPOSAL_WIDTH; i += SVP_WK_PROPOSAL_WIDTH) {
        s32XMin1 = ps32Proposals[i + PROPOSAL_COORDINATE_MIN_X_INDEX];
        s32YMin1 = ps32Proposals[i + PROPOSAL_COORDINATE_MIN_Y_INDEX];
        s32XMax1 = ps32Proposals[i + PROPOSAL_COORDINATE_MAX_X_INDEX];
        s32YMax1 = ps32Proposals[i + PROPOSAL_COORDINATE_MAX_Y_INDEX];

        Area = (s32XMax1 - s32XMin1 + 1) * (s32YMax1 - s32YMin1 + 1);
        // MaxX+1 -MinX MaxY+1 -MinY Area Index Supress
        ps32Proposals_False_List[i + PROPOSAL_COORDINATE_MIN_X_INDEX] = s32XMax1 + 1;
        ps32Proposals_False_List[i + PROPOSAL_COORDINATE_MIN_Y_INDEX] = -s32XMin1;
        ps32Proposals_False_List[i + PROPOSAL_COORDINATE_MAX_X_INDEX] = s32YMax1 + 1;
        ps32Proposals_False_List[i + PROPOSAL_COORDINATE_MAX_Y_INDEX] = -s32YMin1;
        ps32Proposals_False_List[i + PROPOSAL_BBOX_SCORE_INDEX] = Area;
        ps32Proposals_False_List[i + PROPOSAL_RPN_SUPRESS_INDEX] = ps32Proposals[i + PROPOSAL_RPN_SUPRESS_INDEX];
    }

    /* define variables */
    HI_S32 *pi = ps32Proposals_False_List;
    HI_S32 *pj = ps32Proposals_False_List;
    HI_S32 *pj_sup = HI_NULL;
    int32x4_t s32x4XXYYMaxp1_nMin_i;
    int32x4_t s32x4XXYYMaxp1_nMin_j;
    int32x4_t s32x4XXYYMinp1_nMax_i;
    uint32x4_t u32x4NoOverlap;
    HI_U32 u32NoOverlap;
    int32x4_t s32x4XXYYMaxp1_nMin;
    int32x4_t s32x4WH1A, s32x4W1W1, s32x4HAHA, s32x4IAIA;
    int32x4_t s32x401AiAj = vsetq_lane_s32(1, vdupq_n_s32(0), 1);
    HI_S32 s32NmsThresh = (HI_S32)u32NmsThresh;
    int32x4_t s32x400_nTpQ_T = vsetq_lane_s32(-s32NmsThresh - SVP_WK_QUANT_BASE, vdupq_n_s32(0), 2);
    s32x400_nTpQ_T = vsetq_lane_s32(s32NmsThresh, s32x400_nTpQ_T, 3);
    int64x2_t s64x2nIxTpQ_AxT;

    for (HI_U32 i = 0; (i < u32NumAnchors) && (u32Num < u32MaxRoiNum); i++, pi += SVP_WK_PROPOSAL_FALSE_LIST_WIDTH) {
        if (*(pi + PROPOSAL_RPN_SUPRESS_INDEX) == RPN_SUPPRESS_FALSE) {
            // u32Num count to u32MaxRoiNum max
            u32Num++;
            s32x4XXYYMaxp1_nMin_i = vld1q_s32(pi);
            s32x401AiAj = vsetq_lane_s32(*(pi + 4), s32x401AiAj, 2);
            s32x4XXYYMinp1_nMax_i = vrev64q_s32(s32x4XXYYMaxp1_nMin_i);
            s32x4XXYYMinp1_nMax_i = vmlaq_s32(vdupq_n_s32(1), vdupq_n_s32(-1), s32x4XXYYMinp1_nMax_i);
            pj = ps32Proposals_False_List + (i + 1) * SVP_WK_PROPOSAL_FALSE_LIST_WIDTH;
            for (HI_U32 j = i + 1; j < u32NumAnchors; j++, pj += SVP_WK_PROPOSAL_FALSE_LIST_WIDTH) {
                pj_sup = pj + 5;
                // vld1 has 5clock latency
                s32x4XXYYMaxp1_nMin_j = vld1q_s32(pj);
                if (*pj_sup == RPN_SUPPRESS_FALSE) {
                    u32x4NoOverlap = vcgtq_s32(s32x4XXYYMinp1_nMax_i, s32x4XXYYMaxp1_nMin_j);
                    u32NoOverlap = vmaxvq_u32(u32x4NoOverlap);
                    if (u32NoOverlap) {
                        continue;
                    } else {
                        s32x4XXYYMaxp1_nMin = vminq_s32(s32x4XXYYMaxp1_nMin_i, s32x4XXYYMaxp1_nMin_j);
                        s32x401AiAj = vsetq_lane_s32(*(pj + 4), s32x401AiAj, 3);
                        s32x4WH1A = vpaddq_s32(s32x4XXYYMaxp1_nMin, s32x401AiAj);
                        s32x4W1W1 = vuzp1q_s32(s32x4WH1A, s32x4WH1A);
                        s32x4HAHA = vuzp2q_s32(s32x4WH1A, s32x4WH1A);
                        s32x4IAIA = vmulq_s32(s32x4W1W1, s32x4HAHA);
                        s64x2nIxTpQ_AxT = vmull_high_s32(s32x400_nTpQ_T, s32x4IAIA);
                        *pj_sup = (vaddvq_s64(s64x2nIxTpQ_AxT) < 0);
                    }
                }
            }
        }
    }

    for (HI_U32 i = 5; i < u32NumAnchors * SVP_WK_PROPOSAL_WIDTH + 5; i += SVP_WK_PROPOSAL_WIDTH) {
        ps32Proposals[i] = ps32Proposals_False_List[i];
    }

    free(ps32Proposals_False_List);
    return HI_SUCCESS;
}
#endif

/**************************************************
Function: FilterLowScoreBbox
Description: remove low conf score proposal bbox
***************************************************/
HI_S32 FilterLowScoreBbox(HI_S32 *ps32Proposals, HI_U32 u32NumAnchors, HI_U32 u32NmsThresh, HI_U32 u32FilterThresh,
                          HI_U32 *u32NumAfterFilter)
{
    HI_U32 i = 0;
    HI_U32 u32ProposalCnt = u32NumAnchors;

    if (u32FilterThresh > 0) {
        for (i = 0; i < u32NumAnchors; i++) {
            if (ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_BBOX_SCORE_INDEX] < (HI_S32)u32FilterThresh) {
                ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_RPN_SUPRESS_INDEX] = RPN_SUPPRESS_TRUE;
            }
        }

        u32ProposalCnt = 0;
        for (i = 0; i < u32NumAnchors; i++) {
            if (ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_RPN_SUPRESS_INDEX] == RPN_SUPPRESS_FALSE) {
                ps32Proposals[SVP_WK_PROPOSAL_WIDTH * u32ProposalCnt + PROPOSAL_COORDINATE_MIN_X_INDEX] =
                    ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MIN_X_INDEX];

                ps32Proposals[SVP_WK_PROPOSAL_WIDTH * u32ProposalCnt + PROPOSAL_COORDINATE_MIN_Y_INDEX] =
                    ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MIN_Y_INDEX];

                ps32Proposals[SVP_WK_PROPOSAL_WIDTH * u32ProposalCnt + PROPOSAL_COORDINATE_MAX_X_INDEX] =
                    ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MAX_X_INDEX];

                ps32Proposals[SVP_WK_PROPOSAL_WIDTH * u32ProposalCnt + PROPOSAL_COORDINATE_MAX_Y_INDEX] =
                    ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_COORDINATE_MAX_Y_INDEX];

                ps32Proposals[SVP_WK_PROPOSAL_WIDTH * u32ProposalCnt + PROPOSAL_BBOX_SCORE_INDEX] =
                    ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_BBOX_SCORE_INDEX];

                ps32Proposals[SVP_WK_PROPOSAL_WIDTH * u32ProposalCnt + PROPOSAL_RPN_SUPRESS_INDEX] =
                    ps32Proposals[SVP_WK_PROPOSAL_WIDTH * i + PROPOSAL_RPN_SUPRESS_INDEX];

                u32ProposalCnt++;
            }
        }
    }

    *u32NumAfterFilter = u32ProposalCnt;
    return HI_SUCCESS;
}

/**************************************************
Function: BboxClip
Description: clip proposal bbox out of origin image range
***************************************************/
HI_S32 SizeClip(HI_S32 s32inputSize, HI_S32 s32sizeMin, HI_S32 s32sizeMax)
{
    return SVP_MAX(SVP_MIN(s32inputSize, s32sizeMax), s32sizeMin);
}

HI_S32 BboxClip(HI_S32 *ps32Proposals, HI_U32 u32ImageW, HI_U32 u32ImageH)
{
    ps32Proposals[PROPOSAL_COORDINATE_MIN_X_INDEX] =
        SizeClip(ps32Proposals[PROPOSAL_COORDINATE_MIN_X_INDEX], 0, (HI_S32)u32ImageW - 1);
    ps32Proposals[PROPOSAL_COORDINATE_MIN_Y_INDEX] =
        SizeClip(ps32Proposals[PROPOSAL_COORDINATE_MIN_Y_INDEX], 0, (HI_S32)u32ImageH - 1);
    ps32Proposals[PROPOSAL_COORDINATE_MAX_X_INDEX] =
        SizeClip(ps32Proposals[PROPOSAL_COORDINATE_MAX_X_INDEX], 0, (HI_S32)u32ImageW - 1);
    ps32Proposals[PROPOSAL_COORDINATE_MAX_Y_INDEX] =
        SizeClip(ps32Proposals[PROPOSAL_COORDINATE_MAX_Y_INDEX], 0, (HI_S32)u32ImageH - 1);

    return HI_SUCCESS;
}

HI_S32 BboxClip_N(HI_S32 *ps32Proposals, HI_U32 u32ImageW, HI_U32 u32ImageH, HI_U32 u32Num)
{
    HI_S32 s32Ret = HI_FAILURE;
    for (HI_U32 i = 0; i < u32Num; i++) {
        s32Ret = BboxClip(&ps32Proposals[i * SVP_WK_PROPOSAL_WIDTH], u32ImageW, u32ImageH);
        SVP_FALSE_CHECK(s32Ret == HI_SUCCESS, HI_FAILURE);
    }
    return HI_SUCCESS;
}

/**************************************************
Function: BboxSmallSizeFilter
Description: remove the bboxes which are too small
***************************************************/
HI_S32 BboxSmallSizeFilter(HI_S32 *ps32Proposals, HI_U32 u32minW, HI_U32 u32minH)
{
    HI_U32 u32ProposalW =
        (HI_U32)(ps32Proposals[PROPOSAL_COORDINATE_MAX_X_INDEX] - ps32Proposals[PROPOSAL_COORDINATE_MIN_X_INDEX] + 1);
    HI_U32 u32ProposalH =
        (HI_U32)(ps32Proposals[PROPOSAL_COORDINATE_MAX_Y_INDEX] - ps32Proposals[PROPOSAL_COORDINATE_MIN_Y_INDEX] + 1);

    if ((u32ProposalW < u32minW) || (u32ProposalH < u32minH)) {
        ps32Proposals[PROPOSAL_RPN_SUPRESS_INDEX] = RPN_SUPPRESS_TRUE;  // suppressed
    }

    return HI_SUCCESS;
}

HI_S32 BboxSmallSizeFilter_N(HI_S32 *ps32Proposals, HI_U32 u32minW, HI_U32 u32minH, HI_U32 u32NumAnchors)
{
    HI_S32 s32Ret = HI_FAILURE;

    for (HI_U32 i = 0; i < u32NumAnchors; i++) {
        s32Ret = BboxSmallSizeFilter(&ps32Proposals[i * SVP_WK_PROPOSAL_WIDTH], u32minW, u32minH);
        SVP_FALSE_CHECK(s32Ret == HI_SUCCESS, HI_FAILURE);
    }
    return HI_SUCCESS;
}

/**************************************************
Function: dumpProposal
Description: dumpProposal info when DETECION_DBG
***************************************************/
HI_S32 dumpProposal(HI_S32 *ps32Proposals, const HI_CHAR *filename, HI_U32 u32NumAnchors)
{
    if (DETECION_DBG) {
        FILE *file = fopen(filename, "w");
        SVP_FALSE_CHECK(file != HI_NULL, HI_FAILURE);

        for (HI_U32 i = 0; i < u32NumAnchors; i++) {
            /*
            index     content
            0         x0
            1         y0
            2         x1
            3         y1
            4         score
            5         is suppressed or not
            */
            fprintf(file, "%f %d %d %d %d %d\n",
                    (HI_FLOAT)ps32Proposals[PROPOSAL_BBOX_SCORE_INDEX] / SVP_WK_QUANT_BASE,
                    ps32Proposals[PROPOSAL_COORDINATE_MIN_X_INDEX],
                    ps32Proposals[PROPOSAL_COORDINATE_MIN_Y_INDEX],
                    ps32Proposals[PROPOSAL_COORDINATE_MAX_X_INDEX],
                    ps32Proposals[PROPOSAL_COORDINATE_MAX_Y_INDEX],
                    ps32Proposals[PROPOSAL_RPN_SUPRESS_INDEX]);

            ps32Proposals += SVP_WK_PROPOSAL_WIDTH;
        }

        fclose(file);
        file = NULL;
    }
    return HI_SUCCESS;
}

/**************************************************
Function: getRPNresult
Description: rite the final result to output
***************************************************/
HI_S32 getRPNresult(HI_S32 *ps32ProposalResult, HI_U32 *pu32NumRois, HI_U32 u32MaxRois,
                    const HI_S32 *ps32Proposals, HI_U32 u32NumAfterFilter)
{
    HI_U32 u32RoiCount = 0;
    for (HI_U32 i = 0; i < u32NumAfterFilter; i++) {
        if (ps32Proposals[PROPOSAL_RPN_SUPRESS_INDEX] == RPN_SUPPRESS_FALSE) {
            // convert to 20.12 fixed-point number.
            ps32ProposalResult[SVP_WK_COORDI_NUM * u32RoiCount + PROPOSAL_COORDINATE_MIN_X_INDEX] =
                ps32Proposals[PROPOSAL_COORDINATE_MIN_X_INDEX] * SVP_WK_QUANT_BASE;
            ps32ProposalResult[SVP_WK_COORDI_NUM * u32RoiCount + PROPOSAL_COORDINATE_MIN_Y_INDEX] =
                ps32Proposals[PROPOSAL_COORDINATE_MIN_Y_INDEX] * SVP_WK_QUANT_BASE;
            ps32ProposalResult[SVP_WK_COORDI_NUM * u32RoiCount + PROPOSAL_COORDINATE_MAX_X_INDEX] =
                ps32Proposals[PROPOSAL_COORDINATE_MAX_X_INDEX] * SVP_WK_QUANT_BASE;
            ps32ProposalResult[SVP_WK_COORDI_NUM * u32RoiCount + PROPOSAL_COORDINATE_MAX_Y_INDEX] =
                ps32Proposals[PROPOSAL_COORDINATE_MAX_Y_INDEX] * SVP_WK_QUANT_BASE;

            u32RoiCount++;
        }
        ps32Proposals += SVP_WK_PROPOSAL_WIDTH;
        if (u32RoiCount >= u32MaxRois) {
            break;
        }
    }

    *pu32NumRois = u32RoiCount;
    /****************** write rpn_result *********************/
    if (DETECION_DBG) {
        FILE *rpn_result = fopen("rpn_result.txt", "w");
        SVP_FALSE_CHECK(rpn_result != HI_NULL, HI_FAILURE);

        for (HI_U32 i = 0; i < u32RoiCount; i++) {
            fprintf(rpn_result, "%d %d %d %d\n",
                    ps32ProposalResult[SVP_WK_COORDI_NUM * i + PROPOSAL_COORDINATE_MIN_X_INDEX],
                    ps32ProposalResult[SVP_WK_COORDI_NUM * i + PROPOSAL_COORDINATE_MIN_Y_INDEX],
                    ps32ProposalResult[SVP_WK_COORDI_NUM * i + PROPOSAL_COORDINATE_MAX_X_INDEX],
                    ps32ProposalResult[SVP_WK_COORDI_NUM * i + PROPOSAL_COORDINATE_MAX_Y_INDEX]);
        }
        fclose(rpn_result);
        rpn_result = NULL;
    }

    return HI_SUCCESS;
}

HI_VOID GetParam(const HI_PROPOSAL_Param_S *pPluginParam, PROPOSAL_PARA_S *param)
{
    if (pPluginParam == HI_NULL) {
        return;
    }
    param->model_info.u32SrcWidth = pPluginParam->stNodePluginParam.u32SrcWidth;
    param->model_info.u32SrcHeight = pPluginParam->stNodePluginParam.u32SrcHeight;
    param->model_info.enNetType = pPluginParam->stNodePluginParam.enNetType;
    param->model_info.u32NumRatioAnchors = pPluginParam->stNodePluginParam.u32NumRatioAnchors;
    param->model_info.u32NumScaleAnchors = pPluginParam->stNodePluginParam.u32NumScaleAnchors;
    for (HI_U32 ratioAnchorNum = 0; ratioAnchorNum < pPluginParam->stNodePluginParam.u32NumRatioAnchors;
         ++ratioAnchorNum) {
        param->model_info.au32Ratios[ratioAnchorNum] =
            (HI_S32)(pPluginParam->stNodePluginParam.pfRatio[ratioAnchorNum] * SVP_WK_QUANT_BASE);
    }
    for (HI_U32 scaleAnchorNum = 0; scaleAnchorNum < pPluginParam->stNodePluginParam.u32NumScaleAnchors;
         ++scaleAnchorNum) {
        param->model_info.au32Scales[scaleAnchorNum] =
            (HI_S32)(pPluginParam->stNodePluginParam.pfScales[scaleAnchorNum] * SVP_WK_QUANT_BASE);
    }

    param->model_info.u32ClassSize = pPluginParam->stNodePluginParam.u32ClassSize;
    param->model_info.u32MaxRoiFrameCnt = pPluginParam->stNodePluginParam.u32MaxRoiFrameCnt;
    param->model_info.u32MinSize = pPluginParam->stNodePluginParam.u32MinSize;
    param->model_info.u32SpatialScale = (HI_S32)(pPluginParam->stNodePluginParam.fSpatialScale * SVP_WK_QUANT_BASE);
    param->u32NmsThresh = (HI_S32)(pPluginParam->stNodePluginParam.fNmsThresh * SVP_WK_QUANT_BASE);
    param->u32FilterThresh = (HI_U32)(pPluginParam->stNodePluginParam.fFilterThresh * SVP_WK_QUANT_BASE);
    param->u32NumBeforeNms = pPluginParam->stNodePluginParam.u32NumBeforeNms;
    param->u32ConfThresh = (HI_U32)(pPluginParam->stNodePluginParam.fConfThresh * SVP_WK_QUANT_BASE);
    param->u32ValidNmsThresh = (HI_U32)(pPluginParam->stNodePluginParam.fValidNmsThresh * SVP_WK_QUANT_BASE);
    memcpy(&(param->stBachorMemInfo), &(pPluginParam->stBachorMemInfo), sizeof(HI_RUNTIME_MEM_S));
}

/************************* BBox Transform *****************************/
/* use parameters from Conv3 to adjust the coordinates of anchors */
HI_S32 BboxTransform(HI_S32 *ps32Proposals,
                     HI_S32 *ps32Anchors,
                     HI_S32 *ps32BboxDelta,
                     HI_FLOAT *pf32Scores,
                     PROPOSAL_TYPE_E enProposalType)
{
    HI_S32 s32ProposalWidth =
        ps32Anchors[PROPOSAL_COORDINATE_MAX_X_INDEX] -
        ps32Anchors[PROPOSAL_COORDINATE_MIN_X_INDEX] + 1;
    HI_S32 s32ProposalHeight =
        ps32Anchors[PROPOSAL_COORDINATE_MAX_Y_INDEX] -
        ps32Anchors[PROPOSAL_COORDINATE_MIN_Y_INDEX] + 1;
    HI_S32 s32ProposalCenterX =
        (HI_S32)(ps32Anchors[PROPOSAL_COORDINATE_MIN_X_INDEX] +
        s32ProposalWidth * PROPOSAL_LENGTH_HALF_COEFFICIENT);
    HI_S32 s32ProposalCenterY =
        (HI_S32)(ps32Anchors[PROPOSAL_COORDINATE_MIN_Y_INDEX] +
        s32ProposalHeight * PROPOSAL_LENGTH_HALF_COEFFICIENT);
    HI_S32 s32PredCenterX =
        (HI_S32)(((HI_FLOAT)ps32BboxDelta[BBOX_DELTA_CENTER_X_INDEX] / SVP_WK_QUANT_BASE) * s32ProposalWidth +
            s32ProposalCenterX);
    HI_S32 s32PredCenterY =
        (HI_S32)(((HI_FLOAT)ps32BboxDelta[BBOX_DELTA_CENTER_Y_INDEX] / SVP_WK_QUANT_BASE) * s32ProposalHeight +
            s32ProposalCenterY);

    HI_S32 s32PredW = (HI_S32)(s32ProposalWidth * QuickExp(ps32BboxDelta[BBOX_DELTA_PRED_W_INDEX]));
    HI_S32 s32PredH = (HI_S32)(s32ProposalHeight * QuickExp(ps32BboxDelta[BBOX_DELTA_PRED_H_INDEX]));

    ps32Proposals[PROPOSAL_COORDINATE_MIN_X_INDEX] =
        (HI_S32)(s32PredCenterX - PROPOSAL_LENGTH_HALF_COEFFICIENT * s32PredW);
    ps32Proposals[PROPOSAL_COORDINATE_MIN_Y_INDEX] =
        (HI_S32)(s32PredCenterY - PROPOSAL_LENGTH_HALF_COEFFICIENT * s32PredH);
    ps32Proposals[PROPOSAL_COORDINATE_MAX_X_INDEX] =
        (HI_S32)(s32PredCenterX + PROPOSAL_LENGTH_HALF_COEFFICIENT * s32PredW);
    ps32Proposals[PROPOSAL_COORDINATE_MAX_Y_INDEX] =
        (HI_S32)(s32PredCenterY + PROPOSAL_LENGTH_HALF_COEFFICIENT * s32PredH);
    if (enProposalType == PROPOSAL_WITH_PERMUTE) {
        ps32Proposals[PROPOSAL_BBOX_SCORE_INDEX] = (HI_S32)(*pf32Scores * SVP_WK_QUANT_BASE);
    } else if (enProposalType == PROPOSAL_WITHOUT_PERMUTE) {
        ps32Proposals[PROPOSAL_BBOX_SCORE_INDEX] = *(HI_S32*)(pf32Scores);
    } else {
        return HI_FAILURE;
    }
    ps32Proposals[PROPOSAL_RPN_SUPRESS_INDEX] = RPN_SUPPRESS_FALSE;

    return HI_SUCCESS;
}

HI_S32 BboxTransform_N(BBOX_PARAM_S *pstBboxParam, HI_U32 u32NumAnchors, PROPOSAL_TYPE_E enProposalType)
{
    HI_S32 s32Ret = HI_FAILURE;
    for (HI_U32 i = 0; i < u32NumAnchors; i++) {
        if (enProposalType == PROPOSAL_WITH_PERMUTE) {
            s32Ret = BboxTransform(&(pstBboxParam->ps32Proposals[i * SVP_WK_PROPOSAL_WIDTH]),
                                   &(pstBboxParam->ps32Anchors[i * SVP_WK_COORDI_NUM]),
                                   &(pstBboxParam->ps32BboxDelta[i * SVP_WK_COORDI_NUM]),
                                   &(pstBboxParam->pf32Scores[i * SVP_WK_SCORE_NUM + 1]), /* be careful this +1 */
                                   enProposalType);
        } else if (enProposalType == PROPOSAL_WITHOUT_PERMUTE) {
            s32Ret = BboxTransform(&(pstBboxParam->ps32Proposals[i * SVP_WK_PROPOSAL_WIDTH]),
                                   &(pstBboxParam->ps32Anchors[i * SVP_WK_COORDI_NUM]),
                                   &(pstBboxParam->ps32BboxDelta[i * SVP_WK_COORDI_NUM]),
                                   &(pstBboxParam->pf32Scores[i]),
                                   enProposalType);
        } else {
            return HI_FAILURE;
        }
        SVP_FALSE_CHECK(s32Ret == HI_SUCCESS, HI_FAILURE);
    }
    return HI_SUCCESS;
}
