/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_runtime_group_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef ON_BOARD
#include "mpi_sys.h"
#include "mpi_vb.h"
#else
#include "hi_comm_svp.h"
#include "hi_nnie.h"
#include "mpi_nnie.h"
#endif
#include "hi_runtime_api.h"
#include "sample_mutex_ops.h"
#include "sample_log.h"
#include "sample_runtime_define.h"
#include "sample_memory_ops.h"
#include "math.h"
#include "sample_save_blob.h"
#include "sample_resize_roi.h"
#include "sample_data_utils.h"
#include "sample_cop_param.h"
#include "sample_model_rcnn.h"

static const HI_U32 GROUP_SRC_BLOB_NUM = 1;
enum {
    MAX_TARGETS_NUM = 2,
    TRACKER_MEM_NUM = 3,
    IMAGE_WIDTH = 352,
    IMAGE_HIGHT = 288,
    CONNECTOR_NUM = 10,
    CLASSIFY_IMAGE_WIDTH = 227,
    CLASSIFY_IMAGE_HIGHT = 227,
    CLASSIFY_NUM = 1000,
    FILE_NAME_LENGTH = 16,
    ALL_MODEL_OUTPUT_NUM = 13,
    COP_NUM = 1,
    WK_NUM = 5,
};

#ifndef TRACKER_DEBUG
// #define TRACKER_DEBUG
#endif

#ifndef CLASSIFY
// #define CLASSIFY
#endif

#ifdef CLASSIFY
    const HI_U32 GROUP_DST_BLOB_NUM = 12;
#else
    const HI_U32 GROUP_DST_BLOB_NUM = 6;
#endif

typedef struct hiSAMPLE_PC_MODEL_FILE_S {
    HI_CHAR *pcModelFileRFCN;
    HI_CHAR *pcModelFileAlex;
    HI_CHAR *pcModelFileGoturn;
} HI_SAMPLE_PC_MODEL_FILE_S;

static BlobInfo s_stSrcBlobInfo = {
    "", "image", "",
    HI_RUNTIME_BLOB_TYPE_U8,
    { 1, 3, IMAGE_HIGHT, IMAGE_WIDTH },
    HI_FALSE, ALIGN_16
};

static BlobInfo s_astDstBlobInfo[] = {
    {   "rfcn_after", "detection_targets", "",
        HI_RUNTIME_BLOB_TYPE_VEC_S32,
        { MAX_TARGETS_NUM, 4, 1, 1 },
        HI_FALSE, ALIGN_16
    },
    {   "feature_extra_1", "targets_feature_d", "",
        HI_RUNTIME_BLOB_TYPE_VEC_S32,
        { MAX_TARGETS_NUM, 4, 1, 1 },
        HI_FALSE, ALIGN_16
    },
    {   "go_turn_pre", "location_edge", "",
        HI_RUNTIME_BLOB_TYPE_VEC_S32,
        { MAX_TARGETS_NUM, 8, 1, 1 },
        HI_FALSE, ALIGN_16
    },
    {   "feature_extra_2", "targets_feature_t", "",
        HI_RUNTIME_BLOB_TYPE_VEC_S32,
        { MAX_TARGETS_NUM, 4, 1, 1 },
        HI_FALSE, ALIGN_16
    },
    {   "compare", "targets", "",
        HI_RUNTIME_BLOB_TYPE_U8,
        { 1, 3, IMAGE_HIGHT, IMAGE_WIDTH },
        HI_FALSE, ALIGN_16
    },
    {   "compare", "targets_bbox", "",
        HI_RUNTIME_BLOB_TYPE_VEC_S32,
        { MAX_TARGETS_NUM, 4, 1, 1 },
        HI_FALSE, ALIGN_16
    },
#ifdef CLASSIFY
    {   "resize_1", "targets_resize", "",
        HI_RUNTIME_BLOB_TYPE_U8,
        { MAX_TARGETS_NUM, 3, CLASSIFY_IMAGE_HIGHT, CLASSIFY_IMAGE_WIDTH },
        HI_FALSE, ALIGN_16
    },
    {   "resize_2", "targets_resize", "",
        HI_RUNTIME_BLOB_TYPE_U8,
        { MAX_TARGETS_NUM, 3, CLASSIFY_IMAGE_HIGHT, CLASSIFY_IMAGE_WIDTH },
        HI_FALSE, ALIGN_16
    },
    {   "resize_3", "targets_resize", "",
        HI_RUNTIME_BLOB_TYPE_U8,
        { MAX_TARGETS_NUM, 3, CLASSIFY_IMAGE_HIGHT, CLASSIFY_IMAGE_WIDTH },
        HI_FALSE, ALIGN_16
    },
    {   "alexnet_1", "prob", "",
        HI_RUNTIME_BLOB_TYPE_VEC_S32,
        { MAX_TARGETS_NUM, CLASSIFY_NUM, 1, 1 },
        HI_FALSE, ALIGN_16
    },
    {   "alexnet_2", "prob", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { MAX_TARGETS_NUM, CLASSIFY_NUM, 1, 1 },
        HI_FALSE, ALIGN_16
    },
    {   "alexnet_3", "prob", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { MAX_TARGETS_NUM, CLASSIFY_NUM, 1, 1 },
        HI_FALSE, ALIGN_16
    }
#endif
};

typedef struct tagTRACKER_BLOBS {
    HI_RUNTIME_GROUP_SRC_BLOB_ARRAY_S stGroupSrc;
    HI_RUNTIME_GROUP_DST_BLOB_ARRAY_S stGroupDst;
} TRACKER_BLOBS_S;

static TRACKER_BLOBS_S *g_gstTrackerBlobs;
static HI_RUNTIME_GROUP_INFO_S g_stGroupInfo;
static HI_BOOL g_bFinish = HI_FALSE;
static HI_BOOL g_bResult = HI_TRUE;
static HI_U64 g_u64CurFrame;
static SAMPLE_COND g_finishCond;
static SAMPLE_MUTEX g_finishMutex;
static HI_BOOL g_bIsFirstFrame = HI_TRUE;

