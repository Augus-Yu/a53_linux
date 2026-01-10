/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#ifndef __SVP_SAMPLE_YOLO_H__
#define __SVP_SAMPLE_YOLO_H__
#include "hi_runtime_comm.h"

/* YOLO result box */
typedef struct SVP_SAMPLE_BOX {
    HI_FLOAT f32Xmin;
    HI_FLOAT f32Xmax;
    HI_FLOAT f32Ymin;
    HI_FLOAT f32Ymax;
    HI_FLOAT f32ClsScore;
    HI_U32 u32MaxScoreIndex;
    HI_U32 u32Mask;
} SVP_SAMPLE_BOX_S;

/* YOLO V1 */
enum {
    SVP_SAMPLE_YOLOV1_IMG_WIDTH = 448,
    SVP_SAMPLE_YOLOV1_IMG_HEIGHT = 448,
    SVP_SAMPLE_YOLOV1_BBOX_CNT = 98,
    SVP_SAMPLE_YOLOV1_CLASS_CNT = 20,
    SVP_SAMPLE_YOLOV1_GRID_NUM = 7,
    SVP_SAMPLE_YOLOV1_CHANNEL_NUM = 30,
    SVP_SAMPLE_YOLOV1_BOX_NUM = 2
};

#define SVP_SAMPLE_YOLOV1_GRID_SQR_NUM             (SVP_SAMPLE_YOLOV1_GRID_NUM * SVP_SAMPLE_YOLOV1_GRID_NUM)
#define SVP_SAMPLE_YOLOV1_CHANNEL_GRID_NUM         (SVP_SAMPLE_YOLOV1_CHANNEL_NUM * SVP_SAMPLE_YOLOV1_GRID_SQR_NUM)
#define SVP_SAMPLE_YOLOV1_SCORE_FILTER_THREASH     0.3f
#define SVP_SAMPLE_YOLOV1_NMS_THREASH              0.5f
#define DET_EPS  0.000001f

typedef struct st_yolov1_score {
    HI_U32 idx;
    HI_FLOAT value;
} yolov1_score;

typedef struct st_position {
    HI_FLOAT x;
    HI_FLOAT y;
    HI_FLOAT w;
    HI_FLOAT h;
} position;

#ifdef __cplusplus
extern "C" {
#endif

HI_VOID SAMPLE_RUNTIME_YoloV1GetResult(HI_RUNTIME_BLOB_S *pstDstBlob);

#ifdef __cplusplus
}
#endif

/* YOLO V2 */
enum {
    SVP_SAMPLE_YOLOV2_IMG_WIDTH = 416,
    SVP_SAMPLE_YOLOV2_IMG_HEIGHT = 416,
    SVP_SAMPLE_YOLOV2_GRIDNUM = 13,
    SVP_SAMPLE_YOLOV2_CHANNLENUM = 50,
    SVP_SAMPLE_YOLOV2_PARAMNUM = 10,
    SVP_SAMPLE_YOLOV2_BOXNUM = 5,
    SVP_SAMPLE_YOLOV2_CLASSNUM = 5,
    SVP_SAMPLE_YOLOV2_MAX_BOX_NUM = 10,
    SVP_SAMPLE_YOLOV2_WIDTH = 7,
    SVP_SAMPLE_YOLOV2_OUTBOX_NUM = 5,
    SVP_SAMPLE_SRC_BLOB_NUM = 1
};

#define SVP_SAMPLE_YOLOV2_GRIDNUM_SQR (SVP_SAMPLE_YOLOV2_GRIDNUM * SVP_SAMPLE_YOLOV2_GRIDNUM)
#define SVP_SAMPLE_YOLOV2_BOXTOTLENUM (SVP_SAMPLE_YOLOV2_GRIDNUM * SVP_SAMPLE_YOLOV2_GRIDNUM * SVP_SAMPLE_YOLOV2_BOXNUM)
#define SVP_SAMPLE_YOLOV2_SCORE_FILTER_THREASH 0.01f
#define SVP_SAMPLE_YOLOV2_NMS_THREASH          0.3f

typedef struct hiSVP_SAMPLE_STACK_S {
    HI_S32 s32Min; /* The minimum position coordinate */
    HI_S32 s32Max; /* The maximum position coordinate */
} SVP_SAMPLE_STACK_S;

#ifdef __cplusplus
extern "C" {
#endif

void SAMPLE_RUNTIME_YoloV2GetResult(HI_RUNTIME_BLOB_S *astDstBlobs, HI_S32 *ps32ResultMem);
HI_S32 *GetResultMem_YoloV2();

#ifdef __cplusplus
}
#endif

/* Yolo V3 */
typedef struct hiSVP_SAMPLE_YOLOV3_DATA {
    HI_U8 *pu8InputData;
    HI_S32 *ps32ResultMem;
} SAMPLE_YOLOV3_DATA;

#define SVP_SAMPLE_YOLOV3_SCORE_FILTER_THREASH     0.5f
#define SVP_SAMPLE_YOLOV3_NMS_THREASH              0.45f
enum {
    SVP_SAMPLE_YOLOV3_SRC_WIDTH = 416,
    SVP_SAMPLE_YOLOV3_SRC_HEIGHT = 416,
    SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_82 = 13,
    SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_94 = 26,
    SVP_SAMPLE_YOLOV3_GRIDNUM_CONV_106 = 52,
    SVP_SAMPLE_YOLOV3_CHANNLENUM = 255,
    SVP_SAMPLE_YOLOV3_PARAMNUM = 85,
    SVP_SAMPLE_YOLOV3_BOXNUM = 3,
    SVP_SAMPLE_YOLOV3_CLASSNUM = 80,
    SVP_SAMPLE_YOLOV3_MAX_BOX_NUM = 10
};

typedef enum hiSVP_SAMPLE_YOLOV3_SCALE_TYPE {
    CONV_82 = 0,
    CONV_94,
    CONV_106,
    SVP_SAMPLE_YOLOV3_SCALE_TYPE_MAX
} SVP_SAMPLE_YOLOV3_SCALE_TYPE_E;

typedef struct SVP_SAMPLE_BOX_RESULT_INFO {
    HI_U32 u32OriImHeight;
    HI_U32 u32OriImWidth;
    SVP_SAMPLE_BOX_S* pstBbox;
} SVP_SAMPLE_BOX_RESULT_INFO_S;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SVP_SAMPLE_RESULT_MEM_HEAD {
    HI_U32 u32Type;
    HI_U32 u32Len;
} SVP_SAMPLE_RESULT_MEM_HEAD_S;

HI_VOID SAMPLE_RUNTIME_YoloV3GetResult(HI_RUNTIME_BLOB_S *pstDstBlob, HI_S32 *ps32ResultMem,
    SVP_SAMPLE_BOX_RESULT_INFO_S *pstResultBoxInfo, HI_U32 *pu32BoxNum);
HI_S32* GetResultMem_YoloV3();

#ifdef __cplusplus
}
#endif

#endif  // __SVP_SAMPLE_WK_H__
