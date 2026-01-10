/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_cop_param.h"
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "sample_log.h"
#include "hi_runtime_comm.h"
#include "sample_data_utils.h"
#include "sample_memory_ops.h"

enum {
    SVP_NNIE_MAX_RATIO_ANCHOR_NUM = 32 /* NNIE max ratio anchor num */
};

#define SAFE_ROUND(val)     (double)(((double)(val) > 0) ? floor((double)(val)+0.5) : ceil((double)(val)-0.5))

typedef struct hiSAMPLE_ANCHOR_PARAM_S{
    HI_FLOAT *pf32RatioAnchors;
    HI_U32 *pu32Ratios;
    HI_U32 u32NumRatioAnchors;
    HI_FLOAT *pf32ScaleAnchors;
    HI_U32 *pu32Scales;
    HI_U32 u32NumScaleAnchors;
    HI_U32 *au32BaseAnchor;
} HI_SAMPLE_ANCHOR_PARAM_S;

static HI_S32 GenBaseAnchor(HI_SAMPLE_ANCHOR_PARAM_S *pstAnchorParam)
{
    /********************* Generate the base anchor ***********************/
    HI_FLOAT f32BaseW =
        (HI_FLOAT)(pstAnchorParam->au32BaseAnchor[X_MAX_IDX] - pstAnchorParam->au32BaseAnchor[X_MIN_IDX] + 1);
    HI_FLOAT f32BaseH =
        (HI_FLOAT)(pstAnchorParam->au32BaseAnchor[Y_MAX_IDX] - pstAnchorParam->au32BaseAnchor[Y_MIN_IDX] + 1);
    HI_FLOAT f32BaseXCtr = (HI_FLOAT)(pstAnchorParam->au32BaseAnchor[X_MIN_IDX] + ((f32BaseW - 1) / MIDDLE));
    HI_FLOAT f32BaseYCtr = (HI_FLOAT)(pstAnchorParam->au32BaseAnchor[Y_MIN_IDX] + ((f32BaseH - 1) / MIDDLE));
    /*************** Generate Ratio Anchors for the base anchor ***********/
    HI_FLOAT f32Ratios = 0.0f;
    HI_FLOAT f32SizeRatios = 0.0f;
    HI_FLOAT f32Size = f32BaseW * f32BaseH;

    for (HI_U32 i = 0; i < pstAnchorParam->u32NumRatioAnchors; i++) {
        f32Ratios = (HI_FLOAT)pstAnchorParam->pu32Ratios[i] / SVP_WK_QUANT_BASE;
        f32SizeRatios = f32Size / f32Ratios;
        f32BaseW = (HI_FLOAT)SAFE_ROUND(sqrt(f32SizeRatios));
        f32BaseH = (HI_FLOAT)SAFE_ROUND(f32BaseW * f32Ratios);
        pstAnchorParam->pf32RatioAnchors[i * SVP_WK_COORDI_NUM + X_MIN_IDX] = f32BaseXCtr - (f32BaseW - 1) / MIDDLE;
        pstAnchorParam->pf32RatioAnchors[i * SVP_WK_COORDI_NUM + Y_MIN_IDX] = f32BaseYCtr - (f32BaseH - 1) / MIDDLE;
        pstAnchorParam->pf32RatioAnchors[i * SVP_WK_COORDI_NUM + X_MAX_IDX] = f32BaseXCtr + (f32BaseW - 1) / MIDDLE;
        pstAnchorParam->pf32RatioAnchors[i * SVP_WK_COORDI_NUM + Y_MAX_IDX] = f32BaseYCtr + (f32BaseH - 1) / MIDDLE;
    }

    /********* Generate Scale Anchors for each Ratio Anchor **********/
    /* Generate Scale Anchors for one pixel */
    HI_FLOAT f32Scales = 0.0f;

    for (HI_U32 i = 0; i < pstAnchorParam->u32NumRatioAnchors; i++) {
        for (HI_U32 j = 0; j < pstAnchorParam->u32NumScaleAnchors; j++) {
            f32BaseW = pstAnchorParam->pf32RatioAnchors[X_MAX_IDX] - pstAnchorParam->pf32RatioAnchors[X_MIN_IDX] + 1;
            f32BaseH = pstAnchorParam->pf32RatioAnchors[Y_MAX_IDX] - pstAnchorParam->pf32RatioAnchors[Y_MIN_IDX] + 1;
            f32BaseXCtr = pstAnchorParam->pf32RatioAnchors[X_MIN_IDX] + (f32BaseW - 1) / MIDDLE;
            f32BaseYCtr = pstAnchorParam->pf32RatioAnchors[Y_MIN_IDX] + (f32BaseH - 1) / MIDDLE;
            f32Scales = (HI_FLOAT)pstAnchorParam->pu32Scales[j] / SVP_WK_QUANT_BASE;
            pstAnchorParam->pf32ScaleAnchors[X_MIN_IDX] = f32BaseXCtr - (f32BaseW * f32Scales - 1) / MIDDLE;
            pstAnchorParam->pf32ScaleAnchors[Y_MIN_IDX] = f32BaseYCtr - (f32BaseH * f32Scales - 1) / MIDDLE;
            pstAnchorParam->pf32ScaleAnchors[X_MAX_IDX] = f32BaseXCtr + (f32BaseW * f32Scales - 1) / MIDDLE;
            pstAnchorParam->pf32ScaleAnchors[Y_MAX_IDX] = f32BaseYCtr + (f32BaseH * f32Scales - 1) / MIDDLE;
            pstAnchorParam->pf32ScaleAnchors += SVP_WK_COORDI_NUM;
        }

        pstAnchorParam->pf32RatioAnchors += SVP_WK_COORDI_NUM;
    }

    return HI_SUCCESS;
}