const HI_DOUBLE kScaleFactor = 10;
static HI_VOID unscaleBbox(HI_S32 width, HI_S32 height, BondingBox_s *pstBbox)
{
    pstBbox->x1 /= kScaleFactor;
    pstBbox->x2 /= kScaleFactor;
    pstBbox->y1 /= kScaleFactor;
    pstBbox->y2 /= kScaleFactor;

    pstBbox->x1 *= width;
    pstBbox->x2 *= width;
    pstBbox->y1 *= height;
    pstBbox->y2 *= height;
}

static HI_VOID uncenterBbox(HI_S32 width, HI_S32 height, BondingBox_s *pstLocation,
                            HI_DOUBLE edgeX, HI_DOUBLE edgeY,
                            BondingBox_s *pstEstimateBbox, BondingBox_s *pstDstBbox)
{
    pstDstBbox->x1 = max(0.0, pstEstimateBbox->x1 + pstLocation->x1 - edgeX);
    pstDstBbox->y1 = max(0.0, pstEstimateBbox->y1 + pstLocation->y1 - edgeY);
    pstDstBbox->x2 = min(width, pstEstimateBbox->x2 + pstLocation->x1 - edgeX);
    pstDstBbox->y2 = min(height, pstEstimateBbox->y2 + pstLocation->y1 - edgeY);
}

static HI_S32 Connector_RfcnPre(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("\n Connector_RfcnPre \n");
#endif
    resizeBlob(&pstConnectorSrc->pstBlobs[0], &pstConnectorDst->pstBlobs[0]);
    return HI_SUCCESS;
}

static HI_S32 Connector_RfcnAfter(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                  HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("\n Connector_RfcnAfter \n");
#endif
    if (pstConnectorSrc->pstBlobs[1].unShape.stWhc.u32Width == 0) {
        return HI_FAILURE;
    }
    HI_U32 u32ResultROISize = (pstConnectorSrc->pstBlobs[1].unShape.stWhc.u32Width) * MAX_ROI_NUM *
                              SVP_WK_PROPOSAL_WIDTH *
                              sizeof(HI_S32);
    HI_S32 *ps32ResultROI = (HI_S32 *)malloc(u32ResultROISize);
    SAMPLE_CHK_RET((ps32ResultROI == NULL), HI_FAILURE, "malloc resultROISize fail\n");
    memset(ps32ResultROI, 0x0, u32ResultROISize);
    HI_U32 u32ResultROICnt = 0;
    HI_U32 *pu32DstAddr = HI_NULL;

    SAMPLE_RUNTIME_ROI_PARAM_S stRoiParam = {
        &pstConnectorSrc->pstBlobs[1], &pstConnectorSrc->pstBlobs[2], &pstConnectorSrc->pstBlobs[0], HI_NULL,
        RFCN_IMAGE_WIDTH, RFCN_IMAGE_HEIGHT,
    };
    HI_S32 s32Ret = SAMPLE_DATA_GetRoiResultFromOriginSize(SAMPLE_RUNTIME_MODEL_TYPE_RFCN,
        &stRoiParam, ps32ResultROI, &u32ResultROICnt);
    SAMPLE_CHK_GOTO((s32Ret != HI_SUCCESS), GET_ROI_FAILED, "SAMPLE_DATA_GetRoiResult failed\n");
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("roi cnt: %u\n", u32ResultROICnt);
#endif
    pu32DstAddr = (HI_U32 *)((uintptr_t)(pstConnectorDst->pstBlobs[0]).u64VirAddr);
    // only support MAX_TARGETS_NUM targets
    pstConnectorDst->pstBlobs[0].u32Num = MAX_TARGETS_NUM;

    for (HI_U32 i = 0; i < pstConnectorDst->pstBlobs[0].u32Num; i++) {
        pu32DstAddr[X_MIN_IDX] = ps32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + X_MIN_IDX];
        pu32DstAddr[Y_MIN_IDX] = ps32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + Y_MIN_IDX];
        pu32DstAddr[X_MAX_IDX] = ps32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + X_MAX_IDX];
        pu32DstAddr[Y_MAX_IDX] = ps32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + Y_MAX_IDX];
        pu32DstAddr += (pstConnectorDst->pstBlobs[0]).u32Stride / sizeof(HI_U32);
    }
    SAMPLE_FREE(ps32ResultROI);
    return HI_SUCCESS;
GET_ROI_FAILED:
    SAMPLE_FREE(ps32ResultROI);
    return HI_FAILURE;
}

static HI_S32 Connector_FeatureExtra1(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                      HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("\n Connector_FeatureExtra1 \n");
#endif
    HI_U8 *pu8SrcAddr = (HI_U8 *)((uintptr_t)pstConnectorSrc->pstBlobs[0].u64VirAddr);
    HI_U8 *pu8DstAddr = (HI_U8 *)((uintptr_t)pstConnectorDst->pstBlobs[0].u64VirAddr);
    HI_U32 u32CopySize = pstConnectorSrc->pstBlobs[0].u32Num * pstConnectorSrc->pstBlobs[0].unShape.stWhc.u32Chn *
                         pstConnectorSrc->pstBlobs[0].unShape.stWhc.u32Height * pstConnectorSrc->pstBlobs[0].u32Stride;
    memcpy(pu8DstAddr, pu8SrcAddr, u32CopySize);
    return HI_SUCCESS;
}

static HI_VOID getLocation(HI_RUNTIME_BLOB_S *pstLocationEdge, TRACKER_LOCATION_S *pstLocation,
                           HI_S32 *ps32EdgeX, HI_S32 *ps32EdgeY, HI_S32 *ps32RegionX, HI_S32 *ps32RegionY)
{
    HI_S32 *ps32Dst = HI_NULL;
    HI_U32 u32OffSet = pstLocationEdge->unShape.stWhc.u32Chn * pstLocationEdge->unShape.stWhc.u32Height *
                       pstLocationEdge->u32Stride;
    for (HI_U32 i = 0; i < pstLocationEdge->u32Num; i++) {
        ps32Dst = (HI_S32 *)(uintptr_t)(((uintptr_t)(pstLocationEdge->u64VirAddr) + i * u32OffSet));
        *ps32Dst = pstLocation[i].x1;
        HI_U32 j = 1;
        *(ps32Dst + (j++)) = pstLocation[i].y1;
        *(ps32Dst + (j++)) = pstLocation[i].x2;
        *(ps32Dst + (j++)) = pstLocation[i].y2;
        *(ps32Dst + (j++)) = ps32EdgeX[i];
        *(ps32Dst + (j++)) = ps32EdgeY[i];
        *(ps32Dst + (j++)) = ps32RegionX[i];
        *(ps32Dst + (j++)) = ps32RegionY[i];
    }
}

