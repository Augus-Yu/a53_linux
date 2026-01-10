/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include <math.h>
#include "hi_runtime_comm.h"
#include "sample_data_utils.h"
#include "sample_memory_ops.h"
#include "sample_log.h"

enum {
    SAMPLE_SSD_REPORT_NODE_NUM = 12,
    SAMPLE_SSD_PRIORBOX_NUM = 6,
    SAMPLE_SSD_SOFTMAX_NUM = 6,
    SAMPLE_SSD_ASPECT_RATIO_NUM = 6
};
/*
* SSD software parameter
* Array subscript reference SSD algorithm
*/
typedef struct hiSAMPLE_SSD_PARAM_S {
    /* ----------------- Model Parameters --------------- */
    HI_U32 au32ConvHeight[12];
    HI_U32 au32ConvWidth[12];
    HI_U32 au32ConvChannel[12];
    /* ----------------- PriorBox Parameters --------------- */
    HI_U32 au32PriorBoxWidth[6];
    HI_U32 au32PriorBoxHeight[6];
    HI_FLOAT af32PriorBoxMinSize[6][1];
    HI_FLOAT af32PriorBoxMaxSize[6][1];
    HI_U32 u32MinSizeNum;
    HI_U32 u32MaxSizeNum;
    HI_U32 u32OriImHeight;
    HI_U32 u32OriImWidth;
    HI_U32 au32InputAspectRatioNum[6];
    HI_FLOAT af32PriorBoxAspectRatio[6][2];
    HI_FLOAT af32PriorBoxStepWidth[6];
    HI_FLOAT af32PriorBoxStepHeight[6];
    HI_FLOAT f32Offset;
    HI_BOOL bFlip;
    HI_BOOL bClip;
    HI_S32 as32PriorBoxVar[4]; // detection box have 4 value
    /* ----------------- Softmax Parameters --------------- */
    HI_U32 au32SoftMaxInChn[6];
    HI_U32 u32SoftMaxInHeight;
    HI_U32 u32ConcatNum;
    HI_U32 u32SoftMaxOutWidth;
    HI_U32 u32SoftMaxOutHeight;
    HI_U32 u32SoftMaxOutChn;
    /* ----------------- DetectionOut Parameters --------------- */
    HI_U32 u32ClassNum;
    HI_U32 u32TopK;
    HI_U32 u32KeepTopK;
    HI_U32 u32NmsThresh;
    HI_U32 u32ConfThresh;
    HI_U32 au32DetectInputChn[6];
    HI_U32 au32ConvStride[6];
    HI_RUNTIME_MEM_S stPriorBoxTmpBuf;
    HI_RUNTIME_MEM_S stSoftMaxTmpBuf;
    HI_RUNTIME_MEM_S stGetResultTmpBuf;
    HI_RUNTIME_MEM_S stClassRoiNum;
    HI_RUNTIME_MEM_S stDstRoi;
    HI_RUNTIME_MEM_S stDstScore;
} HI_SAMPLE_SSD_PARAM_S;

typedef struct hiSAMPLE_SSD_PRIOR_BOX_S{
    HI_U32 u32PriorBoxWidth;
    HI_U32 u32PriorBoxHeight;
    HI_U32 u32OriImWidth;
    HI_U32 u32OriImHeight;
    HI_FLOAT* pf32PriorBoxMinSize;
    HI_U32 u32MinSizeNum;
    HI_FLOAT* pf32PriorBoxMaxSize;
    HI_U32 u32MaxSizeNum;
    HI_BOOL bFlip;
    HI_BOOL bClip;
    HI_U32 u32InputAspectRatioNum;
    HI_FLOAT f32PriorBoxStepWidth;
    HI_FLOAT f32PriorBoxStepHeight;
    HI_FLOAT f32Offset;
    HI_S32* ps32PriorboxOutputData;
} HI_SAMPLE_SSD_PRIOR_BOX_S;

typedef struct hiSAMPLE_SSD_DetOut_PARAM_S{
    HI_U32 u32ClassNum;
    HI_U32 u32PriorNum;
    HI_S32 *ps32SingleProposal;
    HI_S32 *ps32AllDecodeBoxes;
    HI_S32 *ps32ConfScores;
    HI_U32 u32TopK;
    HI_U32 u32NmsThresh;
    HI_S32 *ps32DstScoreSrc;
    HI_S32 *ps32DstBboxSrc;
    HI_S32 *ps32RoiOutCntSrc;
    NNIE_STACK_S *pstStack;
    HI_S32 *ps32ClassRoiNum;
    HI_U32 u32AfterTopK;
} HI_SAMPLE_SSD_DetOut_PARAM_S;

static HI_U32 SAMPLE_Ssd_GetResultTmpBuf(HI_SAMPLE_SSD_PARAM_S *pstSsdParam)
{
    HI_U32 u32PriorBoxSize = 0;
    HI_U32 u32SoftMaxSize = 0;
    HI_U32 u32DetectionSize = 0;
    HI_U32 u32TotalSize = 0;
    HI_U32 u32PriorNum = 0;
    HI_U32 i = 0;

    /* priorbox size */
    for (i = 0; i < SAMPLE_SSD_PRIORBOX_NUM; i++) {
        u32PriorBoxSize += pstSsdParam->au32PriorBoxHeight[i] * pstSsdParam->au32PriorBoxWidth[i] *
            SVP_WK_COORDI_NUM * 2 * (pstSsdParam->u32MaxSizeNum + pstSsdParam->u32MinSizeNum +
            pstSsdParam->au32InputAspectRatioNum[i] * 2 * pstSsdParam->u32MinSizeNum) * sizeof(HI_U32);
    }
    pstSsdParam->stPriorBoxTmpBuf.u32Size = u32PriorBoxSize;
    u32TotalSize += u32PriorBoxSize;
    /* softmax size */
    for (i = 0; i < pstSsdParam->u32ConcatNum; i++) {
        u32SoftMaxSize += pstSsdParam->au32SoftMaxInChn[i] * sizeof(HI_U32);
    }
    pstSsdParam->stSoftMaxTmpBuf.u32Size = u32SoftMaxSize;
    u32TotalSize += u32SoftMaxSize;

    /* detection size */
    for (i = 0; i < pstSsdParam->u32ConcatNum; i++) {
        u32PriorNum += pstSsdParam->au32DetectInputChn[i] / SVP_WK_COORDI_NUM;
    }
    u32DetectionSize += u32PriorNum * SVP_WK_COORDI_NUM * sizeof(HI_U32);
    u32DetectionSize += u32PriorNum * SVP_WK_PROPOSAL_WIDTH * sizeof(HI_U32) * 2;
    u32DetectionSize += u32PriorNum * 2 * sizeof(HI_U32);
    pstSsdParam->stGetResultTmpBuf.u32Size = u32DetectionSize;
    u32TotalSize += u32DetectionSize;

    return u32TotalSize;
}

