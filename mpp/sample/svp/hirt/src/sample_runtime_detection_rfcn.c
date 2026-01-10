/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: rfcn sample
 * Author:
 * Create: 2018-05-19
 */

#include "sample_runtime_detection_rfcn.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef ON_BOARD
#include "mpi_sys.h"
#include "mpi_vb.h"
#else
#include "hi_comm_svp.h"
#include "hi_nnie.h"
#include "mpi_nnie.h"
#endif
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
#include "sample_data_utils.h"

HI_S32 SAMPLE_RUNTIME_LoadModelGroup_RFCN(const HI_CHAR *pcModelFileRFCN,
                                          HI_RUNTIME_WK_INFO_S *pstWkInfo,
                                          HI_PROPOSAL_Param_S *copParam,
                                          HI_RUNTIME_GROUP_HANDLE *phGroupHandle)
{
    HI_RUNTIME_GROUP_INFO_S stGroupInfo = {0};
    HI_CHAR *pacConfig = NULL;

    memset(&stGroupInfo, 0, sizeof(HI_RUNTIME_GROUP_INFO_S));

    strncpy(pstWkInfo[0].acModelName, "rfcn", MAX_NAME_LEN);
    HI_S32 s32Ret = SAMPLE_RUNTIME_LoadModelFile(pcModelFileRFCN, &(pstWkInfo[0].stWKMemory));
    SAMPLE_CHK_GOTO((s32Ret != HI_SUCCESS), FAIL,
                    "SAMPLE_RUNTIME_LoadModelFile %s failed!\n", pcModelFileRFCN);
    stGroupInfo.stWKsInfo.u32WKNum = 1;
    stGroupInfo.stWKsInfo.pstAttrs = &(pstWkInfo[0]);

    HI_RUNTIME_COP_ATTR_S stCopAttr;
    memset(&stCopAttr, 0, sizeof(HI_RUNTIME_COP_ATTR_S));
    strncpy(stCopAttr.acModelName, "rfcn", MAX_NAME_LEN);
    strncpy(stCopAttr.acCopName, "proposal", MAX_NAME_LEN);
    stCopAttr.u32ConstParamSize = sizeof(HI_PROPOSAL_Param_S);
    s32Ret = createRfcnAndFasterrcnnCopParam(1, &stCopAttr, copParam, SAMPLE_WK_DETECT_NET_RFCN);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL,
                    "createRfcnAndFasterrcnnCopParam failed!\n");
    stGroupInfo.stCopsAttr.u32CopNum = 1;
    stGroupInfo.stCopsAttr.pstAttrs = &stCopAttr;
    SAMPLE_RUNTIME_ReadConfig(CONFIG_DIR "rfcn.modelgroup", &pacConfig);
    SAMPLE_CHK_GOTO(pacConfig == NULL, FAIL, "HI_SVPRT_RUNTIME_ReadConfig error\n");

    s32Ret = HI_SVPRT_RUNTIME_LoadModelGroup(pacConfig, &stGroupInfo, phGroupHandle);
    SAMPLE_CHK_GOTO((s32Ret != HI_SUCCESS), FAIL,
                    "HI_SVPRT_RUNTIME_LoadModelGroup error\n");

    SAMPLE_LOG_INFO("LoadGroup succ, group handle[%p]\n", phGroupHandle);

    SAMPLE_FREE(pacConfig);
    return HI_SUCCESS;
FAIL:
    SAMPLE_FREE(pacConfig);
    return HI_FAILURE;
}

