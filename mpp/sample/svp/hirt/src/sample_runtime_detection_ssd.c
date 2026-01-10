/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_runtime_detection_ssd.h"
#include <stdio.h>
#include <string.h>
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
#include "sample_memory_ops.h"
#include "sample_log.h"
#include "sample_runtime_define.h"
#include "math.h"
#include "sample_save_blob.h"
#include "sample_resize_roi.h"
#include "sample_data_utils.h"
#include "sample_model_ssd.h"

enum {
    SSD_IMAGE_WIDTH = 300,
    SSD_IMAGE_HEIGHT = 300,
    SSD_IMAGE_CHANNEL = 3,
    SSD_CONV4_LOC_CHANNEL = 38,
    SSD_CONV4_LOC_HEIGHT = 38,
    SSD_CONV4_LOC_WEIGHT = 16,
    SSD_CONV4_CONF_CHANNEL = 38,
    SSD_CONV4_CONF_HEIGHT = 38,
    SSD_CONV4_CONF_WEIGHT = 84,
    SSD_FC7_LOC_CHANNEL = 19,
    SSD_FC7_LOC_HEIGHT = 19,
    SSD_FC7_LOC_WEIGHT = 24,
    SSD_FC7_CONF_CHANNEL = 19,
    SSD_FC7_CONF_HEIGHT = 19,
    SSD_FC7_CONF_WEIGHT = 126,
    SSD_CONV6_LOC_CHANNEL = 10,
    SSD_IMAGE_CHANNELCONV6_LOC_HEIGHT = 10,
    SSD_CONV6_LOC_WEIGHT = 24,
    SSD_CONV6_CONF_CHANNEL = 10,
    SSD_CONV6_CONF_HEIGHT = 10,
    SSD_CONV6_CONF_WEIGHT = 126,
    SSD_CONV7_LOC_CHANNEL = 5,
    SSD_CONV7_LOC_HEIGHT = 5,
    SSD_CONV7_LOC_WEIGHT = 24,
    SSD_CONV7_CONF_CHANNEL = 5,
    SSD_CONV7_CONF_HEIGHT = 5,
    SSD_CONV7_CONF_WEIGHT = 126,
    SSD_CONV8_LOC_CHANNEL = 3,
    SSD_CONV8_LOC_HEIGHT = 3,
    SSD_CONV8_LOC_WEIGHT = 16,
    SSD_CONV8_CONF_CHANNEL = 3,
    SSD_CONV8_CONF_HEIGHT = 3,
    SSD_CONV8_CONF_WEIGHT = 84,
    SSD_CONV9_LOC_CHANNEL = 1,
    SSD_CONV9_LOC_HEIGHT = 1,
    SSD_CONV9_LOC_WEIGHT = 16,
    SSD_CONV9_CONF_CHANNEL = 1,
    SSD_CONV9_CONF_HEIGHT = 1,
    SSD_CONV9_CONF_WEIGHT = 84,
    SSD_OUTPUT_NUM = 12,
    SSD_CONV4_LOC_IDX = 0,
    SSD_CONV4_CONF_IDX = 1,
    SSD_FC7_LOC_IDX = 2,
    SSD_FC7_CONF_IDX = 3,
    SSD_CONV6_LOC_IDX = 4,
    SSD_CONV6_CONF_IDX = 5,
    SSD_CONV7_LOC_IDX = 6,
    SSD_CONV7_CONF_IDX = 7,
    SSD_CONV8_LOC_IDX = 8,
    SSD_CONV8_CONF_IDX = 9,
    SSD_CONV9_LOC_IDX = 10,
    SSD_CONV9_CONF_IDX = 11
};

HI_S32 SAMPLE_RUNTIME_LoadModelGroup_SSD(const HI_CHAR *pcModelFileSSD,
                                         HI_RUNTIME_WK_INFO_S *pstWKInfo,
                                         HI_RUNTIME_GROUP_HANDLE *phGroupHandle)
{
    HI_RUNTIME_GROUP_INFO_S stGroupInfo;
    HI_CHAR *pacConfig = NULL;

    memset(&stGroupInfo, 0, sizeof(HI_RUNTIME_GROUP_INFO_S));

    stGroupInfo.stWKsInfo.u32WKNum = 1;
    strncpy(pstWKInfo->acModelName, "ssd", MAX_NAME_LEN);
    HI_S32 s32Ret = SAMPLE_RUNTIME_LoadModelFile(pcModelFileSSD, &pstWKInfo->stWKMemory);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "LoadFile fail");
    stGroupInfo.stWKsInfo.pstAttrs = pstWKInfo;

    SAMPLE_RUNTIME_ReadConfig(CONFIG_DIR "ssd.modelgroup", &pacConfig);
    SAMPLE_CHK_GOTO((pacConfig == NULL), FAIL, "HI_SVPRT_RUNTIME_ReadConfig error\n");

    s32Ret = HI_SVPRT_RUNTIME_LoadModelGroup(pacConfig, &stGroupInfo, phGroupHandle);
    SAMPLE_CHK_GOTO((s32Ret != HI_SUCCESS), FAIL,
                    "HI_SVPRT_RUNTIME_LoadModelGroup error\n");

    SAMPLE_LOG_INFO("LoadGroup succ, group handle[%p]\n", phGroupHandle);
    SAMPLE_FREE(pacConfig);
    return HI_SUCCESS;