static HI_SAMPLE_SSD_PARAM_S s_stSSDParam = {
    { 0,  0,  0,  0, 0, 0,  0, 0, 0, 0, 0, 0 },                                     // height
    { 0,  0,  0,  0, 0, 0,  0, 0, 0, 0, 0, 0 },                                     // height
    { 0,  0,  0,  0, 0, 0,  0, 0, 0, 0, 0, 0 },                                     // channel
    { 38, 19, 10, 5, 3, 1 },                                                    // prior box width
    { 38, 19, 10, 5, 3, 1 },                                                    // prior box height
    {{ 30.0f }, { 60.0f }, { 111.0f }, { 162.0f }, { 213.0f }, { 264.0f }},   // prior box min size
    {{ 60.0f }, { 111.0f }, { 162.0f }, { 213.0f }, { 264.0f }, { 315.0f }},  // prior box max size
    1, 1, 300, 300,                   // min size num, max size num, ori img height, ori img width
    { 1, 2,  2,  2,  1,   1 },                                                       // input aspect ration num
    {{ 2, 0 }, { 2, 3 }, { 2, 3 }, { 2, 3 }, { 2, 0 }, { 2, 0 }},             // prior bbox aspect ratio
    { 8, 16, 32, 64, 100, 300 },                                                // prior bbox step width
    { 8, 16, 32, 64, 100, 300 },                                                // prior bbox step height
    0.5f,                                                                       // offset
    HI_TRUE, HI_FALSE,                                                          // flip, clip
    {
        (HI_S32)(0.1 * SVP_WK_QUANT_BASE),
        (HI_S32)(0.1 * SVP_WK_QUANT_BASE),
        (HI_S32)(0.2 * SVP_WK_QUANT_BASE),
        (HI_S32)(0.2 * SVP_WK_QUANT_BASE)
    },  // prior bbox var
    { 121296, 45486, 12600, 3150, 756, 84 },  // softmax in channel
    21,                                       // softmax in height
    6, 1, 21, 8732,                           // concatnum, softmax out width, height, channel
    21, 400, 200,                             // class num,topk,keep topk
    (HI_U32)(0.3f * SVP_WK_QUANT_BASE),       // nms threshold
    1,                                        // conf threshold
    { 23104, 8664, 2400, 600, 144, 16 },      // detect in channel
};

static HI_VOID InitPriorBoxParam(HI_RUNTIME_BLOB_S *pstDstBlob, HI_SAMPLE_SSD_PARAM_S *pstSsdParam)
{
    memcpy(pstSsdParam, &s_stSSDParam, sizeof(HI_SAMPLE_SSD_PARAM_S));
    for (HI_U32 i = 0; i < SAMPLE_SSD_REPORT_NODE_NUM; i++) {
        pstSsdParam->au32ConvHeight[i] = pstDstBlob[i].unShape.stWhc.u32Chn;
        pstSsdParam->au32ConvWidth[i] = pstDstBlob[i].unShape.stWhc.u32Height;
        pstSsdParam->au32ConvChannel[i] = pstDstBlob[i].unShape.stWhc.u32Width;
        if (i % 2 == 1) {
            pstSsdParam->au32ConvStride[i / 2] =
                getAlignSize(ALIGN_16, pstSsdParam->au32ConvChannel[i] * sizeof(HI_U32)) / sizeof(HI_U32);
        }
    }
}

static HI_S32 SAMPLE_Ssd_InitParam(HI_RUNTIME_BLOB_S *pstSrcBlob, HI_RUNTIME_BLOB_S *pstDstBlob,
                                   HI_SAMPLE_SSD_PARAM_S *pstSsdParam)
{
    HI_U8 *pu8VirAddr = NULL;
    HI_RUNTIME_MEM_S stMem;
    memset(&stMem, 0, sizeof(stMem));

    InitPriorBoxParam(pstDstBlob, pstSsdParam);
    /* Malloc assist buffer memory */
    HI_U32 u32ClassNum = pstSsdParam->u32ClassNum;
    HI_U32 u32TotalSize = SAMPLE_Ssd_GetResultTmpBuf(pstSsdParam);
    HI_U32 u32DstRoiSize = getAlignSize(ALIGN_16, u32ClassNum * pstSsdParam->u32TopK * sizeof(HI_U32) * SVP_WK_COORDI_NUM);
    HI_U32 u32DstScoreSize = getAlignSize(ALIGN_16, u32ClassNum * pstSsdParam->u32TopK * sizeof(HI_U32));
    HI_U32 u32ClassRoiNumSize = getAlignSize(ALIGN_16, u32ClassNum * sizeof(HI_U32));
    u32TotalSize = u32TotalSize + u32DstRoiSize + u32DstScoreSize + u32ClassRoiNumSize;

    stMem.u32Size = u32TotalSize;
    HI_S32 s32Ret = SAMPLE_AllocMem(&stMem, HI_TRUE);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0,
                    "SAMPLE_Utils_AllocMem failed!\n");
    HI_U64 u64PhyAddr = stMem.u64PhyAddr;
    pu8VirAddr = (HI_U8 *)((uintptr_t)stMem.u64VirAddr);
    memset(pu8VirAddr, 0, u32TotalSize);
    s32Ret = SAMPLE_FlushCache(&stMem);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0,
                    "SAMPLE_Utils_FlushCache failed!\n");

    /* set each tmp buffer addr */
    pstSsdParam->stPriorBoxTmpBuf.u64PhyAddr = u64PhyAddr;
    pstSsdParam->stPriorBoxTmpBuf.u64VirAddr = (HI_U64)((uintptr_t)pu8VirAddr);

    pstSsdParam->stSoftMaxTmpBuf.u64PhyAddr = u64PhyAddr +
                                              pstSsdParam->stPriorBoxTmpBuf.u32Size;
    pstSsdParam->stSoftMaxTmpBuf.u64VirAddr = (HI_U64)((uintptr_t)(pu8VirAddr +
                                                                   pstSsdParam->stPriorBoxTmpBuf.u32Size));

    pstSsdParam->stGetResultTmpBuf.u64PhyAddr = u64PhyAddr +
        pstSsdParam->stPriorBoxTmpBuf.u32Size + pstSsdParam->stSoftMaxTmpBuf.u32Size;
    pstSsdParam->stGetResultTmpBuf.u64VirAddr = (HI_U64)((uintptr_t)(pu8VirAddr +
        pstSsdParam->stPriorBoxTmpBuf.u32Size + pstSsdParam->stSoftMaxTmpBuf.u32Size));

    HI_U32 u32TmpBufTotalSize = pstSsdParam->stPriorBoxTmpBuf.u32Size +
                                pstSsdParam->stSoftMaxTmpBuf.u32Size + pstSsdParam->stGetResultTmpBuf.u32Size;

    /* set result blob */
    pstSsdParam->stDstRoi.u64PhyAddr = u64PhyAddr + u32TmpBufTotalSize;
    pstSsdParam->stDstRoi.u64VirAddr = (HI_U64)((uintptr_t)(pu8VirAddr + u32TmpBufTotalSize));

    pstSsdParam->stDstScore.u64PhyAddr = u64PhyAddr + u32TmpBufTotalSize + u32DstRoiSize;
    pstSsdParam->stDstScore.u64VirAddr = (HI_U64)((uintptr_t)(pu8VirAddr + u32TmpBufTotalSize + u32DstRoiSize));

    pstSsdParam->stClassRoiNum.u64PhyAddr = u64PhyAddr + u32TmpBufTotalSize +
                                            u32DstRoiSize + u32DstScoreSize;
    pstSsdParam->stClassRoiNum.u64VirAddr = (HI_U64)((uintptr_t)(pu8VirAddr + u32TmpBufTotalSize +
                                                                 u32DstRoiSize + u32DstScoreSize));

    return s32Ret;