HI_VOID PostProcess(const HI_CHAR *pcSrcFile,
                    HI_RUNTIME_GROUP_BLOB_S *pstInputBlob, HI_RUNTIME_GROUP_BLOB_S *pstOutputBlob)
{
    HI_S32 as32ResultROI[MAX_ROI_NUM * SVP_WK_PROPOSAL_WIDTH] = {0};
    HI_U32 u32ResultROICnt = 0;

    SAMPLE_RUNTIME_ROI_PARAM_S stRoiParam = {
        pstOutputBlob[1].pstBlob,
        pstOutputBlob[2].pstBlob,
        pstOutputBlob[0].pstBlob,
        pstInputBlob[0].pstBlob,
    };
    HI_S32 s32Ret = SAMPLE_DATA_GetRoiResult(SAMPLE_RUNTIME_MODEL_TYPE_RFCN,
                                             &stRoiParam, as32ResultROI, &u32ResultROICnt);
    SAMPLE_CHK_PRINTF(s32Ret != HI_SUCCESS,
                      "SAMPLE_DATA_GetRoiResult failed!\n");
    SAMPLE_LOG_INFO("====Pic: %s====\nroi cnt: %d\n", pcSrcFile, u32ResultROICnt);
    for (HI_U32 i = 0; i < u32ResultROICnt; i++) {
        // 0.8 is for rfcn to use depends on your config
        if (as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] > 0.8 * SVP_WK_QUANT_BASE) {
            SAMPLE_LOG_INFO("%d %d %d %d %lf(%d)\n",
                            as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + X_MIN_IDX],
                            as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + Y_MIN_IDX],
                            as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + X_MAX_IDX],
                            as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + Y_MAX_IDX],
                            (HI_DOUBLE)(as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX] *
                            ((HI_DOUBLE)1.0) / SVP_WK_QUANT_BASE),
                            as32ResultROI[i * SVP_WK_PROPOSAL_WIDTH + SCORE_IDX]);
        }
    }
    SAMPLE_LOG_INFO("====detection info end====\n");

    drawImageRect("rfcn", pstInputBlob[0].pstBlob, as32ResultROI, u32ResultROICnt, SVP_WK_PROPOSAL_WIDTH);

#if DEBUG
    HI_CHAR apcLayerName[3][64] = { "rois", "cls_prob", "bbox_pred" }; // 3 dst_blob,64 is blob name max length
    for (HI_U32 i = 0; i < 3; i++) { // 3 dst_blob
        printDebugData(apcLayerName[i], stDst[i].u64VirAddr, 10); // Debug print 10
    }

    printf("proposal bbox num is: %d\n", stDst[0].unShape.stWhc.u32Height);
#endif
}

static BlobInfo s_stSrcBlobInfo = {
    "", "data", "",
    HI_RUNTIME_BLOB_TYPE_U8,
    { 1, 3, 600, 800 },
    HI_TRUE, ALIGN_16
};

static BlobInfo s_astDstBlobInfo[] = {
    {   "rfcn", "rois", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 1, 300, 4 },
        HI_FALSE, ALIGN_16
    },
    {   "rfcn", "cls_prob", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 300, 1, 1, 21 },
        HI_FALSE, ALIGN_16
    },
    {   "rfcn", "bbox_pred", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 300, 1, 1, 8 },
        HI_FALSE, ALIGN_16
    }
};

HI_S32 SAMPLE_RUNTIME_ForwardGroup_RFCN(const HI_CHAR *pcSrcFile, HI_RUNTIME_GROUP_HANDLE hGroupHandle)
{
#if DEBUG
    HI_CHAR *pcOutName = HI_NULL;
#endif
    HI_RUNTIME_GROUP_SRC_BLOB_ARRAY_S stGroupSrcBlob;
    HI_RUNTIME_GROUP_DST_BLOB_ARRAY_S stGroupDstBlob;

    strncpy(s_stSrcBlobInfo.acSrcFilePath, pcSrcFile, sizeof(s_stSrcBlobInfo.acSrcFilePath));
    s_stSrcBlobInfo.acSrcFilePath[sizeof(s_stSrcBlobInfo.acSrcFilePath) - 1] = '\0';
    HI_S32 s32Ret = InitBlobs(&stGroupSrcBlob, 1, &s_stSrcBlobInfo);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "InitBlobs fail");
    s32Ret = InitBlobs(&stGroupDstBlob, 3, s_astDstBlobInfo); // 3 dst_blob
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, DEINIT_SRC_BLOBS, "InitBlobs fail");

#if PERFORMANCE_TEST
    long spend;
    struct timespec start, end;
    clock_gettime(0, &start);
#endif
    s32Ret = HI_SVPRT_RUNTIME_ForwardGroupSync(hGroupHandle, &stGroupSrcBlob, &stGroupDstBlob, 0);
