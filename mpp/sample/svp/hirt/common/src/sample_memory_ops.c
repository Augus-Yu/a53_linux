/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-05-19
 */

#include "sample_memory_ops.h"
#include <stdlib.h>
#ifdef ON_BOARD
#include "mpi_sys.h"
#endif
#include "sample_log.h"

HI_S32 SAMPLE_AllocMem(HI_RUNTIME_MEM_S *pstMemInfo, HI_BOOL bCached)
{
    SAMPLE_CHK_RET((pstMemInfo == HI_NULL), HI_FAILURE, "param pstMemInfo is NULL\n");
    // SAMPLE_CHK_RET((HI_NULL == pstMemInfo->u64PhyAddr), HI_FAILURE, "param u64PhyAddr is 0\n");
#ifdef ON_BOARD
    HI_S32 s32Ret = HI_SUCCESS;
    if (bCached) {
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&pstMemInfo->u64PhyAddr,
                                            (HI_VOID **)&(pstMemInfo->u64VirAddr), NULL, HI_NULL,
                                            pstMemInfo->u32Size);
    } else {
        s32Ret = HI_MPI_SYS_MmzAlloc(&pstMemInfo->u64PhyAddr, (HI_VOID **)&(pstMemInfo->u64VirAddr), NULL, HI_NULL,
                                     pstMemInfo->u32Size);
    }
#else
    pstMemInfo->u64VirAddr = (HI_U64)((uintptr_t)malloc(pstMemInfo->u32Size));
    HI_S32 s32Ret = pstMemInfo->u64VirAddr == 0 ? HI_FAILURE : HI_SUCCESS;
    pstMemInfo->u64PhyAddr = pstMemInfo->u64VirAddr;
#endif

    return s32Ret;
}

HI_S32 SAMPLE_FlushCache(HI_RUNTIME_MEM_S *pstMemInfo)
{
    SAMPLE_CHK_RET((pstMemInfo->u64VirAddr == 0), HI_FAILURE, "param pu8VirAddr is NULL\n");
#ifdef ON_BOARD
    return HI_MPI_SYS_MmzFlushCache(pstMemInfo->u64PhyAddr, (HI_VOID *)((uintptr_t)pstMemInfo->u64VirAddr),
                                    pstMemInfo->u32Size);
#endif
    return HI_SUCCESS;
}

HI_S32 SAMPLE_FreeMem(HI_RUNTIME_MEM_S *pstMemInfo)
{
    SAMPLE_CHK_RET((pstMemInfo->u64VirAddr == 0), HI_FAILURE, "param pu8VirAddr is NULL\n");
#ifdef ON_BOARD
    return HI_MPI_SYS_MmzFree (pstMemInfo->u64PhyAddr, (HI_VOID *)((uintptr_t)pstMemInfo->u64VirAddr));
#else
    free ((HI_U8 *)((uintptr_t) pstMemInfo->u64VirAddr));
    pstMemInfo->u64PhyAddr = 0;
    pstMemInfo->u64VirAddr = 0;
#endif
    return HI_SUCCESS;
}