static HI_S32 SetAnchorInPixel(HI_S32 *ps32Anchors, const HI_FLOAT *pf32ScaleAnchors, HI_U32 u32ConvHeight,
                               HI_U32 u32ConvWidth, HI_U32 u32NumAnchorPerPixel, HI_U32 u32SpatialScale)
{
    HI_U32 u32anchorCentorX = 0;
    HI_U32 u32anchorCentorY = 0;
    HI_U32 u32ScaleAnchorIndexBase = 0;
    /******************* Copy the anchors to every pixel in the feature map ******************/
    HI_FLOAT f32PixelInterval = SVP_WK_QUANT_BASE / (HI_FLOAT)u32SpatialScale;

    for (HI_U32 h = 0; h < u32ConvHeight; h++) {
        for (HI_U32 w = 0; w < u32ConvWidth; w++) {
            u32anchorCentorX = (HI_U32)(w * f32PixelInterval);
            u32anchorCentorY = (HI_U32)(h * f32PixelInterval);

            for (HI_U32 n = 0; n < u32NumAnchorPerPixel; n++) {
                u32ScaleAnchorIndexBase = n * SVP_WK_COORDI_NUM;
                ps32Anchors[X_MIN_IDX] =
                    (HI_S32)(u32anchorCentorX + pf32ScaleAnchors[u32ScaleAnchorIndexBase + X_MIN_IDX]);
                ps32Anchors[Y_MIN_IDX] =
                    (HI_S32)(u32anchorCentorY + pf32ScaleAnchors[u32ScaleAnchorIndexBase + Y_MIN_IDX]);
                ps32Anchors[X_MAX_IDX] =
                    (HI_S32)(u32anchorCentorX + pf32ScaleAnchors[u32ScaleAnchorIndexBase + X_MAX_IDX]);
                ps32Anchors[Y_MAX_IDX] =
                    (HI_S32)(u32anchorCentorY + pf32ScaleAnchors[u32ScaleAnchorIndexBase + Y_MAX_IDX]);
                ps32Anchors += SVP_WK_COORDI_NUM;
            }
        }
    }

    return HI_SUCCESS;
}