FAIL_0:
    SAMPLE_FreeMem(&stMem);
    return s32Ret;
}

static HI_S32 GetPriorBox(HI_U32 *pu32Index, HI_U32 u32PriorBoxWidth, HI_U32 u32PriorBoxHeight,
    HI_FLOAT f32PriorBoxStepWidth, HI_FLOAT f32PriorBoxStepHeight, HI_FLOAT f32Offset, HI_U32 u32MinSizeNum,
    HI_FLOAT *pf32PriorBoxMaxSize, HI_U32 u32MaxSizeNum, HI_FLOAT *pf32PriorBoxMinSize,
    HI_FLOAT af32AspectRatio[SAMPLE_SSD_ASPECT_RATIO_NUM], HI_U32 u32AspectRatioNum, HI_S32 *ps32PriorboxOutputData)
{
    for (HI_U32 h = 0; h < u32PriorBoxHeight; h++) {
        for (HI_U32 w = 0; w < u32PriorBoxWidth; w++) {
            HI_FLOAT f32CenterX = (w + f32Offset) * f32PriorBoxStepWidth;
            HI_FLOAT f32CenterY = (h + f32Offset) * f32PriorBoxStepHeight;
            for (HI_U32 n = 0; n < u32MinSizeNum; n++) {
                /* first prior */
                HI_FLOAT f32BoxHeight = pf32PriorBoxMinSize[n];
                HI_FLOAT f32BoxWidth = pf32PriorBoxMinSize[n];
                ps32PriorboxOutputData[(*pu32Index)++] = (HI_S32)(f32CenterX - (f32BoxWidth * SAMPLE_SVP_NNIE_HALF));
                ps32PriorboxOutputData[(*pu32Index)++] = (HI_S32)(f32CenterY - (f32BoxHeight * SAMPLE_SVP_NNIE_HALF));
                ps32PriorboxOutputData[(*pu32Index)++] = (HI_S32)(f32CenterX + (f32BoxWidth * SAMPLE_SVP_NNIE_HALF));
                ps32PriorboxOutputData[(*pu32Index)++] = (HI_S32)(f32CenterY + (f32BoxHeight * SAMPLE_SVP_NNIE_HALF));
                /* second prior */
                if (u32MaxSizeNum > 0) {
                    f32BoxHeight = (HI_FLOAT)sqrt(pf32PriorBoxMinSize[n] * pf32PriorBoxMaxSize[n]);
                    f32BoxWidth = (HI_FLOAT)sqrt(pf32PriorBoxMinSize[n] * pf32PriorBoxMaxSize[n]);
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterX - (f32BoxWidth * SAMPLE_SVP_NNIE_HALF));
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterY - (f32BoxHeight * SAMPLE_SVP_NNIE_HALF));
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterX + (f32BoxWidth * SAMPLE_SVP_NNIE_HALF));
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterY + (f32BoxHeight * SAMPLE_SVP_NNIE_HALF));
                }
                /* rest of priors, skip AspectRatio == 1 */
                for (HI_U32 i = 1; i < u32AspectRatioNum; i++) {
                    f32BoxWidth = (HI_FLOAT)(pf32PriorBoxMinSize[n] * sqrt(af32AspectRatio[i]));
                    f32BoxHeight = (HI_FLOAT)(pf32PriorBoxMinSize[n] / sqrt(af32AspectRatio[i]));
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterX - (f32BoxWidth * SAMPLE_SVP_NNIE_HALF));
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterY - (f32BoxHeight * SAMPLE_SVP_NNIE_HALF));
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterX + (f32BoxWidth * SAMPLE_SVP_NNIE_HALF));
                    ps32PriorboxOutputData[(*pu32Index)++] =
                        (HI_S32)(f32CenterY + (f32BoxHeight * SAMPLE_SVP_NNIE_HALF));
                }
            }
        }
    }
    return HI_SUCCESS;
}