static HI_S32 Connector_GoTurnPre(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                  HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("\n Connector_GoTurnPre srdID[%llu]\n", srcId);
#endif
    if ((pstConnectorSrc->u32BlobNum != 3) || (pstConnectorDst->u32BlobNum != 3)) {
        SAMPLE_LOG_INFO("Invalid input number [%u] or output number [%u]\n", pstConnectorSrc->u32BlobNum,
                        pstConnectorDst->u32BlobNum);
        return HI_FAILURE;
    }

    if (g_bIsFirstFrame == HI_TRUE) {
        pstConnectorDst->pstBlobs[RFCN_OUTPUT_PROPOSAL_IDX].u32Num = 0;
        pstConnectorDst->pstBlobs[RFCN_OUTPUT_CLS_IDX].u32Num = 0;
        pstConnectorDst->pstBlobs[RFCN_OUTPUT_BBOX_IDX].u32Num = 0;
        g_bIsFirstFrame = HI_FALSE;
        return HI_SUCCESS;
    }

    HI_U32 i = 0;
    HI_RUNTIME_BLOB_S *pstImageBlob = &pstConnectorSrc->pstBlobs[i++];
    HI_RUNTIME_BLOB_S *pstTargetBlob = &pstConnectorSrc->pstBlobs[i++];
    HI_RUNTIME_BLOB_S *pstTargetsBboxBlob = &pstConnectorSrc->pstBlobs[i++];
    i = 0;
    HI_RUNTIME_BLOB_S *pstImageCropBlob = &pstConnectorDst->pstBlobs[i++];
    HI_RUNTIME_BLOB_S *pstTargetCropBlob = &pstConnectorDst->pstBlobs[i++];
    HI_RUNTIME_BLOB_S *pstLocationEdge = &pstConnectorDst->pstBlobs[i++];

    TRACKER_LOCATION_S astLocation[MAX_TARGETS_NUM];
    HI_S32 as32EdgeX[MAX_TARGETS_NUM];
    HI_S32 as32EdgeY[MAX_TARGETS_NUM];
    HI_S32 as32RegionX[MAX_TARGETS_NUM];
    HI_S32 as32RegionY[MAX_TARGETS_NUM];

    memset(astLocation, 0x0, sizeof(astLocation));
    memset(as32EdgeX, 0x0, sizeof(as32EdgeX));
    memset(as32EdgeY, 0x0, sizeof(as32EdgeY));
    memset(as32RegionX, 0x0, sizeof(as32RegionX));
    memset(as32RegionY, 0x0, sizeof(as32RegionY));

    cropPadBlob(pstImageBlob, pstTargetsBboxBlob, pstImageCropBlob,
                &astLocation[0], &as32EdgeX[0], &as32EdgeY[0], &as32RegionX[0], &as32RegionY[0]);

    getLocation(pstLocationEdge, astLocation, as32EdgeX, as32EdgeY, as32RegionX, as32RegionY);

    cropPadBlob(pstTargetBlob, pstTargetsBboxBlob, pstTargetCropBlob,
                &astLocation[0], &as32EdgeX[0], &as32EdgeY[0], &as32RegionX[0], &as32RegionY[0]);

    return HI_SUCCESS;
}

static HI_S32 GoTurnAfterProcess(HI_RUNTIME_BLOB_S *pstSrcEstimateBlob, HI_RUNTIME_BLOB_S *pstSrcLocationEdgeBlob,
                                 HI_RUNTIME_BLOB_S *pstDstBlob, HI_U32 u32SrcEstimateOffSet,
                                 HI_U32 u32SrcLocationEdgeOffSet, HI_U32 u32DstBboxOffSet)
{
    HI_S32 *ps32SrcEstimate = NULL;
    HI_S32 *ps32SrcLocationEdge = NULL;
    HI_S32 *ps32Dst = NULL;
    BondingBox_s stEstimate;
    BondingBox_s stLocation;
    BondingBox_s stDstBbox;
    HI_DOUBLE dEdgeX;
    HI_DOUBLE dEdgeY;
    HI_S32 s32RegionW;
    HI_S32 s32RegionH;
    HI_U32 j = 0;

    for (HI_U32 i = 0; i < pstSrcEstimateBlob->u32Num; i++) {
        ps32SrcEstimate = (HI_S32 *)(uintptr_t)(((uintptr_t)(pstSrcEstimateBlob->u64VirAddr) +
                                                             i * u32SrcEstimateOffSet));
        j = 0;
        stEstimate.x1 = (HI_DOUBLE)(*(ps32SrcEstimate + (j++))) / SVP_WK_QUANT_BASE;
        stEstimate.y1 = (HI_DOUBLE)(*(ps32SrcEstimate + (j++))) / SVP_WK_QUANT_BASE;
        stEstimate.x2 = (HI_DOUBLE)(*(ps32SrcEstimate + (j++))) / SVP_WK_QUANT_BASE;
        stEstimate.y2 = (HI_DOUBLE)(*(ps32SrcEstimate + (j++))) / SVP_WK_QUANT_BASE;

        ps32SrcLocationEdge = (HI_S32 *)(uintptr_t)(((uintptr_t)(pstSrcLocationEdgeBlob->u64VirAddr) + i *
                                                     u32SrcLocationEdgeOffSet));
        j = 0;
        stLocation.x1 = (HI_DOUBLE)(*(ps32SrcLocationEdge + (j++))) / SVP_WK_QUANT_BASE;
        stLocation.y1 = (HI_DOUBLE)(*(ps32SrcLocationEdge + (j++))) / SVP_WK_QUANT_BASE;
        stLocation.x2 = (HI_DOUBLE)(*(ps32SrcLocationEdge + (j++))) / SVP_WK_QUANT_BASE;
        stLocation.y2 = (HI_DOUBLE)(*(ps32SrcLocationEdge + (j++))) / SVP_WK_QUANT_BASE;
        dEdgeX = (HI_DOUBLE)(*(ps32SrcLocationEdge + (j++))) / SVP_WK_QUANT_BASE;
        dEdgeY = (HI_DOUBLE)(*(ps32SrcLocationEdge + (j++))) / SVP_WK_QUANT_BASE;
        s32RegionW = *(ps32SrcLocationEdge + (j++));
        s32RegionH = *(ps32SrcLocationEdge + (j++));

        SAMPLE_LOG_INFO("estimate %d : (%f, %f)(%f, %f)\n", i, stEstimate.x1, stEstimate.y1, stEstimate.x2,
                        stEstimate.y2);
        unscaleBbox(s32RegionW, s32RegionH, &stEstimate);
        uncenterBbox(IMAGE_WIDTH, IMAGE_HIGHT, &stLocation, dEdgeX, dEdgeY, &stEstimate, &stDstBbox);

        ps32Dst = (HI_S32 *)(uintptr_t)(((uintptr_t)(pstDstBlob->u64VirAddr) + i * u32DstBboxOffSet));
        j = 0;
        *(ps32Dst + (j++)) = (HI_S32)stDstBbox.x1;
        *(ps32Dst + (j++)) = (HI_S32)stDstBbox.y1;
        *(ps32Dst + (j++)) = (HI_S32)stDstBbox.x2;
        *(ps32Dst + (j++)) = (HI_S32)stDstBbox.y2;
    }
    return HI_SUCCESS;
}

