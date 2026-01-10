/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_model_yolo.h"
#include <math.h>
#include <stdint.h>
#include "sample_data_utils.h"
#include "sample_memory_ops.h"
#include "sample_log.h"

#ifndef SVP_MAX
#define SVP_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef SVP_MIN
#define SVP_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static HI_FLOAT Sigmoid(HI_FLOAT f32Val)
{
    return (1.0f / (1.0f + exp(-f32Val)));
}

static HI_S32 BoxArgswap(SVP_SAMPLE_BOX_S* pstBox1, SVP_SAMPLE_BOX_S* pstBox2)
{
    // check
    SVP_SAMPLE_BOX_S stBoxTmp = { 0 };

    memcpy(&stBoxTmp, pstBox1, sizeof(stBoxTmp));
    memcpy(pstBox1, pstBox2, sizeof(stBoxTmp));
    memcpy(pstBox2, &stBoxTmp, sizeof(stBoxTmp));

    return HI_SUCCESS;
}

static HI_S32 NonRecursiveArgQuickSortWithBox(SVP_SAMPLE_BOX_S* pstBoxs, HI_S32 s32Low,
    HI_S32 s32High, SVP_SAMPLE_STACK_S *pstStack)
{
    HI_S32 i = s32Low;
    HI_S32 j = s32High;
    HI_S32 s32Top = 0;

    pstStack[s32Top].s32Min = s32Low;
    pstStack[s32Top].s32Max = s32High;

    HI_FLOAT f32KeyConfidence = pstBoxs[s32Low].f32ClsScore;
    while (s32Top > -1) {
        s32Low = pstStack[s32Top].s32Min;
        s32High = pstStack[s32Top].s32Max;
        i = s32Low;
        j = s32High;
        s32Top--;

        f32KeyConfidence = pstBoxs[s32Low].f32ClsScore;
        while (i < j) {
            while ((i < j) && (f32KeyConfidence > pstBoxs[j].f32ClsScore)) {
                j--;
            }
            if (i < j) {
                BoxArgswap(&pstBoxs[i], &pstBoxs[j]);
                i++;
            }

            while ((i < j) && (f32KeyConfidence < pstBoxs[i].f32ClsScore)) {
                i++;
            }
            if (i < j) {
                BoxArgswap(&pstBoxs[i], &pstBoxs[j]);
                j--;
            }
        }
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
    return HI_SUCCESS;
}

static HI_DOUBLE SvpDetYoloCalIou(SVP_SAMPLE_BOX_S *pstBox1, SVP_SAMPLE_BOX_S *pstBox2)
{
    // Check the input
    HI_FLOAT f32XMin = SVP_MAX(pstBox1->f32Xmin, pstBox2->f32Xmin);
    HI_FLOAT f32YMin = SVP_MAX(pstBox1->f32Ymin, pstBox2->f32Ymin);
    HI_FLOAT f32XMax = SVP_MIN(pstBox1->f32Xmax, pstBox2->f32Xmax);
    HI_FLOAT f32YMax = SVP_MIN(pstBox1->f32Ymax, pstBox2->f32Ymax);

    HI_FLOAT InterWidth = f32XMax - f32XMin;
    HI_FLOAT InterHeight = f32YMax - f32YMin;

    if (InterWidth <= 0 || InterHeight <= 0) {
        return HI_SUCCESS;
    }

    HI_DOUBLE f64InterArea = InterWidth * InterHeight;
    HI_DOUBLE f64Box1Area = (pstBox1->f32Xmax - pstBox1->f32Xmin) * (pstBox1->f32Ymax - pstBox1->f32Ymin);
    HI_DOUBLE f64Box2Area = (pstBox2->f32Xmax - pstBox2->f32Xmin) * (pstBox2->f32Ymax - pstBox2->f32Ymin);

    HI_DOUBLE f64UnionArea = f64Box1Area + f64Box2Area - f64InterArea;

    return f64InterArea / f64UnionArea;
}

static HI_S32 YoloNonMaxSuppression(SVP_SAMPLE_BOX_S* pstBoxs, HI_U32 u32BoxNum,
    HI_FLOAT f32NmsThresh, HI_U32 u32MaxRoiNum)
{
    HI_U32 u32Num = 0;
    for (HI_U32 i = 0; i < u32BoxNum && u32Num < u32MaxRoiNum; i++) {
        if (pstBoxs[i].u32Mask == 0) {
            u32Num++;
            for (HI_U32 j = i + 1; j < u32BoxNum; j++) {
                if (pstBoxs[j].u32Mask == 0) {
                    HI_DOUBLE f64Iou = SvpDetYoloCalIou(&pstBoxs[i], &pstBoxs[j]);
                    if (f64Iou >= (HI_DOUBLE)f32NmsThresh) {
                        pstBoxs[j].u32Mask = 1;
                    }
                }
            }
        }
    }
    return HI_SUCCESS;
}


static HI_VOID SvpDetYoloResultPrint(const SVP_SAMPLE_BOX_RESULT_INFO_S *pstResultBoxesInfo, HI_U32 u32BoxNum)
{
    if ((pstResultBoxesInfo == NULL) || (pstResultBoxesInfo->pstBbox == NULL)) {
        return;
    }
    for (HI_U32 i = 0; i < u32BoxNum; i++) {
        HI_CHAR resultLine[512];

        snprintf(resultLine, 512, "%4u  %9.8f  %4.2f  %4.2f  %4.2f  %4.2f\n",
            pstResultBoxesInfo->pstBbox[i].u32MaxScoreIndex,
            pstResultBoxesInfo->pstBbox[i].f32ClsScore,
            pstResultBoxesInfo->pstBbox[i].f32Xmin, pstResultBoxesInfo->pstBbox[i].f32Ymin,
            pstResultBoxesInfo->pstBbox[i].f32Xmax, pstResultBoxesInfo->pstBbox[i].f32Ymax);
        printf("   %s", resultLine);
    }
}

// yolov1
static HI_S32 YoloV1ScoreCmp(const void *a, const void *b)
{
    return ((yolov1_score*)a)->value < ((yolov1_score*)b)->value;
}

static void YoloV1ConvertPosition(position bbox, SVP_SAMPLE_BOX_S *result)
{
    HI_FLOAT xMin = bbox.x - 0.5f * bbox.w;
    HI_FLOAT yMin = bbox.y - 0.5f * bbox.h;
    HI_FLOAT xMax = bbox.x + 0.5f * bbox.w;
    HI_FLOAT yMax = bbox.y + 0.5f * bbox.h;

    xMin = xMin > 0 ? xMin : 0;
    yMin = yMin > 0 ? yMin : 0;
    xMax = xMax > SVP_SAMPLE_YOLOV1_IMG_WIDTH  ? SVP_SAMPLE_YOLOV1_IMG_WIDTH  : xMax;
    yMax = yMax > SVP_SAMPLE_YOLOV1_IMG_HEIGHT ? SVP_SAMPLE_YOLOV1_IMG_HEIGHT : yMax;

    result->f32Xmin = xMin;
    result->f32Ymin = yMin;
    result->f32Xmax = xMax;
    result->f32Ymax = yMax;
}

static HI_FLOAT YoloV1CalIou(position *bbox, HI_U32 bb1, HI_U32 bb2)
{
    HI_FLOAT tb = SVP_MIN(bbox[bb1].x + 0.5f*bbox[bb1].w, bbox[bb2].x + 0.5f*bbox[bb2].w)
                - SVP_MAX(bbox[bb1].x - 0.5f*bbox[bb1].w, bbox[bb2].x - 0.5f*bbox[bb2].w);
    HI_FLOAT lr = SVP_MIN(bbox[bb1].y + 0.5f*bbox[bb1].h, bbox[bb2].y + 0.5f*bbox[bb2].h)
                - SVP_MAX(bbox[bb1].y - 0.5f*bbox[bb1].h, bbox[bb2].y - 0.5f*bbox[bb2].h);

    HI_FLOAT intersection = 0.0f;

    if (tb < 0 || lr < 0) {
        intersection = 0;
    }
    else {
        intersection = tb * lr;
    }

    return (intersection / (bbox[bb1].w * bbox[bb1].h + bbox[bb2].w * bbox[bb2].h - intersection));
}

static HI_U32 FloatEqual(HI_FLOAT a, HI_FLOAT b)
{
    return fabs(a - b) < DET_EPS;
}

static void YoloV1GetSortedIdx(HI_FLOAT *array, HI_U32 *idx)
{
    yolov1_score tmp[SVP_SAMPLE_YOLOV1_BBOX_CNT] = { 0 };

    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_BBOX_CNT; ++i) {
        tmp[i].idx = i;
        tmp[i].value = array[i];
    }

    qsort(tmp, SVP_SAMPLE_YOLOV1_BBOX_CNT, sizeof(yolov1_score), YoloV1ScoreCmp);

    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_BBOX_CNT; ++i) {
        idx[i] = tmp[i].idx;
    }
}

