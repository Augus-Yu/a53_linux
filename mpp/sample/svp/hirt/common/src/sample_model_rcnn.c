/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_model_rcnn.h"
#include <stdint.h>
#include <math.h>
#include "sample_data_utils.h"
#include "sample_memory_ops.h"
#include "sample_log.h"

static HI_S32 SizeClip(HI_S32 s32inputSize, HI_S32 s32sizeMin, HI_S32 s32sizeMax)
{
    return max(min(s32inputSize, s32sizeMax), s32sizeMin);
}

static HI_S32 BboxClip(HI_S32 *ps32Proposals, HI_U32 u32ImageW, HI_U32 u32ImageH)
{
    ps32Proposals[X_MIN_IDX] = SizeClip(ps32Proposals[X_MIN_IDX], 0, (HI_S32)u32ImageW - 1);
    ps32Proposals[Y_MIN_IDX] = SizeClip(ps32Proposals[Y_MIN_IDX], 0, (HI_S32)u32ImageH - 1);
    ps32Proposals[X_MAX_IDX] = SizeClip(ps32Proposals[X_MAX_IDX], 0, (HI_S32)u32ImageW - 1);
    ps32Proposals[Y_MAX_IDX] = SizeClip(ps32Proposals[Y_MAX_IDX], 0, (HI_S32)u32ImageH - 1);

    return HI_SUCCESS;
}

static HI_S32 BboxClip_N(HI_S32 *ps32Proposals, HI_U32 u32ImageW, HI_U32 u32ImageH, HI_U32 u32Num)
{
    HI_S32 s32Ret = HI_FAILURE;

    for (HI_U32 i = 0; i < u32Num; i++) {
        s32Ret = BboxClip(&ps32Proposals[i * SVP_WK_PROPOSAL_WIDTH], u32ImageW, u32ImageH);
        SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "BboxClip fail");
    }

    return HI_SUCCESS;
}

typedef struct {
    SAMPLE_RUNTIME_MODEL_TYPE_E enType;
    HI_U32 u32RoiCnt;
    HI_U32 classType;
    HI_U32 dataWeight;
    HI_U32 dataHeight;
    HI_U32 u32BBoxStride;
    HI_U32 u32ScoreStride;
    NNIE_STACK_S *pstStack;
    HI_S32 *ps32Score;
    HI_S32 *ps32Proposal;
    HI_S32 *ps32BBox;
    HI_S32 *ps32AllBoxes;
} GetBoxParam;

static HI_S32 GetBBox(HI_U32 index, HI_S32 *ps32BBox, HI_U32 u32BBoxStride, HI_U32 classType,
    HI_FLOAT *fBBoxCenterXDelta, HI_FLOAT *fBBoxCenterYDelta, HI_FLOAT *fBBoxWidthDelta, HI_FLOAT *fBBoxHeightDelta)
{
    *fBBoxCenterXDelta = (HI_FLOAT)(ps32BBox[index * u32BBoxStride / sizeof(HI_S32)
        + classType * SVP_WK_COORDI_NUM + X_MIN_IDX] / SVP_WK_QUANT_BASE);
    *fBBoxCenterYDelta = (HI_FLOAT)(ps32BBox[index * u32BBoxStride / sizeof(HI_S32)
        + classType * SVP_WK_COORDI_NUM + Y_MIN_IDX] / SVP_WK_QUANT_BASE);
    *fBBoxWidthDelta = (HI_FLOAT)(ps32BBox[index * u32BBoxStride / sizeof(HI_S32)
        + classType * SVP_WK_COORDI_NUM + X_MAX_IDX] / SVP_WK_QUANT_BASE);
    *fBBoxHeightDelta = (HI_FLOAT)(ps32BBox[index * u32BBoxStride / sizeof(HI_S32)
        + classType * SVP_WK_COORDI_NUM + Y_MAX_IDX] / SVP_WK_QUANT_BASE);
    return HI_SUCCESS;
}