static HI_S32 Connector_GoTurnAfter(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                    HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("\n Connector_GoTurnAfter \n");
#endif
    if ((pstConnectorSrc->u32BlobNum != 2) || (pstConnectorDst->u32BlobNum != 1)) {
        SAMPLE_LOG_INFO("Invalid input number [%u] or output number [%u]\n", pstConnectorSrc->u32BlobNum,
                        pstConnectorDst->u32BlobNum);
        return HI_FAILURE;
    }
    HI_RUNTIME_BLOB_S *pstSrcEstimateBlob = &pstConnectorSrc->pstBlobs[0];
    HI_RUNTIME_BLOB_S *pstSrcLocationEdgeBlob = &pstConnectorSrc->pstBlobs[1];
    HI_RUNTIME_BLOB_S *pstDstBlob = &pstConnectorDst->pstBlobs[0];
    HI_U32 u32SrcEstimateOffSet = pstSrcEstimateBlob->u32Stride * pstSrcEstimateBlob->unShape.stWhc.u32Height *
                                  pstSrcEstimateBlob->unShape.stWhc.u32Chn;
    HI_U32 u32SrcLocationEdgeOffSet = pstSrcLocationEdgeBlob->u32Stride *
                                      pstSrcLocationEdgeBlob->unShape.stWhc.u32Height *
                                      pstSrcLocationEdgeBlob->unShape.stWhc.u32Chn;
    HI_U32 u32DstBboxOffSet = pstDstBlob->u32Stride * pstDstBlob->unShape.stWhc.u32Height *
                              pstDstBlob->unShape.stWhc.u32Chn;

    return GoTurnAfterProcess(pstSrcEstimateBlob, pstSrcLocationEdgeBlob, pstDstBlob, u32SrcEstimateOffSet,
                              u32SrcLocationEdgeOffSet, u32DstBboxOffSet);
}

static HI_S32 Connector_FeatureExtra2(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                      HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("\n Connector_FeatureExtra2 \n");
#endif
    HI_U8 *pu8SrcAddr = (HI_U8 *)((uintptr_t)(pstConnectorSrc->pstBlobs[0].u64VirAddr));
    HI_U8 *pu8DstAddr = (HI_U8 *)((uintptr_t)(pstConnectorDst->pstBlobs[0].u64VirAddr));
    HI_U32 u32CopySize = pstConnectorSrc->pstBlobs[0].u32Num * pstConnectorSrc->pstBlobs[0].unShape.stWhc.u32Chn *
                         pstConnectorSrc->pstBlobs[0].unShape.stWhc.u32Height * pstConnectorSrc->pstBlobs[0].u32Stride;
    memcpy(pu8DstAddr, pu8SrcAddr, u32CopySize);

    return HI_SUCCESS;
}