#if PERFORMANCE_TEST
    clock_gettime(0, &end);
    spend = (end.tv_sec - start.tv_sec) * MICROSECOND_PER_SECOND +
            (end.tv_nsec - start.tv_nsec) / MILLISECOND_PER_SECOND;
    printf("\n\n[Forward]===== TIME SPEND: %ldms, %ldus =====\n\n", spend / MILLISECOND_PER_SECOND, spend);
#endif
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, DEINIT_DST_BLOBS,
                    "HI_SVPRT_RUNTIME_ForwardGroupSync failed!\n");

    PostProcess(pcSrcFile, stGroupSrcBlob.pstBlobs, stGroupDstBlob.pstBlobs);
DEINIT_DST_BLOBS:
    DeinitBlobs(&stGroupDstBlob);
DEINIT_SRC_BLOBS:
    DeinitBlobs(&stGroupSrcBlob);
    return s32Ret;
}

HI_S32 SAMPLE_RFCN()
{
    const HI_CHAR *pcRuntimeModelName = MODEL_RFCN_NAME;
    const HI_CHAR *pcSrcFile = IMAGE_RFCN_NAME;

    HI_RUNTIME_GROUP_HANDLE hGroupHandle;
    HI_PROPOSAL_Param_S copParam[1] = {0};
    HI_RUNTIME_WK_INFO_S astWkInfo[1] = {0};
    memset(&copParam[0], 0, sizeof(copParam));
    memset(&astWkInfo[0], 0, sizeof(astWkInfo));

    long spend;
    struct timespec start, next, end;
    clock_gettime(0, &start);

    printf("\n============================= rfcn net begin ================================\n");
    HI_S32 s32Ret = HI_SVPRT_RUNTIME_Init(CPU_TASK_AFFINITY, NULL);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_1, "HI_SVPRT_RUNTIME_Init failed!\n");

    s32Ret = SAMPLE_RUNTIME_LoadModelGroup_RFCN(pcRuntimeModelName, astWkInfo, copParam, &hGroupHandle);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0,
                    "SAMPLE_RUNTIME_LoadModelGroup_RFCN failed!\n");

    clock_gettime(0, &end);
    spend = ((long)end.tv_sec - (long)start.tv_sec) * MILLISECOND_PER_SECOND + ((long)end.tv_nsec -
        (long)start.tv_nsec) / MICROSECOND_PER_SECOND;
    SAMPLE_LOG_INFO("\n[Load]===== TIME SPEND: %ld ms =====\n", spend);

    s32Ret = SAMPLE_RUNTIME_ForwardGroup_RFCN(pcSrcFile, hGroupHandle);
    SAMPLE_CHK_PRINTF((s32Ret != HI_SUCCESS), "SAMPLE_RUNTIME_ForwardGroup_RFCN error\n");

    clock_gettime(0, &next);
    spend = ((long)next.tv_sec - (long)end.tv_sec) * MILLISECOND_PER_SECOND + ((long)next.tv_nsec -
        (long)end.tv_nsec) / MICROSECOND_PER_SECOND;
    SAMPLE_LOG_INFO("\n[Forward total]===== TIME SPEND: %ld ms =====\n", spend);

    SAMPLE_CHK_PRINTF((HI_SVPRT_RUNTIME_UnloadModelGroup(hGroupHandle) != HI_SUCCESS),
                      "HI_SVPRT_RUNTIME_UnloadModelGroup error\n");

FAIL_0:
    HI_SVPRT_RUNTIME_DeInit();
    releaseRfcnAndFrcnnCopParam(1, copParam);
    SAMPLE_FreeMem(&(astWkInfo[0].stWKMemory));

    clock_gettime(0, &end);
    spend = ((long)end.tv_sec - (long)start.tv_sec) * MILLISECOND_PER_SECOND + ((long)end.tv_nsec -
        (long)start.tv_nsec) / MICROSECOND_PER_SECOND;
    SAMPLE_LOG_INFO("\n[Total]===== TIME SPEND: %ld ms =====\n", spend);
FAIL_1:
    SAMPLE_LOG_INFO("SAMPLE_RFCN result %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", s32Ret);
    return s32Ret;
}
