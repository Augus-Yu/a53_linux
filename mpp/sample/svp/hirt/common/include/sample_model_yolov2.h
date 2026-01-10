/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#ifndef SAMPLE_MODEL_YOLOV2_H
#define SAMPLE_MODEL_YOLOV2_H
#include "hi_runtime_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

void SAMPLE_RUNTIME_DetOneSegGetResult(HI_RUNTIME_BLOB_S *astDstBlobs, HI_S32 *ps32ResultMem);
HI_S32 *GetResultMem();

#ifdef __cplusplus
}
#endif

#endif