static HI_S32 copyToTarget(HI_RUNTIME_BLOB_S *pDstTarget,
                           HI_RUNTIME_BLOB_S *pOrigImageBlob,
                           HI_RUNTIME_BLOB_S *pSrcBlob,
                           HI_BOOL bIsScale,
                           HI_U64 srcId)
{
    HI_U8 *pu8DstAddr = HI_NULL;
    HI_U32 *pu32TmpDstAddr = HI_NULL;
    HI_U32 *pu32TmpSrcAddr = HI_NULL;
    HI_S32 as32ResultROI[MAX_ROI_NUM * 4] = {0};
    HI_CHAR aszResizeFileName[FILE_NAME_LENGTH];
    memset(aszResizeFileName, 0x0, sizeof(aszResizeFileName));
    pu8DstAddr = (HI_U8 *)((uintptr_t)(pDstTarget->u64VirAddr));
    HI_U32 u32ResultCnt = pSrcBlob->u32Num;

    pu32TmpDstAddr = (HI_U32 *)pu8DstAddr;
    pu32TmpSrcAddr = (HI_U32 *)((uintptr_t)(pSrcBlob->u64VirAddr));
    for (HI_U32 i = 0; i < u32ResultCnt; i++) {
        if (bIsScale == HI_TRUE) {
            as32ResultROI[i * SCORE_IDX + X_MIN_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + X_MIN_IDX] *
                                               ((HI_FLOAT)pOrigImageBlob->unShape.stWhc.u32Width / RFCN_IMAGE_WIDTH));
            as32ResultROI[i * SCORE_IDX + Y_MIN_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + Y_MIN_IDX] *
                                               ((HI_FLOAT)pOrigImageBlob->unShape.stWhc.u32Height / RFCN_IMAGE_HEIGHT));
            as32ResultROI[i * SCORE_IDX + X_MAX_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + X_MAX_IDX] *
                                               ((HI_FLOAT)pOrigImageBlob->unShape.stWhc.u32Width / RFCN_IMAGE_WIDTH));
            as32ResultROI[i * SCORE_IDX + Y_MAX_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + Y_MAX_IDX] *
                                               ((HI_FLOAT)pOrigImageBlob->unShape.stWhc.u32Height / RFCN_IMAGE_HEIGHT));
        } else {
            as32ResultROI[i * SCORE_IDX + X_MIN_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + X_MIN_IDX]);
            as32ResultROI[i * SCORE_IDX + Y_MIN_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + Y_MIN_IDX]);
            as32ResultROI[i * SCORE_IDX + X_MAX_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + X_MAX_IDX]);
            as32ResultROI[i * SCORE_IDX + Y_MAX_IDX] = (HI_U32)(pu32TmpSrcAddr[i * SCORE_IDX + Y_MAX_IDX]);
        }
        pu32TmpDstAddr[i * SCORE_IDX + X_MIN_IDX] = as32ResultROI[i * SCORE_IDX + X_MIN_IDX];
        pu32TmpDstAddr[i * SCORE_IDX + Y_MIN_IDX] = as32ResultROI[i * SCORE_IDX + Y_MIN_IDX];
        pu32TmpDstAddr[i * SCORE_IDX + X_MAX_IDX] = as32ResultROI[i * SCORE_IDX + X_MAX_IDX];
        pu32TmpDstAddr[i * SCORE_IDX + Y_MAX_IDX] = as32ResultROI[i * SCORE_IDX + Y_MAX_IDX];
#ifdef TRACKER_DEBUG
        SAMPLE_LOG_INFO("==========cord %d,%d,%d,%d========\n", as32ResultROI[i * SCORE_IDX + X_MIN_IDX],
                        as32ResultROI[i * SCORE_IDX + Y_MIN_IDX], as32ResultROI[i * SCORE_IDX + X_MAX_IDX],
                        as32ResultROI[i * SCORE_IDX + Y_MAX_IDX]);
#endif
    }

    snprintf(aszResizeFileName, sizeof(aszResizeFileName), "%llu_out", srcId);
    drawImageRect(aszResizeFileName, pOrigImageBlob, as32ResultROI, u32ResultCnt, 4);

    return HI_SUCCESS;
}
static HI_S32 Connector_Compare(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
#ifdef TRACKER_DEBUG
    SAMPLE_LOG_INFO("\n Connector_Compare \n");
#endif
    HI_S32 s32Ret = HI_FAILURE;
    if (pstConnectorSrc->pstBlobs[2].u32Num == 0) {
        s32Ret = copyToTarget(&(pstConnectorDst->pstBlobs[1]),
                              &(pstConnectorSrc->pstBlobs[0]), &(pstConnectorSrc->pstBlobs[1]),
                              HI_TRUE, srcId);
    } else {
        s32Ret = copyToTarget(&(pstConnectorDst->pstBlobs[1]),
                              &(pstConnectorSrc->pstBlobs[0]), &(pstConnectorSrc->pstBlobs[2]),
                              HI_FALSE, srcId);
    }

    HI_U8 *pu8SrcAddr = (HI_U8 *)((uintptr_t)(pstConnectorSrc->pstBlobs[0].u64VirAddr));
    HI_U8 *pu8DstAddr = (HI_U8 *)((uintptr_t)(pstConnectorDst->pstBlobs[0].u64VirAddr));
    HI_U32 u32CopySize = pstConnectorSrc->pstBlobs[0].u32Num * pstConnectorSrc->pstBlobs[0].unShape.stWhc.u32Chn *
                         pstConnectorSrc->pstBlobs[0].unShape.stWhc.u32Height * pstConnectorSrc->pstBlobs[0].u32Stride;
    memcpy(pu8DstAddr, pu8SrcAddr, u32CopySize);

    return s32Ret;
}
#ifdef CLASSIFY
static HI_S32 Connector_Resize1(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
    return HI_SUCCESS;
}

static HI_S32 Connector_Resize2(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
    return HI_SUCCESS;
}

static HI_S32 Connector_Resize3(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 srcId, HI_VOID *pParam)
{
    return HI_SUCCESS;
}
#endif

static HI_S32 LoadWks(const HI_CHAR *pcModelFileRFCN,
                      const HI_CHAR *pcModelFileGoturn,
                      const HI_CHAR *pcModelFileAlex,
                      HI_RUNTIME_WK_INFO_S astWkInfo[WK_NUM])
{
    HI_U32 i = 0;
    // wk mem
    strncpy(astWkInfo[i++].acModelName, "rfcn", MAX_NAME_LEN);
    HI_S32 s32Ret = SAMPLE_RUNTIME_LoadModelFile(pcModelFileRFCN, &astWkInfo[0].stWKMemory);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "SAMPLE_RUNTIME_LoadModelFile %s failed!\n", pcModelFileRFCN);

    strncpy(astWkInfo[i++].acModelName, "go_turn", MAX_NAME_LEN);
    s32Ret = SAMPLE_RUNTIME_LoadModelFile(pcModelFileGoturn, &astWkInfo[1].stWKMemory);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "SAMPLE_RUNTIME_LoadModelFile %s failed!\n", pcModelFileGoturn);
#ifdef CLASSIFY
    strncpy(astWkInfo[i++].acModelName, "alexnet_1", MAX_NAME_LEN);
    s32Ret = SAMPLE_RUNTIME_LoadModelFile(pcModelFileAlex, &astWkInfo[2].stWKMemory);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "SAMPLE_RUNTIME_LoadModelFile %s failed!\n", pcModelFileAlex);

    strncpy(astWkInfo[i].acModelName, "alexnet_2", MAX_NAME_LEN);
    memcpy(&astWkInfo[i++].stWKMemory, &astWkInfo[Two].stWKMemory, sizeof(HI_RUNTIME_MEM_S));

    strncpy(astWkInfo[i].acModelName, "alexnet_3", MAX_NAME_LEN);
    memcpy(&astWkInfo[i++].stWKMemory, &astWkInfo[Two].stWKMemory, sizeof(HI_RUNTIME_MEM_S));
