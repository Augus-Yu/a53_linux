/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_runtime_group_rfcnalex.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef ON_BOARD
#include "mpi_sys.h"
#include "mpi_vb.h"
#else
#include "hi_comm_svp.h"
#include "hi_nnie.h"
#include "mpi_nnie.h"
#endif
#include <sample_mutex_ops.h>
#include "hi_runtime_api.h"
#include "string.h"
#include "sample_log.h"
#include "sample_runtime_define.h"
#include "sample_memory_ops.h"
#include "math.h"
#include "sample_save_blob.h"
#include "sample_resize_roi.h"
#include "sample_model_rcnn.h"
#include "sample_cop_param.h"
#include "sample_model_classification.h"

const HI_U32 CONNECTOR_SRC_NUM = 4;
static const HI_U32 CONNECTOR_SRC_DATA_IDX = 0;
static const HI_U32 CONNECTOR_SRC_ROI_IDX = 1;
static const HI_U32 CONNECTOR_SRC_CLS_IDX = 2;
static const HI_U32 CONNECTOR_SRC_BBOX_IDX = 3;

typedef struct hiSAMPLE_PC_MODEL_FILE_S {
    HI_CHAR *pcModelFileRFCN;
    HI_CHAR *pcModelFileAlex;
} HI_SAMPLE_PC_MODEL_FILE_S;

static HI_U32 g_u32RoiNum = 0;

static HI_VOID DebugData(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc)
{
#if DEBUG
    HI_CHAR *pcOutName = HI_NULL;
    HI_U32 i = 0;

    for (i = 0; i < CONNECTOR_SRC_NUM; i++) {
        if (i == CONNECTOR_SRC_DATA_IDX) {
            pcOutName = (HI_CHAR *)"data";
        }

        if (i == CONNECTOR_SRC_ROI_IDX) {
            pcOutName = (HI_CHAR *)"rois";
        }

        if (i == CONNECTOR_SRC_CLS_IDX) {
            pcOutName = (HI_CHAR *)"cls_prob";
        }

        if (i == CONNECTOR_SRC_BBOX_IDX) {
            pcOutName = (HI_CHAR *)"bbox_pred";
        }

        printDebugData(pcOutName, pstConnectorSrc->pstBlobs[i].u64VirAddr, 10); // debug print 10
    }
#endif
    return;
}

static HI_VOID DebugROIImage(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorDst)
{
#if DEBUG
    HI_CHAR aszResizeFileName[RFCN_ALEX_FILE_NAME_LENGTH];

    for (i = 0; i < pstConnectorDst->pstBlobs[0].u32Num; i++) {
        snprintf(aszResizeFileName, sizeof(aszResizeFileName), "%d_ra", i);
        saveBlob(aszResizeFileName, &pstConnectorDst->pstBlobs[0], i);
    }

    FILE *fp = NULL;
    HI_RUNTIME_BLOB_S *pstBlob = &pstConnectorDst->pstBlobs[0];
    fp = fopen("rfcn_out.bgr", "w");
    SAMPLE_CHK_RET((fp == HI_NULL), HI_FAILURE, "open rfcn_out.bgr failed\n");
    HI_U32 c, h;
    HI_U8 *pu8vir = NULL;
    pu8vir = (HI_U8 *)((uintptr_t)(pstConnectorDst->pstBlobs[0].u64VirAddr));

    for (c = 0; c < pstBlob->unShape.stWhc.u32Chn; c++)
        for (h = 0; h < pstBlob->unShape.stWhc.u32Height; h++) {
            fwrite(pu8vir, 1, pstBlob->unShape.stWhc.u32Width * sizeof(HI_U8), fp);
            pu8vir += pstBlob->u32Stride;
        }

    fclose(fp);
#endif

}

static HI_VOID DebugROI(HI_S32 as32ResultROI[MAX_ROI_NUM * SVP_WK_PROPOSAL_WIDTH], HI_U32 u32ResultROICnt)
{
#if DEBUG
    for (i = 0; i < u32ResultROICnt; i++) {
        printf("ROI[%d]: x1=%d y1=%d x2=%d y2=%d\n", i,
               as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH],
               as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + Y_MIN_IDX],
               as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + X_MAX_IDX],
               as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + Y_MAX_IDX]);
    }
#endif
}