static void YoloV1NMS(HI_FLOAT *array, position *bbox)
{
    HI_U32 result[SVP_SAMPLE_YOLOV1_BBOX_CNT] = { 0 };

    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_BBOX_CNT; ++i) {
        if (array[i] < SVP_SAMPLE_YOLOV1_SCORE_FILTER_THREASH) {
            array[i] = 0.0f;
        }
    }
    YoloV1GetSortedIdx(array, result);

    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_BBOX_CNT; ++i) {
        HI_U32 idx_i = result[i];
        if (FloatEqual(array[idx_i], 0.0)) {
            continue;
        }

        for (HI_U32 j = i + 1; j < SVP_SAMPLE_YOLOV1_BBOX_CNT; ++j) {
            HI_U32 idx_j = result[j];

            if (FloatEqual(array[idx_j], 0.0f)) {
                continue;
            }
            if (YoloV1CalIou(bbox, idx_i, idx_j) > SVP_SAMPLE_YOLOV1_NMS_THREASH) {
                array[idx_j] = 0.0f;
            }
        }
    }
}

static HI_U32 YoloV1Detect(HI_FLOAT af32Score[][SVP_SAMPLE_YOLOV1_BBOX_CNT],
    position *bbox, SVP_SAMPLE_BOX_S *pstBoxesResult)
{
    HI_U32 ans_idx = 0;
    HI_U32 U32_MAX = 0xFFFFFFFF;
    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_CLASS_CNT; ++i) {
        YoloV1NMS(af32Score[i], bbox);
    }
    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_BBOX_CNT; ++i) {
        HI_FLOAT maxScore = af32Score[0][0];
        HI_U32 idx = U32_MAX;

        for (HI_U32 j = 0; j < SVP_SAMPLE_YOLOV1_CLASS_CNT; ++j) {
            if (af32Score[j][i] > 0.15) {
                maxScore = af32Score[j][i];
                idx = j;
            }
        }

        if (idx != U32_MAX) {
            pstBoxesResult[ans_idx].u32MaxScoreIndex = idx;
            pstBoxesResult[ans_idx].f32ClsScore = maxScore;
            YoloV1ConvertPosition(bbox[i], &pstBoxesResult[ans_idx]);
            ++ans_idx;
        }
    }
    return ans_idx;
}

