/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include <stdlib.h>
#include <string.h>
#include "hi_plugin.h"
#include "hi_runtime_comm.h"
#include "sample_log.h"
#include "sample_memory_ops.h"
#include "math.h"
#include "proposal_common.h"

/*********************************************************
Function: SoftMax
Description: Do softmax on a vector of length s32ArraySize
*********************************************************/
HI_S32 SoftMax(HI_S32 *ps32BgSrc, HI_S32 *ps32FgSrc, HI_S32 s32ArraySize)
{
    /* define parameters */
    HI_S32 s32BgSrc = *ps32BgSrc;
    HI_S32 s32FgSrc = *ps32FgSrc;
    HI_S32 s32Max = s32BgSrc > s32FgSrc ? s32BgSrc : s32FgSrc;

    HI_FLOAT f32BgExp = QuickExp(s32BgSrc - s32Max);
    HI_FLOAT f32FgExp = QuickExp(s32FgSrc - s32Max);
    HI_FLOAT f32Sum = f32BgExp + f32FgExp;

    *ps32BgSrc = (HI_S32)((f32BgExp / f32Sum) * SVP_WK_QUANT_BASE);
    *ps32FgSrc = (HI_S32)((f32FgExp / f32Sum) * SVP_WK_QUANT_BASE);

    return HI_SUCCESS;
}

HI_S32 SoftMax_N(HI_S32 *as32BgSrc, HI_S32 *as32FgSrc, HI_S32 s32ArraySize, HI_U32 u32Num)
{
    HI_S32 s32Ret = HI_FAILURE;
    for (HI_U32 i = 0; i < u32Num; i++) {
        s32Ret = SoftMax(&as32BgSrc[i], &as32FgSrc[i], s32ArraySize);
        SVP_FALSE_CHECK(s32Ret == HI_SUCCESS, HI_FAILURE);
    }
    return HI_SUCCESS;
}

static HI_U32 GetRFCNAssistMemSize(PROPOSAL_PARA_S *para)
{
    HI_U32 u32NumAnchors = (para->model_info.u32NumRatioAnchors) *
                           (para->model_info.u32NumScaleAnchors) *
                           (para->model_info.astReportNodeInfo[0].u32ConvMapNum) *
                           (para->model_info.astReportNodeInfo[0].u32ConvHeight);

    HI_U32 u32AnchorSize = u32NumAnchors * SVP_WK_COORDI_NUM * sizeof(HI_U32);
    HI_U32 u32BboxDeltaSize = u32AnchorSize;
    HI_U32 u32ProposalSize = u32NumAnchors * SVP_WK_PROPOSAL_WIDTH * sizeof(HI_U32);
    HI_U32 u32ScoresSize = u32NumAnchors * SVP_WK_SCORE_NUM * sizeof(HI_FLOAT);
    HI_U32 u32StackSize = MAX_STACK_DEPTH * sizeof(NNIE_STACK_S);
    HI_U32 u32TotalSize = u32BboxDeltaSize + u32ProposalSize + u32ScoresSize + u32StackSize;

    return u32TotalSize;
}

static HI_S32 NNIE_RPN_Softmax(HI_FLOAT *pf32Softmax, HI_S32 s32Src1, HI_S32 s32Src2)
{
    HI_FLOAT f32Src1 = 1.0f;
    HI_FLOAT f32Src2 = 1.0f;
    HI_FLOAT f32Sum = 1.0f;
    if (s32Src1 < s32Src2) {
        f32Src1 = (HI_FLOAT)QuickExp(s32Src1 - s32Src2);
        f32Sum += f32Src1;
    } else {
        f32Src2 = (HI_FLOAT)QuickExp(s32Src2 - s32Src1);
        f32Sum += f32Src2;
    }
    *(pf32Softmax) = f32Src1 / f32Sum;
    *(pf32Softmax + 1) = f32Src2 / f32Sum;
    return HI_SUCCESS;
}