static HI_S32 SAMPLE_Ssd_PriorBoxForward(HI_SAMPLE_SSD_PRIOR_BOX_S *pstSsdBoxParam,
    HI_FLOAT af32PriorBoxAspectRatio[], HI_S32 as32PriorBoxVar[4])
{
    HI_FLOAT af32AspectRatio[SAMPLE_SSD_ASPECT_RATIO_NUM] = {0};
    SAMPLE_CHK_RET((pstSsdBoxParam->bFlip && pstSsdBoxParam->u32InputAspectRatioNum >
                   (SAMPLE_SSD_ASPECT_RATIO_NUM - 1) / 2),
                   HI_FAILURE, "Error,when bFlip is true, u32InputAspectRatioNum(%d) can't be greater than %d!\n",
                   pstSsdBoxParam->u32InputAspectRatioNum, (SAMPLE_SSD_ASPECT_RATIO_NUM - 1) / 2);
    SAMPLE_CHK_RET((!pstSsdBoxParam->bFlip && pstSsdBoxParam->u32InputAspectRatioNum >
                   (SAMPLE_SSD_ASPECT_RATIO_NUM - 1)),
                   HI_FAILURE, "Error,when bFlip is false, u32InputAspectRatioNum(%d) can't be greater than %d!\n",
                   pstSsdBoxParam->u32InputAspectRatioNum, (SAMPLE_SSD_ASPECT_RATIO_NUM - 1));

    // generate aspect_ratios
    HI_U32 u32AspectRatioNum = 1;
    af32AspectRatio[0] = 1;
    for (HI_U32 i = 0; i < pstSsdBoxParam->u32InputAspectRatioNum; i++) {
        af32AspectRatio[u32AspectRatioNum++] = af32PriorBoxAspectRatio[i];
        if (pstSsdBoxParam->bFlip) {
            af32AspectRatio[u32AspectRatioNum++] = 1.0f / af32PriorBoxAspectRatio[i];
        }
    }
    HI_U32 u32NumPrior = pstSsdBoxParam->u32MinSizeNum * u32AspectRatioNum + pstSsdBoxParam->u32MaxSizeNum;

    HI_U32 u32Index = 0;
    GetPriorBox(&u32Index, pstSsdBoxParam->u32PriorBoxWidth, pstSsdBoxParam->u32PriorBoxHeight,
        pstSsdBoxParam->f32PriorBoxStepWidth, pstSsdBoxParam->f32PriorBoxStepHeight,
        pstSsdBoxParam->f32Offset, pstSsdBoxParam->u32MinSizeNum, pstSsdBoxParam->pf32PriorBoxMaxSize,
        pstSsdBoxParam->u32MaxSizeNum, pstSsdBoxParam->pf32PriorBoxMinSize,
        af32AspectRatio, u32AspectRatioNum, pstSsdBoxParam->ps32PriorboxOutputData);

    /************ clip the priors' coordidates, within [0, u32ImgWidth] & [0, u32ImgHeight] *************/
    if (pstSsdBoxParam->bClip) {
        for (HI_U32 i = 0; i < (HI_U32)(pstSsdBoxParam->u32PriorBoxWidth * pstSsdBoxParam->u32PriorBoxHeight *
            SVP_WK_COORDI_NUM * u32NumPrior / 2); i++) {
            pstSsdBoxParam->ps32PriorboxOutputData[2 * i] =
            min((HI_U32)max(pstSsdBoxParam->ps32PriorboxOutputData[2 * i], 0), pstSsdBoxParam->u32OriImWidth);
            pstSsdBoxParam->ps32PriorboxOutputData[2 * i + 1] =
            min((HI_U32)max(pstSsdBoxParam->ps32PriorboxOutputData[2 * i + 1], 0), pstSsdBoxParam->u32OriImHeight);
        }
    }
    /*********************** get var **********************/
    for (HI_U32 h = 0; h < pstSsdBoxParam->u32PriorBoxHeight; h++) {
        for (HI_U32 w = 0; w < pstSsdBoxParam->u32PriorBoxWidth; w++) {
            for (HI_U32 i = 0; i < u32NumPrior; i++) {
                for (HI_U32 j = 0; j < SVP_WK_COORDI_NUM; j++) {
                    pstSsdBoxParam->ps32PriorboxOutputData[u32Index++] = (HI_S32)as32PriorBoxVar[j];
                }
            }
        }
    }
    return HI_SUCCESS;
}

static HI_S32 SAMPLE_SSD_SoftMax(HI_S32 *ps32Src, HI_S32 s32ArraySize, HI_S32 *ps32Dst)
{
    /* define parameters */
    HI_S32 s32Max = 0;
    HI_S32 s32Sum = 0;
    HI_S32 i = 0;
    for (i = 0; i < s32ArraySize; ++i) {
        if (s32Max < ps32Src[i]) {
            s32Max = ps32Src[i];
        }
    }
    for (i = 0; i < s32ArraySize; ++i) {
        ps32Dst[i] = (HI_S32)(SVP_WK_QUANT_BASE * exp ((HI_FLOAT)(ps32Src[i] - s32Max) / SVP_WK_QUANT_BASE));
        s32Sum += ps32Dst[i];
    }
    for (i = 0; i < s32ArraySize; ++i) {
        ps32Dst[i] = (HI_S32)(((HI_FLOAT)ps32Dst[i] / (HI_FLOAT)s32Sum) * SVP_WK_QUANT_BASE);
    }
    return HI_SUCCESS;
}

static HI_S32 SAMPLE_Ssd_SoftmaxForward(HI_U32 u32SoftMaxInHeight,
                                        HI_U32 au32SoftMaxInChn[], HI_U32 u32ConcatNum, HI_U32 au32ConvStride[],
                                        HI_U32 u32SoftMaxOutWidth, HI_U32 u32SoftMaxOutHeight, HI_U32 u32SoftMaxOutChn,
                                        HI_S32 *aps32SoftMaxInputData[], HI_S32 *ps32SoftMaxOutputData)
{
    HI_S32 *ps32InputData = NULL;
    HI_S32 *ps32OutputTmp = NULL;
    HI_U32 u32OuterNum = 0;
    HI_U32 u32InnerNum = 0;
    HI_U32 u32InputChannel = 0;
    HI_U32 i = 0;
    HI_U32 u32ConcatCnt = 0;
    HI_S32 s32Ret = 0;
    HI_U32 u32Stride = 0;
    HI_U32 u32Skip = 0;
    HI_U32 u32Left = 0;
    ps32OutputTmp = ps32SoftMaxOutputData;
    for (u32ConcatCnt = 0; u32ConcatCnt < u32ConcatNum; u32ConcatCnt++) {
        ps32InputData = aps32SoftMaxInputData[u32ConcatCnt];
        u32Stride = au32ConvStride[u32ConcatCnt];
        u32InputChannel = au32SoftMaxInChn[u32ConcatCnt];
        u32OuterNum = u32InputChannel / u32SoftMaxInHeight;
        u32InnerNum = u32SoftMaxInHeight;
        u32Skip = u32Stride / u32InnerNum;
        u32Left = u32Stride % u32InnerNum;  // do softmax
        for (i = 0; i < u32OuterNum; i++) {
            s32Ret = SAMPLE_SSD_SoftMax(ps32InputData, (HI_S32)u32InnerNum, ps32OutputTmp);
            if ((i + 1) % u32Skip == 0) {
                ps32InputData += u32Left;
            }
            ps32InputData += u32InnerNum;
            ps32OutputTmp += u32InnerNum;
        }
    }
    return s32Ret;
}

