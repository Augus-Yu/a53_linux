/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_resize_roi.h"
#include <time.h>
#include <stdio.h>
#ifdef ON_BOARD
#include <unistd.h>
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_ive.h"
#else
#include "hi_comm_svp.h"
#include "hi_nnie.h"
#include "mpi_nnie.h"
#include "sample_draw_rect.h"
#include "sample_cv_resize.h"
#endif
#include "sample_log.h"
#include "sample_memory_ops.h"
#include "sample_data_utils.h"
#include "sample_ive_image.h"

#ifdef ON_BOARD
static const HI_U32 REQUEST_TIME = 100;
static const HI_U32 MAX_RESIZE_BATCH_NUM = 300;
HI_S32 blob2IveSrc(const HI_RUNTIME_BLOB_S *pstSrcBlob, const HI_S32 as32Coord[], IVE_SRC_IMAGE_S *pstIve)
{
    if (pstSrcBlob->enBlobType == HI_RUNTIME_BLOB_TYPE_U8) {
        pstIve->enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
    } else {
        printf("Not support blob type: %u \n", pstSrcBlob->enBlobType);
        return HI_FAILURE;
    }

    HI_S32 s32X1 = getAlignSize(ALIGN_16, as32Coord[X_MIN_IDX]);
    HI_S32 s32X2 = getAlignSize(ALIGN_16, as32Coord[X_MAX_IDX]);
    pstIve->u32Height = as32Coord[Y_MAX_IDX] - as32Coord[Y_MIN_IDX];
    pstIve->u32Width = s32X2 - s32X1;

    if (pstIve->u32Height % 2 != 0) {
        pstIve->u32Height--;
    }

    if (pstIve->u32Width % 2 != 0) {
        pstIve->u32Width--;
    }

    pstIve->au32Stride[0] = pstSrcBlob->u32Stride;
    pstIve->au32Stride[1] = pstSrcBlob->u32Stride;
    pstIve->au32Stride[2] = pstSrcBlob->u32Stride;

    HI_U32 u32BlobOneChnSize = pstSrcBlob->u32Stride * pstSrcBlob->unShape.stWhc.u32Height;
    HI_U32 u32IveOffset = as32Coord[1] * pstSrcBlob->u32Stride + s32X1;
    pstIve->au64PhyAddr[0] = pstSrcBlob->u64PhyAddr + u32IveOffset;
    pstIve->au64PhyAddr[1] = pstIve->au64PhyAddr[0] + u32BlobOneChnSize;
    pstIve->au64PhyAddr[2] = pstIve->au64PhyAddr[1] + u32BlobOneChnSize;

    pstIve->au64VirAddr[0] = pstSrcBlob->u64VirAddr + u32IveOffset;
    pstIve->au64VirAddr[1] = pstIve->au64VirAddr[0] + u32BlobOneChnSize;
    pstIve->au64VirAddr[2] = pstIve->au64VirAddr[1] + u32BlobOneChnSize;
    return HI_SUCCESS;
}