#endif

    return HI_SUCCESS;

}

static HI_S32 PreparaProposalAttr(HI_RUNTIME_COP_ATTR_S *pstProposalAttr, HI_PROPOSAL_Param_S *pProposalParam)
{
    memset(pstProposalAttr, 0, sizeof(HI_RUNTIME_COP_ATTR_S));

    // cop param
    strncpy(pstProposalAttr->acModelName, "rfcn", MAX_NAME_LEN);
    strncpy(pstProposalAttr->acCopName, "proposal", MAX_NAME_LEN);
    pstProposalAttr->u32ConstParamSize = sizeof(HI_PROPOSAL_Param_S);

    HI_S32 s32Ret = createRfcnAndFasterrcnnCopParam(1, pstProposalAttr, pProposalParam, SAMPLE_WK_DETECT_NET_RFCN);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "createRfcnAndFasterrcnnCopParam failed!\n");

    return HI_SUCCESS;
}

static HI_VOID PreparaConnectorAttr(HI_RUNTIME_CONNECTOR_ATTR_S astConnectorAttr[CONNECTOR_NUM])
{
    HI_U32 i = 0;
    memset(&astConnectorAttr[0], 0, CONNECTOR_NUM * sizeof(HI_RUNTIME_CONNECTOR_ATTR_S));

    // conector
    strncpy(astConnectorAttr[i].acName, "rfcn_pre", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_RfcnPre;

    strncpy(astConnectorAttr[i].acName, "rfcn_after", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_RfcnAfter;

    strncpy(astConnectorAttr[i].acName, "feature_extra_1", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_FeatureExtra1;

    strncpy(astConnectorAttr[i].acName, "go_turn_pre", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_GoTurnPre;

    strncpy(astConnectorAttr[i].acName, "go_turn_after", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_GoTurnAfter;

    strncpy(astConnectorAttr[i].acName, "feature_extra_2", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_FeatureExtra2;

    strncpy(astConnectorAttr[i].acName, "compare", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_Compare;
#ifdef CLASSIFY
    strncpy(astConnectorAttr[i].acName, "resize_1", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_Resize1;

    strncpy(astConnectorAttr[i].acName, "resize_2", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_Resize2;

    strncpy(astConnectorAttr[i].acName, "resize_3", MAX_NAME_LEN);
    astConnectorAttr[i].pParam = NULL;
    astConnectorAttr[i++].pConnectorFun = Connector_Resize3;
#endif
    return;
}

static HI_S32 SAMPLE_RUNTIME_LoadModelGroup_RFCNGoTrunAlex(HI_SAMPLE_PC_MODEL_FILE_S *pstModelFile,
                                                           HI_RUNTIME_WK_INFO_S astWkInfo[WK_NUM],
                                                           HI_PROPOSAL_Param_S *pProposalParam,
                                                           HI_RUNTIME_GROUP_HANDLE *phGroupHandle)
{
    HI_CHAR *pacConfig = NULL;
    HI_S32 s32Ret = LoadWks(pstModelFile->pcModelFileRFCN, pstModelFile->pcModelFileGoturn,
        pstModelFile->pcModelFileAlex, astWkInfo);

    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL, "Load wks failed!\n");

    HI_RUNTIME_COP_ATTR_S stProposalAttr = {0};
    HI_RUNTIME_CONNECTOR_ATTR_S astConnectorAttr[CONNECTOR_NUM] = {0};

    s32Ret = PreparaProposalAttr(&stProposalAttr, pProposalParam);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL, "PreparaProposalAttr failed!\n");

    PreparaConnectorAttr(astConnectorAttr);

    SAMPLE_RUNTIME_ReadConfig(CONFIG_DIR "rfcn_goturn_alexnet_tracker.modelgroup", &pacConfig);
    SAMPLE_CHK_GOTO(NULL == pacConfig, READCONFIG_FAIL, "HI_SVPRT_RUNTIME_ReadConfig failed!\n");

    g_stGroupInfo.stWKsInfo.u32WKNum = 2; // 2 wk
#ifdef CLASSIFY
    g_stGroupInfo.stWKsInfo.u32WKNum = 5; // more than 3 alexnet
#endif
    g_stGroupInfo.stWKsInfo.pstAttrs = &astWkInfo[0];

    g_stGroupInfo.stCopsAttr.u32CopNum = 1;
    g_stGroupInfo.stCopsAttr.pstAttrs = &stProposalAttr;
    g_stGroupInfo.stConnectorsAttr.u32ConnectorNum = 7; // 7 connector
#ifdef CLASSIFY
    g_stGroupInfo.stConnectorsAttr.u32ConnectorNum = 10; // more than 3 alexnet
#endif
    g_stGroupInfo.stConnectorsAttr.pstAttrs = &astConnectorAttr[0];

    s32Ret = HI_SVPRT_RUNTIME_LoadModelGroup(pacConfig, &g_stGroupInfo, phGroupHandle);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL, "HI_SVPRT_RUNTIME_LoadModelGroupSync failed!\n");

    SAMPLE_LOG_INFO("LoadGroup succ\n");

READCONFIG_FAIL:
    SAMPLE_FREE(pacConfig);
    return s32Ret;
FAIL:
    SAMPLE_FREE(pacConfig);
    return HI_FAILURE;
}

static HI_S32 forward_finish(HI_RUNTIME_FORWARD_STATUS_CALLBACK_E enEvent,
                             HI_RUNTIME_GROUP_HANDLE hGroupHandle,
                             HI_U64 u64FrameId,
                             HI_RUNTIME_GROUP_DST_BLOB_ARRAY_PTR pstDstBlobs)
{
    SAMPLE_LOG_INFO("forward finish: %llu\n", u64FrameId);
    SAMPLE_CHK_GOTO(enEvent != HI_RUNTIME_FORWARD_STATUS_SUCC, FAIL_0, "Forward finish failed[%u]!\n", enEvent);
    SAMPLE_CHK_GOTO(u64FrameId != g_u64CurFrame + 1, FAIL_0,
                    "Forward finish with error frameId[%llu], current frameId[%llu]\n", u64FrameId, g_u64CurFrame);

    SAMPLE_MUTEX_Lock(&g_finishMutex);
    g_u64CurFrame = u64FrameId;
    SAMPLE_COND_Broadcast(&g_finishCond);
    SAMPLE_MUTEX_Unlock(&g_finishMutex);
    return HI_SUCCESS;

FAIL_0:
    SAMPLE_MUTEX_Lock(&g_finishMutex);
    g_bFinish = HI_TRUE;
    g_bResult = HI_FALSE;
    g_u64CurFrame = u64FrameId;
    SAMPLE_COND_Broadcast(&g_finishCond);
    SAMPLE_MUTEX_Unlock(&g_finishMutex);
    return HI_SUCCESS;
}

HI_VOID blobFree(TRACKER_BLOBS_S *pstBlobs)
{
    for (HI_U32 i = 0; i < TRACKER_MEM_NUM; i++) {
        DeinitBlobs(&pstBlobs[i].stGroupSrc);
        DeinitBlobs(&pstBlobs[i].stGroupDst);
    }
    SAMPLE_FREE(pstBlobs);
}

static HI_S32 blobsInit(TRACKER_BLOBS_S **ppstBlobs)
{
    HI_S32 s32Ret = HI_SUCCESS;
    *ppstBlobs = (TRACKER_BLOBS_S *)malloc(sizeof(TRACKER_BLOBS_S) * TRACKER_MEM_NUM);
    if (*ppstBlobs == HI_NULL) {
        return HI_FAILURE;
    }
    memset((HI_CHAR *)*ppstBlobs, 0, sizeof(TRACKER_BLOBS_S));
    TRACKER_BLOBS_S *pstBlobs = *ppstBlobs;
    for (HI_U32 i = 0; i < TRACKER_MEM_NUM; i++) {

        s32Ret = InitBlobs(&pstBlobs[i].stGroupSrc, GROUP_SRC_BLOB_NUM, &s_stSrcBlobInfo);
        SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "Init src blobs failed!\n");

        s32Ret = InitBlobs(&pstBlobs[i].stGroupDst, GROUP_DST_BLOB_NUM, s_astDstBlobInfo);
        SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "Init dst blobs failed!\n");
    }

    return s32Ret;
FAIL_0:
    blobFree(pstBlobs);
    return s32Ret;
}

static HI_VOID blobsUpdate(TRACKER_BLOBS_S *pstBlobs)
{
    pstBlobs->stGroupSrc.u32BlobNum = 1;

    for (HI_U32 i = 0; i < GROUP_DST_BLOB_NUM; i++) {
        pstBlobs->stGroupDst.pstBlobs[i].pstBlob->u32Num = MAX_TARGETS_NUM;
    }
}

static HI_BOOL FirstNForward(const HI_CHAR *pcSrcFile, HI_RUNTIME_GROUP_HANDLE hGroupHandle,
                             HI_U64 u64StartFrame, HI_U64 u64EndFrame, HI_U32 *pIndex)
{
    HI_CHAR acFileName[MAX_FILE_NAME_LENGTH] = {0};
    HI_U32 s32Ret = HI_SUCCESS;

    for (HI_U32 i = 0; i < TRACKER_MEM_NUM;) {
        snprintf(acFileName, sizeof(acFileName), pcSrcFile, i + u64StartFrame);
        s32Ret = SAMPLE_RUNTIME_ReadSrcFile(acFileName, g_gstTrackerBlobs[i].stGroupSrc.pstBlobs->pstBlob);
        SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_TRUE, "Read image fail, frameNum = %llu\n", i + u64StartFrame);

        s32Ret = HI_SVPRT_RUNTIME_ForwardGroupASync(hGroupHandle, &(g_gstTrackerBlobs[i].stGroupSrc),
                                                    &(g_gstTrackerBlobs[i].stGroupDst),
                                                    i + u64StartFrame, forward_finish);
        SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_TRUE, "Forward fail, srcId = %llu\n", i + u64StartFrame - 1);
        i++;
        *pIndex = i;
        SAMPLE_CHK_RET(i + u64StartFrame - 1 >= u64EndFrame, HI_TRUE, "Reach end frame, srcId = %llu\n",
            i + u64StartFrame - 1);
    }
    return HI_FALSE;
}

static HI_BOOL NextForward(const HI_CHAR *pcSrcFile, HI_RUNTIME_GROUP_HANDLE hGroupHandle,
                           HI_U64 u64StartFrame, HI_U32 index)
{
    HI_CHAR acFileName[MAX_FILE_NAME_LENGTH] = {0};

    HI_U32 u32MemIndex = (g_u64CurFrame - u64StartFrame) % TRACKER_MEM_NUM;
    blobsUpdate(&g_gstTrackerBlobs[u32MemIndex]);
    snprintf(acFileName, sizeof(acFileName), pcSrcFile, index + u64StartFrame);
    HI_S32 s32Ret = SAMPLE_RUNTIME_ReadSrcFile(acFileName,
        g_gstTrackerBlobs[u32MemIndex].stGroupSrc.pstBlobs->pstBlob);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_LOG_INFO("Read image fail, frameNum = %llu\n", index + u64StartFrame);
        return HI_TRUE;
    }

    s32Ret = HI_SVPRT_RUNTIME_ForwardGroupASync(hGroupHandle, &(g_gstTrackerBlobs[u32MemIndex].stGroupSrc),
                                                &(g_gstTrackerBlobs[u32MemIndex].stGroupDst),
                                                index + u64StartFrame, forward_finish);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_LOG_INFO("Forward fail, srcId = %llu\n", g_u64CurFrame + TRACKER_MEM_NUM);
        return HI_TRUE;
    }

    return HI_FALSE;
}