static HI_S32 GetTheBox(GetBoxParam *boxParam)
{
    HI_FLOAT fProposalXMin, fProposalXMax, fProposalYMin, fProposalYMax, fProposalWidth, fProposalHeight,
             fProposalCenterX, fProposalCenterY;
    HI_FLOAT fBBoxCenterXDelta, fBBoxCenterYDelta, fBBoxWidthDelta, fBBoxHeightDelta;
    HI_FLOAT fPredWidth, fPredHeight, fPredCenterX, fPredCenterY;
    HI_S32 *ps32AllBoxesTmp = HI_NULL;

    for (HI_U32 i = 0; i < boxParam->u32RoiCnt; i++) {
        fProposalXMin = (HI_FLOAT)(boxParam->ps32Proposal[i * SVP_WK_COORDI_NUM + X_MIN_IDX] / SVP_WK_QUANT_BASE);
        fProposalYMin = (HI_FLOAT)(boxParam->ps32Proposal[i * SVP_WK_COORDI_NUM + Y_MIN_IDX] / SVP_WK_QUANT_BASE);
        fProposalXMax = (HI_FLOAT)(boxParam->ps32Proposal[i * SVP_WK_COORDI_NUM + X_MAX_IDX] / SVP_WK_QUANT_BASE);
        fProposalYMax = (HI_FLOAT)(boxParam->ps32Proposal[i * SVP_WK_COORDI_NUM + Y_MAX_IDX] / SVP_WK_QUANT_BASE);

        if (boxParam->enType == SAMPLE_RUNTIME_MODEL_TYPE_RFCN) {
            GetBBox(i, boxParam->ps32BBox, boxParam->u32BBoxStride, 1, &fBBoxCenterXDelta,
                &fBBoxCenterYDelta, &fBBoxWidthDelta, &fBBoxHeightDelta);
        } else {
            GetBBox(i, boxParam->ps32BBox, boxParam->u32BBoxStride, boxParam->classType, &fBBoxCenterXDelta,
                &fBBoxCenterYDelta, &fBBoxWidthDelta, &fBBoxHeightDelta);
        }

        fProposalWidth = fProposalXMax - fProposalXMin + 1;
        fProposalHeight = fProposalYMax - fProposalYMin + 1;
        fProposalCenterX = (HI_FLOAT)(fProposalXMin + fProposalWidth / MIDDLE);
        fProposalCenterY = (HI_FLOAT)(fProposalYMin + fProposalHeight / MIDDLE);
        fPredWidth = (HI_FLOAT)(fProposalWidth * exp(fBBoxWidthDelta));
        fPredHeight = (HI_FLOAT)(fProposalHeight * exp(fBBoxHeightDelta));
        fPredCenterX = fProposalCenterX + fProposalWidth * fBBoxCenterXDelta;
        fPredCenterY = fProposalCenterY + fProposalHeight * fBBoxCenterYDelta;
        boxParam->ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + X_MIN_IDX] = (HI_S32)(fPredCenterX - fPredWidth / MIDDLE);
        boxParam->ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + Y_MIN_IDX] = (HI_S32)(fPredCenterY - fPredHeight / MIDDLE);
        boxParam->ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + X_MAX_IDX] = (HI_S32)(fPredCenterX + fPredWidth / MIDDLE);
        boxParam->ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + Y_MAX_IDX] = (HI_S32)(fPredCenterY + fPredHeight / MIDDLE);
        boxParam->ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] =
                        boxParam->ps32Score[i * boxParam->u32ScoreStride / sizeof(HI_S32) + boxParam->classType];
        boxParam->ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] = RPN_SUPPRESS_FALSE;  // RPN Suppress
    }

    BboxClip_N(boxParam->ps32AllBoxes, boxParam->dataWeight,
        boxParam->dataHeight, boxParam->u32RoiCnt);
    ps32AllBoxesTmp = boxParam->ps32AllBoxes;
    NonRecursiveArgQuickSort(ps32AllBoxesTmp, 0, boxParam->u32RoiCnt - 1, boxParam->pstStack, boxParam->u32RoiCnt);
    // 0.7 is for rfcn to use depends on your config
    NonMaxSuppression(ps32AllBoxesTmp, boxParam->u32RoiCnt, (HI_U32)(0.7 * SVP_WK_QUANT_BASE));
    return HI_SUCCESS;
}

