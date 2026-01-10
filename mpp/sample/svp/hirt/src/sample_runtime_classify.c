/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_runtime_classify.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
#include "sample_data_utils.h"
#include "sample_model_classification.h"

static const HI_U32 ALEXNET_OUTPUT_TOP_FIVE = 5;
const HI_U32 ALEXNET_DEBUG_PRINT_NUM = 10;

static BlobInfo s_stSrcBlobInfo = {
    "", "data", "",
    HI_RUNTIME_BLOB_TYPE_U8,
    { 1, 3, 227, 227 },
    HI_TRUE, ALIGN_16
};

static BlobInfo s_stDstBlobInfo = {
    "alexnet", "prob", "",
    HI_RUNTIME_BLOB_TYPE_VEC_S32,
    { 1, 1000, 1, 1 },
    HI_FALSE, ALIGN_16
};

HI_S32 SAMPLE_RUNTIME_LoadModelGroup_Alexnet(const HI_CHAR *pcModelFileAlex, HI_RUNTIME_WK_INFO_S *pstWkInfo,
                                             HI_RUNTIME_GROUP_HANDLE *phGroupHandle)
{
    HI_RUNTIME_GROUP_INFO_S stGroupInfo = {0};
    HI_CHAR *pacConfig = NULL;

    memset(&stGroupInfo, 0, sizeof(HI_RUNTIME_GROUP_INFO_S));
    strncpy(pstWkInfo[0].acModelName, "alexnet", MAX_NAME_LEN);

    HI_S32 s32Ret = SAMPLE_RUNTIME_LoadModelFile(pcModelFileAlex, &(pstWkInfo[0].stWKMemory));
    SAMPLE_CHK_GOTO((s32Ret != HI_SUCCESS), FAIL, "SAMPLE_RUNTIME_LoadModelFile %s failed!\n", pcModelFileAlex);
    stGroupInfo.stWKsInfo.u32WKNum = 1;
    stGroupInfo.stWKsInfo.pstAttrs = &(pstWkInfo[0]);

    SAMPLE_RUNTIME_ReadConfig(CONFIG_DIR "alexnet.modelgroup", &pacConfig);
    SAMPLE_CHK_GOTO((pacConfig == NULL), FAIL, "HI_SVPRT_RUNTIME_ReadConfig error\n");

    s32Ret = HI_SVPRT_RUNTIME_LoadModelGroup(pacConfig, &stGroupInfo, phGroupHandle);
    SAMPLE_CHK_GOTO((s32Ret != HI_SUCCESS), FAIL, "HI_SVPRT_RUNTIME_LoadModelGroup error\n");

    SAMPLE_LOG_INFO("LoadGroup succ, group handle[%p]\n", phGroupHandle);

    SAMPLE_FREE(pacConfig);
    return HI_SUCCESS;
FAIL:
    SAMPLE_FREE(pacConfig);
    return HI_FAILURE;
}


HI_S32 SAMPLE_RUNTIME_ForwardGroup_Alexnet(const HI_CHAR *pcSrcFile, HI_RUNTIME_GROUP_HANDLE hGroupHandle)
{
    HI_RUNTIME_GROUP_SRC_BLOB_ARRAY_S stGroupSrcBlob = {0};
    HI_RUNTIME_GROUP_DST_BLOB_ARRAY_S stGroupDstBlob = {0};

    strncpy(s_stSrcBlobInfo.acSrcFilePath, pcSrcFile, sizeof(s_stSrcBlobInfo.acSrcFilePath));
    s_stSrcBlobInfo.acSrcFilePath[sizeof(s_stSrcBlobInfo.acSrcFilePath) - 1] = '\0';
    HI_S32 s32Ret = InitBlobs(&stGroupSrcBlob, 1, &s_stSrcBlobInfo);
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "InitBlobs fail");
    s32Ret = InitBlobs(&stGroupDstBlob, 1, &s_stDstBlobInfo);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "InitBlobs fail");

#if PERFORMANCE_TEST
    struct timespec start, end;
    clock_gettime(0, &start);
#endif

    s32Ret = HI_SVPRT_RUNTIME_ForwardGroupSync(hGroupHandle, &stGroupSrcBlob, &stGroupDstBlob, 0);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "HI_SVPRT_RUNTIME_ForwardGroupSync failed!\n");
#if PERFORMANCE_TEST
    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Forward");
#endif

    SAMPLE_LOG_INFO("Pic: %s\n", pcSrcFile);
    s32Ret = SAMPLE_RUNTIME_Cnn_TopN_Output(stGroupDstBlob.pstBlobs[0].pstBlob, ALEXNET_OUTPUT_TOP_FIVE);
    SAMPLE_CHK_PRINTF((s32Ret != HI_SUCCESS), "SAMPLE_RUNTIME_Cnn_TopN_Output error\n");
#if DEBUG
    printDebugData((HI_CHAR *)"alexnet", stDst[0].u64VirAddr, ALEXNET_DEBUG_PRINT_NUM);
    s32Ret = HI_SUCCESS;
#endif

FAIL_0:
    DeinitBlobs(&stGroupDstBlob);
    DeinitBlobs(&stGroupSrcBlob);
    return s32Ret;
}

HI_S32 SAMPLE_AlexNet()
{
    const HI_CHAR *pcRuntimeModelName = MODEL_ALEXNET_NAME;
    const HI_CHAR *pcSrcFile = IMAGE_ALEXNET_NAME;

    HI_RUNTIME_GROUP_HANDLE hGroupHandle = HI_NULL;
    HI_RUNTIME_WK_INFO_S astWkInfo[1];

    memset(&astWkInfo[0], 0, sizeof(astWkInfo));

    struct timespec start, next, end;
    clock_gettime(0, &start);

    printf("\n============================= alex net begin ================================\n");
    HI_S32 s32Ret = HI_SVPRT_RUNTIME_Init(CPU_TASK_AFFINITY, NULL);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_1, "HI_SVPRT_RUNTIME_Init failed!\n");

    s32Ret = SAMPLE_RUNTIME_LoadModelGroup_Alexnet(pcRuntimeModelName, astWkInfo, &hGroupHandle);
    SAMPLE_CHK_GOTO(s32Ret != HI_SUCCESS, FAIL_0, "SAMPLE_RUNTIME_LoadModelGroup_Alexnet failed!\n");

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Load");

    s32Ret = SAMPLE_RUNTIME_ForwardGroup_Alexnet(pcSrcFile, hGroupHandle);
    SAMPLE_CHK_PRINTF((s32Ret != HI_SUCCESS), "SAMPLE_RUNTIME_ForwardGroup_Alexnet error\n");

    clock_gettime(0, &next);
    timeSpendMs(&end, &next, "Forward total");

    SAMPLE_CHK_PRINTF((HI_SVPRT_RUNTIME_UnloadModelGroup(hGroupHandle) != HI_SUCCESS),
                      "HI_SVPRT_RUNTIME_UnloadModelGroup error\n");

FAIL_0:
    HI_SVPRT_RUNTIME_DeInit();
    SAMPLE_FreeMem(&(astWkInfo[0].stWKMemory));

    clock_gettime(0, &end);
    timeSpendMs(&start, &end, "Total");
FAIL_1:
    SAMPLE_LOG_INFO("SAMPLE_AlexNet result %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", s32Ret);
    return s32Ret;
}
