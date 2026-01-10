/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#ifndef _DETECTION_COM_H_
#define _DETECTION_COM_H_

#include "hi_type.h"
#include "hi_runtime_comm.h"
#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#define _mkdir(a) mkdir((a), 0755)
#endif

enum {
    DETECION_DBG = 0,
    SVP_WK_PROPOSAL_WIDTH = 6,
    SVP_WK_COORDI_NUM = 4,
    SVP_WK_SCORE_NUM = 2,
    MAX_STACK_DEPTH = 50000,
    SVP_NNIE_MAX_REPORT_NODE_CNT = 16, /* NNIE max report num */
    SVP_NNIE_MAX_RATIO_ANCHOR_NUM = 32, /* NNIE max ratio anchor num */
    RPN_NODE_NUM = 3,
    OPERAND_INPUT_NUM = 2
};

#ifndef SVP_WK_QUANT_BASE
#define SVP_WK_QUANT_BASE 0x1000
#endif

#ifndef ALIGN32
#define ALIGN32(addr) ((((addr) + 32 - 1) / 32) * 32)
#endif

#ifndef ALIGN16
#define ALIGN16(addr) ((((addr) + 16 - 1) / 16) * 16)
#endif

#ifndef SVP_MAX
#define SVP_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef SVP_MIN
#define SVP_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define SAFE_ROUND(val) (double)(((double)(val) > 0) ? floor((double)(val) + 0.5) : ceil((double)(val) - 0.5))

#define SVP_FALSE_CHECK(cond, ec)                                                                \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            printf("%s %d CHECK error! cond = %d, do ret = %d\n", __FILE__, __LINE__, cond, ec); \
            return ec;                                                                           \
        }                                                                                        \
    } while (0)

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#define SVP_WK_QUANT_BASE             0x1000

enum RPN_SUPRESS_FLAG {
    RPN_SUPPRESS_FALSE = 0,
    RPN_SUPPRESS_TRUE = 1,

    RPN_SUPPRESS_BUTT
};

typedef struct hiNNIE_STACK {
    HI_S32 s32Min;
    HI_S32 s32Max;
} NNIE_STACK_S;

typedef struct hiSVP_NNIE_SRC_NODE_S {
    HI_U32 u32Height;
    HI_U32 u32Width;
    HI_U32 u32Chn;
    HI_U8 u8Format;
    HI_U8 u8Reserved;
    HI_U16 u16LayerId;
} SVP_NNIE_SRC_NODE_S;

typedef struct hiSVP_NNIE_ROI_NODE_S {
    HI_U32 u32SrcPoolHeight;
    HI_U32 u32SrcPoolWidth;
    float f32Scale;
    HI_U8 u8UsePingPong;
    HI_U8 u8IsHighPrecision;
    HI_U8 u8RoiPoolType;
    HI_U8 RSV1;

    HI_U32 u32DstPoolHeight;
    HI_U32 u32DstPoolWidth;
    HI_U32 u32Channel;
    HI_U32 u32MaxRoiNum;

    HI_U32 u32BlockNum;
    HI_U32 u32BlockHeight;
    HI_U32 u32MaxROIInfoSize;
    HI_U32 RSV2;
} SVP_NNIE_ROI_NODE_S;

/* Network type */
typedef enum hiSVP_NNIE_NET_TYPE_E {
    SVP_NNIE_NET_TYPE_CNN = 0x0,       /* Non-ROI input cnn net */
    SVP_NNIE_NET_TYPE_ROI = 0x1,       /* With ROI input cnn net */
    SVP_NNIE_NET_TYPE_RECURRENT = 0x2, /* RNN or LSTM net */

    SVP_NNIE_NET_TYPE_BUTT
} SVP_NNIE_NET_TYPE_E;

typedef struct hiNNIE_REPORT_NODE_INFO_S {
    HI_U32 u32ConvWidth;  /* width */
    HI_U32 u32ConvHeight; /* height */
    HI_U32 u32ConvMapNum; /* map num */
    HI_U32 u32ConvStride; /* stride */
} NNIE_REPORT_NODE_INFO_S;