FAIL:
    SAMPLE_FREE(pacConfig);
    SAMPLE_FreeMem(&pstWKInfo->stWKMemory);
    return HI_FAILURE;
}

#if DEBUG
static HI_VOID SAMPLE_RUNTIME_DebugPrint_SSD(HI_RUNTIME_BLOB_S *pastDst)
{
    HI_CHAR *pcOutName = HI_NULL;
    for (int i = 0; i < SSD_OUTPUT_NUM; i++) {
        switch (i) {
            case SSD_CONV4_LOC_IDX:
                pcOutName = (HI_CHAR *)"conv4_3_norm_mbox_loc_perm";
                break;
            case SSD_CONV4_CONF_IDX:
                pcOutName = (HI_CHAR *)"conv4_3_norm_mbox_conf_perm";
                break;
            case SSD_FC7_LOC_IDX:
                pcOutName = (HI_CHAR *)"fc7_mbox_loc_perm";
                break;
            case SSD_FC7_CONF_IDX:
                pcOutName = (HI_CHAR *)"fc7_mbox_conf_perm";
                break;
            case SSD_CONV6_LOC_IDX:
                pcOutName = (HI_CHAR *)"conv6_2_mbox_loc_perm";
                break;
            case SSD_CONV6_CONF_IDX:
                pcOutName = (HI_CHAR *)"conv6_2_mbox_conf_perm";
                break;
            case SSD_CONV7_LOC_IDX:
                pcOutName = (HI_CHAR *)"conv7_2_mbox_loc_perm";
                break;
            case SSD_CONV7_CONF_IDX:
                pcOutName = (HI_CHAR *)"conv7_2_mbox_conf_perm";
                break;
            case SSD_CONV8_LOC_IDX:
                pcOutName = (HI_CHAR *)"conv8_2_mbox_loc_perm";
                break;
            case SSD_CONV8_CONF_IDX:
                pcOutName = (HI_CHAR *)"conv8_2_mbox_conf_perm";
                break;
            case SSD_CONV9_LOC_IDX:
                pcOutName = (HI_CHAR *)"conv9_2_mbox_loc_perm";
                break;
            case SSD_CONV9_CONF_IDX:
                pcOutName = (HI_CHAR *)"conv9_2_mbox_conf_perm";
                break;
            default:
                break;
        }
        printDebugData(pcOutName, pastDst[i].u64VirAddr, 10);
    }
}
#endif

static BlobInfo s_stSrcBlobInfo = {
    "", "data", "",
    HI_RUNTIME_BLOB_TYPE_U8,
    { 1, 3, 300, 300 },
    HI_TRUE, ALIGN_16
};

static BlobInfo s_astDstBlobInfo[] = {
    {   "ssd", "conv4_3_norm_mbox_loc_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 38, 38, 16 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv4_3_norm_mbox_conf_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 38, 38, 84 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "fc7_mbox_loc_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 19, 19, 24 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "fc7_mbox_conf_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 19, 19, 126 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv6_2_mbox_loc_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 10, 10, 24 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv6_2_mbox_conf_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 10, 10, 126 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv7_2_mbox_loc_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 5, 5, 24 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv7_2_mbox_conf_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 5, 5, 126 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv8_2_mbox_loc_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 3, 3, 16 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv8_2_mbox_conf_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 3, 3, 84 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv9_2_mbox_loc_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 1, 1, 16 },
        HI_FALSE, ALIGN_16
    },
    {   "ssd", "conv9_2_mbox_conf_perm", "",
        HI_RUNTIME_BLOB_TYPE_S32,
        { 1, 1, 1, 84 },
        HI_FALSE, ALIGN_16
    }
};