HI_BOOL IsSmallIveResizeRoi(const HI_S32 as32Coord[])
{
    HI_S32 s32Width = as32Coord[X_MAX_IDX] - as32Coord[X_MIN_IDX];
    HI_S32 s32Hight = as32Coord[Y_MAX_IDX] - as32Coord[Y_MIN_IDX];

    if ((s32Width < ALIGN_32) && (s32Hight < ALIGN_16)) {
        return HI_TRUE;
    }

    return HI_FALSE;
}
HI_S32 checkIveResizeRoi(const HI_S32 as32Coord[])
{
    HI_S32 s32Width = as32Coord[X_MAX_IDX] - as32Coord[X_MIN_IDX];
    HI_S32 s32Hight = as32Coord[Y_MAX_IDX] - as32Coord[Y_MIN_IDX];
    // no greater than 1080P
    if ((s32Width < ALIGN_32) || (s32Width > 1920) || (s32Hight < ALIGN_16) || (s32Hight > 1080)) {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 setIveDst(HI_RUNTIME_BLOB_S *pstDstBlob, HI_U16 u16Index, IVE_DST_IMAGE_S *pstIve)
{
    if (pstDstBlob->enBlobType == HI_RUNTIME_BLOB_TYPE_U8) {
        pstIve->enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
    } else {
        printf("Not support blob type: %u \n", pstDstBlob->enBlobType);
        return HI_FAILURE;
    }

    pstIve->u32Height = pstDstBlob->unShape.stWhc.u32Height;
    pstIve->u32Width = pstDstBlob->unShape.stWhc.u32Width;

    if (pstIve->u32Height % 2 != 0) {
        pstIve->u32Height--;
    }

    if (pstIve->u32Width % 2 != 0) {
        pstIve->u32Width--;
    }

    if (pstDstBlob->u32Stride % ALIGN_16 != 0) {
        printf("Stride error, not align 16 \n");
        return HI_FAILURE;
    }

    HI_U64 us64OneChnSize = (HI_U64)pstDstBlob->u32Stride * pstDstBlob->unShape.stWhc.u32Height;
    HI_U64 us64Size = us64OneChnSize * pstDstBlob->unShape.stWhc.u32Chn;

    for (HI_U8 i = 0; i < pstDstBlob->unShape.stWhc.u32Chn; i++) {
        pstIve->au32Stride[i] = pstDstBlob->u32Stride;
        pstIve->au64PhyAddr[i] = pstDstBlob->u64PhyAddr + u16Index * us64Size + i * us64OneChnSize;
        pstIve->au64VirAddr[i] = pstDstBlob->u64VirAddr + u16Index * us64Size + i * us64OneChnSize;
    }

    return HI_SUCCESS;
}

static HI_S32 StartResize(IVE_RESIZE_CTRL_S *pstResizeCtrl, IVE_SRC_IMAGE_S *pstIveSrc,
    IVE_DST_IMAGE_S *pstIveDst, HI_RUNTIME_BLOB_S *pstDstBlob)
{
    HI_BOOL bFinish = HI_FALSE;
    IVE_HANDLE iveHandle = 0;
    struct timespec request_time;
    request_time.tv_sec = 0;
    request_time.tv_nsec = REQUEST_TIME * NANOSECOND_PER_MILLISECOND;
    HI_RUNTIME_MEM_S stMem;

    HI_S32 s32Ret = HI_MPI_IVE_Resize(&iveHandle, pstIveSrc, pstIveDst, pstResizeCtrl, HI_TRUE);
    SAMPLE_CHK_RET((s32Ret != HI_SUCCESS), HI_FAILURE, "HI_MPI_IVE_Resize fail:%#x", s32Ret);

    s32Ret = HI_MPI_IVE_Query(iveHandle, &bFinish, HI_TRUE);
    while (s32Ret == HI_ERR_IVE_QUERY_TIMEOUT) {
        nanosleep(&request_time, HI_NULL);
        printf("HI_MPI_IVE_Query timeout, wait 100us\n");
        s32Ret = HI_MPI_IVE_Query(iveHandle, &bFinish, HI_TRUE);
    }
    SAMPLE_CHK_RET(s32Ret != HI_SUCCESS, HI_FAILURE, "Error(%#x), HI_MPI_IVE_Query failed!\n", s32Ret);

    HI_U32 u32BlobSize = SAMPLE_DATA_GetBlobSize(pstDstBlob->u32Stride, pstDstBlob->u32Num,
                                                 pstDstBlob->unShape.stWhc.u32Height, pstDstBlob->unShape.stWhc.u32Chn);
    stMem.u64PhyAddr = pstDstBlob->u64PhyAddr;
    stMem.u64VirAddr = pstDstBlob->u64VirAddr;
    stMem.u32Size = u32BlobSize;
    s32Ret = SAMPLE_FlushCache(&stMem);
    if (s32Ret != HI_SUCCESS) {
        printf("SAMPLE_FlushCache error after IVE\n");
    }

    return s32Ret;
}

static HI_S32 ResizeRoi(const HI_RUNTIME_BLOB_S *pstSrcBlob, const HI_S32 as32Coord[],
    HI_U32 u32CoordStride, HI_U16 u16Cnt, HI_RUNTIME_BLOB_S *pstDstBlob, HI_U16 u16DstOffSetCnt,
    IVE_SRC_IMAGE_S *pstIveSrc, IVE_DST_IMAGE_S *pstIveDst)
{
    IVE_RESIZE_CTRL_S stResizeCtrl;
    HI_RUNTIME_MEM_S stResizeMem;
    memset(&stResizeCtrl, 0x0, sizeof(IVE_RESIZE_CTRL_S));
    HI_S32 s32Ret = HI_FAILURE;

    for (HI_U32 i = 0; i < u16Cnt; i++) {
        s32Ret = checkIveResizeRoi(&as32Coord[i * u32CoordStride]);
        SAMPLE_CHK_RET((s32Ret != HI_SUCCESS), HI_FAILURE,
            "Check ive resize roi[x1=%d, y1=%d, x2=%d, y2=%d] failed!\n",
            as32Coord[i * u32CoordStride + X_MIN_IDX], as32Coord[i * u32CoordStride + Y_MIN_IDX],
            as32Coord[i * u32CoordStride + X_MAX_IDX], as32Coord[i * u32CoordStride + Y_MAX_IDX]);

        s32Ret = blob2IveSrc(pstSrcBlob, &as32Coord[i * u32CoordStride], &pstIveSrc[i]);
        SAMPLE_CHK_RET((s32Ret != HI_SUCCESS), HI_FAILURE, "blob2IveSrc fail:%#x", s32Ret);
        s32Ret = setIveDst(pstDstBlob, i + u16DstOffSetCnt, &pstIveDst[i]);
        SAMPLE_CHK_RET((s32Ret != HI_SUCCESS), HI_FAILURE, "setIveDst fail:%#x", s32Ret);
    }

    stResizeCtrl.enMode = IVE_RESIZE_MODE_LINEAR;
    stResizeCtrl.u16Num = (HI_U16)u16Cnt;
    stResizeCtrl.stMem.u32Size = 49 * stResizeCtrl.u16Num;
    stResizeMem.u32Size = stResizeCtrl.stMem.u32Size;
    s32Ret = SAMPLE_AllocMem(&stResizeMem, HI_FALSE);
    stResizeCtrl.stMem.u64PhyAddr = stResizeMem.u64PhyAddr;
    stResizeCtrl.stMem.u64VirAddr = stResizeMem.u64VirAddr;
    SAMPLE_CHK_RET((s32Ret != HI_SUCCESS), HI_FAILURE, "SAMPLE_Utils_AllocMem fail:%#x", s32Ret);

    s32Ret = StartResize(&stResizeCtrl, pstIveSrc, pstIveDst, pstDstBlob);
    SAMPLE_CHK_PRINTF(s32Ret != HI_SUCCESS, "StartResize failed]\n");

    SAMPLE_FreeMem(&stResizeMem);
    return s32Ret;
}

HI_S32 resizeROIByIVE(const HI_RUNTIME_BLOB_S *pstSrcBlob,
                      const HI_S32 as32Coord[],
                      HI_U32 u32CoordStride,
                      HI_U16 u16Cnt,
                      HI_RUNTIME_BLOB_S *pstDstBlob,
                      HI_U16 u16DstOffSetCnt)
{
    IVE_SRC_IMAGE_S *pstIveSrc = HI_NULL;
    IVE_DST_IMAGE_S *pstIveDst = HI_NULL;
    pstIveSrc = (IVE_SRC_IMAGE_S *)malloc(sizeof(IVE_SRC_IMAGE_S) * MAX_RESIZE_BATCH_NUM);
    SAMPLE_CHK_GOTO((pstIveSrc == HI_NULL), MALLOC_FREE, "malloc error!\n");
    pstIveDst = (IVE_DST_IMAGE_S *)malloc(sizeof(IVE_DST_IMAGE_S) * MAX_RESIZE_BATCH_NUM);
    SAMPLE_CHK_GOTO((pstIveDst == HI_NULL), MALLOC_FREE, "malloc error!\n");

    memset(pstIveSrc, 0x0, sizeof(IVE_SRC_IMAGE_S) * MAX_RESIZE_BATCH_NUM);
    memset(pstIveDst, 0x0, sizeof(IVE_DST_IMAGE_S) * MAX_RESIZE_BATCH_NUM);

    SAMPLE_CHK_GOTO((u16Cnt > MAX_RESIZE_BATCH_NUM), MALLOC_FREE, "Roi number[%u] exceed limit[%u]!", u16Cnt,
                    MAX_RESIZE_BATCH_NUM);

    {
        HI_S32 s32Ret = ResizeRoi(pstSrcBlob, as32Coord, u32CoordStride, u16Cnt, pstDstBlob,
            u16DstOffSetCnt, pstIveSrc, pstIveDst);
        SAMPLE_CHK_PRINTF(s32Ret != HI_SUCCESS, "ResizeRoi failed\n");
        SAMPLE_FREE(pstIveSrc);
        SAMPLE_FREE(pstIveDst);
        return s32Ret;
    }
MALLOC_FREE:
    SAMPLE_FREE(pstIveSrc);
    SAMPLE_FREE(pstIveDst);
    return HI_FAILURE;
}
#endif

HI_S32 resizeROI(const HI_RUNTIME_BLOB_S *pstSrcBlob,
                 const HI_S32 as32Coord[],
                 HI_U32 u32CoordStride,
                 const HI_U16 u16RoiCnt,
                 HI_RUNTIME_BLOB_S *pstDstBlob,
                 const HI_U16 u16DstOffSetCnt)
{
#ifdef ON_BOARD
    HI_U16 i = 0;
    HI_BOOL bRet = HI_TRUE;

    for (i = 0; i < u16RoiCnt; i++) {
        bRet = IsSmallIveResizeRoi(&as32Coord[i * u32CoordStride]);

        if (bRet == HI_TRUE) {
            printf("Small resize roi[x1=%d, y1=%d, x2=%d, y2=%d]!\n",
                   as32Coord[i * u32CoordStride + X_MIN_IDX], as32Coord[i * u32CoordStride + Y_MIN_IDX],
                   as32Coord[i * u32CoordStride + X_MAX_IDX], as32Coord[i * u32CoordStride + Y_MAX_IDX]);
            printf("OPENCV hasn't be linked, plz open it first!\n");
            return HI_FAILURE;
        }
    }

    return resizeROIByIVE(pstSrcBlob, as32Coord, u32CoordStride, u16RoiCnt, pstDstBlob, u16DstOffSetCnt);
#else
    HI_SAMPLE_SRC_DST_BLOB_S stBlob;
    stBlob.pstSrcBlob = pstSrcBlob;
    stBlob.pstDstBlob = pstDstBlob;
    return resizeROIByCV(&stBlob, as32Coord, u32CoordStride, u16RoiCnt, u16DstOffSetCnt);
#endif
}

HI_S32 resizeBlob(const HI_RUNTIME_BLOB_S *pstSrcBlob,
                  HI_RUNTIME_BLOB_S *pstDstBlob)
{
    if ((pstSrcBlob->unShape.stWhc.u32Chn != IMG_CHANNEL) || (pstDstBlob->unShape.stWhc.u32Chn != IMG_CHANNEL)) {
        SAMPLE_LOG_INFO("Invalid input channel number, only support 3 for resizeBlob\n");
        return HI_FAILURE;
    }

#ifdef ON_BOARD
    return resizeByIVE(pstSrcBlob, pstDstBlob);
#else
    return resizeByCV(pstSrcBlob, pstDstBlob);
#endif
}

HI_S32 cropPadBlob(const HI_RUNTIME_BLOB_S *pstSrcBlob,
                   const HI_RUNTIME_BLOB_S *pstBboxBlob,
                   HI_RUNTIME_BLOB_S *pstDstBlob,
                   TRACKER_LOCATION_S *pstLocation,
                   HI_S32 *ps32EdgeX,
                   HI_S32 *ps32EdgeY,
                   HI_S32 *ps32RegionW,
                   HI_S32 *ps32RegionH)
{
#ifdef ON_BOARD
    return cropPadBlobByIVE(pstSrcBlob, pstBboxBlob, pstDstBlob, pstLocation, ps32EdgeX, ps32EdgeY, ps32RegionW,
                            ps32RegionH);
#else
    HI_SAMPLE_SRC_DST_BLOB_S stBlob;
    stBlob.pstSrcBlob = pstSrcBlob;
    stBlob.pstDstBlob = pstDstBlob;
    HI_SAMPLE_EDGE_S stEdge;
    stEdge.ps32EdgeX = ps32EdgeX;
    stEdge.ps32EdgeY = ps32EdgeY;
    HI_RUNTIME_REGION_S stRegion;
    stRegion.ps32RegionW = ps32RegionW;
    stRegion.ps32RegionH = ps32RegionH;
    return cropPadBlobByCV(&stBlob, pstBboxBlob, pstLocation, &stEdge, &stRegion);
#endif
}

HI_VOID drawImageRect(const HI_CHAR *pszPicPath, const HI_RUNTIME_BLOB_S *pstBlob,
                      HI_S32 as32Coord[], HI_U32 u32CoordCnt, HI_U32 u32CoordStride)
{
    HI_CHAR aszFileName[PATH_MAX] = {0};
#ifdef ON_BOARD
    snprintf(aszFileName, sizeof(aszFileName) - 1, "%s.ppm", pszPicPath);
    drawImageRectByIVE(aszFileName, pstBlob, as32Coord, u32CoordCnt, u32CoordStride);
    return;
#else
    snprintf(aszFileName, sizeof(aszFileName) - 1, "%s.png", pszPicPath);
    drawImageRectByCV(aszFileName, pstBlob, as32Coord, u32CoordCnt, u32CoordStride);
    return;
#endif
}