typedef struct hiNNIE_MODEL_INFO_S {
    /* MPI layer input */
    HI_U32 u32MemPoolSize;         /* memory pool size */
    SVP_NNIE_NET_TYPE_E enNetType; /* net type */

    HI_U32 u32SrcWidth;  /* input pic width */
    HI_U32 u32SrcHeight; /* input pic height */
    HI_U32 u32SrcStride; /* input pic stride */

    NNIE_REPORT_NODE_INFO_S astReportNodeInfo[SVP_NNIE_MAX_REPORT_NODE_CNT]; /* report node info */
    HI_U32 u32ReportNodeNum;                                                 /* report node number */

    HI_U32 u32MinSize;                                /* min anchor size */
    HI_U32 u32SpatialScale;                           /* spatial scale */
    HI_U32 au32Ratios[SVP_NNIE_MAX_RATIO_ANCHOR_NUM]; /* anchors' ratios */
    HI_U32 u32NumRatioAnchors;                        /* num of ratio anchors */
    HI_U32 au32Scales[SVP_NNIE_MAX_RATIO_ANCHOR_NUM]; /* anchors' scales */
    HI_U32 u32NumScaleAnchors;                        /* num of scale anchors */

    HI_U32 u32RoiWidth;       /* rcnn roi width */
    HI_U32 u32RoiHeight;      /* rcnn roi height */
    HI_U32 u32RoiMapNum;      /* rcnn roi map num */
    HI_U32 u32RoiStride;      /* rcnn roi stride */
    HI_U32 u32MaxRoiFrameCnt; /* max roi frame cnt */

    HI_U32 u32DnnChannelNum; /* dnn input channel num, current version rsv */
    HI_U32 u32ChannelNum;
    HI_U8 u8RunMode;

    // support pooling report
    HI_U32 u32ReportMode;  /* final report mode: 0-fc report, 1-conv or pooling report */
    HI_U32 u32ClassSize;   /* class category */
    HI_U32 u32ClassStride; /* class stride */
} NNIE_MODEL_INFO_S;

typedef struct {
    HI_S32 *ps32Proposals;
    HI_S32 *ps32Anchors;
    HI_S32 *ps32BboxDelta;
    HI_FLOAT *pf32Scores;
} BBOX_PARAM_S;

typedef enum {
    PROPOSAL_WITH_PERMUTE,
    PROPOSAL_WITHOUT_PERMUTE,
} PROPOSAL_TYPE_E;

typedef struct hiNodePlugin_Param_S {
    HI_U32 u32SrcWidth;
    HI_U32 u32SrcHeight;
    SVP_NNIE_NET_TYPE_E enNetType;
    HI_U32 u32NumRatioAnchors;
    HI_U32 u32NumScaleAnchors;
    HI_FLOAT *pfRatio;
    HI_FLOAT *pfScales;
    HI_U32 u32MaxRoiFrameCnt;
    HI_U32 u32MinSize;
    HI_FLOAT fSpatialScale;
    HI_FLOAT fNmsThresh;
    HI_FLOAT fFilterThresh;
    HI_U32 u32NumBeforeNms;
    HI_FLOAT fConfThresh;
    HI_FLOAT fValidNmsThresh;
    HI_U32 u32ClassSize;
} HI_NODEPlugin_Param_S;

typedef struct hiProposal_param_s {
    HI_RUNTIME_MEM_S stBachorMemInfo;
    HI_NODEPlugin_Param_S stNodePluginParam;
} HI_PROPOSAL_Param_S;

typedef struct _PROPOSAL_PARA_S {
    // ---------- parameters for PriorBox ---------
    NNIE_MODEL_INFO_S model_info;
    HI_U32 u32NumBeforeNms;
    HI_U32 u32NmsThresh;
    HI_U32 u32ValidNmsThresh;
    HI_U32 u32FilterThresh;
    HI_U32 u32ConfThresh;
    HI_RUNTIME_MEM_S stBachorMemInfo;
} PROPOSAL_PARA_S;