static HI_S32 SetRatioScale(SAMPLE_WK_DETECT_NET_FASTER_RCNN_TYPE_E enNetType,
                            HI_NODEPlugin_Param_S *pBaseAnchorInfo, HI_U32 *au32RatioScaleHW)
{
    /*
    * au32RatioScaleHW value reference net dot Graph input size
    * Anchors number value reference net algorithm
    */
    switch (enNetType) {
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_VGG16:
            au32RatioScaleHW[X_MIN_IDX] = 24;
            au32RatioScaleHW[Y_MIN_IDX] = 78;
            pBaseAnchorInfo->u32NumRatioAnchors = 3;
            pBaseAnchorInfo->u32NumScaleAnchors = 3;
            break;
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_PVANET:
            au32RatioScaleHW[X_MIN_IDX] = 14;
            au32RatioScaleHW[Y_MIN_IDX] = 14;
            pBaseAnchorInfo->u32NumRatioAnchors = 7;
            pBaseAnchorInfo->u32NumScaleAnchors = 6;
            break;
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_ALEX:
            au32RatioScaleHW[X_MIN_IDX] = 23;
            au32RatioScaleHW[Y_MIN_IDX] = 77;
            pBaseAnchorInfo->u32NumRatioAnchors = 1;
            pBaseAnchorInfo->u32NumScaleAnchors = 9;
            break;
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_RES18:
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_RES34:
            au32RatioScaleHW[X_MIN_IDX] = 24;
            au32RatioScaleHW[Y_MIN_IDX] = 78;
            pBaseAnchorInfo->u32NumRatioAnchors = 1;
            pBaseAnchorInfo->u32NumScaleAnchors = 9;
            break;
        case SAMPLE_WK_DETECT_NET_RFCN:
            au32RatioScaleHW[X_MIN_IDX] = 38;
            au32RatioScaleHW[Y_MIN_IDX] = 50;
            pBaseAnchorInfo->u32NumRatioAnchors = 3;
            pBaseAnchorInfo->u32NumScaleAnchors = 3;
            break;
        default:
            printf("Function SetRatioScale input enNetType[%d] error\n", enNetType);
            return HI_FAILURE;
    }
    return HI_SUCCESS;
}

static HI_S32 SetBaseAnchorInfoForNet(HI_NODEPlugin_Param_S *pBaseAnchorInfo,
    SAMPLE_WK_DETECT_NET_FASTER_RCNN_TYPE_E enNetType)
{
    /*
    * pBaseAnchorInfo reference net algorithm
    */
    switch (enNetType) {
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_VGG16:
        case SAMPLE_WK_DETECT_NET_RFCN:
            pBaseAnchorInfo->pfRatio[0] = (HI_FLOAT)0.5;
            pBaseAnchorInfo->pfRatio[1] = (HI_FLOAT)1.0;
            pBaseAnchorInfo->pfRatio[2] = (HI_FLOAT)2.0;

            pBaseAnchorInfo->pfScales[0] = (HI_FLOAT)8;
            pBaseAnchorInfo->pfScales[1] = (HI_FLOAT)16;
            pBaseAnchorInfo->pfScales[2] = (HI_FLOAT)32;
            break;

        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_PVANET:
            pBaseAnchorInfo->u32MaxRoiFrameCnt = 200;
            pBaseAnchorInfo->u32NumBeforeNms = 12000;
            pBaseAnchorInfo->pfRatio[0] = (HI_FLOAT)0.333;
            pBaseAnchorInfo->pfRatio[1] = (HI_FLOAT)0.5;
            pBaseAnchorInfo->pfRatio[2] = (HI_FLOAT)0.667;
            pBaseAnchorInfo->pfRatio[3] = (HI_FLOAT)1;
            pBaseAnchorInfo->pfRatio[4] = (HI_FLOAT)1.5;
            pBaseAnchorInfo->pfRatio[5] = (HI_FLOAT)2;
            pBaseAnchorInfo->pfRatio[6] = (HI_FLOAT)3;

            pBaseAnchorInfo->pfScales[0] = (HI_FLOAT)2;
            pBaseAnchorInfo->pfScales[1] = (HI_FLOAT)3;
            pBaseAnchorInfo->pfScales[2] = (HI_FLOAT)5;
            pBaseAnchorInfo->pfScales[3] = (HI_FLOAT)9;
            pBaseAnchorInfo->pfScales[4] = (HI_FLOAT)16;
            pBaseAnchorInfo->pfScales[5] = (HI_FLOAT)32;
            break;
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_ALEX:
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_RES18:
        case SAMPLE_WK_DETECT_NET_FASTER_RCNN_RES34:
            pBaseAnchorInfo->pfRatio[0] = (HI_FLOAT)2.44;

            pBaseAnchorInfo->pfScales[0] = (HI_FLOAT)1.5;
            pBaseAnchorInfo->pfScales[1] = (HI_FLOAT)2.1;
            pBaseAnchorInfo->pfScales[2] = (HI_FLOAT)2.9;
            pBaseAnchorInfo->pfScales[3] = (HI_FLOAT)4.1;
            pBaseAnchorInfo->pfScales[4] = (HI_FLOAT)5.8;
            pBaseAnchorInfo->pfScales[5] = (HI_FLOAT)8;
            pBaseAnchorInfo->pfScales[6] = (HI_FLOAT)11.3;
            pBaseAnchorInfo->pfScales[7] = (HI_FLOAT)15.8;
            pBaseAnchorInfo->pfScales[8] = (HI_FLOAT)22.1;
            break;
        default:
            printf("Function SampleFasterRCNNAnchorInfoInit enNetType[%d] error\n", enNetType);
            return HI_FAILURE;
    }
    return HI_SUCCESS;
}