static HI_VOID genPriorBox(HI_U32 *pu32SrcIdx,
    HI_S32 *ps32LocPreds,
    HI_U32 u32DetectInputChn,
    HI_S32 *ps32PriorBoxes,
    HI_S32 *ps32AllDecodeBoxes)
{
    /********** get loc predictions ************/
    HI_U32 u32NumPredsPerClass = u32DetectInputChn / SVP_WK_COORDI_NUM;
    /********** get Prior Bboxes ************/
    HI_S32 *ps32PriorVar = ps32PriorBoxes + u32NumPredsPerClass * SVP_WK_COORDI_NUM;
    for (HI_U32 j = 0; j < u32NumPredsPerClass; j++) {
        HI_FLOAT f32PriorWidth = (HI_FLOAT)(ps32PriorBoxes[j * SVP_WK_COORDI_NUM + X_MAX_IDX] -
            ps32PriorBoxes[j * SVP_WK_COORDI_NUM + X_MIN_IDX]);
        HI_FLOAT f32PriorHeight = (HI_FLOAT)(ps32PriorBoxes[j * SVP_WK_COORDI_NUM + Y_MAX_IDX] -
            ps32PriorBoxes[j * SVP_WK_COORDI_NUM + Y_MIN_IDX]);
        HI_FLOAT f32PriorCenterX = (ps32PriorBoxes[j * SVP_WK_COORDI_NUM + X_MAX_IDX]
                + ps32PriorBoxes[j * SVP_WK_COORDI_NUM + X_MIN_IDX]) *  SAMPLE_SVP_NNIE_HALF;
        HI_FLOAT f32PriorCenterY = (ps32PriorBoxes[j * SVP_WK_COORDI_NUM + Y_MAX_IDX]
                + ps32PriorBoxes[j * SVP_WK_COORDI_NUM + Y_MIN_IDX]) * SAMPLE_SVP_NNIE_HALF;

        HI_FLOAT f32DecodeBoxCenterX = ((HI_FLOAT)ps32PriorVar[j * SVP_WK_COORDI_NUM + X_MIN_IDX] / SVP_WK_QUANT_BASE) *
            ((HI_FLOAT)ps32LocPreds[j * SVP_WK_COORDI_NUM + X_MIN_IDX] / SVP_WK_QUANT_BASE) * f32PriorWidth +
            f32PriorCenterX;

        HI_FLOAT f32DecodeBoxCenterY = ((HI_FLOAT)ps32PriorVar[j * SVP_WK_COORDI_NUM + Y_MIN_IDX] / SVP_WK_QUANT_BASE) *
            ((HI_FLOAT)ps32LocPreds[j * SVP_WK_COORDI_NUM + Y_MIN_IDX] / SVP_WK_QUANT_BASE) * f32PriorHeight +
            f32PriorCenterY;

        HI_FLOAT f32DecodeBoxWidth = (HI_FLOAT)exp(
                ((HI_FLOAT)ps32PriorVar[j * SVP_WK_COORDI_NUM + X_MAX_IDX] / SVP_WK_QUANT_BASE) *
                ((HI_FLOAT)ps32LocPreds[j * SVP_WK_COORDI_NUM + X_MAX_IDX] / SVP_WK_QUANT_BASE)) * f32PriorWidth;

        HI_FLOAT f32DecodeBoxHeight = (HI_FLOAT)exp(
                ((HI_FLOAT)ps32PriorVar[j * SVP_WK_COORDI_NUM + Y_MAX_IDX] / SVP_WK_QUANT_BASE) *
                ((HI_FLOAT)ps32LocPreds[j * SVP_WK_COORDI_NUM + Y_MAX_IDX] / SVP_WK_QUANT_BASE)) * f32PriorHeight;

        ps32AllDecodeBoxes[(*pu32SrcIdx)++] = (HI_S32)
            (f32DecodeBoxCenterX - (f32DecodeBoxWidth * SAMPLE_SVP_NNIE_HALF));
        ps32AllDecodeBoxes[(*pu32SrcIdx)++] = (HI_S32)
            (f32DecodeBoxCenterY - (f32DecodeBoxHeight * SAMPLE_SVP_NNIE_HALF));
        ps32AllDecodeBoxes[(*pu32SrcIdx)++] = (HI_S32)
            (f32DecodeBoxCenterX + (f32DecodeBoxWidth * SAMPLE_SVP_NNIE_HALF));
        ps32AllDecodeBoxes[(*pu32SrcIdx)++] = (HI_S32)
            (f32DecodeBoxCenterY + (f32DecodeBoxHeight * SAMPLE_SVP_NNIE_HALF));
    }
}

static HI_VOID DoNms(HI_SAMPLE_SSD_DetOut_PARAM_S *pstDetParam, HI_U32 u32NmsThresh)
{
    HI_U32 u32AfterTopK = 0;
    HI_U32 u32AfterFilter = 0;
    for (HI_U32 i = 0; i < pstDetParam->u32ClassNum; i++) {
        for (HI_U32 j = 0; j < pstDetParam->u32PriorNum; j++) {
            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + X_MIN_IDX] =
                               pstDetParam->ps32AllDecodeBoxes[j * SVP_WK_COORDI_NUM + X_MIN_IDX];
            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + Y_MIN_IDX] =
                               pstDetParam->ps32AllDecodeBoxes[j * SVP_WK_COORDI_NUM + Y_MIN_IDX];
            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + X_MAX_IDX] =
                               pstDetParam->ps32AllDecodeBoxes[j * SVP_WK_COORDI_NUM + X_MAX_IDX];
            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + Y_MAX_IDX] =
                               pstDetParam->ps32AllDecodeBoxes[j * SVP_WK_COORDI_NUM + Y_MAX_IDX];
            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] =
                               pstDetParam->ps32ConfScores[j * pstDetParam->u32ClassNum + i];
            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] = 0;
        }
        NonRecursiveArgQuickSort(pstDetParam->ps32SingleProposal, 0,
            pstDetParam->u32PriorNum - 1, pstDetParam->pstStack, pstDetParam->u32TopK);
        u32AfterFilter = (pstDetParam->u32PriorNum < pstDetParam->u32TopK) ?
            pstDetParam->u32PriorNum : pstDetParam->u32TopK;
        NonMaxSuppression(pstDetParam->ps32SingleProposal, u32AfterFilter, u32NmsThresh);
        HI_U32 u32RoiOutCnt = 0;
        HI_S32 *ps32DstScore = (HI_S32 *)pstDetParam->ps32DstScoreSrc;
        HI_S32 *ps32DstBbox = (HI_S32 *)pstDetParam->ps32DstBboxSrc;
        pstDetParam->ps32ClassRoiNum = (HI_S32 *)pstDetParam->ps32RoiOutCntSrc;
        ps32DstScore += (HI_S32)u32AfterTopK;
        ps32DstBbox += (HI_S32)(u32AfterTopK * SVP_WK_COORDI_NUM);
        for (HI_U32 j = 0; j < pstDetParam->u32TopK; j++) {
            if ((pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] == 0) &&
                (pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] > (HI_S32)u32NmsThresh)) {
                ps32DstScore[u32RoiOutCnt] = pstDetParam->ps32SingleProposal[j * SAMPLE_SSD_PRIORBOX_NUM + SCORE_IDX];
                ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + X_MIN_IDX] =
                            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + X_MIN_IDX];
                ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + Y_MIN_IDX] =
                            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + Y_MIN_IDX];
                ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + X_MAX_IDX] =
                            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + X_MAX_IDX];
                ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + Y_MAX_IDX] =
                            pstDetParam->ps32SingleProposal[j * SVP_WK_PROPOSAL_WIDTH + Y_MAX_IDX];
                u32RoiOutCnt++;
            }
        }
        pstDetParam->ps32ClassRoiNum[i] = (HI_S32)u32RoiOutCnt;
        u32AfterTopK += u32RoiOutCnt;
    }
    pstDetParam->u32AfterTopK = u32AfterTopK;
    return;
}