/*********************************************************
Function: QuickExp
Description: Do QuickExp...
*********************************************************/
HI_FLOAT QuickExp(HI_U32 u32X);

/**************************************************
Function: Argswap
Description: used in NonRecursiveQuickSort
***************************************************/
HI_S32 Argswap(HI_S32 *ps32Src1, HI_S32 *ps32Src2);

/**************************************************
Function: NonRecursiveArgQuickSort
Description: sort with NonRecursiveArgQuickSort
***************************************************/
HI_S32 NonRecursiveArgQuickSort(HI_S32 *aResultArray, HI_S32 s32Low, HI_S32 s32High, NNIE_STACK_S *pstStack,
                                HI_U32 u32MaxNum);

/**************************************************
Function: NonMaxSuppression
Description: proposal NMS with u32NmsThresh
***************************************************/
HI_S32 NonMaxSuppression(HI_S32 *pu32Proposals, HI_U32 u32NumAnchors, HI_U32 u32NmsThresh, HI_U32 u32MaxRoiNum);

/**************************************************
Function: FilterLowScoreBbox
Description: remove low conf score proposal bbox
***************************************************/
HI_S32 FilterLowScoreBbox(HI_S32 *pu32Proposals, HI_U32 u32NumAnchors, HI_U32 u32NmsThresh,
                          HI_U32 u32FilterThresh, HI_U32 *u32NumAfterFilter);

/**************************************************
Function: BBox Transform
Description: parameters from Conv3 to adjust the coordinates of anchor
***************************************************/
HI_S32 BboxTransform(HI_S32 *ps32Proposals,
                     HI_S32 *ps32Anchors,
                     HI_S32 *ps32BboxDelta,
                     HI_FLOAT *pf32Scores,
                     PROPOSAL_TYPE_E enProposalType);

/* deal with num */
HI_S32 BboxTransform_N(BBOX_PARAM_S *pstBboxParam, HI_U32 u32NumAnchors, PROPOSAL_TYPE_E enProposalType);

/**************************************************
Function: BboxClip
Description: clip proposal bbox out of origin image range
***************************************************/
HI_S32 BboxClip(HI_S32 *ps32Proposals, HI_U32 u32ImageW, HI_U32 u32ImageH);

/* deal with num */
HI_S32 BboxClip_N(HI_S32 *ps32Proposals, HI_U32 u32ImageW, HI_U32 u32ImageH, HI_U32 u32Num);

/* single size clip */
HI_S32 SizeClip(HI_S32 s32inputSize, HI_S32 s32sizeMin, HI_S32 s32sizeMax);

/**************************************************
Function: BboxSmallSizeFilter
Description: remove the bboxes which are too small
***************************************************/
HI_S32 BboxSmallSizeFilter(HI_S32 *ps32Proposals, HI_U32 u32minW, HI_U32 u32minH);
HI_S32 BboxSmallSizeFilter_N(HI_S32 *ps32Proposals, HI_U32 u32minW, HI_U32 u32minH, HI_U32 u32NumAnchors);

/**************************************************
Function: dumpProposal
Description: dumpProposal info when DETECION_DBG
***************************************************/
HI_S32 dumpProposal(HI_S32 *ps32Proposals, const HI_CHAR *filename, HI_U32 u32NumAnchors);

/**************************************************
Function: getRPNresult
Description: rite the final result to output
***************************************************/
HI_S32 getRPNresult(HI_S32 *ps32ProposalResult, HI_U32 *pu32NumRois, HI_U32 u32MaxRois,
                    const HI_S32 *ps32Proposals, HI_U32 u32NumAfterFilter);

/**************************************************
Function: BreakLine
Description:
***************************************************/
void PrintBreakLine(HI_BOOL flag);

/**************************************************
Function: GetParam
Description:
***************************************************/
HI_VOID GetParam(const HI_PROPOSAL_Param_S *pPluginParam, PROPOSAL_PARA_S *param);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