static HI_S32 HI_MPI_SVP_NNIE_WK_CNN_FASTER_RPN_Ref(HI_S32 *ps32Src[],
                                                    PROPOSAL_PARA_S *pProposalParam,
                                                    HI_U32 *pu32MemPool,
                                                    HI_S32 *ps32ProposalResult,
                                                    HI_U32 *pu32NumRois)
{
    /******************** define parameters ****************/
    HI_S32 *ps32Anchors = NULL;
    HI_S32 *ps32BboxDelta = NULL;
    HI_S32 *ps32Proposals = NULL;
    HI_U32 *pu32Ptr = NULL;

    HI_U32 u32NumAfterFilter = 0;
    HI_FLOAT *pf32Ptr = NULL;
    HI_FLOAT *pf32Scores = NULL;

    NNIE_STACK_S *pstStack = NULL;

    NNIE_REPORT_NODE_INFO_S astReportNode[RPN_NODE_NUM];
    memset(astReportNode, 0, sizeof(NNIE_REPORT_NODE_INFO_S) * RPN_NODE_NUM);

    /******************** Get parameters from Model and Config ***********************/
    HI_U32 u32OriImHeight = pProposalParam->model_info.u32SrcHeight;
    HI_U32 u32OriImWidth = pProposalParam->model_info.u32SrcWidth;

    if (pProposalParam->model_info.enNetType != SVP_NNIE_NET_TYPE_ROI) {
        for (HI_U32 i = 0; i < RPN_NODE_NUM; i++) {
            astReportNode[i].u32ConvHeight = pProposalParam->model_info.astReportNodeInfo[i].u32ConvHeight;
            astReportNode[i].u32ConvWidth = pProposalParam->model_info.astReportNodeInfo[i].u32ConvWidth;
            astReportNode[i].u32ConvMapNum = pProposalParam->model_info.astReportNodeInfo[i].u32ConvMapNum;
            astReportNode[i].u32ConvStride = pProposalParam->model_info.astReportNodeInfo[i].u32ConvStride;
        }
    } else {
        for (HI_U32 i = 0; i < RPN_NODE_NUM - 1; i++) {
            astReportNode[i + 1].u32ConvHeight = pProposalParam->model_info.astReportNodeInfo[i].u32ConvHeight;
            astReportNode[i + 1].u32ConvWidth = pProposalParam->model_info.astReportNodeInfo[i].u32ConvWidth;
            astReportNode[i + 1].u32ConvMapNum = pProposalParam->model_info.astReportNodeInfo[i].u32ConvMapNum;
            astReportNode[i + 1].u32ConvStride = pProposalParam->model_info.astReportNodeInfo[i].u32ConvStride;
        }
    }

    HI_U32 u32MaxRois = pProposalParam->model_info.u32MaxRoiFrameCnt;

    /* ********************************* Faster RCNN ******************************************* */
    /* ******* calculate the start pointer of each part in MemPool ******* */
    /* base RatioAnchors and ScaleAnchors */
    HI_U32 u32NumAnchors = (pProposalParam->model_info.u32NumRatioAnchors) *
                           (pProposalParam->model_info.u32NumScaleAnchors) *
                           (astReportNode[1].u32ConvMapNum) * (astReportNode[1].u32ConvHeight);
    HI_U32 u32Size = SVP_WK_COORDI_NUM * u32NumAnchors;

    ps32Anchors = (HI_S32 *)((uintptr_t)(pProposalParam->stBachorMemInfo.u64VirAddr));
    pu32Ptr = (HI_U32 *)pu32MemPool;
    /* BboxDelta */
    ps32BboxDelta = (HI_S32 *)ps32Src[1];
    pu32Ptr += u32Size;

    /* Proposal info */
    ps32Proposals = (HI_S32 *)pu32Ptr;
    u32Size = SVP_WK_PROPOSAL_WIDTH * u32NumAnchors;
    pu32Ptr += u32Size;

    pf32Ptr = (HI_FLOAT *)pu32Ptr;
    /* Proposal scores */
    pf32Scores = pf32Ptr;
    u32Size = u32NumAnchors * SVP_WK_SCORE_NUM;
    pf32Ptr += u32Size;

    /* quick sort Stack */
    pstStack = (NNIE_STACK_S *)pf32Ptr;

    HI_U32 u32HalfScoreWidth = astReportNode[1].u32ConvWidth / 2;
    HI_U32 u32ScoreStrideWidth = astReportNode[1].u32ConvStride / 4;
    pf32Ptr = pf32Scores;
    HI_S32 *ps32Ptr2 = ps32Src[0];
    for (HI_U32 p = 0; p < astReportNode[1].u32ConvMapNum; p++) {
        for (HI_U32 q = 0; q < astReportNode[1].u32ConvHeight; q++) {
            for (HI_U32 z = 0; z < astReportNode[1].u32ConvWidth; z++) {
                NNIE_RPN_Softmax(pf32Ptr + 2 * z, ps32Ptr2[z], ps32Ptr2[u32HalfScoreWidth + z]);
            }
            ps32Ptr2 += u32ScoreStrideWidth;
            pf32Ptr += astReportNode[1].u32ConvWidth;
        }
    }

    /************************* BBox Transform *****************************/
    BBOX_PARAM_S bboxParam = { ps32Proposals, ps32Anchors, ps32BboxDelta, pf32Scores };
    HI_S32 s32Ret = BboxTransform_N(&bboxParam, u32NumAnchors, PROPOSAL_WITHOUT_PERMUTE);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "BboxTransform_N error, error code is %d", s32Ret);

    /************************ clip bbox *****************************/
    s32Ret = BboxClip_N(ps32Proposals, u32OriImWidth, u32OriImHeight, u32NumAnchors);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "BboxClip_N error, error code is %d", s32Ret);

    /************ remove the bboxes which are too small ***********/
    s32Ret = BboxSmallSizeFilter_N(ps32Proposals, pProposalParam->model_info.u32MinSize,
                                   pProposalParam->model_info.u32MinSize, u32NumAnchors);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "BboxSmallSizeFilter_N error, error code is %d", s32Ret);

    /********** remove low score bboxes ************/
    s32Ret = FilterLowScoreBbox(ps32Proposals, u32NumAnchors, pProposalParam->u32NmsThresh,
                                pProposalParam->u32FilterThresh, &u32NumAfterFilter);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "FilterLowScoreBbox error, error code is %d", s32Ret);

    /********** sort ***********/
    s32Ret = NonRecursiveArgQuickSort(ps32Proposals, 0, (HI_S32)u32NumAfterFilter - 1, pstStack,
                                      pProposalParam->u32NumBeforeNms);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "NonRecursiveArgQuickSort error, error code is %d", s32Ret);

    u32NumAfterFilter = SVP_MIN(u32NumAfterFilter, pProposalParam->u32NumBeforeNms);

    /* do nms to remove highly overlapped bbox */
    s32Ret = NonMaxSuppression(ps32Proposals, u32NumAfterFilter, pProposalParam->u32NmsThresh,
                               pProposalParam->model_info.u32MaxRoiFrameCnt);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "NonMaxSuppression error, error code is %d", s32Ret);

    /************** write the final result to output ***************/
    s32Ret = getRPNresult(ps32ProposalResult, pu32NumRois, u32MaxRois, ps32Proposals, u32NumAfterFilter);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "getRPNresult error, error code is %d", s32Ret);

    /******************** end of FasterRCNN RPN **********************/
    return s32Ret;
}