static HI_S32 Connector_RFCNToAlexNet(HI_RUNTIME_SRC_BLOB_ARRAY_PTR pstConnectorSrc,
                                      HI_RUNTIME_DST_BLOB_ARRAY_PTR pstConnectorDst, HI_U64 u64SrcId, HI_VOID *pParam)
{
    HI_S32 as32ResultROI[MAX_ROI_NUM * SVP_WK_PROPOSAL_WIDTH] = {0};
    HI_U32 u32ResultROICnt = 0;
    HI_U32 *pu32RoiNum = (HI_U32 *)pParam;

    struct timespec start, end;
    clock_gettime(0, &start);

    DebugData(pstConnectorSrc);
    SAMPLE_RUNTIME_ROI_PARAM_S stRoiParam = {
        &pstConnectorSrc->pstBlobs[CONNECTOR_SRC_CLS_IDX], &pstConnectorSrc->pstBlobs[CONNECTOR_SRC_BBOX_IDX],
        &pstConnectorSrc->pstBlobs[CONNECTOR_SRC_ROI_IDX], &pstConnectorSrc->pstBlobs[CONNECTOR_SRC_DATA_IDX],
    };
    HI_S32 s32Ret = SAMPLE_DATA_GetRoiResult(SAMPLE_RUNTIME_MODEL_TYPE_RFCN,
        &stRoiParam,
        as32ResultROI,
        &u32ResultROICnt);
    SAMPLE_CHK_RET((s32Ret != HI_SUCCESS), HI_FAILURE, "SAMPLE_DATA_GetRoiResult failed\n");
    printf("roi cnt: %u\n", u32ResultROICnt);
    HI_U32 u32TempCnt = u32ResultROICnt;
    *pu32RoiNum = u32ResultROICnt;

    DebugROI(as32ResultROI, u32ResultROICnt);

    while (u32TempCnt > SAMPLE_IVE_RESIZE_BATCH_MAX) {
        resizeROI(&pstConnectorSrc->pstBlobs[0],
                  &as32ResultROI[(u32ResultROICnt - u32TempCnt) * SVP_WK_PROPOSAL_WIDTH],
                  SVP_WK_PROPOSAL_WIDTH,
                  SAMPLE_IVE_RESIZE_BATCH_MAX,
                  &pstConnectorDst->pstBlobs[0],
                  u32ResultROICnt - u32TempCnt);
        u32TempCnt -= SAMPLE_IVE_RESIZE_BATCH_MAX;
    }

    resizeROI(&pstConnectorSrc->pstBlobs[0],
              &as32ResultROI[(u32ResultROICnt - u32TempCnt) * SVP_WK_PROPOSAL_WIDTH],
              SVP_WK_PROPOSAL_WIDTH,
              (HI_U16)u32TempCnt,
              &pstConnectorDst->pstBlobs[0],
              u32ResultROICnt - u32TempCnt);
    pstConnectorDst->pstBlobs[0].u32Num = u32ResultROICnt;

    DebugROIImage(pstConnectorDst);

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Connector");

    return HI_SUCCESS;
}

static HI_RUNTIME_GROUP_INFO_S g_stGroupInfo;

static HI_S32 SAMPLE_RUNTIME_LoadModelGroup_RFCNAlex(HI_SAMPLE_PC_MODEL_FILE_S *pstModelFile,
                                                     HI_RUNTIME_WK_INFO_S astWkInfo[RFCN_ALEX_WK_NUM],
                                                     HI_PROPOSAL_Param_S *pProposalParam,
                                                     HI_RUNTIME_GROUP_HANDLE *phGroupHandle)
{
    HI_RUNTIME_COP_ATTR_S stProposalAttr = {0};
    HI_RUNTIME_CONNECTOR_ATTR_S stConnectorAttr = {0};
    HI_CHAR *pacConfig = NULL;

    memset(&stProposalAttr, 0, sizeof(HI_RUNTIME_COP_ATTR_S));
    memset(&stConnectorAttr, 0, sizeof(HI_RUNTIME_CONNECTOR_ATTR_S));
    // wk mem
    strncpy(astWkInfo[0].acModelName, "rfcn", MAX_NAME_LEN);
    HI_S32 s32Ret = SAMPLE_RUNTIME_LoadModelFile(pstModelFile->pcModelFileRFCN, &astWkInfo[0].stWKMemory);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL, "SAMPLE_RUNTIME_LoadModelFile %s failed!\n",
        pstModelFile->pcModelFileRFCN);
    strncpy(astWkInfo[1].acModelName, "alexnet", MAX_NAME_LEN);
    s32Ret = SAMPLE_RUNTIME_LoadModelFile(pstModelFile->pcModelFileAlex, &astWkInfo[1].stWKMemory);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL, "SAMPLE_RUNTIME_LoadModelFile %s failed!\n",
        pstModelFile->pcModelFileAlex);

    // cop param
    strncpy(stProposalAttr.acModelName, "rfcn", MAX_NAME_LEN);
    strncpy(stProposalAttr.acCopName, "proposal", MAX_NAME_LEN);
    stProposalAttr.u32ConstParamSize = sizeof(HI_PROPOSAL_Param_S);
    s32Ret = createRfcnAndFasterrcnnCopParam(1, &stProposalAttr, pProposalParam, SAMPLE_WK_DETECT_NET_RFCN);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL, "createRfcnAndFasterrcnnCopParam failed!\n");

    // conector
    strncpy(stConnectorAttr.acName, "rfcn_conn_alexnet", MAX_NAME_LEN);
    stConnectorAttr.pParam = &g_u32RoiNum;
    stConnectorAttr.pConnectorFun = Connector_RFCNToAlexNet;

    SAMPLE_RUNTIME_ReadConfig(CONFIG_DIR "rfcn_alexnet.modelgroup", &pacConfig);
    SAMPLE_CHK_GOTO(pacConfig == NULL, READCONFIG_FAIL, "HI_SVPRT_RUNTIME_ReadConfig failed!\n");

    g_stGroupInfo.stWKsInfo.u32WKNum = RFCN_ALEX_WK_NUM;
    g_stGroupInfo.stWKsInfo.pstAttrs = &astWkInfo[0];

    g_stGroupInfo.stCopsAttr.u32CopNum = 1;
    g_stGroupInfo.stCopsAttr.pstAttrs = &stProposalAttr;

    g_stGroupInfo.stConnectorsAttr.u32ConnectorNum = 1;
    g_stGroupInfo.stConnectorsAttr.pstAttrs = &stConnectorAttr;

    s32Ret = HI_SVPRT_RUNTIME_LoadModelGroup(pacConfig, &g_stGroupInfo, phGroupHandle);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, LOADMODEL_FAIL, "HI_SVPRT_RUNTIME_LoadModelGroupSync failed!\n");

    SAMPLE_LOG_INFO("LoadGroup succ\n");

    SAMPLE_FREE(pacConfig);
    return s32Ret;