HI_VOID SAMPLE_RUNTIME_YoloV1GetResult(HI_RUNTIME_BLOB_S *pstDstBlob)
{
    HI_FLOAT af32Scores[SVP_SAMPLE_YOLOV1_CLASS_CNT][SVP_SAMPLE_YOLOV1_BBOX_CNT];
    HI_FLOAT f32InputData[SVP_SAMPLE_YOLOV1_CHANNEL_GRID_NUM] = { 0.0f };
    HI_FLOAT *pf32ClassProbs = f32InputData;
    HI_FLOAT *pf32Confs = pf32ClassProbs + SVP_SAMPLE_YOLOV1_CLASS_CNT * SVP_SAMPLE_YOLOV1_GRID_SQR_NUM;
    HI_FLOAT *pf32boxes = pf32Confs + SVP_SAMPLE_YOLOV1_BBOX_CNT;
    HI_U32 *pu32BoxNum = (HI_U32*)malloc(sizeof(HI_U32));
    SAMPLE_CHK_RET_VOID(pu32BoxNum == HI_NULL, "pu32BoxNum malloc fail\n");
    memset(pu32BoxNum, 0, sizeof(HI_U32));

    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_CHANNEL_GRID_NUM; ++i) {
            HI_U8* u8dataAddr = (HI_U8*)(uintptr_t)pstDstBlob->u64VirAddr;
            f32InputData[i] = ((HI_S32*)u8dataAddr)[i] * 1.0f / SVP_WK_QUANT_BASE;
        }
    HI_U32 idx = 0;
    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_GRID_SQR_NUM; ++i) {
        for (HI_U32 j = 0; j < SVP_SAMPLE_YOLOV1_BOX_NUM; ++j) {
            for (HI_U32 k = 0; k < SVP_SAMPLE_YOLOV1_CLASS_CNT; ++k) {
                HI_FLOAT f32ClassProbs = *(pf32ClassProbs + i * SVP_SAMPLE_YOLOV1_CLASS_CNT + k);

                HI_FLOAT f32Confs = *(pf32Confs + i * SVP_SAMPLE_YOLOV1_BOX_NUM + j);
                af32Scores[k][idx] = f32ClassProbs * f32Confs;
            }
            ++idx;
        }
    }
    for (HI_U32 i = 0; i < SVP_SAMPLE_YOLOV1_GRID_SQR_NUM; ++i) {
        HI_U32 row = i % SVP_SAMPLE_YOLOV1_GRID_NUM;
        HI_U32 col = i / SVP_SAMPLE_YOLOV1_GRID_NUM;
        for (HI_U32 j = 0; j < SVP_SAMPLE_YOLOV1_BOX_NUM; ++j) {
            HI_U32 u32boxIdx = (i * SVP_SAMPLE_YOLOV1_BOX_NUM + j) * SVP_WK_COORDI_NUM;
            HI_FLOAT* pf32box_X = pf32boxes + u32boxIdx + X_MIN_IDX;
            HI_FLOAT* pf32box_Y = pf32boxes + u32boxIdx + Y_MIN_IDX;
            HI_FLOAT* pf32box_W = pf32boxes + u32boxIdx + X_MAX_IDX;
            HI_FLOAT* pf32box_H = pf32boxes + u32boxIdx + Y_MAX_IDX;

            *pf32box_X = (*pf32box_X + row) / SVP_SAMPLE_YOLOV1_GRID_NUM * SVP_SAMPLE_YOLOV1_IMG_WIDTH;  // x
            *pf32box_Y = (*pf32box_Y + col) / SVP_SAMPLE_YOLOV1_GRID_NUM * SVP_SAMPLE_YOLOV1_IMG_HEIGHT;  // y
            *pf32box_W = (*pf32box_W) * (*pf32box_W) * SVP_SAMPLE_YOLOV1_IMG_WIDTH;  // w
            *pf32box_H = (*pf32box_H) * (*pf32box_H) * SVP_SAMPLE_YOLOV1_IMG_HEIGHT;  // h
        }
    }
    SVP_SAMPLE_BOX_S astBoxesResult[1024] = { 0 };
    pu32BoxNum[0] = YoloV1Detect(af32Scores, (position*)(pf32boxes), astBoxesResult);

    SVP_SAMPLE_BOX_RESULT_INFO_S pstResultBoxesInfo;
    pstResultBoxesInfo.pstBbox = astBoxesResult;
    SvpDetYoloResultPrint(&pstResultBoxesInfo, pu32BoxNum[0]);
    SAMPLE_FREE(pu32BoxNum);
}