static HI_VOID DoNms2(HI_SAMPLE_SSD_DetOut_PARAM_S *pstDetParam, HI_U32 u32KeepTopK, HI_S32 *ps32AfterTopK)
{
    HI_U32 u32KeepCnt = 0;
    if (pstDetParam->u32AfterTopK > u32KeepTopK) {
        SAMPLE_CHK_RET_VOID(pstDetParam->ps32ClassRoiNum == HI_NULL, "ps32ClassRoiNum is null");
        HI_U32 u32Offset = pstDetParam->ps32ClassRoiNum[0];
        for (HI_U32 i = 1; i < pstDetParam->u32ClassNum; i++) {
            HI_S32* ps32DstScore = (HI_S32 *)pstDetParam->ps32DstScoreSrc;
            HI_S32* ps32DstBbox = (HI_S32 *)pstDetParam->ps32DstBboxSrc;
            pstDetParam->ps32ClassRoiNum = (HI_S32 *)pstDetParam->ps32RoiOutCntSrc;
            ps32DstScore += (HI_S32)(u32Offset);
            ps32DstBbox += (HI_S32)(u32Offset * SVP_WK_COORDI_NUM);
            for (HI_S32 j = 0; j < pstDetParam->ps32ClassRoiNum[i]; j++) {
                // 0 1 2 3 is detection box coordinate index
                ps32AfterTopK[u32KeepCnt * SVP_WK_PROPOSAL_WIDTH + 0] = ps32DstBbox[j * SVP_WK_COORDI_NUM + 0];
                ps32AfterTopK[u32KeepCnt * SVP_WK_PROPOSAL_WIDTH + 1] = ps32DstBbox[j * SVP_WK_COORDI_NUM + 1];
                ps32AfterTopK[u32KeepCnt * SVP_WK_PROPOSAL_WIDTH + 2] = ps32DstBbox[j * SVP_WK_COORDI_NUM + 2];
                ps32AfterTopK[u32KeepCnt * SVP_WK_PROPOSAL_WIDTH + 3] = ps32DstBbox[j * SVP_WK_COORDI_NUM + 3];
                ps32AfterTopK[u32KeepCnt * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] = ps32DstScore[j];
                ps32AfterTopK[u32KeepCnt * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] = i;
                u32KeepCnt++;
            }
            u32Offset = u32Offset + pstDetParam->ps32ClassRoiNum[i];
        }
        HI_S32 s32High = (HI_S32)(u32KeepCnt) - 1;
        NonRecursiveArgQuickSort(ps32AfterTopK, 0, s32High, pstDetParam->pstStack, u32KeepCnt);

        u32Offset = pstDetParam->ps32ClassRoiNum[0];
        for (HI_U32 i = 1; i < pstDetParam->u32ClassNum; i++) {
            HI_U32 u32RoiOutCnt = 0;
            HI_S32* ps32DstScore = (HI_S32 *)pstDetParam->ps32DstScoreSrc;
            HI_S32* ps32DstBbox = (HI_S32 *)pstDetParam->ps32DstBboxSrc;
            pstDetParam->ps32ClassRoiNum = (HI_S32 *)pstDetParam->ps32RoiOutCntSrc;
            ps32DstScore += (HI_S32)(u32Offset);
            ps32DstBbox += (HI_S32)(u32Offset * SVP_WK_COORDI_NUM);
            for (HI_U32 j = 0; j < u32KeepTopK; j++) {
                // 0 1 2 3 is detection box coordinate index
                if (ps32AfterTopK[j * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] == i) {
                    ps32DstScore[u32RoiOutCnt] = ps32AfterTopK[j * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX];
                    ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + 0] = ps32AfterTopK[j * SVP_WK_PROPOSAL_WIDTH + 0];
                    ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + 1] = ps32AfterTopK[j * SVP_WK_PROPOSAL_WIDTH + 1];
                    ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + 2] = ps32AfterTopK[j * SVP_WK_PROPOSAL_WIDTH + 2];
                    ps32DstBbox[u32RoiOutCnt * SVP_WK_COORDI_NUM + 3] = ps32AfterTopK[j * SVP_WK_PROPOSAL_WIDTH + 3];
                    u32RoiOutCnt++;
                }
            }
            pstDetParam->ps32ClassRoiNum[i] = (HI_S32)u32RoiOutCnt;
            u32Offset += u32RoiOutCnt;
        }
    }
    return;
}