FAIL:
    return s32Ret;

READCONFIG_FAIL:
    return HI_FAILURE;

LOADMODEL_FAIL:
    SAMPLE_FREE(pacConfig);
    return s32Ret;
}

static HI_BOOL g_bFinish = HI_FALSE;
static HI_BOOL g_bResult = HI_TRUE;
static SAMPLE_COND g_finishCond;
static SAMPLE_MUTEX g_finishMutex;

static HI_S32 rfcn_alexnet_forward_finish(HI_RUNTIME_FORWARD_STATUS_CALLBACK_E enEvent,
                                          HI_RUNTIME_GROUP_HANDLE hGroupHandle,
                                          HI_U64 u64FrameId,
                                          HI_RUNTIME_GROUP_DST_BLOB_ARRAY_PTR pstDstBlobs)
{
    printf("forward finish: %llu\n", u64FrameId);
    SAMPLE_MUTEX_Lock(&g_finishMutex);
    g_bFinish = HI_TRUE;
    if (enEvent != HI_RUNTIME_FORWARD_STATUS_SUCC) {
        g_bResult = HI_FALSE;
    }
    SAMPLE_COND_Broadcast(&g_finishCond);
    SAMPLE_MUTEX_Unlock(&g_finishMutex);
    return HI_SUCCESS;
}

BlobInfo s_stSrcBlobInfo = {
    "", "data", "",
    HI_RUNTIME_BLOB_TYPE_U8,
    { 1, 3, 600, 800 },
    HI_TRUE, ALIGN_16
};

BlobInfo s_stDstBlobInfo = {
    "alexnet", "prob", "",
    HI_RUNTIME_BLOB_TYPE_VEC_S32,
    { MAX_ROI_NUM, ALEXNET_OUTPUT_BLOB_CHANNEL, 1, 1 },
    HI_FALSE, ALIGN_16
};

static HI_S32 SAMPLE_RUNTIME_ForwardGroup_RFCNAlex(const HI_CHAR *pcSrcFile, HI_RUNTIME_GROUP_HANDLE hGroupHandle)
{
    HI_RUNTIME_GROUP_SRC_BLOB_ARRAY_S stGroupSrc;
    HI_RUNTIME_GROUP_DST_BLOB_ARRAY_S stGroupDst;

    strncpy(s_stSrcBlobInfo.acSrcFilePath, pcSrcFile, sizeof(s_stSrcBlobInfo.acSrcFilePath));
    s_stSrcBlobInfo.acSrcFilePath[sizeof(s_stSrcBlobInfo.acSrcFilePath) - 1] = '\0';
    HI_S32 s32Ret = InitBlobs(&stGroupSrc, 1, &s_stSrcBlobInfo);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "InitBlobs src fail");
    s32Ret = InitBlobs(&stGroupDst, 1, &s_stDstBlobInfo);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, DEINIT_SRC_BLOBS, "InitBlobs dst fail");