HI_S32 SAMPLE_DATA_GetRoiResultFromOriginSize(SAMPLE_RUNTIME_MODEL_TYPE_E enType,
    SAMPLE_RUNTIME_ROI_PARAM_S *pstRoiParam, HI_S32 *ps32ResultROI, HI_U32 *pu32ResultROICnt)
{
    HI_U32 i, j;
    HI_U32 u32RoiCnt = pstRoiParam->pstProposalBlob->unShape.stWhc.u32Height;
    HI_U32 u32ClassNum = pstRoiParam->pstScoreBlob->unShape.stWhc.u32Width;
    HI_S32 *ps32AllBoxes = HI_NULL;
    NNIE_STACK_S *pstStack = HI_NULL;
    HI_U32 u32ResultBoxNum = 0;

    SAMPLE_CHK_GOTO(u32RoiCnt == 0, FAIL_0, "u32RoiCnt equals 0\n");

    ps32AllBoxes = (HI_S32 *)malloc(u32RoiCnt * SVP_WK_PROPOSAL_WIDTH * sizeof(HI_S32));
    SAMPLE_CHK_GOTO(ps32AllBoxes == HI_NULL, FAIL_0, "u32RoiCnt equals 0\n");

    pstStack = (NNIE_STACK_S *)malloc(sizeof(NNIE_STACK_S) * u32RoiCnt);
    SAMPLE_CHK_GOTO(pstStack == HI_NULL, FAIL_0, "u32RoiCnt equals 0\n");

    GetBoxParam boxParam = {enType, u32RoiCnt, 0, pstRoiParam->u32Width, pstRoiParam->u32Height,
        getAlignSize(ALIGN_32, pstRoiParam->pstBBoxBlob->unShape.stWhc.u32Width * sizeof(HI_S32)),
        pstRoiParam->pstScoreBlob->u32Stride, pstStack, (HI_S32 *)((uintptr_t)pstRoiParam->pstScoreBlob->u64VirAddr),
        (HI_S32 *)((uintptr_t)pstRoiParam->pstProposalBlob->u64VirAddr),
        (HI_S32 *)((uintptr_t)pstRoiParam->pstBBoxBlob->u64VirAddr), ps32AllBoxes};

    for (j = 0; j < u32ClassNum; j++) {
        boxParam.classType = j;
        GetTheBox(&boxParam);
        u32ResultBoxNum = 0;

        for (i = 0; i < u32RoiCnt; i++) {
            if ((ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] == RPN_SUPPRESS_FALSE) &&
                // 0.8 is for rfcn to use depends on your config
                (ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] > 0.8 * SVP_WK_QUANT_BASE)) {

                u32ResultBoxNum++;

                if (j != 0) {  // not background
                    memcpy(&ps32ResultROI[*pu32ResultROICnt * SVP_WK_PROPOSAL_WIDTH],
                           &ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH], sizeof(HI_S32) * SVP_WK_PROPOSAL_WIDTH);
                    *pu32ResultROICnt = *pu32ResultROICnt + 1;
                }
            }
        }

#if SAMPLE_DEBUG
        printf("class %d has %d boxes\n", j, u32ResultBoxNum);
#endif
    }

    SAMPLE_FREE(pstStack);
    SAMPLE_FREE(ps32AllBoxes);
    return HI_SUCCESS;