static HI_S32 SAMPLE_Ssd_DetectionOutForward(HI_SAMPLE_SSD_PARAM_S *pstSsdParam,
                                             HI_S32 *aps32AllLocPreds[], HI_S32 *aps32AllPriorBoxes[],
                                             HI_S32 *ps32ConfScores, HI_S32 *ps32AssistMemPool)
{
    /************* check input parameters ****************/
    /******** define variables **********/
    HI_S32 *ps32AllDecodeBoxes = NULL;
    HI_S32 *ps32SingleProposal = NULL;
    HI_S32 *ps32AfterTopK = NULL;
    NNIE_STACK_S *pstStack = NULL;
    HI_U32 u32PriorNum = 0;
    for (HI_U32 i = 0; i < pstSsdParam->u32ConcatNum; i++) {
        u32PriorNum += pstSsdParam->au32DetectInputChn[i] / SVP_WK_COORDI_NUM;
    }
    // prepare for Assist MemPool
    ps32AllDecodeBoxes = ps32AssistMemPool;
    ps32SingleProposal = ps32AllDecodeBoxes + u32PriorNum * SVP_WK_COORDI_NUM;
    ps32AfterTopK = ps32SingleProposal + SVP_WK_PROPOSAL_WIDTH * u32PriorNum;
    pstStack = (NNIE_STACK_S *)(ps32AfterTopK + u32PriorNum * SVP_WK_PROPOSAL_WIDTH);
    HI_U32 u32SrcIdx = 0;
    for (HI_U32 i = 0; i < pstSsdParam->u32ConcatNum; i++) {
        genPriorBox(&u32SrcIdx, aps32AllLocPreds[i], pstSsdParam->au32DetectInputChn[i],
            aps32AllPriorBoxes[i], ps32AllDecodeBoxes);
    }
    // do NMS for each class
    HI_SAMPLE_SSD_DetOut_PARAM_S stDetParam;
    stDetParam.u32PriorNum = u32PriorNum;
    stDetParam.u32ClassNum = pstSsdParam->u32ClassNum;
    stDetParam.ps32SingleProposal = ps32SingleProposal;
    stDetParam.ps32AllDecodeBoxes = ps32AllDecodeBoxes;
    stDetParam.ps32ConfScores = ps32ConfScores;
    stDetParam.u32TopK = pstSsdParam->u32TopK;
    stDetParam.ps32DstScoreSrc = (HI_S32 *)((uintptr_t)pstSsdParam->stDstScore.u64VirAddr);
    stDetParam.ps32DstBboxSrc = (HI_S32 *)((uintptr_t)pstSsdParam->stDstRoi.u64VirAddr);
    stDetParam.ps32RoiOutCntSrc = (HI_S32 *)((uintptr_t)pstSsdParam->stClassRoiNum.u64VirAddr);
    stDetParam.pstStack = pstStack;
    stDetParam.ps32ClassRoiNum = HI_NULL;
    stDetParam.u32AfterTopK = 0;
    DoNms(&stDetParam, pstSsdParam->u32NmsThresh);
    DoNms2(&stDetParam, pstSsdParam->u32KeepTopK, ps32AfterTopK);

    return HI_SUCCESS;
}

static HI_S32 SAMPLE_SSD_Detection_PrintResult(HI_RUNTIME_MEM_S *pstDstScore,
                                               HI_RUNTIME_MEM_S *pstDstRoi, HI_RUNTIME_MEM_S *pstClassRoiNum,
                                               HI_FLOAT f32PrintResultThresh,
                                               HI_S32 *ps32ResultROI, HI_U32 *pu32ResultROICnt)
{
    HI_U32 i = 0;
    HI_U32 j = 0;
    HI_U32 u32RoiNumBias = 0;
    HI_U32 u32ScoreBias = 0;
    HI_U32 u32BboxBias = 0;
    HI_FLOAT f32Score = 0.0f;
    HI_S32 *ps32Score = (HI_S32 *)((uintptr_t)pstDstScore->u64VirAddr);
    HI_S32 *ps32Roi = (HI_S32 *)((uintptr_t)pstDstRoi->u64VirAddr);
    HI_S32 *ps32ClassRoiNum = (HI_S32 *)((uintptr_t)pstClassRoiNum->u64VirAddr);
    HI_U32 u32ClassNum = 21;
    HI_S32 s32XMin = 0;
    HI_S32 s32YMin = 0;
    HI_S32 s32XMax = 0;
    HI_S32 s32YMax = 0;

    u32RoiNumBias += ps32ClassRoiNum[0];
    for (i = 1; i < u32ClassNum; i++) {
        u32ScoreBias = u32RoiNumBias;
        u32BboxBias = u32RoiNumBias * SVP_WK_COORDI_NUM;
        // if the confidence score greater than result threshold, the result will be printed
        if ((HI_FLOAT)ps32Score[u32ScoreBias] / SVP_WK_QUANT_BASE >=
            f32PrintResultThresh &&
            (ps32ClassRoiNum[i] != 0)) {
            printf("==== The %dth class box info====\n", i);
        }
        for (j = 0; j < (HI_U32)ps32ClassRoiNum[i]; j++) {
            f32Score = (HI_FLOAT)ps32Score[u32ScoreBias + j] / SVP_WK_QUANT_BASE;
            if (f32Score < f32PrintResultThresh) {
                break;
            }
            s32XMin = ps32Roi[u32BboxBias + j * SVP_WK_COORDI_NUM + X_MIN_IDX];
            s32YMin = ps32Roi[u32BboxBias + j * SVP_WK_COORDI_NUM + Y_MIN_IDX];
            s32XMax = ps32Roi[u32BboxBias + j * SVP_WK_COORDI_NUM + X_MAX_IDX];
            s32YMax = ps32Roi[u32BboxBias + j * SVP_WK_COORDI_NUM + Y_MAX_IDX];
            printf("%d %d %d %d %f\n", s32XMin, s32YMin, s32XMax, s32YMax, f32Score);

            memcpy(&ps32ResultROI[*pu32ResultROICnt * SVP_WK_PROPOSAL_WIDTH],
                   &ps32Roi[u32BboxBias + j * SVP_WK_COORDI_NUM], sizeof(HI_S32) * SVP_WK_COORDI_NUM);
            ps32ResultROI[*pu32ResultROICnt * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] = ps32Score[u32ScoreBias + j];
            ps32ResultROI[*pu32ResultROICnt * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] = RPN_SUPPRESS_FALSE;
            *pu32ResultROICnt = *pu32ResultROICnt + 1;
        }
        u32RoiNumBias += ps32ClassRoiNum[i];
    }
    return HI_SUCCESS;
}