#if PERFORMANCE_TEST
    struct timespec start, end;
    clock_gettime(0, &start);
#endif

    s32Ret = HI_SVPRT_RUNTIME_ForwardGroupASync(hGroupHandle, &stGroupSrc, &stGroupDst, 0,
                                                rfcn_alexnet_forward_finish);

    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, DEINIT_DST_BLOBS, "Forward fail");

    {
        SAMPLE_MUTEX_Lock(&g_finishMutex);

        while (!g_bFinish) {
            SAMPLE_COND_Wait(&g_finishCond, &g_finishMutex);
        }
        SAMPLE_MUTEX_Unlock(&g_finishMutex);
    }

    {
        SAMPLE_MUTEX_Lock(&g_finishMutex);
        g_bFinish = HI_FALSE;
        if (g_bResult == HI_FALSE) {
            g_bResult = HI_TRUE;
            s32Ret = HI_FAILURE;
        }
        SAMPLE_MUTEX_Unlock(&g_finishMutex);
    }

    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, DEINIT_DST_BLOBS, "forward failed!\n");

#if PERFORMANCE_TEST
    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Forward");
#endif

    stGroupDst.pstBlobs[0].pstBlob->u32Num = g_u32RoiNum;
    s32Ret = SAMPLE_RUNTIME_Cnn_TopN_Output(stGroupDst.pstBlobs[0].pstBlob, 1);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, DEINIT_DST_BLOBS, "SAMPLE_RUNTIME_Cnn_TopN_Output failed!\n");
DEINIT_DST_BLOBS:
    DeinitBlobs(&stGroupDst);
DEINIT_SRC_BLOBS:
    DeinitBlobs(&stGroupSrc);
    return s32Ret;
}

// fasterRcnn->connector->alexNet
HI_S32 SAMPLE_Model_Group_RFCNAlexNet()
{
    const HI_CHAR *pcSrcFile = IMAGE_RFCN_NAME;
    struct timespec start, next, end;
    clock_gettime(0, &start);

    HI_RUNTIME_GROUP_HANDLE hGroupHandle = HI_NULL;
    HI_PROPOSAL_Param_S copParam[1];
    HI_RUNTIME_WK_INFO_S astWkInfo[RFCN_ALEX_WK_NUM];

    memset(&copParam[0], 0, sizeof(copParam));
    memset(&astWkInfo[0], 0, sizeof(astWkInfo));

    SAMPLE_MUTEX_Init(&g_finishMutex);
    SAMPLE_COND_Init(&g_finishCond);
    printf("\n============================= rfcn & alex net group begin ================================\n");
    HI_S32 s32Ret = HI_SVPRT_RUNTIME_Init(CPU_TASK_AFFINITY, NULL);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_1, "HI_SVPRT_RUNTIME_Init failed!\n");
    HI_SAMPLE_PC_MODEL_FILE_S stModelFile;
    stModelFile.pcModelFileRFCN = MODEL_RFCN_NAME;
    stModelFile.pcModelFileAlex = MODEL_ALEXNET_NAME;
    s32Ret = SAMPLE_RUNTIME_LoadModelGroup_RFCNAlex(&stModelFile,
                                                    astWkInfo,
                                                    copParam,
                                                    &hGroupHandle);
    printf("============loadmodel hGroupHandle[%p]\n", hGroupHandle);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "SAMPLE_RUNTIME_LoadModelGroup failed!\n");

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Load");

    printf("============forward hGroupHandle[%p]\n", hGroupHandle);
    s32Ret = SAMPLE_RUNTIME_ForwardGroup_RFCNAlex(pcSrcFile, hGroupHandle);
    SAMPLE_CHK_PRINTF((s32Ret != HI_SUCCESS), "SAMPLE_RUNTIME_ForwardGroup_Alexnet error\n");

    clock_gettime(0, &next);
    timeSpendMs(&end, &next, "Forward");

    SAMPLE_CHK_PRINTF((HI_SVPRT_RUNTIME_UnloadModelGroup(hGroupHandle) != HI_SUCCESS),
                      "HI_SVPRT_RUNTIME_UnloadModelGroup error\n");

FAIL_0:
    HI_SVPRT_RUNTIME_DeInit();

    releaseRfcnAndFrcnnCopParam(1, copParam);

    SAMPLE_FreeMem(&(astWkInfo[0].stWKMemory));
    SAMPLE_FreeMem(&(astWkInfo[1].stWKMemory));

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Total");
FAIL_1:
    SAMPLE_MUTEX_Deinit(&g_finishMutex);
    SAMPLE_COND_Deinit(&g_finishCond);
    SAMPLE_LOG_INFO("SAMPLE_Model_Group_RFCNAlexNet result %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", s32Ret);
    return s32Ret;
}