static HI_S32 SampleFasterRCNNAnchorInfoInit(SAMPLE_WK_DETECT_NET_FASTER_RCNN_TYPE_E enNetType,
                                             HI_NODEPlugin_Param_S *pBaseAnchorInfo,
                                             HI_U32 *au32RatioScaleHW)
{
    SAMPLE_CHK_RET(pBaseAnchorInfo == HI_NULL, HI_FAILURE,
                   "SampleFasterRCNNAnchorInfoInit input baseAnchorInfo nullptr");
    SAMPLE_CHK_RET((enNetType < SAMPLE_WK_DETECT_NET_FASTER_RCNN_ALEX) ||
                   (enNetType >= SAMPLE_WK_DETECT_NET_FASTER_RCNN_TYPE_BUTT), HI_FAILURE,
                   "SampleFasterRCNNAnchorInfoInit enNetType(%d) out of range", enNetType);
    SAMPLE_CHK_RET((au32RatioScaleHW == NULL), HI_FAILURE,
        "SampleFasterRCNNAnchorInfoInit input au32RatioScaleHW nullptr");

    HI_S32 s32Ret = SetRatioScale(enNetType, pBaseAnchorInfo, au32RatioScaleHW);
    SAMPLE_CHK_RET((s32Ret == HI_FAILURE), HI_FAILURE, "set RatioHW ScaleHW error\n");
    pBaseAnchorInfo->pfRatio = (HI_FLOAT *)malloc(pBaseAnchorInfo->u32NumRatioAnchors * sizeof(HI_FLOAT));
    SAMPLE_CHK_RET((pBaseAnchorInfo->pfRatio == NULL), HI_FAILURE, "pfRatio malloc error\n");
    pBaseAnchorInfo->pfScales = (HI_FLOAT *)malloc(pBaseAnchorInfo->u32NumScaleAnchors * sizeof(HI_FLOAT));
    SAMPLE_CHK_GOTO((pBaseAnchorInfo->pfScales == NULL), MALLOC_ERROR, "pfScale malloc error\n");

    /*
    * pBaseAnchorInfo value reference FasterRCNN algorithm
    */
    pBaseAnchorInfo->u32SrcWidth = FASTERRCNN_IMAGE_WIDTH;
    pBaseAnchorInfo->u32SrcHeight = FASTERRCNN_IMAGE_HIGH;
    pBaseAnchorInfo->enNetType = SVP_NNIE_NET_TYPE_ROI;
    pBaseAnchorInfo->u32MaxRoiFrameCnt = MAX_ROI_NUM;
    pBaseAnchorInfo->u32MinSize = 16;
    pBaseAnchorInfo->fSpatialScale = 0.0625;
    pBaseAnchorInfo->fNmsThresh = (HI_FLOAT)0.7;
    pBaseAnchorInfo->fFilterThresh = 0;
    pBaseAnchorInfo->u32NumBeforeNms = 6000;
    pBaseAnchorInfo->fConfThresh = (HI_FLOAT)0.0003;
    pBaseAnchorInfo->fValidNmsThresh = (HI_FLOAT)0.3;
    pBaseAnchorInfo->u32ClassSize = 2;
    s32Ret = SetBaseAnchorInfoForNet(pBaseAnchorInfo, enNetType);
    SAMPLE_CHK_GOTO((s32Ret != HI_SUCCESS), MALLOC_ERROR, "enNetType error\n");

    if (enNetType == SAMPLE_WK_DETECT_NET_RFCN) {
        pBaseAnchorInfo->u32SrcWidth = RFCN_IMAGE_WIDTH;
        pBaseAnchorInfo->u32SrcHeight = RFCN_IMAGE_HEIGHT;
        pBaseAnchorInfo->fConfThresh = (HI_FLOAT)0.3;
    }
    return HI_SUCCESS;

MALLOC_ERROR:
    SAMPLE_FREE(pBaseAnchorInfo->pfRatio);
    return HI_FAILURE;
}

