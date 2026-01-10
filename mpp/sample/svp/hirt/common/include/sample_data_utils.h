/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#ifndef __SAMPLE_DATA_UTILS_H
#define __SAMPLE_DATA_UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "string.h"
#include "sample_save_blob.h"

#ifdef _WIN32
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <linux/limits.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALIGN_16
#define ALIGN_16        16
#endif
#ifndef ALIGN_32
#define ALIGN_32        32
#endif

#define SVP_WK_QUANT_BASE 4096.0
enum {
    SVP_WK_PROPOSAL_WIDTH = 6,
    MAX_STACK_DEPTH = 50000,
    RPN_SUPPRESS_FALSE = 0,
    RPN_SUPPRESS_TRUE = 1,
    MIDDLE = 2,
    MAX_ROI_NUM = 300,
    SAMPLE_IVE_RESIZE_BATCH_MAX = 64
};

enum{
    ALEXNET_OUTPUT_BLOB_CHANNEL = 1000,
    ALEXNET_OUTPUT_BLOB_HEIGHT = 1,
    ALEXNET_OUTPUT_BLOB_WIDTH = 1
};

enum {
    RFCN_OUTPUT_BBOX_WIDTH = 8,
    RFCN_OUTPUT_CLS_WIDTH  = 21,
    RFCN_OUTPUT_NUM = 3,
    RFCN_OUTPUT_BBOX_IDX = 2,
    RFCN_OUTPUT_CLS_IDX = 1,
    RFCN_OUTPUT_PROPOSAL_IDX = 0,
    RFCN_IMAGE_CHANNEL = 3
};

enum {
    SVP_WK_COORDI_NUM = 4,
    X_MIN_IDX = 0,
    Y_MIN_IDX = 1,
    X_MAX_IDX = 2,
    Y_MAX_IDX = 3,
    SCORE_IDX = 4,
    BBOX_SCORE_IDX = 4,
    RPN_SUPRESS_IDX = 5
};

enum {
    RFCN_IMAGE_HEIGHT = 600,
    RFCN_IMAGE_WIDTH = 800,
    FASTERRCNN_IMAGE_WIDTH = 1240,
    FASTERRCNN_IMAGE_HIGH = 375,
    ALEXNET_IMAGE_HEIGHT = 227,
    ALEXNET_IMAGE_WIDTH = 227
};

enum {
    IMG_CHANNEL = 3,
    IMG_MEAN_B_VAL = 104,
    IMG_MEAN_G_VAL = 117,
    IMG_MEAN_R_VAL = 123,
    ALEXNET_IMAGE_CHANNEL = 3
};

#define SAMPLE_SVP_NNIE_HALF  0.5f
#define MILLISECOND_PER_SECOND 1000
#define MICROSECOND_PER_SECOND (1000 * 1000)
#define MICROSECOND_PER_MILLISECOND 1000
#define NANOSECOND_PER_MICROSECOND 1000
#define NANOSECOND_PER_MILLISECOND (1000 * 1000)

#ifndef max
#define max(x, y) (x > y ? x : y)
#endif
#ifndef min
#define min(x, y) (x < y ? x : y)
#endif

typedef struct hiNNIE_STACK {
    HI_S32 s32Min;
    HI_S32 s32Max;
} NNIE_STACK_S;

typedef struct tagTRACKER_LOCATION {
    HI_S32 x1;
    HI_S32 y1;
    HI_S32 x2;
    HI_S32 y2;
} TRACKER_LOCATION_S;

typedef struct tagBondingBox {
    HI_DOUBLE x1;
    HI_DOUBLE y1;
    HI_DOUBLE x2;
    HI_DOUBLE y2;
} BondingBox_s;

typedef struct {
    HI_U32 u32Num;
    HI_U32 u32Chn;
    HI_U32 u32Height;
    HI_U32 u32Width;
} BlobShape;

typedef struct tagBlobInfo {
    HI_CHAR acOwnerName[MAX_NAME_LEN + 1];
    HI_CHAR acBlobName[MAX_NAME_LEN + 1];
    HI_CHAR acSrcFilePath[PATH_MAX];
    HI_RUNTIME_BLOB_TYPE_E enBlobType;
    BlobShape stShape;
    HI_BOOL bIsSrc;
    HI_U32 u32Align;
} BlobInfo;

HI_U32 getAlignSize(HI_U32 align, HI_U32 num);
HI_S32 InitBlobs(HI_RUNTIME_GROUP_BLOB_ARRAY_S *pstGroupBlob, HI_U32 u32BlobNum, BlobInfo *pBlobInfos);
HI_VOID DeinitBlobs(HI_RUNTIME_GROUP_BLOB_ARRAY_S *pstGroupBlob);

HI_S32 NonRecursiveArgQuickSort(HI_S32 *aResultArray,
                                HI_S32 s32Low, HI_S32 s32High, NNIE_STACK_S *pstStack, HI_U32 u32MaxNum);
HI_S32 NonMaxSuppression(HI_S32 *pu32Proposals, HI_U32 u32NumAnchors, HI_U32 u32NmsThresh);
HI_VOID SAMPLE_DATA_GetStride(HI_RUNTIME_BLOB_TYPE_E type, HI_U32 width, HI_U32 align, HI_U32 *pStride);
HI_U32 SAMPLE_DATA_GetBlobSize(HI_U32 stride, HI_U32 num, HI_U32 height, HI_U32 chn);
HI_S32 SAMPLE_RUNTIME_HiMemAlloc(HI_RUNTIME_MEM_S *pstMem, HI_BOOL bCached);
HI_S32 SAMPLE_RUNTIME_LoadModelFile(const HI_CHAR *pcModelFile, HI_RUNTIME_MEM_S *pstMemInfo);
HI_S32 SAMPLE_RUNTIME_HiBlobAlloc(HI_RUNTIME_BLOB_S *pstBlob, HI_U32 u32BlobSize, HI_BOOL bCached);
HI_S32 SAMPLE_RUNTIME_SetBlob(HI_RUNTIME_BLOB_S *pstBlob,
                              HI_RUNTIME_BLOB_TYPE_E enType,
                              BlobShape *pstBlobShape,
                              HI_U32 u32Align);
HI_S32 SAMPLE_RUNTIME_ReadSrcFile(const HI_CHAR acFileName[MAX_FILE_NAME_LENGTH],
    HI_RUNTIME_BLOB_S *pstSrcBlob);
HI_S32 SAMPLE_RUNTIME_ReadConfig(const HI_CHAR *pcConfigFile, HI_CHAR **acBuff);
HI_DOUBLE compute_output_w(HI_DOUBLE x1, HI_DOUBLE x2);
HI_DOUBLE compute_output_h(HI_DOUBLE y1, HI_DOUBLE y2);
HI_VOID computeCropLocation(const BondingBox_s *pstTightBbox, HI_DOUBLE dWidth, HI_DOUBLE dHeight,
                            BondingBox_s *pstLocationBbox);
HI_DOUBLE compute_edge_x(HI_DOUBLE x1, HI_DOUBLE x2);
HI_DOUBLE compute_edge_y(HI_DOUBLE y1, HI_DOUBLE y2);

#ifdef __cplusplus
}
#endif

#endif