static HI_S32 SAMPLE_RUNTIME_ForwardGroup_RFCNGoTurnAlex(const HI_CHAR *pcSrcFile,
                                                         HI_RUNTIME_GROUP_HANDLE hGroupHandle,
                                                         HI_U64 u64StartFrame, HI_U64 u64EndFrame)
{
    HI_S32 s32Ret = HI_SUCCESS;

    if (blobsInit(&g_gstTrackerBlobs) != HI_SUCCESS) {
        SAMPLE_LOG_INFO("Malloc fail!\n");
        return HI_FAILURE;
    }
    g_u64CurFrame = u64StartFrame - 1;

    HI_U32 i = 0;
    HI_BOOL isFinish = FirstNForward(pcSrcFile, hGroupHandle, u64StartFrame, u64EndFrame, &i);
    SAMPLE_CHK_GOTO(isFinish == HI_TRUE, END_0, "Data input finish.\n");

    {
        SAMPLE_MUTEX_Lock(&g_finishMutex);

        while (!g_bFinish) {
            SAMPLE_COND_Wait(&g_finishCond, &g_finishMutex);
            if ((g_u64CurFrame == u64EndFrame) || (i + u64StartFrame > u64EndFrame)) {
                SAMPLE_LOG_INFO("Finish forward from %llu to %llu\n", u64StartFrame, u64EndFrame);
                break;
            } else {
                isFinish = NextForward(pcSrcFile, hGroupHandle, u64StartFrame, i);
                if (isFinish == HI_TRUE) {
                    SAMPLE_MUTEX_Unlock(&g_finishMutex);
                    goto END_0;
                }
                i++;
            }
        }

        if (g_bResult == HI_FALSE) {
            g_bResult = HI_TRUE;
            s32Ret = HI_FAILURE;
        }

        SAMPLE_MUTEX_Unlock(&g_finishMutex);
    }

    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, END_0, "forward failed\n");