HI_S32 SAMPLE_RUNTIME_ForwardGroup_SSD(const HI_CHAR *pcSrcFile, HI_RUNTIME_GROUP_HANDLE hGroupHandle)
{
    HI_S32 as32ResultROI[200 * SVP_WK_PROPOSAL_WIDTH] = {0}; // 200 is ssd max bbox num
    HI_U32 u32ResultROICnt = 0;

    HI_RUNTIME_GROUP_SRC_BLOB_ARRAY_S stGroupSrcBlob;
    HI_RUNTIME_GROUP_DST_BLOB_ARRAY_S stGroupDstBlob;

    strncpy(s_stSrcBlobInfo.acSrcFilePath, pcSrcFile, sizeof(s_stSrcBlobInfo.acSrcFilePath));
    s_stSrcBlobInfo.acSrcFilePath[sizeof(s_stSrcBlobInfo.acSrcFilePath) - 1] = '\0';
    HI_S32 s32Ret = InitBlobs(&stGroupSrcBlob, 1, &s_stSrcBlobInfo);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "InitBlobs fail");
    s32Ret = InitBlobs(&stGroupDstBlob, SSD_OUTPUT_NUM, s_astDstBlobInfo);
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
        (end.tv_nsec - start.tv_nsec) / NANOSECOND_PER_MICROSECOND;
    printf("\n\n[Forward]===== TIME SPEND: %ldms, %ldus =====\n\n", spend / MICROSECOND_PER_MILLISECOND, spend);
#endif
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, DEINIT_DST_BLOBS,
                    "HI_SVPRT_RUNTIME_ForwardGroupSync failed!\n");

    SAMPLE_LOG_INFO("Pic: %s\n", pcSrcFile);
    SAMPLE_Ssd_GetResult(stGroupSrcBlob.pstBlobs->pstBlob, stGroupDstBlob.pstBlobs->pstBlob,
        as32ResultROI, &u32ResultROICnt);
    SAMPLE_LOG_INFO("roi cnt: %d\n", u32ResultROICnt);
    drawImageRect("ssd", stGroupSrcBlob.pstBlobs->pstBlob, as32ResultROI, u32ResultROICnt, 6);
#if DEBUG
    SAMPLE_RUNTIME_DebugPrint_SSD(astDst);
#endif

DEINIT_DST_BLOBS:
    DeinitBlobs(&stGroupDstBlob);
DEINIT_SRC_BLOBS:
    DeinitBlobs(&stGroupSrcBlob);
    return s32Ret;
}

HI_S32 SAMPLE_SSD()
{
    const HI_CHAR *pcRuntimeModelName = MODEL_SSD_NAME;
    const HI_CHAR *pcSrcFile = IMAGE_SSD_NAME;
    HI_RUNTIME_WK_INFO_S stWKInfo;

    HI_RUNTIME_GROUP_HANDLE hGroupHandle;

    memset(&stWKInfo, 0, sizeof(HI_RUNTIME_WK_INFO_S));

    struct timespec start, next, end;
    clock_gettime(0, &start);

    printf("\n============================= ssd net begin ================================\n");
    HI_S32 s32Ret = HI_SVPRT_RUNTIME_Init(CPU_TASK_AFFINITY, NULL);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_1, "HI_SVPRT_RUNTIME_Init failed!\n");

    s32Ret = SAMPLE_RUNTIME_LoadModelGroup_SSD(pcRuntimeModelName, &stWKInfo, &hGroupHandle);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0,
                    "SAMPLE_RUNTIME_LoadModelGroup_SSD failed!\n");

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Load");

    s32Ret = SAMPLE_RUNTIME_ForwardGroup_SSD(pcSrcFile, hGroupHandle);
    SAMPLE_CHK_PRINTF((s32Ret != HI_SUCCESS), "SAMPLE_RUNTIME_ForwardGroup_SSD error\n");

    clock_gettime(0, &next);
    timeSpendMs(&end, &next, "Forward total");

    SAMPLE_CHK_PRINTF((HI_SVPRT_RUNTIME_UnloadModelGroup(hGroupHandle) != HI_SUCCESS),
                      "HI_SVPRT_RUNTIME_UnloadModelGroup error\n");

FAIL_0:
    SAMPLE_FreeMem(&stWKInfo.stWKMemory);
    (HI_VOID)HI_SVPRT_RUNTIME_DeInit();

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Total");
FAIL_1:
    SAMPLE_LOG_INFO("SAMPLE_SSD result %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", s32Ret);
    return s32Ret;
}