// yolov2
static HI_FLOAT af32ExpCoef[10][16] = { // reference QuickExp algorithm
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

static HI_DOUBLE YoloV2Bias[10] = { 1.08, 1.19, 3.42, 4.41, 6.63, 11.38, 9.42, 5.11, 16.62, 10.52 };

static HI_FLOAT QuickExp(HI_U32 u32X)
{
    if (u32X & 0x80000000) {
        u32X = ~u32X + 0x00000001;
        return af32ExpCoef[5][u32X & 0x0000000F] * af32ExpCoef[6][(u32X >> 4) & 0x0000000F] *
            af32ExpCoef[7][(u32X >> 8) & 0x0000000F] * af32ExpCoef[8][(u32X >> 12) & 0x0000000F]
            * af32ExpCoef[9][(u32X >> 16) & 0x0000000F];
    } else {
        return af32ExpCoef[0][u32X & 0x0000000F] * af32ExpCoef[1][(u32X >> 4) & 0x0000000F] *
            af32ExpCoef[2][(u32X >> 8) & 0x0000000F] * af32ExpCoef[3][(u32X >> 12) & 0x0000000F]
            * af32ExpCoef[4][(u32X >> 16) & 0x0000000F];
    }
}

static HI_S32 SoftMax(HI_FLOAT *af32Src, HI_S32 s32ArraySize)
{
    if ((af32Src == HI_NULL) || (s32ArraySize <= 0)) {
        printf("SoftMax input error! af32Src(%p), s32ArraySize(%d)\n", af32Src, s32ArraySize);
        return -1;
    }

    /* define parameters */
    HI_FLOAT f32Sum = 0;
    HI_FLOAT f32Max = af32Src[0];
    for (HI_S32 i = 1; i < s32ArraySize; ++i) {
        if (f32Max < af32Src[i]) {
            f32Max = af32Src[i];
        }
    }

    for (HI_S32 i = 0; i < s32ArraySize; ++i) {
        af32Src[i] = QuickExp((HI_S32)((af32Src[i] - f32Max) * SVP_WK_QUANT_BASE));
        f32Sum += af32Src[i];
    }

    for (HI_S32 i = 0; i < s32ArraySize; ++i) {
        af32Src[i] /= f32Sum;
    }

    return HI_SUCCESS;
}

static HI_FLOAT GetMaxVal(HI_FLOAT *pf32Val, HI_U32 u32Num, HI_U32 *pu32MaxValueIndex)
{
    HI_FLOAT f32MaxTmp = pf32Val[0];
    *pu32MaxValueIndex = 0;
    for (HI_U32 i = 1; i < u32Num; i++) {
        if (pf32Val[i] > f32MaxTmp) {
            f32MaxTmp = pf32Val[i];
            *pu32MaxValueIndex = i;
        }
    }

    return f32MaxTmp;
}

HI_S32 *GetResultMem_YoloV2()
{
    HI_S32 *ps32Mem = HI_NULL;

    HI_U32 inputdate_size = SVP_SAMPLE_YOLOV2_GRIDNUM * SVP_SAMPLE_YOLOV2_GRIDNUM * SVP_SAMPLE_YOLOV2_CHANNLENUM *
                            sizeof(HI_FLOAT);
    HI_U32 u32TmpBoxSize = SVP_SAMPLE_YOLOV2_GRIDNUM * SVP_SAMPLE_YOLOV2_GRIDNUM * SVP_SAMPLE_YOLOV2_CHANNLENUM *
                           sizeof(HI_U32);
    HI_U32 u32BoxSize = SVP_SAMPLE_YOLOV2_BOXTOTLENUM * sizeof(SVP_SAMPLE_BOX_S);
    HI_U32 u32StackSize = SVP_SAMPLE_YOLOV2_BOXTOTLENUM * sizeof(SVP_SAMPLE_STACK_S);
    HI_U32 u32ResultBoxSize = SVP_SAMPLE_YOLOV2_MAX_BOX_NUM * sizeof(SVP_SAMPLE_BOX_S);
    HI_U32 u32ResultMemSize = inputdate_size + u32TmpBoxSize + u32BoxSize + u32StackSize + u32ResultBoxSize;
    ps32Mem = (HI_S32 *)malloc(u32ResultMemSize);
    if (ps32Mem != HI_NULL) {
        memset(ps32Mem, 0, u32ResultMemSize);
    } else {
        printf("malloc fail\n");
        return NULL;
    }
    return ps32Mem;
}

HI_VOID SAMPLE_RUNTIME_YoloV2_Box(HI_FLOAT* pf32BoxTmp, SVP_SAMPLE_BOX_S* pstBox, HI_U32* u32BoxsNum)
{
    for (HI_U32 n = 0;n < SVP_SAMPLE_YOLOV2_GRIDNUM_SQR; n++) {
        // Grid
        HI_U32 w = n % SVP_SAMPLE_YOLOV2_GRIDNUM;
        HI_U32 h = n / SVP_SAMPLE_YOLOV2_GRIDNUM;
        for (HI_U32 k = 0; k < SVP_SAMPLE_YOLOV2_BOXNUM; k++) {
            HI_U32 u32Index = (n * SVP_SAMPLE_YOLOV2_BOXNUM + k) * SVP_SAMPLE_YOLOV2_MAX_BOX_NUM;
            HI_FLOAT f32CenterX = ((HI_FLOAT)w + Sigmoid(pf32BoxTmp[u32Index + X_MIN_IDX])) / SVP_SAMPLE_YOLOV2_GRIDNUM;
            HI_FLOAT f32CenterY = ((HI_FLOAT)h + Sigmoid(pf32BoxTmp[u32Index + Y_MIN_IDX])) / SVP_SAMPLE_YOLOV2_GRIDNUM;
            HI_FLOAT f32Width = (HI_FLOAT)(exp(pf32BoxTmp[u32Index + X_MAX_IDX]) *
                YoloV2Bias[X_MAX_IDX * k + X_MIN_IDX]) / SVP_SAMPLE_YOLOV2_GRIDNUM;
            HI_FLOAT f32Height = (HI_FLOAT)(exp(pf32BoxTmp[u32Index + Y_MAX_IDX]) *
                YoloV2Bias[X_MAX_IDX * k + Y_MIN_IDX]) / SVP_SAMPLE_YOLOV2_GRIDNUM;
            HI_FLOAT f32ObjScore = Sigmoid(pf32BoxTmp[u32Index + BBOX_SCORE_IDX]);  // objsocre
            SoftMax(&pf32BoxTmp[u32Index + SVP_SAMPLE_YOLOV2_CLASSNUM], SVP_SAMPLE_YOLOV2_CLASSNUM); // MaxClassScore
            // get maxValue in array
            HI_U32 u32MaxValueIndex = 0;
            HI_FLOAT f32MaxScore = GetMaxVal(&pf32BoxTmp[u32Index + SVP_SAMPLE_YOLOV2_CLASSNUM],
                SVP_SAMPLE_YOLOV2_CLASSNUM, &u32MaxValueIndex);
            HI_FLOAT f32ClassScore = f32MaxScore * f32ObjScore;
            if (f32ClassScore > SVP_SAMPLE_YOLOV2_SCORE_FILTER_THREASH) {
                pstBox[*u32BoxsNum].f32Xmin = f32CenterX - f32Width * SAMPLE_SVP_NNIE_HALF;
                pstBox[*u32BoxsNum].f32Xmax = f32CenterX + f32Width * SAMPLE_SVP_NNIE_HALF;
                pstBox[*u32BoxsNum].f32Ymin = f32CenterY - f32Height * SAMPLE_SVP_NNIE_HALF;
                pstBox[*u32BoxsNum].f32Ymax = f32CenterY + f32Height * SAMPLE_SVP_NNIE_HALF;
                pstBox[*u32BoxsNum].f32ClsScore = f32ClassScore;
                pstBox[*u32BoxsNum].u32MaxScoreIndex = u32MaxValueIndex + 1;
                pstBox[*u32BoxsNum].u32Mask = 0;
                (*u32BoxsNum)++;
            }
        }
    }
}

HI_VOID SAMPLE_RUNTIME_YoloV2GetResult(HI_RUNTIME_BLOB_S *astDstBlobs, HI_S32 *ps32ResultMem)
{
    HI_FLOAT *pf32InputData = (HI_FLOAT*)ps32ResultMem;
    HI_FLOAT* pf32BoxTmp = (HI_FLOAT*)(pf32InputData + SVP_SAMPLE_YOLOV2_BOXTOTLENUM * SVP_SAMPLE_YOLOV2_MAX_BOX_NUM);
    SVP_SAMPLE_BOX_S* pstBox = (SVP_SAMPLE_BOX_S*)(pf32BoxTmp + SVP_SAMPLE_YOLOV2_BOXTOTLENUM *
        SVP_SAMPLE_YOLOV2_MAX_BOX_NUM);
    SVP_SAMPLE_STACK_S* pstAssistStack = (SVP_SAMPLE_STACK_S*)(pstBox + SVP_SAMPLE_YOLOV2_BOXTOTLENUM);
    SVP_SAMPLE_BOX_S* pstBoxResult = (SVP_SAMPLE_BOX_S*)(pstAssistStack + SVP_SAMPLE_YOLOV2_BOXTOTLENUM);

    HI_U32 u32BoxResultNum = 0;
    HI_U32 u32BoxsNum = 0;
    HI_S32* ps32InputData = (HI_S32*)((uintptr_t)astDstBlobs[0].u64VirAddr);

    HI_U32* p32BoxNum = (HI_U32*)malloc(sizeof(HI_U32));
    SAMPLE_CHK_RET_VOID((p32BoxNum == NULL), "malloc p32BoxNum fail\n");
    memset(p32BoxNum, 0, sizeof(HI_U32));
    for (HI_U32 n = 0; n < SVP_SAMPLE_YOLOV2_BOXTOTLENUM * SVP_SAMPLE_YOLOV2_MAX_BOX_NUM; n++) {
        pf32InputData[n] = (HI_FLOAT)ps32InputData[n] / SVP_WK_QUANT_BASE;
    }
    HI_U32 n = 0;
    for (HI_U32 h = 0; h < SVP_SAMPLE_YOLOV2_GRIDNUM; h++) {
        for (HI_U32 w = 0; w < SVP_SAMPLE_YOLOV2_GRIDNUM; w++) {
            for (HI_U32 c = 0; c < SVP_SAMPLE_YOLOV2_CHANNLENUM; c++) {
                pf32BoxTmp[n++] = pf32InputData[c * SVP_SAMPLE_YOLOV2_GRIDNUM_SQR +
                                                h * SVP_SAMPLE_YOLOV2_GRIDNUM + w];
            }
        }
    }
    SAMPLE_RUNTIME_YoloV2_Box(pf32BoxTmp, pstBox, &u32BoxsNum);
    // quick_sort
    NonRecursiveArgQuickSortWithBox(pstBox, 0, u32BoxsNum - 1, pstAssistStack);
    // Nms
    YoloNonMaxSuppression(pstBox, u32BoxsNum, SVP_SAMPLE_YOLOV2_NMS_THREASH, SVP_SAMPLE_YOLOV2_MAX_BOX_NUM);
    for (HI_U32 n = 0; (n < u32BoxsNum) && (u32BoxResultNum < SVP_SAMPLE_YOLOV2_MAX_BOX_NUM) ; n++) {
        if (pstBox[n].u32Mask == 0) {
            pstBoxResult[u32BoxResultNum].f32Xmin = SVP_MAX(pstBox[n].f32Xmin * SVP_SAMPLE_YOLOV2_IMG_WIDTH, 0);
            pstBoxResult[u32BoxResultNum].f32Xmax = SVP_MIN(pstBox[n].f32Xmax *
                SVP_SAMPLE_YOLOV2_IMG_WIDTH, SVP_SAMPLE_YOLOV2_IMG_WIDTH);
            pstBoxResult[u32BoxResultNum].f32Ymax = SVP_MIN(pstBox[n].f32Ymax *
                SVP_SAMPLE_YOLOV2_IMG_WIDTH, SVP_SAMPLE_YOLOV2_IMG_WIDTH);
            pstBoxResult[u32BoxResultNum].f32Ymin = SVP_MAX(pstBox[n].f32Ymin * SVP_SAMPLE_YOLOV2_IMG_WIDTH, 0);
            pstBoxResult[u32BoxResultNum].f32ClsScore = pstBox[n].f32ClsScore;
            pstBoxResult[u32BoxResultNum].u32MaxScoreIndex = pstBox[n].u32MaxScoreIndex;
            u32BoxResultNum++;
        }
    }
    memcpy(pstBox, pstBoxResult, sizeof(SVP_SAMPLE_BOX_S) * SVP_SAMPLE_YOLOV2_OUTBOX_NUM);
    p32BoxNum[1] = SVP_SAMPLE_YOLOV2_OUTBOX_NUM;

    SVP_SAMPLE_BOX_RESULT_INFO_S pstResultBoxesInfo;
    pstResultBoxesInfo.pstBbox = pstBox;
    SvpDetYoloResultPrint(&pstResultBoxesInfo, p32BoxNum[1]);
    SAMPLE_FREE(p32BoxNum);
}

// yolov3
static HI_U32 SvpSampleWkYoloV3GetBoxTotleNum(SVP_SAMPLE_YOLOV3_SCALE_TYPE_E enScaleType)
{
    switch (enScaleType) {
        case CONV_82:
            return SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_82 * SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_82 * SVP_SAMPLE_YOLOV3_BOXNUM;
        case CONV_94:
            return SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_94 * SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_94 * SVP_SAMPLE_YOLOV3_BOXNUM;
        case CONV_106:
            return SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_106 * SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_82 * SVP_SAMPLE_YOLOV3_BOXNUM;
        default:
            return 0;
    }
}

static HI_DOUBLE s_SvpSampleYoloV3Bias[SVP_SAMPLE_YOLOV3_SCALE_TYPE_MAX][6] = {
    {116, 90, 156, 198, 373, 326},
    {30, 61, 62, 45, 59, 119},
    {10, 13, 16, 30, 33, 23}
};

static HI_U32 SvpSampleWkYoloV3GetGridNum(SVP_SAMPLE_YOLOV3_SCALE_TYPE_E enScaleType)
{
    switch (enScaleType) {
        case CONV_82:
            return SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_82;
            break;
        case CONV_94:
            return SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_94;
            break;
        case CONV_106:
            return SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_106;
            break;
        default:
            return 0;
    }
}

static HI_U32 SvpSampleGetYolov3ResultMemSize(SVP_SAMPLE_YOLOV3_SCALE_TYPE_E enScaleType)
{
    HI_U32 u32GridNum = SvpSampleWkYoloV3GetGridNum(enScaleType);
    HI_U32 inputdate_size = u32GridNum * u32GridNum * SVP_SAMPLE_YOLOV3_CHANNLENUM * sizeof(HI_FLOAT);
    HI_U32 u32TmpBoxSize = u32GridNum * u32GridNum * SVP_SAMPLE_YOLOV3_CHANNLENUM * sizeof(HI_U32);
    HI_U32 u32BoxSize = u32GridNum * u32GridNum * SVP_SAMPLE_YOLOV3_BOXNUM *
        SVP_SAMPLE_YOLOV3_CLASSNUM * sizeof(SVP_SAMPLE_BOX_S);
    return (inputdate_size + u32TmpBoxSize + u32BoxSize);

}

static HI_VOID GetBoxResult(SVP_SAMPLE_BOX_S *pstBox, HI_U32 *u32BoxsNum, HI_U32 u32GridNum, HI_FLOAT* pf32BoxTmp,
                            SVP_SAMPLE_YOLOV3_SCALE_TYPE_E enScaleType)
{
    HI_FLOAT f32ScoreFilterThresh = SVP_SAMPLE_YOLOV3_SCORE_FILTER_THREASH;
    for (HI_U32 n = 0; n < u32GridNum * u32GridNum; n++) {
        // Grid
        HI_U32 w = n % u32GridNum;
        HI_U32 h = n / u32GridNum;
        for (HI_U32 k = 0; k < SVP_SAMPLE_YOLOV3_BOXNUM; k++) {
            HI_U32 u32Index = (n * SVP_SAMPLE_YOLOV3_BOXNUM + k) * SVP_SAMPLE_YOLOV3_PARAMNUM;
            HI_FLOAT x = ((HI_FLOAT)w + Sigmoid(pf32BoxTmp[u32Index + 0])) / u32GridNum;
            HI_FLOAT y = ((HI_FLOAT)h + Sigmoid(pf32BoxTmp[u32Index + 1])) / u32GridNum;
            HI_FLOAT f32Width = (HI_FLOAT)(exp(pf32BoxTmp[u32Index + 2]) *
                s_SvpSampleYoloV3Bias[enScaleType][2 * k]) / SVP_SAMPLE_YOLOV3_SRC_WIDTH;
            HI_FLOAT f32Height = (HI_FLOAT)(exp(pf32BoxTmp[u32Index + 3]) *
                s_SvpSampleYoloV3Bias[enScaleType][2 * k + 1]) / SVP_SAMPLE_YOLOV3_SRC_HEIGHT;

            HI_FLOAT f32ObjScore = Sigmoid(pf32BoxTmp[u32Index + 4]); // objscore;
            if (f32ObjScore <= f32ScoreFilterThresh) {
                continue;
            }

            for (HI_U32 classIdx = 0; classIdx < SVP_SAMPLE_YOLOV3_CLASSNUM; classIdx++) {
                HI_U32 u32ClassIdxBase = u32Index + 4 + 1;
                HI_FLOAT f32ClassScore = Sigmoid(pf32BoxTmp[u32ClassIdxBase + classIdx]); // objscore;
                HI_FLOAT f32Prob = f32ObjScore * f32ClassScore;
                f32Prob = (f32Prob > f32ScoreFilterThresh) ? f32Prob : 0.0f;
                pstBox[*u32BoxsNum].f32Xmin = x - f32Width * 0.5f;  // xmin
                pstBox[*u32BoxsNum].f32Xmax = x + f32Width * 0.5f;  // xmax
                pstBox[*u32BoxsNum].f32Ymin = y - f32Height * 0.5f; // ymin
                pstBox[*u32BoxsNum].f32Ymax = y + f32Height * 0.5f; // ymax
                pstBox[*u32BoxsNum].f32ClsScore = f32Prob;          // predict prob
                pstBox[*u32BoxsNum].u32MaxScoreIndex = classIdx;    // class score index
                pstBox[*u32BoxsNum].u32Mask = 0;                    // Suppression mask

                (*u32BoxsNum)++;
            }
        }
    }
}

static HI_VOID SvpSampleWkYoloV3GetResultForOneBlob(HI_RUNTIME_BLOB_S *pstDstBlob,
                                                    SAMPLE_YOLOV3_DATA *pstBlobData,
                                                    SVP_SAMPLE_YOLOV3_SCALE_TYPE_E enScaleType,
                                                    SVP_SAMPLE_BOX_S** ppstBox,
                                                    HI_U32 *pu32BoxNum)
{
    // result calc para config
    HI_U32 u32GridNum = SvpSampleWkYoloV3GetGridNum(enScaleType);
    if (u32GridNum == 0) {
        printf("enScaleType error!\n");
        return;
    }
    HI_U32 u32CStep = u32GridNum * u32GridNum;
    HI_U32 u32HStep = u32GridNum;

    HI_U32 inputdate_size = u32GridNum * u32GridNum * SVP_SAMPLE_YOLOV3_CHANNLENUM;
    HI_U32 u32TmpBoxSize = u32GridNum * u32GridNum * SVP_SAMPLE_YOLOV3_CHANNLENUM;

    HI_FLOAT* pf32InputData = (HI_FLOAT*)pstBlobData->ps32ResultMem;
    HI_FLOAT* pf32BoxTmp = (HI_FLOAT*)(pf32InputData + inputdate_size); // tep_box_size
    SVP_SAMPLE_BOX_S* pstBox = (SVP_SAMPLE_BOX_S*)(pf32BoxTmp + u32TmpBoxSize); // assit_box_size
    HI_U32 u32BoxsNum = 0;
    if ((u32GridNum != pstDstBlob->unShape.stWhc.u32Height) ||
        (u32GridNum != pstDstBlob->unShape.stWhc.u32Width)  ||
        (SVP_SAMPLE_YOLOV3_CHANNLENUM != pstDstBlob->unShape.stWhc.u32Chn)) {
        printf("error grid number!\n");
        return;
    }
    HI_U32 u32OneCSize = pstDstBlob->u32Stride * pstDstBlob->unShape.stWhc.u32Height;
    for (HI_U32 c = 0; c < SVP_SAMPLE_YOLOV3_CHANNLENUM; c++) {
        for (HI_U32 h = 0; h < u32GridNum; h++) {
            for (HI_U32 w = 0; w < u32GridNum; w++) {
                HI_S32* ps32Temp = (HI_S32*)(pstBlobData->pu8InputData + c *
                    u32OneCSize + h * pstDstBlob->u32Stride) + w;
                *pf32InputData++ = (HI_FLOAT)(*ps32Temp) / SVP_WK_QUANT_BASE;

            }
        }
    }
    pf32InputData = (HI_FLOAT*)pstBlobData->ps32ResultMem;
    for (HI_U32 h = 0; h < u32GridNum; h++) {
        for (HI_U32 w = 0; w < u32GridNum; w++) {
            for (HI_U32 c = 0; c < SVP_SAMPLE_YOLOV3_CHANNLENUM; c++) {
                *pf32BoxTmp++ = pf32InputData[c * u32CStep + h * u32HStep + w];
            }
        }
    }
    pf32BoxTmp = (HI_FLOAT*)(pf32InputData + inputdate_size);
    GetBoxResult(pstBox, &u32BoxsNum, u32GridNum, pf32BoxTmp, enScaleType);
    *pu32BoxNum = u32BoxsNum;
    *ppstBox = pstBox;
}

static HI_VOID SvpSampleWkYoloV3GetResultForThreeBlob(HI_RUNTIME_BLOB_S *pstDstBlob, HI_S32 *ps32ResultMem,
    SVP_SAMPLE_BOX_S* apstBox[3], HI_U32 au32BoxNum[3])
{
    HI_U32 u32NumIndex = 0;
    HI_RUNTIME_BLOB_S* pstTempBlob = NULL;

    pstTempBlob = pstDstBlob;
    HI_U32 u32OneCSize = pstTempBlob->u32Stride * pstTempBlob->unShape.stWhc.u32Height;
    HI_U32 u32FrameStride = u32OneCSize * pstTempBlob->unShape.stWhc.u32Chn;
    HI_U8* pu8InputData = (HI_U8*)(intptr_t)pstTempBlob->u64VirAddr + u32NumIndex * u32FrameStride;
    SVP_SAMPLE_RESULT_MEM_HEAD_S *pstHead = (SVP_SAMPLE_RESULT_MEM_HEAD_S *)ps32ResultMem;
    SAMPLE_YOLOV3_DATA stBlobData;
    stBlobData.pu8InputData = pu8InputData;
    stBlobData.ps32ResultMem = (HI_S32*)(pstHead + 1);
    /*********** 1st blob *********/
    SvpSampleWkYoloV3GetResultForOneBlob(pstTempBlob, &stBlobData, (SVP_SAMPLE_YOLOV3_SCALE_TYPE_E)pstHead->u32Type,
        &apstBox[0], &au32BoxNum[0]);
    pstTempBlob = pstDstBlob + 1;
    u32OneCSize = pstTempBlob->u32Stride * pstTempBlob->unShape.stWhc.u32Height;
    u32FrameStride = u32OneCSize * pstTempBlob->unShape.stWhc.u32Chn;
    stBlobData.pu8InputData = (HI_U8*)(intptr_t)pstTempBlob->u64VirAddr + u32NumIndex * u32FrameStride;
    pstHead = (SVP_SAMPLE_RESULT_MEM_HEAD_S*)((HI_U8 *)(pstHead + 1) + pstHead->u32Len);
    stBlobData.ps32ResultMem = (HI_S32*)(pstHead + 1);
    /*********** 2nd blob *********/
    SvpSampleWkYoloV3GetResultForOneBlob(pstTempBlob, &stBlobData, (SVP_SAMPLE_YOLOV3_SCALE_TYPE_E)pstHead->u32Type,
        &apstBox[1], &au32BoxNum[1]);
    pstTempBlob = pstDstBlob + 2;
    u32OneCSize = pstTempBlob->u32Stride * pstTempBlob->unShape.stWhc.u32Height;
    u32FrameStride = u32OneCSize * pstTempBlob->unShape.stWhc.u32Chn;
    stBlobData.pu8InputData = (HI_U8*)(intptr_t)pstTempBlob->u64VirAddr + u32NumIndex * u32FrameStride;
    pstHead = (SVP_SAMPLE_RESULT_MEM_HEAD_S*)((HI_U8 *)(pstHead + 1) + pstHead->u32Len);
    stBlobData.ps32ResultMem = (HI_S32*)(pstHead + 1);
    /*********** 3rd blob *********/
    SvpSampleWkYoloV3GetResultForOneBlob(pstTempBlob, &stBlobData, (SVP_SAMPLE_YOLOV3_SCALE_TYPE_E)pstHead->u32Type,
        &apstBox[2], &au32BoxNum[2]);
}

static HI_VOID SvpSampleWKYoloV3BoxPostProcess(SVP_SAMPLE_BOX_S* pstInputBbox, HI_U32 u32InputBboxNum,
    SVP_SAMPLE_BOX_S* pstResultBbox, HI_U32 *pu32BoxNum)
{
    HI_FLOAT f32NmsThresh = SVP_SAMPLE_YOLOV3_NMS_THREASH;
    HI_U32 u32MaxBoxNum = SVP_SAMPLE_YOLOV3_MAX_BOX_NUM;
    HI_U32 u32SrcWidth  = SVP_SAMPLE_YOLOV3_SRC_WIDTH;
    HI_U32 u32SrcHeight = SVP_SAMPLE_YOLOV3_SRC_HEIGHT;

    HI_U32 u32AssistStackNum = SvpSampleWkYoloV3GetBoxTotleNum(CONV_82)
                               + SvpSampleWkYoloV3GetBoxTotleNum(CONV_94)
                               + SvpSampleWkYoloV3GetBoxTotleNum(CONV_106);

    HI_U32 u32AssistStackSize = u32AssistStackNum * sizeof(SVP_SAMPLE_STACK_S);
    SVP_SAMPLE_STACK_S* pstAssistStack = (SVP_SAMPLE_STACK_S*)malloc(u32AssistStackSize); // assit_size
    if (NULL == pstAssistStack) {
        printf("Malloc fail, size %d!\n", u32AssistStackSize);
        return;
    }
    memset(pstAssistStack, 0, u32AssistStackSize);
    // quick_sort
    NonRecursiveArgQuickSortWithBox(pstInputBbox, 0, u32InputBboxNum - 1, pstAssistStack);
    free(pstAssistStack);
    // Nms
    YoloNonMaxSuppression(pstInputBbox, u32InputBboxNum, f32NmsThresh, u32MaxBoxNum);
    // Get the result
    HI_U32 u32BoxResultNum = 0;
    for (HI_U32 n = 0; (n < u32InputBboxNum) && (u32BoxResultNum < u32MaxBoxNum); n++) {
        if (pstInputBbox[n].u32Mask == 0) {
            pstResultBbox[u32BoxResultNum].f32Xmin = SVP_MAX(pstInputBbox[n].f32Xmin * u32SrcWidth, 0);
            pstResultBbox[u32BoxResultNum].f32Xmax = SVP_MIN(pstInputBbox[n].f32Xmax * u32SrcWidth, u32SrcWidth);
            pstResultBbox[u32BoxResultNum].f32Ymax = SVP_MIN(pstInputBbox[n].f32Ymax * u32SrcHeight, u32SrcHeight);
            pstResultBbox[u32BoxResultNum].f32Ymin = SVP_MAX(pstInputBbox[n].f32Ymin * u32SrcHeight, 0);
            pstResultBbox[u32BoxResultNum].f32ClsScore = pstInputBbox[n].f32ClsScore;
            pstResultBbox[u32BoxResultNum].u32MaxScoreIndex = pstInputBbox[n].u32MaxScoreIndex;

            u32BoxResultNum++;
        }
    }
    printf("u32BoxResultNum: %d\n", u32BoxResultNum);
    if (u32BoxResultNum == 0) {
        return;
    }
    *pu32BoxNum += u32BoxResultNum;
}

HI_S32* GetResultMem_YoloV3()
{
    HI_S32 *ps32Mem = HI_NULL;
    HI_U32 u32ResultMemSize = 0;

    SVP_SAMPLE_RESULT_MEM_HEAD_S *pstHead = { 0 };
    HI_U32 u32ResultMemSize1 = SvpSampleGetYolov3ResultMemSize(CONV_82);
    HI_U32 u32ResultMemSize2 = SvpSampleGetYolov3ResultMemSize(CONV_94);
    HI_U32 u32ResultMemSize3 = SvpSampleGetYolov3ResultMemSize(CONV_106);
    if ((u32ResultMemSize1 + u32ResultMemSize2 + u32ResultMemSize3) != 0) {
        u32ResultMemSize = u32ResultMemSize1 + u32ResultMemSize2 + u32ResultMemSize3 +
            sizeof(SVP_SAMPLE_RESULT_MEM_HEAD_S) * 3;
        ps32Mem = (HI_S32*)malloc(u32ResultMemSize);
        if (ps32Mem != HI_NULL) {
            memset(ps32Mem, 0, u32ResultMemSize);

            pstHead = (SVP_SAMPLE_RESULT_MEM_HEAD_S *)ps32Mem;
            pstHead->u32Type = CONV_82;
            pstHead->u32Len = u32ResultMemSize1;

            pstHead = (SVP_SAMPLE_RESULT_MEM_HEAD_S *)(((HI_U8 *)pstHead) +
                sizeof(SVP_SAMPLE_RESULT_MEM_HEAD_S) + u32ResultMemSize1);
            pstHead->u32Type = CONV_94;
            pstHead->u32Len = u32ResultMemSize2;

            pstHead = (SVP_SAMPLE_RESULT_MEM_HEAD_S *)(((HI_U8 *)pstHead) +
                sizeof(SVP_SAMPLE_RESULT_MEM_HEAD_S) + u32ResultMemSize2);
            pstHead->u32Type = CONV_106;
            pstHead->u32Len = u32ResultMemSize3;
            }
    }
    return ps32Mem;
}

HI_VOID SAMPLE_RUNTIME_YoloV3GetResult(HI_RUNTIME_BLOB_S *pstDstBlob, HI_S32 *ps32ResultMem,
    SVP_SAMPLE_BOX_RESULT_INFO_S *pstResultBoxInfo, HI_U32 *pu32BoxNum)
{
    HI_U32 u32NumIndex = 0;
    HI_U32 u32ResultBoxNum = 0;

    SVP_SAMPLE_BOX_S* pstResultBbox = pstResultBoxInfo->pstBbox;
    SVP_SAMPLE_BOX_RESULT_INFO_S stTempBoxResultInfo;
    SVP_SAMPLE_BOX_S* pstTempBbox = NULL;

    SVP_SAMPLE_BOX_S* apstBox[3] = { NULL };
    HI_U32 au32BoxNum[3] = { 0 };

    SvpSampleWkYoloV3GetResultForThreeBlob(pstDstBlob, ps32ResultMem, apstBox, au32BoxNum);

    HI_U32 u32TempBoxNum = au32BoxNum[0] + au32BoxNum[1] + au32BoxNum[2];
    if (u32TempBoxNum == 0) {
        printf("%s get boxNum == 0, return directly\n", __FUNCTION__);
        return;
    }

    pstTempBbox = (SVP_SAMPLE_BOX_S*)malloc(sizeof(SVP_SAMPLE_BOX_S) * u32TempBoxNum);
    SAMPLE_CHK_RET_VOID((pstTempBbox == NULL), "malloc pstTempBbox fail\n");

    memset(pstTempBbox, 0, sizeof(SVP_SAMPLE_BOX_S) * u32TempBoxNum);

    memcpy((HI_U8*)(pstTempBbox), (HI_U8*)apstBox[0], sizeof(SVP_SAMPLE_BOX_S) * au32BoxNum[0]);
    memcpy((HI_U8*)(pstTempBbox + au32BoxNum[0]), (HI_U8*)apstBox[1], sizeof(SVP_SAMPLE_BOX_S) * au32BoxNum[1]);
    memcpy((HI_U8*)(pstTempBbox + au32BoxNum[0] + au32BoxNum[1]), (HI_U8*)apstBox[2],
        sizeof(SVP_SAMPLE_BOX_S) * au32BoxNum[2]);

    SvpSampleWKYoloV3BoxPostProcess(pstTempBbox, u32TempBoxNum,
        &pstResultBbox[u32ResultBoxNum], &pu32BoxNum[u32NumIndex]);

    stTempBoxResultInfo.u32OriImHeight = pstResultBoxInfo->u32OriImHeight;
    stTempBoxResultInfo.u32OriImWidth = pstResultBoxInfo->u32OriImWidth;
    stTempBoxResultInfo.pstBbox = &pstResultBbox[u32ResultBoxNum];
    SvpDetYoloResultPrint(&stTempBoxResultInfo, pu32BoxNum[u32NumIndex]);
    u32ResultBoxNum = u32ResultBoxNum + pu32BoxNum[u32NumIndex];
    if (u32ResultBoxNum >= 1024) {
        printf("Box number reach max 1024!\n");
        SAMPLE_FREE(pstTempBbox);
        return;
    }
    SAMPLE_FREE(pstTempBbox);
}