static HI_S32 AllocAnchorMemory(HI_RUNTIME_MEM_S *pstAnchorMem, HI_PROPOSAL_Param_S *pPropsalPram,
    HI_U32 *au32RatioScaleHW)
{
    HI_U32 u32AnchorMemSize = pPropsalPram->stNodePluginParam.u32NumRatioAnchors *
        pPropsalPram->stNodePluginParam.u32NumScaleAnchors * au32RatioScaleHW[0] * au32RatioScaleHW[1] *
        SVP_WK_COORDI_NUM * sizeof(HI_U32) + pPropsalPram->stNodePluginParam.u32NumRatioAnchors * SVP_WK_COORDI_NUM *
        sizeof(HI_FLOAT) + pPropsalPram->stNodePluginParam.u32NumRatioAnchors *
        pPropsalPram->stNodePluginParam.u32NumScaleAnchors * SVP_WK_COORDI_NUM * sizeof(HI_FLOAT);

    pstAnchorMem->u32Size = u32AnchorMemSize;
    HI_S32 s32Ret = SAMPLE_RUNTIME_HiMemAlloc(pstAnchorMem, HI_TRUE);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "alloc Memory fail");
    return HI_SUCCESS;
}

static HI_S32 SetBaseAnchor(HI_U32 *au32RatioScaleHW, HI_FLOAT **pf32ScaleAnchors, HI_PROPOSAL_Param_S *pPropsalPram,
    HI_RUNTIME_MEM_S *pstAnchorMem)
{
    HI_SAMPLE_ANCHOR_PARAM_S stAnchorParam;
    HI_U32 au32Ratios[SVP_NNIE_MAX_RATIO_ANCHOR_NUM]; /* anchors' ratios */
    HI_U32 au32Scales[SVP_NNIE_MAX_RATIO_ANCHOR_NUM]; /* anchors' scales */
    HI_U32 au32BaseAnchor[SVP_WK_COORDI_NUM] = { 0, 0, 0, 0 };

    au32BaseAnchor[X_MIN_IDX] = 0;
    au32BaseAnchor[Y_MIN_IDX] = 0;
    au32BaseAnchor[X_MAX_IDX] = pPropsalPram->stNodePluginParam.u32MinSize - 1;
    au32BaseAnchor[Y_MAX_IDX] = pPropsalPram->stNodePluginParam.u32MinSize - 1;

    HI_FLOAT *pf32RatioAnchors = (HI_FLOAT *)((uintptr_t)(pstAnchorMem->u64VirAddr)) +
        pPropsalPram->stNodePluginParam.u32NumRatioAnchors * pPropsalPram->stNodePluginParam.u32NumScaleAnchors *
        au32RatioScaleHW[0] * au32RatioScaleHW[1] * SVP_WK_COORDI_NUM;
    *pf32ScaleAnchors = (HI_FLOAT *)((uintptr_t)(pstAnchorMem->u64VirAddr)) +
        pPropsalPram->stNodePluginParam.u32NumRatioAnchors * pPropsalPram->stNodePluginParam.u32NumScaleAnchors *
        au32RatioScaleHW[0] * au32RatioScaleHW[1] * SVP_WK_COORDI_NUM +
        pPropsalPram->stNodePluginParam.u32NumRatioAnchors * SVP_WK_COORDI_NUM;

    for (HI_U32 ratioAnchorNum = 0; ratioAnchorNum < pPropsalPram->stNodePluginParam.u32NumRatioAnchors;
         ++ratioAnchorNum) {
        au32Ratios[ratioAnchorNum] = (HI_S32)(pPropsalPram->stNodePluginParam.pfRatio[ratioAnchorNum] *
            SVP_WK_QUANT_BASE);
    }

    for (HI_U32 scaleAnchorNum = 0; scaleAnchorNum < pPropsalPram->stNodePluginParam.u32NumScaleAnchors;
         ++scaleAnchorNum) {
        au32Scales[scaleAnchorNum] = (HI_S32)(pPropsalPram->stNodePluginParam.pfScales[scaleAnchorNum] *
            SVP_WK_QUANT_BASE);
    }

    stAnchorParam.pf32RatioAnchors = pf32RatioAnchors;
    stAnchorParam.pu32Ratios = au32Ratios;
    stAnchorParam.u32NumRatioAnchors = pPropsalPram->stNodePluginParam.u32NumRatioAnchors;
    stAnchorParam.pf32ScaleAnchors = *pf32ScaleAnchors;
    stAnchorParam.pu32Scales = au32Scales;
    stAnchorParam.u32NumScaleAnchors = pPropsalPram->stNodePluginParam.u32NumScaleAnchors;
    stAnchorParam.au32BaseAnchor = au32BaseAnchor;
    HI_S32 s32Ret = GenBaseAnchor(&stAnchorParam);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "GenBaseAnchor error\n");
    return HI_SUCCESS;
}