static HI_S32 GetAllPriorBox(HI_SAMPLE_SSD_PARAM_S *pSsdParam, HI_S32 **ps32PriorboxOutputData)
{
    HI_SAMPLE_SSD_PRIOR_BOX_S pstSsdBoxParam;
    for (HI_U32 i = 0; i < SAMPLE_SSD_PRIORBOX_NUM; i++) {
        pstSsdBoxParam.u32PriorBoxWidth = pSsdParam->au32PriorBoxWidth[i];
        pstSsdBoxParam.u32PriorBoxHeight = pSsdParam->au32PriorBoxHeight[i];
        pstSsdBoxParam.u32OriImWidth = pSsdParam->u32OriImWidth;
        pstSsdBoxParam.u32OriImHeight = pSsdParam->u32OriImHeight;
        pstSsdBoxParam.pf32PriorBoxMinSize = pSsdParam->af32PriorBoxMinSize[i];
        pstSsdBoxParam.u32MinSizeNum = pSsdParam->u32MinSizeNum;
        pstSsdBoxParam.pf32PriorBoxMaxSize = pSsdParam->af32PriorBoxMaxSize[i];
        pstSsdBoxParam.u32MaxSizeNum = pSsdParam->u32MaxSizeNum;
        pstSsdBoxParam.bFlip = pSsdParam->bFlip;
        pstSsdBoxParam.bClip = pSsdParam->bClip;
        pstSsdBoxParam.u32InputAspectRatioNum = pSsdParam->au32InputAspectRatioNum[i];
        pstSsdBoxParam.f32PriorBoxStepWidth = pSsdParam->af32PriorBoxStepWidth[i];
        pstSsdBoxParam.f32PriorBoxStepHeight = pSsdParam->af32PriorBoxStepHeight[i];
        pstSsdBoxParam.f32Offset = pSsdParam->f32Offset;
        pstSsdBoxParam.ps32PriorboxOutputData = ps32PriorboxOutputData[i];
        HI_S32 s32Ret = SAMPLE_Ssd_PriorBoxForward(&pstSsdBoxParam, pSsdParam->af32PriorBoxAspectRatio[i],
            pSsdParam->as32PriorBoxVar);
        SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE,
            "Error,SAMPLE_Ssd_PriorBoxForward failed!\n");
    }
    return HI_SUCCESS;
}

static HI_S32 GetSsdBox(HI_RUNTIME_BLOB_S *pstDstBlob, HI_SAMPLE_SSD_PARAM_S *pSsdParam)
{
    HI_S32 *aps32PermuteResult[SAMPLE_SSD_REPORT_NODE_NUM];
    HI_S32 *aps32PriorboxOutputData[SAMPLE_SSD_PRIORBOX_NUM];
    HI_S32 *aps32SoftMaxInputData[SAMPLE_SSD_SOFTMAX_NUM];
    HI_S32 *aps32DetectionLocData[SAMPLE_SSD_SOFTMAX_NUM];
    HI_S32 *ps32SoftMaxOutputData = NULL;
    HI_S32 *ps32DetectionOutTmpBuf = NULL;
    HI_U32 u32Size = 0;
    /* get permut result */
    for (HI_U32 i = 0; i < SAMPLE_SSD_REPORT_NODE_NUM; i++) {
        aps32PermuteResult[i] = (HI_S32 *)((uintptr_t)pstDstBlob[i].u64VirAddr);
    }

    /* priorbox */
    aps32PriorboxOutputData[0] = (HI_S32 *)((uintptr_t)pSsdParam->stPriorBoxTmpBuf.u64VirAddr);
    for (HI_U32 i = 1; i < SAMPLE_SSD_PRIORBOX_NUM; i++) {
        u32Size = pSsdParam->au32PriorBoxHeight[i - 1] * pSsdParam->au32PriorBoxWidth[i - 1] *
            SVP_WK_COORDI_NUM * 2 * (pSsdParam->u32MaxSizeNum + pSsdParam->u32MinSizeNum +
            pSsdParam->au32InputAspectRatioNum[i - 1] * 2 * pSsdParam->u32MinSizeNum);
        aps32PriorboxOutputData[i] = aps32PriorboxOutputData[i - 1] + u32Size;
    }
    HI_S32 s32Ret = GetAllPriorBox(pSsdParam, aps32PriorboxOutputData);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "Error,GetAllPriorBox failed!\n");
    /* softmax */
    ps32SoftMaxOutputData = (HI_S32 *)((uintptr_t)pSsdParam->stSoftMaxTmpBuf.u64VirAddr);
    for (HI_U32 i = 0; i < SAMPLE_SSD_SOFTMAX_NUM; i++) {
        aps32SoftMaxInputData[i] = aps32PermuteResult[i * 2 + 1];
    }

    (void)SAMPLE_Ssd_SoftmaxForward(pSsdParam->u32SoftMaxInHeight,
        pSsdParam->au32SoftMaxInChn, pSsdParam->u32ConcatNum,
        pSsdParam->au32ConvStride, pSsdParam->u32SoftMaxOutWidth, pSsdParam->u32SoftMaxOutHeight,
        pSsdParam->u32SoftMaxOutChn, aps32SoftMaxInputData, ps32SoftMaxOutputData);

    /* detection */
    ps32DetectionOutTmpBuf = (HI_S32 *)((uintptr_t)pSsdParam->stGetResultTmpBuf.u64VirAddr);
    for (HI_U32 i = 0; i < SAMPLE_SSD_PRIORBOX_NUM; i++) {
        aps32DetectionLocData[i] = aps32PermuteResult[i * 2];
    }

    (void)SAMPLE_Ssd_DetectionOutForward(pSsdParam, aps32DetectionLocData, aps32PriorboxOutputData,
        ps32SoftMaxOutputData, ps32DetectionOutTmpBuf);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_Ssd_GetResult(HI_RUNTIME_BLOB_S *pstSrcBlob, HI_RUNTIME_BLOB_S *pstDstBlob,
                            HI_S32 *ps32ResultROI, HI_U32 *pu32ResultROICnt)
{
    HI_SAMPLE_SSD_PARAM_S ssdParam;
    HI_FLOAT f32PrintResultThresh = 0.8f;

    memset(&ssdParam, 0, sizeof(ssdParam));

    HI_S32 s32Ret = SAMPLE_Ssd_InitParam(pstSrcBlob, pstDstBlob, &ssdParam);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0,
                    "SAMPLE_Ssd_InitParam failed!\n");

    s32Ret = GetSsdBox(pstDstBlob, &ssdParam);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "Error,GetAllPriorBox failed!\n");

    (void)SAMPLE_SSD_Detection_PrintResult(&ssdParam.stDstScore, &ssdParam.stDstRoi, &ssdParam.stClassRoiNum,
        f32PrintResultThresh, ps32ResultROI, pu32ResultROICnt);

FAIL_0:
    SAMPLE_FreeMem(&ssdParam.stPriorBoxTmpBuf);
    return s32Ret;
}