END_0:
    printf("wait for finish 1, current frame[%llu], end frame[%llu]\n", g_u64CurFrame, i + u64StartFrame - 1);

    SAMPLE_MUTEX_Lock(&g_finishMutex);

    while (g_u64CurFrame < i + u64StartFrame - 1) {
        printf("wait for finish, current frame[%llu], end frame[%llu]\n", g_u64CurFrame, i + u64StartFrame - 1);
        SAMPLE_COND_Wait(&g_finishCond, &g_finishMutex);
    }
    SAMPLE_MUTEX_Unlock(&g_finishMutex);

    blobFree(g_gstTrackerBlobs);
    return s32Ret;
}

static HI_S32 ProcessGroupForward(HI_U64 u64StartFrame, HI_U64 u64EndFrame, const HI_CHAR *pcSrcFile,
                                  HI_PROPOSAL_Param_S *pCopParam, HI_RUNTIME_WK_INFO_S astWkInfo[WK_NUM])
{
    HI_RUNTIME_GROUP_HANDLE hGroupHandle;

    struct timespec start, next, end;
    clock_gettime(0, &start);

    HI_S32 s32Ret = HI_SVPRT_RUNTIME_Init(CPU_TASK_AFFINITY, NULL);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, s32Ret, "HI_SVPRT_RUNTIME_Init failed!\n");
    HI_SAMPLE_PC_MODEL_FILE_S stModelFile;
    stModelFile.pcModelFileRFCN = MODEL_RFCN_NAME;
    stModelFile.pcModelFileAlex = MODEL_ALEXNET_NAME;
    stModelFile.pcModelFileGoturn = MODEL_GOTURN_NAME;
    s32Ret = SAMPLE_RUNTIME_LoadModelGroup_RFCNGoTrunAlex(&stModelFile,
                                                          astWkInfo,
                                                          pCopParam,
                                                          &hGroupHandle);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "SAMPLE_RUNTIME_LoadModelGroup_RFCNGoTrunAlex failed!\n");

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Load");

    s32Ret = SAMPLE_RUNTIME_ForwardGroup_RFCNGoTurnAlex(pcSrcFile, hGroupHandle, u64StartFrame, u64EndFrame);
    SAMPLE_CHK_PRINTF((s32Ret != HI_SUCCESS), "SAMPLE_RUNTIME_ForwardGroup_Alexnet error\n");
    clock_gettime(0, &next);
    timeSpendMs(&end, &next, "Forward total");

    SAMPLE_CHK_PRINTF((HI_SVPRT_RUNTIME_UnloadModelGroup(hGroupHandle) != HI_SUCCESS),
                      "HI_SVPRT_RUNTIME_UnloadModelGroup error\n");

FAIL_0:
    HI_SVPRT_RUNTIME_DeInit();
    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Total");
    return s32Ret;
}

HI_S32 SAMPLE_Model_Group_RFCN_GOTURN_ALEXNET(HI_U64 u64StartFrame, HI_U64 u64EndFrame)
{
    const HI_CHAR *pcSrcFile = IMAGE_GOTURN_NAME;

    HI_PROPOSAL_Param_S copParam[COP_NUM] = {0};
    HI_RUNTIME_WK_INFO_S astWkInfo[WK_NUM] = {0};

    memset(&copParam[0], 0, sizeof(copParam));
    memset(&astWkInfo[0], 0, sizeof(astWkInfo));

    SAMPLE_MUTEX_Init(&g_finishMutex);
    SAMPLE_COND_Init(&g_finishCond);
    SAMPLE_LOG_INFO("\n======================= rfcn&goturn&alexnet group begin =========================\n");

    HI_S32 s32Ret = ProcessGroupForward(u64StartFrame, u64EndFrame, pcSrcFile, copParam, astWkInfo);

    releaseRfcnAndFrcnnCopParam(1, copParam);

    SAMPLE_FreeMem(&(astWkInfo[0].stWKMemory));
    SAMPLE_FreeMem(&(astWkInfo[1].stWKMemory));

    SAMPLE_MUTEX_Deinit(&g_finishMutex);
    SAMPLE_COND_Deinit(&g_finishCond);
    SAMPLE_LOG_INFO("SAMPLE_Model_Group_RFCN_GOTURN_ALEXNET result %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", s32Ret);
    return s32Ret;
}
