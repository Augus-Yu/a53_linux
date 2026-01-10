#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>

#include "sample_comm_ive.h"


static HI_S32 SAMPLE_IVE_GetTimeDiff(struct timeval *pstTimeBegin, struct timeval *pstTimeEnd)
{
    HI_S32 s32Diff;

    s32Diff = ((pstTimeEnd->tv_sec - pstTimeBegin->tv_sec) * 1000000
        + (HI_S32)(pstTimeEnd->tv_usec - pstTimeBegin->tv_usec));

    return s32Diff;
}
HI_VOID SAMPLE_IVE_Pkg2Sp(HI_VOID)
{
    FILE *pSrcFp = NULL;
    FILE *pDstSpFp = NULL;
    FILE *pDstpFp = NULL;
    HI_CHAR *pchSrcFileName = "./data/input/pkg2sp/1920x1080_package_yuv422.yuv";
    HI_CHAR *pchDstFileNameSp = "./data/output/pkg2sp/1920x1080_sp422.yuv";
    HI_CHAR *pchDstFileNamep = "./data/output/pkg2sp/1920x1080_p422.yuv";
    HI_U32 u32Width = 1920;
    HI_U32 u32Height = 1080;
    HI_U32 i, j;
    HI_S32 s32Ret = HI_FAILURE;
    IVE_SRC_DATA_S stDataSrc = {0};
    IVE_DST_DATA_S stDataDst = {0};
    IVE_DMA_CTRL_S  stDmaCtrl = { IVE_DMA_MODE_INTERVAL_COPY, 0, 2, 1, 1 };
    IVE_DST_IMAGE_S stDstImage;
    IVE_SRC_MEM_INFO_S stSrcMemInfo;
    HI_U32 u32Size = 0;
    HI_U32 u32Stride;
    HI_U8 *pu8Tmp;
    IVE_HANDLE IveHandle;
    HI_BOOL bFinish = HI_FALSE;
    struct timeval stTimeBegin;
    struct timeval stTimeEnd;
    SAMPLE_COMM_IVE_CheckIveMpiInit();

    pSrcFp = fopen(pchSrcFileName, "rb");
    SAMPLE_CHECK_EXPR_GOTO(HI_NULL == pSrcFp, FAIL, "Error,Open file %s failed!\n", pchSrcFileName);
    pDstSpFp = fopen(pchDstFileNameSp, "wb");
    SAMPLE_CHECK_EXPR_GOTO(HI_NULL == pDstSpFp, FAIL, "Error,Open file %s failed!\n", pchDstFileNameSp);
    pDstpFp = fopen(pchDstFileNamep, "wb");
    SAMPLE_CHECK_EXPR_GOTO(HI_NULL == pDstpFp, FAIL, "Error,Open file %s failed!\n", pchDstFileNamep);

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&stDstImage, IVE_IMAGE_TYPE_YUV422SP, u32Width, u32Height);
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL, "Error(%#x),Create dst image failed!\n", s32Ret);

    u32Stride = SAMPLE_COMM_IVE_CalcStride(u32Width * 2, IVE_ALIGN);
    u32Size =  u32Stride* u32Height;
    s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&stSrcMemInfo, u32Size);
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL, "Error(%#x),Create mem info failed!\n", s32Ret);

    pu8Tmp = (HI_U8 *)(HI_UL)stSrcMemInfo.u64VirAddr;
    for (i = 0; i< u32Height; i++)
    {
        SAMPLE_CHECK_EXPR_GOTO(1 != fread(pu8Tmp, u32Width * 2, 1, pSrcFp), FAIL, "Error,read file  failed!\n");
        pu8Tmp += u32Stride;
    }
    gettimeofday(&stTimeBegin,NULL);
    /*
    *The max dma resolution is 1920*1080,divide package image to four part:
    *1.Left Y
    *2.right Y
    *3.Left uv
    *4.right uv
    */
    /*
    *Left Y
    */
    stDataSrc.u64VirAddr     = stSrcMemInfo.u64VirAddr;
    stDataSrc.u64PhyAddr     = stSrcMemInfo.u64PhyAddr;
    stDataSrc.u32Width       = u32Width;
    stDataSrc.u32Height     = u32Height;
    stDataSrc.u32Stride     = u32Stride;

    stDataDst.u64VirAddr     = stDstImage.au64VirAddr[0];
    stDataDst.u64PhyAddr     = stDstImage.au64PhyAddr[0];
    stDataDst.u32Width       = u32Width/2;
    stDataDst.u32Height     = u32Height;
    stDataDst.u32Stride     = stDstImage.au32Stride[0];
    s32Ret = HI_MPI_IVE_DMA(&IveHandle, &stDataSrc, &stDataDst,&stDmaCtrl,HI_FALSE);   
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL, "Error(%#x),HI_MPI_IVE_DMA failed!\n",s32Ret);
    /*
    *right Y
    */
    stDataSrc.u64VirAddr     = stSrcMemInfo.u64VirAddr + u32Width;
    stDataSrc.u64PhyAddr     = stSrcMemInfo.u64PhyAddr + u32Width;
    stDataSrc.u32Width       = u32Width;
    stDataSrc.u32Height     = u32Height;
    stDataSrc.u32Stride     = u32Stride;

    stDataDst.u64VirAddr     = stDstImage.au64VirAddr[0] + u32Width/2;
    stDataDst.u64PhyAddr     = stDstImage.au64PhyAddr[0] + u32Width/2;
    stDataDst.u32Width       = u32Width/2;
    stDataDst.u32Height     = u32Height;
    stDataDst.u32Stride     = stDstImage.au32Stride[0];
    s32Ret = HI_MPI_IVE_DMA(&IveHandle, &stDataSrc, &stDataDst,&stDmaCtrl,HI_FALSE);   
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL,
    "Error(%#x),HI_MPI_IVE_DMA failed!\n",s32Ret);
    /*
    *Left uv
    */
    stDataSrc.u64VirAddr     = stSrcMemInfo.u64VirAddr + sizeof(HI_U8);
    stDataSrc.u64PhyAddr     = stSrcMemInfo.u64PhyAddr + sizeof(HI_U8);
    stDataSrc.u32Width       = u32Width;
    stDataSrc.u32Height     = u32Height;
    stDataSrc.u32Stride     = u32Stride;

    stDataDst.u64VirAddr     = stDstImage.au64VirAddr[1];
    stDataDst.u64PhyAddr     = stDstImage.au64PhyAddr[1];
    stDataDst.u32Width       = u32Width/2;
    stDataDst.u32Height     = u32Height;
    stDataDst.u32Stride     = stDstImage.au32Stride[1];
    s32Ret = HI_MPI_IVE_DMA(&IveHandle, &stDataSrc, &stDataDst,&stDmaCtrl,HI_FALSE);
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL,
    "Error(%#x),HI_MPI_IVE_DMA failed!\n",s32Ret);
    /*
    *right uv
    *note: the last row address (width +1) must be is valide.
    */
    stDataSrc.u64VirAddr     = stSrcMemInfo.u64VirAddr + sizeof(HI_U8) + u32Width;
    stDataSrc.u64PhyAddr     = stSrcMemInfo.u64PhyAddr + sizeof(HI_U8) + u32Width;
    stDataSrc.u32Width       = u32Width;
    stDataSrc.u32Height     = u32Height;
    stDataSrc.u32Stride     = u32Stride;

    stDataDst.u64VirAddr     = stDstImage.au64VirAddr[1] + u32Width/2;
    stDataDst.u64PhyAddr     = stDstImage.au64PhyAddr[1] + u32Width/2;
    stDataDst.u32Width       = u32Width/2;
    stDataDst.u32Height     = u32Height;
    stDataDst.u32Stride     = stDstImage.au32Stride[1];
    s32Ret = HI_MPI_IVE_DMA(&IveHandle, &stDataSrc, &stDataDst,&stDmaCtrl,HI_FALSE);
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL,
    "Error(%#x),HI_MPI_IVE_DMA failed!\n",s32Ret);

    s32Ret = HI_MPI_IVE_Query(IveHandle, &bFinish, HI_TRUE);
    while (HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
    {
        usleep(100);
        s32Ret = HI_MPI_IVE_Query(IveHandle, &bFinish, HI_TRUE);
    }
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL, "Error(%#x),HI_MPI_IVE_Query failed!\n",s32Ret);
    gettimeofday(&stTimeEnd,NULL);
    SAMPLE_PRT("Time:%d\n", SAMPLE_IVE_GetTimeDiff(&stTimeBegin,&stTimeEnd));
    s32Ret = SAMPLE_COMM_IVE_WriteFile(&stDstImage, pDstSpFp);
    SAMPLE_CHECK_EXPR_GOTO(HI_SUCCESS != s32Ret, FAIL, "Error(%#x),SAMPLE_COMM_IVE_WriteFile failed!\n",s32Ret);

    pu8Tmp = (HI_U8 *)(HI_UL)stDstImage.au64VirAddr[0];
    for (i = 0; i < u32Height; i++)
    {
        SAMPLE_CHECK_EXPR_GOTO(u32Width != fwrite(pu8Tmp, 1, u32Width, pDstpFp), FAIL, "Error,fwrite failed!\n");
        pu8Tmp += stDstImage.au32Stride[0];
    }

    pu8Tmp = (HI_U8 *)(HI_UL)stDstImage.au64VirAddr[1];
    for (i = 0; i < u32Height; i++)
    {
        for (j = 0; j < u32Width/2;j++)
        {
            SAMPLE_CHECK_EXPR_GOTO(1 != fwrite(pu8Tmp + 2*j, 1, 1, pDstpFp), FAIL, "Error,fwrite failed!\n"); 
        }
        pu8Tmp += stDstImage.au32Stride[1];
    }
    pu8Tmp = (HI_U8 *)(HI_UL)stDstImage.au64VirAddr[1];
    for (i = 0; i < u32Height; i++)
    {
        for (j = 0; j < u32Width/2;j++)
        {
            SAMPLE_CHECK_EXPR_GOTO(1 != fwrite(pu8Tmp + 2*j + 1, 1, 1, pDstpFp), FAIL,"Error,fwrite failed!\n"); 
        }
        pu8Tmp += stDstImage.au32Stride[1];
    }

FAIL:
    IVE_MMZ_FREE(stDstImage.au64PhyAddr[0], stDstImage.au64VirAddr[0]);
    IVE_MMZ_FREE(stSrcMemInfo.u64PhyAddr, stSrcMemInfo.u64VirAddr);

    IVE_CLOSE_FILE(pSrcFp);
    IVE_CLOSE_FILE(pDstSpFp);
    IVE_CLOSE_FILE(pDstpFp);
    SAMPLE_COMM_IVE_IveMpiExit();
}
HI_VOID SAMPLE_IVE_Pkg2Sp_HandleSig(HI_VOID)
{
    SAMPLE_COMM_IVE_IveMpiExit();
}