HI_S32 createRfcnAndFasterrcnnCopParam(HI_U32 u32ParamNum, HI_RUNTIME_COP_ATTR_S *pCopParam,
    HI_PROPOSAL_Param_S *pPropsalPram, HI_U8 u8NetType)
{
    SAMPLE_CHK_RET(((pPropsalPram == HI_NULL) || (pCopParam == HI_NULL)), HI_FAILURE, "param is NULL\n");

    HI_FLOAT *pf32ScaleAnchors = HI_NULL;
    HI_RUNTIME_MEM_S stAnchorMem = {0};
    HI_U32 au32RatioScaleHW[2] = {0}; // height and wdith
    HI_S32 s32Ret = SampleFasterRCNNAnchorInfoInit((SAMPLE_WK_DETECT_NET_FASTER_RCNN_TYPE_E)u8NetType,
                                                   &(pPropsalPram->stNodePluginParam), au32RatioScaleHW);
    SAMPLE_CHK_RET((s32Ret == HI_FAILURE), HI_FAILURE, "SampleFasterRCNNAnchorInfoInit fail");

    s32Ret = AllocAnchorMemory(&stAnchorMem, pPropsalPram, au32RatioScaleHW);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, ERROR1, "alloc Anchor Memory error\n");

    s32Ret = SetBaseAnchor(au32RatioScaleHW, &pf32ScaleAnchors, pPropsalPram, &stAnchorMem);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, ERROR2, "SetBaseAnchor error\n");

    s32Ret = SetAnchorInPixel((HI_S32 *)((uintptr_t)(stAnchorMem.u64VirAddr)), pf32ScaleAnchors,
        au32RatioScaleHW[0], au32RatioScaleHW[1],
        pPropsalPram->stNodePluginParam.u32NumScaleAnchors * pPropsalPram->stNodePluginParam.u32NumRatioAnchors,
        (HI_S32)(pPropsalPram->stNodePluginParam.fSpatialScale * SVP_WK_QUANT_BASE));
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, ERROR2, "SetAnchorInPixel error\n");

    memcpy(&(pPropsalPram->stBachorMemInfo), &stAnchorMem, sizeof(HI_RUNTIME_MEM_S));
    pCopParam[0].pConstParam = (HI_VOID *)pPropsalPram;
    pCopParam[0].u32ConstParamSize = sizeof(HI_PROPOSAL_Param_S);
    memset(pCopParam[0].acCopName, 0, MAX_NAME_LEN + 1);
    strncpy(pCopParam[0].acCopName, "proposal", MAX_NAME_LEN);

    return HI_SUCCESS;
ERROR2:
    SAMPLE_FreeMem(&stAnchorMem);
ERROR1:
    SAMPLE_FREE(pPropsalPram->stNodePluginParam.pfScales);
    SAMPLE_FREE(pPropsalPram->stNodePluginParam.pfRatio);
    return HI_FAILURE;
}

HI_VOID releaseRfcnAndFrcnnCopParam(HI_U32 u32ParamNum, HI_PROPOSAL_Param_S *pCopParam)
{
    SAMPLE_FREE(pCopParam[0].stNodePluginParam.pfScales);
    SAMPLE_FREE(pCopParam[0].stNodePluginParam.pfRatio);
    HI_RUNTIME_MEM_S stMem;
    stMem.u64PhyAddr = pCopParam[0].stBachorMemInfo.u64PhyAddr;
    stMem.u64VirAddr = pCopParam[0].stBachorMemInfo.u64VirAddr;
    SAMPLE_FreeMem(&stMem);
}