FAIL_0:
    SAMPLE_FREE(pstStack);
    SAMPLE_FREE(ps32AllBoxes);
    return HI_FAILURE;
}

HI_S32 SAMPLE_DATA_GetRoiResult(SAMPLE_RUNTIME_MODEL_TYPE_E enType, SAMPLE_RUNTIME_ROI_PARAM_S *pstRoiParam,
    HI_S32 as32ResultROI[MAX_ROI_NUM * SVP_WK_PROPOSAL_WIDTH], HI_U32 *pu32ResultROICnt)
{
    HI_U32 u32RoiCnt = pstRoiParam->pstProposalBlob->unShape.stWhc.u32Height;
    HI_U32 u32ClassNum = pstRoiParam->pstScoreBlob->unShape.stWhc.u32Width;
    HI_S32 *ps32AllBoxes = HI_NULL;
    NNIE_STACK_S *pstStack = HI_NULL;
    HI_U32 u32ResultBoxNum = 0;

    SAMPLE_CHK_GOTO(0 == u32RoiCnt, FAIL_0, "u32RoiCnt equals 0\n");

    ps32AllBoxes = (HI_S32 *)malloc(u32RoiCnt * SVP_WK_PROPOSAL_WIDTH * sizeof(HI_S32));
    SAMPLE_CHK_GOTO(ps32AllBoxes == HI_NULL, FAIL_0, "u32RoiCnt equals 0\n");

    pstStack = (NNIE_STACK_S *)malloc(sizeof(NNIE_STACK_S) * u32RoiCnt);
    SAMPLE_CHK_GOTO(pstStack == HI_NULL, FAIL_0, "u32RoiCnt equals 0\n");

    GetBoxParam boxParam = {enType, u32RoiCnt, 0, pstRoiParam->pstDataBlob->unShape.stWhc.u32Width,
        pstRoiParam->pstDataBlob->unShape.stWhc.u32Height,
        getAlignSize(ALIGN_32, pstRoiParam->pstBBoxBlob->unShape.stWhc.u32Width * sizeof(HI_S32)),
        pstRoiParam->pstScoreBlob->u32Stride, pstStack, (HI_S32 *)((uintptr_t)pstRoiParam->pstScoreBlob->u64VirAddr),
        (HI_S32 *)((uintptr_t)pstRoiParam->pstProposalBlob->u64VirAddr),
        (HI_S32 *)((uintptr_t)pstRoiParam->pstBBoxBlob->u64VirAddr), ps32AllBoxes};

    for (HI_U32 j = 0; j < u32ClassNum; j++) {
        boxParam.classType = j;
        GetTheBox(&boxParam);
        u32ResultBoxNum = 0;

        for (HI_U32 i = 0; i < u32RoiCnt; i++) {
            if ((ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + RPN_SUPRESS_IDX] == RPN_SUPPRESS_FALSE) &&
                // 0.3 is for rfcn to use depends on your config
                ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH + BBOX_SCORE_IDX] > 0.3 * SVP_WK_QUANT_BASE) {

                u32ResultBoxNum++;

                if (j != 0) {  // not background
                    memcpy(&as32ResultROI[*pu32ResultROICnt * SVP_WK_PROPOSAL_WIDTH],
                           &ps32AllBoxes[i * SVP_WK_PROPOSAL_WIDTH], sizeof(HI_S32) * SVP_WK_PROPOSAL_WIDTH);
                    *pu32ResultROICnt = *pu32ResultROICnt + 1;
                }
            }
        }
#if SAMPLE_DEBUG
        printf("class %d has %d boxes\n", j, u32ResultBoxNum);
#endif
    }

    SAMPLE_FREE(pstStack);
    SAMPLE_FREE(ps32AllBoxes);
    return HI_SUCCESS;
FAIL_0:
    SAMPLE_FREE(pstStack);
    SAMPLE_FREE(ps32AllBoxes);
    return HI_FAILURE;
}