HI_S32 HI_NodePlugin_Compute(const HI_NodePlugin_Operand_S *pstInputs, HI_U32 u32InputNum,
                             HI_NodePlugin_Operand_S *pstOutputs,
                             HI_U32 u32Outputs, HI_NodePlugin_NodeParam_S *pstHyperParam,
                             HI_NodePlugin_NodeParam_S *pstTrainingParam)
{
    PROPOSAL_PARA_S para;
    HI_U32 u32RoisNum = 0;
    HI_S32 *pInputArray[OPERAND_INPUT_NUM];
    HI_NodePlugin_Operand_S pstTmpInputs[OPERAND_INPUT_NUM];
    HI_NodePlugin_Operand_S stTmp;
    HI_RUNTIME_MEM_S stMem;
    memset(&para, 0x0, sizeof(PROPOSAL_PARA_S));
    SAMPLE_CHK_RET(u32InputNum != OPERAND_INPUT_NUM, HI_FAILURE,
        "proposal inputs number error,the corrent number is %u\n", u32InputNum);
    memcpy(pstTmpInputs, pstInputs, sizeof(HI_NodePlugin_Operand_S) * u32InputNum);
    if (pstTmpInputs[0].stShape.s32C > pstTmpInputs[1].stShape.s32C) {
        memcpy(&stTmp, &pstTmpInputs[0], sizeof(HI_NodePlugin_Operand_S));
        memcpy(&pstTmpInputs[0], &pstTmpInputs[1], sizeof(HI_NodePlugin_Operand_S));
        memcpy(&pstTmpInputs[1], &stTmp, sizeof(HI_NodePlugin_Operand_S));
    }

    for (HI_U32 i = 0; i < u32InputNum; i++) {
        para.model_info.astReportNodeInfo[i].u32ConvHeight = pstTmpInputs[i].stShape.s32H;
        para.model_info.astReportNodeInfo[i].u32ConvMapNum = pstTmpInputs[i].stShape.s32C;
        para.model_info.astReportNodeInfo[i].u32ConvStride = pstTmpInputs[i].u32Stride;
        para.model_info.astReportNodeInfo[i].u32ConvWidth = pstTmpInputs[i].stShape.s32W;
    }

    GetParam(pstHyperParam->pParam, &para);
    HI_U32 assist_mem_size = GetRFCNAssistMemSize(&para);
    stMem.u32Size = assist_mem_size;
    HI_S32 s32Ret = SAMPLE_AllocMem(&stMem, HI_TRUE);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE,
        "SAMPLE_AllocMem error, return value :%d\n", s32Ret);

    s32Ret = SAMPLE_FlushCache(&stMem);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, COMPUTE_FLUSHCACHE_ERROR,
        "SAMPLE_FlushMem error, return value :%d\n", s32Ret);
    pInputArray[0] = (HI_S32 *)((uintptr_t)pstTmpInputs[0].u64Offset);
    pInputArray[1] = (HI_S32 *)((uintptr_t)pstTmpInputs[1].u64Offset);

    s32Ret = HI_MPI_SVP_NNIE_WK_CNN_FASTER_RPN_Ref(pInputArray,
                                                   &para,
                                                   (HI_U32 *)((uintptr_t)stMem.u64VirAddr),
                                                   (HI_S32 *)((uintptr_t)(pstOutputs[0].u64Offset)),
                                                   &u32RoisNum);

    SAMPLE_FreeMem(&stMem);
    pstOutputs[0].stShape.s32H = u32RoisNum;
    return s32Ret;
COMPUTE_FLUSHCACHE_ERROR:
    SAMPLE_FreeMem(&stMem);
    return HI_FAILURE;
}

HI_S32 HI_NodePlugin_getNodeType(HI_CHAR pszNodeType[])
{
    HI_U32 u32ProposalLength = (HI_U32)strlen("Proposal");
    strncpy(pszNodeType, "Proposal", u32ProposalLength);
    pszNodeType[u32ProposalLength] = '\0';
    return HI_SUCCESS;
}
