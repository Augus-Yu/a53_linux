/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#ifndef __SAMPLE_SAVE_BLOB_H
#define __SAMPLE_SAVE_BLOB_H
#include "hi_runtime_comm.h"
#ifdef ON_BOARD
#include "hi_ive.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MAX_FILE_NAME_LENGTH = 256,
    RFCN_ALEX_FILE_NAME_LENGTH = 16,
    RFCN_ALEX_WK_NUM = 2
};

HI_VOID saveBlob(const HI_CHAR aszPath[RFCN_ALEX_FILE_NAME_LENGTH], const HI_RUNTIME_BLOB_S *pstSrcBlob,
    HI_U16 u16Index);

#ifdef __cplusplus
}
#endif

#endif
