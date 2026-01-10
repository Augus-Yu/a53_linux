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
static HI_S32 SoftMax(HI_FLOAT *af32Src, HI_S32 s32ArraySize)
{
    /* define parameters */
    HI_FLOAT f32Max = 0;
    HI_FLOAT f32Sum = 0;
    HI_S32 i = 0;

    for (i = 0; i < s32ArraySize; ++i) {
        if (f32Max < af32Src[i]) {
            f32Max = af32Src[i];
        }
    }

    for (i = 0; i < s32ArraySize; ++i) {
        af32Src[i] = QuickExp((HI_S32)((af32Src[i] - f32Max) * SVP_WK_QUANT_BASE));
        f32Sum += af32Src[i];
    }

    for (i = 0; i < s32ArraySize; ++i) {
        af32Src[i] /= f32Sum;
    }

    return HI_SUCCESS;
}

static HI_S32 SoftMax_N(HI_FLOAT *af32Src, HI_S32 s32ArraySize, HI_U32 u32Num)
{
    HI_S32 s32Ret = HI_FAILURE;
    for (HI_U32 i = 0; i < u32Num; i++) {
        s32Ret = SoftMax(&af32Src[i * SVP_WK_SCORE_NUM], s32ArraySize);
        SVP_FALSE_CHECK(s32Ret == HI_SUCCESS, HI_FAILURE);
    }
    return HI_SUCCESS;
}

static HI_U32 GetRFCNAssistMemSize(PROPOSAL_PARA_S *para)
{
    HI_U32 u32NumAnchors = (para->model_info.u32NumRatioAnchors) *
                           (para->model_info.u32NumScaleAnchors) *
                           (para->model_info.astReportNodeInfo[0].u32ConvHeight) *
                           (para->model_info.astReportNodeInfo[0].u32ConvWidth);

    HI_U32 u32AnchorSize = u32NumAnchors * SVP_WK_COORDI_NUM * sizeof(HI_U32);
    HI_U32 u32BboxDeltaSize = u32AnchorSize;
    HI_U32 u32ProposalSize = u32NumAnchors * SVP_WK_PROPOSAL_WIDTH * sizeof(HI_U32);
    HI_U32 u32ScoresSize = u32NumAnchors * SVP_WK_SCORE_NUM * sizeof(HI_FLOAT);
    HI_U32 u32StackSize = MAX_STACK_DEPTH * sizeof(NNIE_STACK_S);
    HI_U32 u32TotalSize = u32BboxDeltaSize + u32ProposalSize + u32ScoresSize + u32StackSize;

    return u32TotalSize;
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

    HI_U32 u32SrcBboxIndex = 0;
    HI_U32 u32SrcFgProbIndex = 0;
    HI_U32 u32SrcBgProbIndex = 0;

    HI_U32 u32DesBox = 0;

    HI_U32 u32DesBboxDeltaIndex = 0;
    HI_U32 u32DesScoreIndex = 0;

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
                           (astReportNode[1].u32ConvHeight) * (astReportNode[1].u32ConvWidth);
    HI_U32 u32Size = SVP_WK_COORDI_NUM * u32NumAnchors;

    ps32Anchors = (HI_S32 *)((uintptr_t)(pProposalParam->stBachorMemInfo.u64VirAddr));
    pu32Ptr = (HI_U32 *)pu32MemPool;
    /* BboxDelta */
    ps32BboxDelta = (HI_S32 *)pu32Ptr;
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

    /* ******** do transpose, convert the blob from (M,C,H,W) to (M,H,W,C) ******** */
    HI_U32 u32MapSize = (astReportNode[2].u32ConvHeight) * (astReportNode[2].u32ConvStride / sizeof(HI_U32));
    HI_U32 u32AnchorsPerPixel = pProposalParam->model_info.u32NumRatioAnchors *
                                pProposalParam->model_info.u32NumScaleAnchors;
    HI_U32 u32BgBlobSize = u32AnchorsPerPixel * u32MapSize;
    HI_U32 u32LineSize = (astReportNode[2].u32ConvStride) / sizeof(HI_U32);

    for (HI_U32 c = 0; c < astReportNode[2].u32ConvMapNum; c++) {
        for (HI_U32 h = 0; h < astReportNode[2].u32ConvHeight; h++) {
            for (HI_U32 w = 0; w < astReportNode[2].u32ConvWidth; w++) {
                u32SrcBboxIndex = c * u32MapSize + h * u32LineSize + w;
                u32SrcBgProbIndex = (c / SVP_WK_COORDI_NUM) * u32MapSize + h * u32LineSize + w;
                u32SrcFgProbIndex = u32BgBlobSize + u32SrcBgProbIndex;

                u32DesBox = (u32AnchorsPerPixel) * (h * astReportNode[2].u32ConvWidth + w) + (c / SVP_WK_COORDI_NUM);

                u32DesBboxDeltaIndex = SVP_WK_COORDI_NUM * u32DesBox + (c % SVP_WK_COORDI_NUM);
                ps32BboxDelta[u32DesBboxDeltaIndex] = ps32Src[1][u32SrcBboxIndex];

                u32DesScoreIndex = SVP_WK_SCORE_NUM * u32DesBox;
                pf32Scores[u32DesScoreIndex + 0] = (HI_FLOAT)ps32Src[0][u32SrcBgProbIndex] / SVP_WK_QUANT_BASE;
                pf32Scores[u32DesScoreIndex + 1] = (HI_FLOAT)ps32Src[0][u32SrcFgProbIndex] / SVP_WK_QUANT_BASE;
            }
        }
    }

    /************************* do softmax ****************************/
    HI_S32 s32Ret = SoftMax_N(pf32Scores, SVP_WK_SCORE_NUM, u32NumAnchors);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "softmax error, error code is %d", s32Ret);

    /************************* BBox Transform *****************************/
    BBOX_PARAM_S bboxParam = { ps32Proposals, ps32Anchors, ps32BboxDelta, pf32Scores };
    s32Ret = BboxTransform_N(&bboxParam, u32NumAnchors, PROPOSAL_WITH_PERMUTE);
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
