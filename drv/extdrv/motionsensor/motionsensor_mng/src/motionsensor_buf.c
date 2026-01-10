#include <linux/kernel.h>
#include "hi_debug.h"
#include "motionsensor_buf.h"
#include "motionsensor_proc.h"
#include "motionsensor_exe.h"
#include "mpi_sys.h"


static hi_bool g_bForward = HI_TRUE;
static hi_bool g_bBufInit = HI_FALSE;
MSENSOR_BUF_INFO_S g_astBufInfo[MSENSOR_DATA_BUTT][DATA_TYPE_BUTT];
static hi_s64 g_s64Offset;
hi_bool g_bAlreadyReleased[MAX_USER_NUM];
MSENSOR_BUF_USER_MNG_S g_stUserMng;
#define DATA_RESERVE_NUM 50

extern hi_s32 MOTIONSENSOR_BUF_WriteData2Buf(hi_void);

#define TEST_ON 0
__inline static hi_void* MOTIONSENSOR_Remap_Nocache(hi_u64 phy_addr,hi_u32 u32Size)
{
#if TEST_ON
    return phy_addr;
#else
    return osal_ioremap_nocache(phy_addr, HIALIGN(u32Size, 4));

#endif
}

__inline static hi_void  MOTIONSENSOR_Unmap(hi_void* pVirAddr)
{
#if TEST_ON

#else
    osal_iounmap(pVirAddr);
#endif
}

hi_s32 MOTIONSENSOR_GetUserId(hi_s32* ps32Id)
{
    hi_s32 i;

    for (i = 0; i <MAX_USER_NUM; i++)
    {
        if (HI_NULL == g_stUserMng.pstUserContext[i])
        {
            *ps32Id = i;
            return HI_SUCCESS;
        }
    }

    HI_TRACE_MSENSOR(HI_DBG_ERR,"No free user for use.\n");

    return HI_FAILURE;
}



hi_s32 MOTIONSENSOR_BUF_AddUser(hi_s32* ps32Id)
{
    hi_s32 s32Ret = HI_SUCCESS;
    hi_ulong flags;
    //hi_ulong mngflags;
    hi_s32 i = 0;
    hi_s32 j = 0;

    MSENSOR_BUF_USER_CONTEXT_S* pstUserContext;

    *ps32Id = -1;

    if (HI_FALSE == g_bBufInit)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"Buf not init,g_bBufInit:%d\n",g_bBufInit);
        return HI_FAILURE;
    }

    osal_mutex_lock(&g_stUserMng.buf_mng_mutex);

    //Step 1: Get available ID
    if (g_stUserMng.u32UserCnt > MAX_USER_NUM)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"Motionsensor ID has reached toplimit.\n");
        goto ERROR_0;
    }

    /*get user id for use*/
    s32Ret = MOTIONSENSOR_GetUserId(ps32Id);
    if (HI_SUCCESS != s32Ret)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"GetUserId failed.\n");
        goto ERROR_0;
    }

    if((*ps32Id < 0)||(*ps32Id >= MAX_USER_NUM))
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"*ps32Id(%d) out of range[0,%d].\n",*ps32Id,MAX_USER_NUM);
        goto ERROR_0;
    }

    //Step 2: Create context for new ID
    pstUserContext = (MSENSOR_BUF_USER_CONTEXT_S*)osal_vmalloc(sizeof(MSENSOR_BUF_USER_CONTEXT_S));

    if (HI_NULL == pstUserContext)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"vmalloc failed.\n");
        goto ERROR_0;
    }

    osal_memset(pstUserContext, 0, sizeof(MSENSOR_BUF_USER_CONTEXT_S));
    //osal_spin_lock_init(&pstUserContext->read_lock);

    osal_spin_lock_irqsave(&g_stUserMng.stUserSync.read_lock[*ps32Id], &flags);

    for (i = 0; i < DATA_TYPE_BUTT; i++)
    {

        if (DATA_TYPE_PTS == i)
        {
            for (j = 0; j < MSENSOR_DATA_BUTT; j++)
            {
                if (g_stMotionsensorProcInfo.au32BufSize[j] > DATA_RESERVE_NUM * BUF_BLOCK_NUM)
                {
                    ///osal_printk(" line=%d i:%d j:%d\n", __LINE__,i,j);
                    ///osal_printk("AccelBuff %%%%%% [%d]pStartAddr:%p pWritePointer:%p\n",i,g_astBufInfo[j][i].pWritePointer,g_astBufInfo[j][i].pStartAddr);

                    #if 1
                    pstUserContext->pReadPointer[j][i] = (unsigned long long int *)g_astBufInfo[j][i].pWritePointer;
                    #else
                    if ((unsigned long long*)g_astBufInfo[j][i].pWritePointer - (unsigned long long*)g_astBufInfo[j][i].pStartAddr>= DATA_RESERVE_NUM)
                    {
                        ///osal_printk("###line=%d i:%d j:%d Size:%d\n", __LINE__,i,j,g_stMotionsensorProcInfo.au32BufSize[j]);
                        pstUserContext->pReadPointer[j][i] = (unsigned long long*)g_astBufInfo[j][i].pWritePointer - DATA_RESERVE_NUM;
                    }
                    else
                    {
                        ///osal_printk("***line=%d i:%d j:%d Size:%d\n", __LINE__,i,j,g_stMotionsensorProcInfo.au32BufSize[j]);
                        osal_printk("***line=%d i:%d j:%d\n", __LINE__,i,j);
                        //osal_printk(" line=%d i:%d j:%d\n", __LINE__,i,j);
                        //osal_printk(" line=%d i:%d j:%d pStartAddr:%p au32BufSize:%d pWritePointer:%p pStartAddr:%p\n", __LINE__,i,j,
                        //    g_astBufInfo[j][i].pStartAddr,g_stMotionsensorProcInfo.au32BufSize[j],g_astBufInfo[j][i].pWritePointer,g_astBufInfo[j][i].pStartAddr);

                        //osal_printk(" line=%d Mid:%p\n", __LINE__,
                        //    g_astBufInfo[j][i].pStartAddr + g_stMotionsensorProcInfo.au32BufSize[j] * 2 / BUF_BLOCK_NUM);
                        ///pstUserContext->pReadPointer[j][i] = (unsigned long long*)g_astBufInfo[j][i].pStartAddr + g_stMotionsensorProcInfo.au32BufSize[j]  / BUF_BLOCK_NUM -
                        ///                                     (DATA_RESERVE_NUM - ((unsigned long long*)g_astBufInfo[j][i].pWritePointer - (unsigned long long*)g_astBufInfo[j][i].pStartAddr));

                        pstUserContext->pReadPointer[j][i] = (unsigned long long*)g_astBufInfo[j][i].pStartAddr + g_stMotionsensorProcInfo.au32BufSize[j] / BUF_BLOCK_NUM -
                                                            (DATA_RESERVE_NUM - ((unsigned long long*)g_astBufInfo[j][i].pWritePointer - (unsigned long long*)g_astBufInfo[j][i].pStartAddr));

                    }
                    #endif
                    //osal_printk("XXXXXX j=%d,read=%p,write=%p\n", j, pstUserContext->pReadPointer[j][i], g_astBufInfo[j][i].pWritePointer);
                }
                else
                {
                    /////osal_printk("line=%d\n", __LINE__);
                    pstUserContext->pReadPointer[j][i] = g_astBufInfo[j][i].pWritePointer;
                }
            }

            //osal_printk("------------j=%d,read=0x%x,write=0x%x\n",j,pstUserContext->pReadPointer[j][i],g_astBufInfo[j][i].pWritePointer);
        }
        else
        {
            for (j = 0; j < MSENSOR_DATA_BUTT; j++)
            {
                #if 1
                pstUserContext->pReadPointer[j][i] = (int *)g_astBufInfo[j][i].pWritePointer;
                #else
                if (g_stMotionsensorProcInfo.au32BufSize[j] > DATA_RESERVE_NUM * BUF_BLOCK_NUM)
                {
                    if ((int*)g_astBufInfo[j][i].pWritePointer - (int*)g_astBufInfo[j][i].pStartAddr >= DATA_RESERVE_NUM)
                    {
                        //osal_printk(" line=%d i:%d j:%d\n", __LINE__,i,j);
                        pstUserContext->pReadPointer[j][i] = (int*)g_astBufInfo[j][i].pWritePointer - DATA_RESERVE_NUM;
                    }
                    else
                    {
                        pstUserContext->pReadPointer[j][i] = (int*)g_astBufInfo[j][i].pStartAddr + g_stMotionsensorProcInfo.au32BufSize[j] / BUF_BLOCK_NUM -
                                                             (DATA_RESERVE_NUM - ((int*)g_astBufInfo[j][i].pWritePointer - (int*)g_astBufInfo[j][i].pStartAddr));

                        //pstUserContext->pReadPointer[j][i] = g_astBufInfo[j][i].pStartAddr + g_stMotionsensorProcInfo.au32BufSize[j] * 2 / BUF_BLOCK_NUM -
                        //                                    2*(DATA_RESERVE_NUM - ((unsigned long long*)g_astBufInfo[j][i].pWritePointer - (unsigned long long*)g_astBufInfo[j][i].pStartAddr));
                    }
                }
                else
                {
                    pstUserContext->pReadPointer[j][i] = g_astBufInfo[j][i].pWritePointer;
                }
                #endif

            }
        }

    }

    //osal_spin_lock_irqsave(&g_stUserMng.msensormng_lock, &mngflags);

    g_bForward = HI_TRUE;
    g_stUserMng.pstUserContext[*ps32Id] = pstUserContext;

    //osal_printk("###########fun:%s *ps32Id:%d pReadPointer[GYRO][PTS]:%p\n",__func__,*ps32Id,g_stUserMng.pstUserContext[*ps32Id]->pReadPointer[MSENSOR_DATA_GYRO][DATA_TYPE_PTS]);

    g_stUserMng.u32UserCnt++;

    //osal_spin_unlock_irqrestore(&g_stUserMng.msensormng_lock, &mngflags);

    osal_spin_unlock_irqrestore(&g_stUserMng.stUserSync.read_lock[*ps32Id], &flags);

    osal_mutex_unlock(&g_stUserMng.buf_mng_mutex);

    osal_msleep(200);

    return HI_SUCCESS;
ERROR_0:
    osal_mutex_unlock(&g_stUserMng.buf_mng_mutex);
    return HI_FAILURE;
}

hi_s32 MOTIONSENSOR_BUF_DeleteUser(hi_s32* ps32Id)
{
    MSENSOR_BUF_USER_CONTEXT_S* pstMSenserBufUserContexTemp = HI_NULL;
    unsigned long flags;

    if((*ps32Id < 0)||(*ps32Id >= MAX_USER_NUM))
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"*ps32Id(%d) out of range[0,%d].\n",*ps32Id,MAX_USER_NUM);
        return HI_FAILURE;
    }

    if (HI_FALSE == g_bBufInit)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"When Delete User,Motionsensor buffer hasn't been initialised.\n");
        return HI_FAILURE;
    }

    if (HI_NULL == g_stUserMng.pstUserContext[*ps32Id])
    {
        HI_TRACE_MSENSOR(HI_DBG_DEBUG,"u32Id is NULL.\n");
        return HI_SUCCESS;
    }

    osal_mutex_lock(&g_stUserMng.buf_mng_mutex);

    osal_spin_lock_irqsave(&g_stUserMng.msensormng_lock, &flags);

    //Release context
    pstMSenserBufUserContexTemp = g_stUserMng.pstUserContext[*ps32Id];

    g_stUserMng.pstUserContext[*ps32Id] = HI_NULL;
    g_stUserMng.u32UserCnt--;

    osal_spin_unlock_irqrestore(&g_stUserMng.msensormng_lock, &flags);
    osal_mutex_unlock(&g_stUserMng.buf_mng_mutex);

    if(HI_NULL != pstMSenserBufUserContexTemp)
    {
        osal_vfree(pstMSenserBufUserContexTemp);
    }

    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_BUF_Init(hi_msensor_buf_attr* pstBufAttr,
    hi_u32 u32GyroFreq,
    hi_u32 u32AccelFreq,
    hi_u32 u32MagFreq)
{
    hi_void* pVirAddr = HI_NULL;
    hi_u32 u32GyroBlockSize;
    hi_u32 u32AccelBlockSize;
    hi_u32 u32MagBlockSize;
    hi_s32 i;

    if (HI_TRUE == g_bBufInit)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"buf already inited\n");
        return HI_SUCCESS;
    }

    if (0 == pstBufAttr->phy_addr || 0 == pstBufAttr->buflen)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"buf addr can not be null and buf size must lager than 0\n");
        return HI_FAILURE;
    }

    if (0 == u32GyroFreq && 0 == u32AccelFreq && 0 == u32MagFreq)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"can't all frequency be 0\n");
        return HI_FAILURE;
    }

    pVirAddr = MOTIONSENSOR_Remap_Nocache((unsigned long long int)pstBufAttr->phy_addr, pstBufAttr->buflen);

    if (HI_NULL == pVirAddr)
    {
         HI_TRACE_MSENSOR(HI_DBG_ERR,"ioremap err\n");
        return HI_FAILURE;
    }

    osal_memset(pVirAddr,0,pstBufAttr->buflen);

    HI_TRACE_MSENSOR(HI_DBG_DEBUG,"phy_addr:%llx pVirAddr:%p buflen:%d\n",
        pstBufAttr->phy_addr,pVirAddr,pstBufAttr->buflen);

    g_s64Offset = pstBufAttr->phy_addr - (hi_u64)(hi_ulong)pVirAddr;
    u32GyroBlockSize = pstBufAttr->buflen * u32GyroFreq / (u32GyroFreq + u32AccelFreq + u32MagFreq) / BUF_BLOCK_NUM / 32 * 32;
    u32AccelBlockSize = pstBufAttr->buflen * u32AccelFreq / (u32GyroFreq + u32AccelFreq + u32MagFreq) / BUF_BLOCK_NUM / 32 * 32;
    u32MagBlockSize = (pstBufAttr->buflen / BUF_BLOCK_NUM - u32GyroBlockSize - u32AccelBlockSize) / 32 * 32;

    //osal_printk("u32GyroFreq:%d u32AccelFreq:%d u32MagFreq:%d\n",u32GyroFreq, u32AccelFreq, u32MagFreq);
    //osal_printk("GyroBuff Total_u32Buflen:%d u32GyroBlockSize:%d,u32AccelBlockSize:%d,u32MagBlockSize:%d\n",pstBufAttr->buflen,u32GyroBlockSize,u32AccelBlockSize,u32MagBlockSize);

    for (i = 0; i < DATA_TYPE_BUTT; i++)
    {
        g_astBufInfo[MSENSOR_DATA_GYRO][i].pStartAddr = (hi_u8 *)pVirAddr + u32GyroBlockSize * i;
        g_astBufInfo[MSENSOR_DATA_GYRO][i].pWritePointer = g_astBufInfo[MSENSOR_DATA_GYRO][i].pStartAddr;

        //osal_printk("GyroBuff u32GyroBlockSize:%d [%d]pStartAddr:%p\n",u32GyroBlockSize,i,g_astBufInfo[MOTIONSENSOR_DATA_GYRO][i].pStartAddr);

        g_astBufInfo[MSENSOR_DATA_ACC][i].pStartAddr = (hi_u8 *)pVirAddr + u32GyroBlockSize * BUF_BLOCK_NUM + u32AccelBlockSize * i;
        g_astBufInfo[MSENSOR_DATA_ACC][i].pWritePointer = g_astBufInfo[MSENSOR_DATA_ACC][i].pStartAddr;

        //osal_printk("AccelBuff ***** [%d]pStartAddr:%p pWritePointer:%p\n",i,g_astBufInfo[MOTIONSENSOR_DATA_ACCEL][i].pStartAddr,g_astBufInfo[MOTIONSENSOR_DATA_ACCEL][i].pWritePointer);
        //osal_printk("AccelBuff u32AccelBlockSize:%d [%d]pStartAddr:%p\n",u32AccelBlockSize,i,g_astBufInfo[MOTIONSENSOR_DATA_ACCEL][i].pStartAddr);
        g_astBufInfo[MSENSOR_DATA_MAGN][i].pStartAddr = (hi_u8 *)pVirAddr + u32GyroBlockSize * BUF_BLOCK_NUM + u32AccelBlockSize * BUF_BLOCK_NUM + u32MagBlockSize * i;
        g_astBufInfo[MSENSOR_DATA_MAGN][i].pWritePointer = g_astBufInfo[MSENSOR_DATA_MAGN][i].pStartAddr;
    }

    HI_TRACE_MSENSOR(HI_DBG_DEBUG,"**PTS-pWritePointer:%p\n",g_astBufInfo[MSENSOR_DATA_GYRO][DATA_TYPE_PTS].pWritePointer);

    g_stMotionsensorProcInfo.au64BufAddr[MSENSOR_DATA_GYRO] = (unsigned long long int)(hi_ulong)g_astBufInfo[MSENSOR_DATA_GYRO][DATA_TYPE_X].pStartAddr;
    g_stMotionsensorProcInfo.au64BufAddr[MSENSOR_DATA_ACC] = (unsigned long long  int)(hi_ulong)g_astBufInfo[MSENSOR_DATA_ACC][DATA_TYPE_X].pStartAddr;
    g_stMotionsensorProcInfo.au64BufAddr[MSENSOR_DATA_MAGN] = (unsigned long long  int)(hi_ulong)g_astBufInfo[MSENSOR_DATA_MAGN][DATA_TYPE_X].pStartAddr;

    //osal_printk("au64BufAddr[MOTIONSENSOR_DATA_GYRO]:0x%llx au64BufAddr[MOTIONSENSOR_DATA_ACCEL]:0x%llx\n",g_stMotionsensorProcInfo.au64BufAddr[MOTIONSENSOR_DATA_GYRO],g_stMotionsensorProcInfo.au64BufAddr[MOTIONSENSOR_DATA_ACCEL]);
    g_stMotionsensorProcInfo.au32BufSize[MSENSOR_DATA_GYRO] = u32GyroBlockSize * BUF_BLOCK_NUM;
    g_stMotionsensorProcInfo.au32BufSize[MSENSOR_DATA_ACC] = u32AccelBlockSize * BUF_BLOCK_NUM;
    g_stMotionsensorProcInfo.au32BufSize[MSENSOR_DATA_MAGN] = u32MagBlockSize * BUF_BLOCK_NUM;


    HI_TRACE_MSENSOR(HI_DBG_DEBUG,"#####GYRO-au64BufAddr:%llu ACC-au64BufAddr:%llu MAGN-au64BufAddr:%llu\n",
        g_stMotionsensorProcInfo.au64BufAddr[MSENSOR_DATA_GYRO],
        g_stMotionsensorProcInfo.au64BufAddr[MSENSOR_DATA_ACC],
        g_stMotionsensorProcInfo.au64BufAddr[MSENSOR_DATA_MAGN]);

    //osal_memset(&g_stUserMng, 0, sizeof(MOTIONSENSOR_BUF_USER_MNG_S));
    //MOTIONSENSOR_BUF_SyncInit();
    osal_mutex_init(&g_stUserMng.buf_mng_mutex);

    for (i = 0; i < MAX_USER_NUM; i++)
    {
        g_bAlreadyReleased[i] = true;
    }

    g_bBufInit = HI_TRUE;
    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_BUF_Deinit(hi_void)
{
    hi_s32 i = 0;

    if (HI_FALSE == g_bBufInit)
    {
         HI_TRACE_MSENSOR(HI_DBG_ERR,"buf already deinited\n");
        return HI_SUCCESS;
    }

    for (i = 0; i < MAX_USER_NUM; i++)
    {
        MOTIONSENSOR_BUF_DeleteUser(&i);
    }

    MOTIONSENSOR_Unmap(g_astBufInfo[MSENSOR_DATA_GYRO][DATA_TYPE_X].pStartAddr);

    //osal_printk("####fun:%s line:%d++\n",__func__,__LINE__);
    osal_memset(&g_astBufInfo, 0, sizeof(g_astBufInfo));
    g_bBufInit = HI_FALSE;
    //osal_mutex_destory(&g_stUserMng.buf_mng_mutex);
    //MOTIONSENSOR_BUF_SyncDeInit();

    return HI_SUCCESS;
}

static hi_u64 u64Times = 0;
static hi_u64 u64LastPTS = 0;

hi_s32 inline MOTIONSENSOR_BUF_WriteData(hi_msensor_data_type data_type, hi_s32 x, hi_s32 y, hi_s32 z, hi_s32 temp, hi_u64 pts)
{
    hi_s32 i = 0;
    hi_void* pNextWritePointer = HI_NULL;
    hi_void* pTempPointer = HI_NULL;

    if (HI_FALSE == g_bBufInit)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"when WriteData(pts:%lld),motionsensor buf not be inited yet\n",pts);
        return HI_FAILURE;
    }

    u64LastPTS = pts;
    u64Times++;

    //Step 1:judge if any user overflow
    if ((hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr - (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer > sizeof(x)*WR_GAP)
    {
        pTempPointer = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer + sizeof(x) * WR_GAP;
    }
    else
    {
        pTempPointer = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_X].pStartAddr + sizeof(x) * WR_GAP
                       - ((hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr - (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer);
    }


    for (i = 0; i < MAX_USER_NUM; i++)
    {
        if (HI_NULL != g_stUserMng.pstUserContext[i])
        {
            if ((pTempPointer == g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr
                 && g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_X] == g_astBufInfo[data_type][DATA_TYPE_X].pStartAddr)
                || pTempPointer == g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_X])
            {
                g_stMotionsensorProcInfo.au32BufOverflow[data_type]++;
                g_stMotionsensorProcInfo.as32BufOverflowID[data_type] = i;
                osal_spin_lock(&g_stUserMng.stUserSync.read_lock[i]);


                #if 0
                g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_X]            = g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer;
                g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Y]            = g_astBufInfo[data_type][DATA_TYPE_Y].pWritePointer;
                g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Z]            = g_astBufInfo[data_type][DATA_TYPE_Z].pWritePointer;
                g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_TEMP]         = g_astBufInfo[data_type][DATA_TYPE_TEMP].pWritePointer;
                g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_PTS]          = g_astBufInfo[data_type][DATA_TYPE_PTS].pWritePointer;
                #else
                if((hi_u32 *)g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_X] + 1 >= (hi_u32 *)g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr)
                {
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_X]    = g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Y]    = g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Z]    = g_astBufInfo[data_type][DATA_TYPE_Z].pStartAddr;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_TEMP] = g_astBufInfo[data_type][DATA_TYPE_TEMP].pStartAddr;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_PTS]  = g_astBufInfo[data_type][DATA_TYPE_PTS].pStartAddr;
                }
                else
                {
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_X]    = ((hi_u32 *)g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_X]) + 1;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Y]    = ((hi_u32 *)g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Y])  + 1;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Z]    = ((hi_u32 *)g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_Z]) + 1;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_TEMP] = ((hi_u32 *)g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_TEMP])+ 1;
                    g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_PTS]  = ((hi_u64 *)g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_PTS]) + 1;
                }
                #endif

                //osal_printk("******(*Read):%d\n",*(hi_u64 *)(g_stUserMng.pstUserContext[i]->pReadPointer[data_type][DATA_TYPE_PTS]));
                osal_spin_unlock(&g_stUserMng.stUserSync.read_lock[i]);



            }
        }
    }

    #if 1
    //Step 2:write data
    *(int*)g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer    = x;
    *(int*)g_astBufInfo[data_type][DATA_TYPE_Y].pWritePointer    = y;
    *(int*)g_astBufInfo[data_type][DATA_TYPE_Z].pWritePointer    = z;
    *(int*)g_astBufInfo[data_type][DATA_TYPE_TEMP].pWritePointer = temp;
    *(unsigned long long int*)g_astBufInfo[data_type][DATA_TYPE_PTS].pWritePointer = pts;
    #endif

    //Step 3:calculate next write pointer
    pNextWritePointer = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer + sizeof(x);

    if (pNextWritePointer >= g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr)
    {
        //osal_printk("=======circle round====\n");
        g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer    = g_astBufInfo[data_type][DATA_TYPE_X].pStartAddr;
        g_astBufInfo[data_type][DATA_TYPE_Y].pWritePointer    = g_astBufInfo[data_type][DATA_TYPE_Y].pStartAddr;
        g_astBufInfo[data_type][DATA_TYPE_Z].pWritePointer    = g_astBufInfo[data_type][DATA_TYPE_Z].pStartAddr;
        g_astBufInfo[data_type][DATA_TYPE_TEMP].pWritePointer = g_astBufInfo[data_type][DATA_TYPE_TEMP].pStartAddr;
        g_astBufInfo[data_type][DATA_TYPE_PTS].pWritePointer  = g_astBufInfo[data_type][DATA_TYPE_PTS].pStartAddr;
    }
    else
    {
        g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer    = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_X].pWritePointer + sizeof(x);
        g_astBufInfo[data_type][DATA_TYPE_Y].pWritePointer    = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_Y].pWritePointer + sizeof(y);
        g_astBufInfo[data_type][DATA_TYPE_Z].pWritePointer    = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_Z].pWritePointer + sizeof(z);
        g_astBufInfo[data_type][DATA_TYPE_TEMP].pWritePointer = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_TEMP].pWritePointer + sizeof(temp);
        g_astBufInfo[data_type][DATA_TYPE_PTS].pWritePointer  = (hi_u8 *)g_astBufInfo[data_type][DATA_TYPE_PTS].pWritePointer + sizeof(pts);
    }

    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_BUF_ReleaseData(hi_void* pstMSensorData)
{
    //unsigned long mngflags;
    //unsigned long flags;
    hi_msensor_data_info* pstMSensorDataInfo = HI_NULL;

    pstMSensorDataInfo = (hi_msensor_data_info*)pstMSensorData;

    if((pstMSensorDataInfo->id < 0)||(pstMSensorDataInfo->id >= MAX_USER_NUM))
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"*ps32Id(%d) is out of range[0,%d].\n",pstMSensorDataInfo->id,MAX_USER_NUM);
        return HI_FAILURE;
    }

    if (HI_FALSE == g_bBufInit)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"when ReleaseData, motionsensor buf not be inited yet\n");
        return HI_FAILURE;
    }

    if (HI_NULL == g_stUserMng.pstUserContext[pstMSensorDataInfo->id])
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"MOTIONSENSOR_BUF_Release: s32Id is NULL.\n");
        return HI_FAILURE;
    }

    if(pstMSensorDataInfo->data_type >= MSENSOR_DATA_BUTT)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"data_type:%d is out of range.\n",pstMSensorDataInfo->data_type);
        return HI_FAILURE;
    }

    if (HI_TRUE == g_bAlreadyReleased[pstMSensorDataInfo->id])
    {
        return HI_SUCCESS;
    }

    osal_spin_lock(&g_stUserMng.stUserSync.read_lock[pstMSensorDataInfo->id]);

    if (0 == pstMSensorDataInfo->data[1].num)
    {
        if (pstMSensorDataInfo->data[0].num > 0)
        {
            if (((hi_u64)(hi_ulong)pstMSensorDataInfo->data[0].x_phy_addr + pstMSensorDataInfo->data[0].num * sizeof(int) - g_s64Offset) >= ((hi_u64)(hi_ulong)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr))
            {
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    = g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_X].pStartAddr;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    = g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    = g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Z].pStartAddr;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] = g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP].pStartAddr;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]  = g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr;
            }
            else
            {
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    = (hi_u8 *)pstMSensorDataInfo->data[0].x_phy_addr    + pstMSensorDataInfo->data[0].num * sizeof(int) - g_s64Offset;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    = (hi_u8 *)pstMSensorDataInfo->data[0].y_phy_addr    + pstMSensorDataInfo->data[0].num * sizeof(int) - g_s64Offset;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    = (hi_u8 *)pstMSensorDataInfo->data[0].z_phy_addr    + pstMSensorDataInfo->data[0].num * sizeof(int) - g_s64Offset;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] = (hi_u8 *)pstMSensorDataInfo->data[0].temp_phy_addr + pstMSensorDataInfo->data[0].num * sizeof(int) - g_s64Offset;
                g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]  = (hi_u8 *)pstMSensorDataInfo->data[0].pts_phy_addr  + pstMSensorDataInfo->data[0].num * sizeof(unsigned long long int) - g_s64Offset;
            }
        }
    }
    else
    {
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    = (hi_u8 *)pstMSensorDataInfo->data[1].x_phy_addr    + pstMSensorDataInfo->data[1].num * sizeof(int) - g_s64Offset;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    = (hi_u8 *)pstMSensorDataInfo->data[1].y_phy_addr    + pstMSensorDataInfo->data[1].num * sizeof(int) - g_s64Offset;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    = (hi_u8 *)pstMSensorDataInfo->data[1].z_phy_addr    + pstMSensorDataInfo->data[1].num * sizeof(int) - g_s64Offset;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] = (hi_u8 *)pstMSensorDataInfo->data[1].temp_phy_addr + pstMSensorDataInfo->data[1].num * sizeof(int) - g_s64Offset;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]  = (hi_u8 *)pstMSensorDataInfo->data[1].pts_phy_addr  + pstMSensorDataInfo->data[1].num * sizeof(unsigned long long int) - g_s64Offset;
    }

    //HI_TRACE_MSENSOR(HI_DBG_ERR,"Release::::X-pReadPointer:%p *pPtsReadPointer:%lld N0:%d N1:%d\n",
    //    g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X],
    //    *(hi_u64 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS],
    //    pstMSensorDataInfo->data[0].num,pstMSensorDataInfo->data[1].num);


    pstMSensorDataInfo->data[0].num = 0;
    pstMSensorDataInfo->data[1].num = 0;
    g_bAlreadyReleased[pstMSensorDataInfo->id] = HI_TRUE;

    osal_spin_unlock(&g_stUserMng.stUserSync.read_lock[pstMSensorDataInfo->id]);

    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_BUF_ArriveBackStartAddr(hi_msensor_data_info* pstMSensorDataInfo)
{
    hi_s32           length[2] = {0};
    hi_s32           buftotal_len =0;
    hi_bool          bLoopFlag = false;
    hi_u32  u32Interval;
    hi_u64* pPtsReadPointer;

    buftotal_len = (unsigned int*)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr
                        - (unsigned int*)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_X].pStartAddr;


    //HI_TRACE_MSENSOR(HI_DBG_ERR,"**##### buftotal_len:%d!\n",buftotal_len);

    pPtsReadPointer = g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS];

    while (1)
    {
        if ((*pPtsReadPointer <= pstMSensorDataInfo->begin_pts)
            || pPtsReadPointer == g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer)
        {
            //HI_TRACE_MSENSOR(HI_DBG_ERR,"*pPtsReadPointer:%lld begin_pts:%lld!\n",*pPtsReadPointer,pstMSensorDataInfo->begin_pts);
            //HI_TRACE_MSENSOR(HI_DBG_ERR,"pPtsReadPointer:%p pWritePointer:%p!\n",pPtsReadPointer,g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer);

            if( pPtsReadPointer == g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer)
            {
                HI_TRACE_MSENSOR(HI_DBG_ERR,"Arrive to WriteAddr!\n");
            }

            break;
        }

        if(pPtsReadPointer == g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr)
        {
            bLoopFlag = HI_TRUE;
            pPtsReadPointer = ( unsigned long long int* )g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr + buftotal_len - 1;
            //HI_TRACE_MSENSOR(HI_DBG_ERR,"*************round to end pPtsReadPointer:%p *pPtsReadPointer:%lld buftotal_len:%d!\n",pPtsReadPointer, *pPtsReadPointer, buftotal_len);
        }
        else
        {
            pPtsReadPointer--;
        }

        g_bForward = HI_FALSE;

        HI_TRACE_MSENSOR(HI_DBG_DEBUG,"#####pPtsReadPointer:%p %lld\n",pPtsReadPointer,*pPtsReadPointer);

        if (HI_TRUE == bLoopFlag)
        {
            length[1]++;
        }
        else
        {
            length[0]++;
        }

    }

    if(length[1]>0)
    {
        u32Interval = (hi_u64 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr + buftotal_len - pPtsReadPointer;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    = (hi_u32 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr    - u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    = (hi_u32 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Z].pStartAddr    - u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    = (hi_u32 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP].pStartAddr - u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] = (hi_u32 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr  - u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]  = pPtsReadPointer;
        //osal_printk("#####pts-start %lld\n",*(hi_u64 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]);

    }
    else
    {
        u32Interval = (hi_u64 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS] - pPtsReadPointer;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    =
            (hi_u32 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    - u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    =
            (hi_u32 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    - u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    =
            (hi_u32 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    - u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] =
            (hi_u32 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] - u32Interval;

        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]  = pPtsReadPointer;
    }


    return HI_SUCCESS;
}


int MOTIONSENSOR_BUF_GetData(hi_void* pstMSensorData)
{
    hi_s32 ret = 0;
    hi_u64 * pPtsReadPointer;
    hi_bool bLoopFlag = false;
    hi_bool bFirstFound = false;
    hi_u32 u32Interval;
    //unsigned long flags;
    //unsigned long mngflags;
    hi_msensor_data_info  stDataInfo = {0};
    hi_msensor_data_info* pstMSensorDataInfo = HI_NULL;

    hi_void* pXVirAddr = HI_NULL;
    hi_void* pYVirAddr = HI_NULL;
    hi_void* pZVirAddr = HI_NULL;
    hi_void* pTempVirAddr = HI_NULL;
    //hi_u64 u64nowpts = 0;

    if (HI_FALSE == g_bBufInit)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"Get Data,But Motionsensor buffer hasn't been initialised.\n");
        return HI_FAILURE;
    }

    if(HI_NULL == pstMSensorData)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"pstMotionsensorData(%p) is NULL!!!\n",pstMSensorData);
        return HI_FAILURE;
    }

    pstMSensorDataInfo = &stDataInfo;
    osal_memcpy(pstMSensorDataInfo,pstMSensorData,sizeof(hi_msensor_data_info));
    //osal_printk("---b=%llu,end=%llu\n",pstMSensorDataInfo->begin_pts,pstMSensorDataInfo->end_pts);

    if((pstMSensorDataInfo->id < 0)||(pstMSensorDataInfo->id >= MAX_USER_NUM))
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"id(%d) is out of range[0,%d].\n",pstMSensorDataInfo->id,MAX_USER_NUM);
        return HI_FAILURE;
    }

    if(pstMSensorDataInfo->data_type >= MSENSOR_DATA_BUTT)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"data_type:%d out of range.\n",pstMSensorDataInfo->data_type);
        return HI_FAILURE;
    }

    if(pstMSensorDataInfo->data_type == MSENSOR_DATA_GYRO)
    {
        MOTIONSENSOR_BUF_WriteData2Buf();
    }

    //osal_spin_lock_irqsave(&g_stUserMng.msensormng_lock, &mngflags);
    ret = MOTIONSENSOR_BUF_ArriveBackStartAddr(pstMSensorDataInfo);
    if(HI_SUCCESS!=ret)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"data_type:%d has no data.\n",pstMSensorDataInfo->data_type);
        //osal_spin_lock_irqsave(&g_stUserMng.msensormng_lock, &mngflags);
        return HI_FAILURE;
    }

    if (HI_NULL == g_stUserMng.pstUserContext[pstMSensorDataInfo->id])
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"MOTIONSENSOR_BUF_ReadData: s32Id is NULL.\n");
        //osal_spin_unlock_irqrestore(&g_stUserMng.msensormng_lock, &mngflags);
        return HI_FAILURE;
    }

    if (HI_FALSE == g_bAlreadyReleased[pstMSensorDataInfo->id])
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"please release last read\n");
        //osal_spin_unlock_irqrestore(&g_stUserMng.msensormng_lock, &mngflags);
        return HI_FAILURE;
    }

    pstMSensorDataInfo->data[0].num = 0;
    pstMSensorDataInfo->data[1].num = 0;
    //osal_printk("id:%d data_type:%d DATA_TYPE_PTS:%d \n",pstMotionsensorDataInfo->id,pstMotionsensorDataInfo->data_type,DATA_TYPE_PTS);

    osal_spin_lock(&g_stUserMng.stUserSync.read_lock[pstMSensorDataInfo->id]);

    pPtsReadPointer = g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS];

    //osal_printk("~~~~~fun:%s line:%d pPtsReadPointer:%p *pPtsReadPointer:%lld\n",__func__,__LINE__,pPtsReadPointer,*pPtsReadPointer);

    //osal_printk("fun:%s line:%d pPtsReadPointer:%p\n",__func__,__LINE__,pPtsReadPointer);
    while (1)
    {
        if (((pPtsReadPointer == g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer) && (HI_TRUE == g_bForward))
            || *pPtsReadPointer > pstMSensorDataInfo->end_pts)
        //if (((pPtsReadPointer == g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer) /*&& (HI_TRUE == g_bForward)*/)
        //    || *pPtsReadPointer > pstMSensorDataInfo->end_pts)
        {

            if(pPtsReadPointer == g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer)
            {
                //HI_TRACE_MSENSOR(HI_DBG_ERR,"==g_bForward:%d pPtsReadPointer:%p pWritePointer:%p *(pWritePointer-1):%lld *pPtsReadPointer:%lld BP:%lld EP:%lld N0:%d N1:%d\n",g_bForward,
                //    pPtsReadPointer,g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer,*(hi_u64 *)((hi_u64)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer-sizeof(hi_u64)),
                //    *(pPtsReadPointer-1),pstMSensorDataInfo->begin_pts,pstMSensorDataInfo->end_pts,pstMSensorDataInfo->data[0].num,pstMSensorDataInfo->data[1].num);

               //osal_printk("==g_bForward:%d pPtsReadPointer:%p pWritePointer:%p *(pWritePointer-1):%lld *pPtsReadPointer:%lld BP:%lld EP:%lld u64nowpts:%d\n",g_bForward,
               //     pPtsReadPointer,g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer,*(hi_u64 *)((hi_u64)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer-sizeof(hi_u64)),
               //     *(pPtsReadPointer-1),pstMSensorDataInfo->begin_pts,pstMSensorDataInfo->end_pts);
            }

            if(*pPtsReadPointer > pstMSensorDataInfo->end_pts)
            {
                //osal_printk("pPointer:%p *pPtsRead:%lld begin_pts:%lld end_pts:%lld N0:%d N1:%d\n",pPtsReadPointer,
                //    *pPtsReadPointer,pstMSensorDataInfo->begin_pts,pstMSensorDataInfo->end_pts,pstMSensorDataInfo->data[0].num,pstMSensorDataInfo->data[1].num);
                //HI_TRACE_MSENSOR(HI_DBG_ERR,"pPointer:%p *pPtsRead:%lld begin_pts:%lld end_pts:%lld N0:%d N1:%d\n",pPtsReadPointer,
                //    *pPtsReadPointer,pstMSensorDataInfo->begin_pts,pstMSensorDataInfo->end_pts,pstMSensorDataInfo->data[0].num,pstMSensorDataInfo->data[1].num);
            }

            break;
        }


        if (*pPtsReadPointer >= pstMSensorDataInfo->begin_pts && *pPtsReadPointer <= pstMSensorDataInfo->end_pts)
        {
            if (HI_TRUE == bLoopFlag)
            {
                pstMSensorDataInfo->data[1].num++;
            }
            else
            {
                pstMSensorDataInfo->data[0].num++;

                if (!bFirstFound)
                {
                    u32Interval = ((hi_u8 *)pPtsReadPointer - (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr) >> 1;
                    g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_X].pStartAddr + u32Interval;
                    g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr + u32Interval;
                    g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Z].pStartAddr + u32Interval;
                    g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP].pStartAddr + u32Interval;
                    g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]  = pPtsReadPointer;
                    bFirstFound = true;
                }
            }
        }

        pPtsReadPointer++;
        g_bForward = HI_TRUE;


        ///////HI_TRACE_MSENSOR(HI_DBG_ERR,"#####pPtsReadPointer:%p pWritePointer:%p \n",pPtsReadPointer,g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pWritePointer);

        if (pPtsReadPointer >= ((unsigned long long int*)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr
                                + ((hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr - (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_X].pStartAddr) / sizeof(int)))
        {
            pPtsReadPointer = g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr;

            if (bFirstFound)
            {
                bLoopFlag = true;
            }
        }

    }

    ///osal_printk("====pPtsReadPointer end=%llu\n",*(pPtsReadPointer-1));

    if (pstMSensorDataInfo->data[0].num > 0)
    {
        pXVirAddr    = g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X];
        pYVirAddr    = g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y];
        pZVirAddr    = g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z];
        pTempVirAddr = g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP];

        pstMSensorDataInfo->data[0].x_phy_addr    =  (hi_void *)((hi_u8 *)pXVirAddr + g_s64Offset);
        pstMSensorDataInfo->data[0].y_phy_addr    =  (hi_void *)((hi_u8 *)pYVirAddr + g_s64Offset);
        pstMSensorDataInfo->data[0].z_phy_addr    =  (hi_void *)((hi_u8 *)pZVirAddr + g_s64Offset);
        pstMSensorDataInfo->data[0].temp_phy_addr =  (hi_void *)((hi_u8 *)pTempVirAddr + g_s64Offset);
        pstMSensorDataInfo->data[0].pts_phy_addr  =  (hi_void *)((hi_u8 *)g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS] + g_s64Offset);
        //if(pstMotionsensorDataInfo->begin_pts > *((unsigned long long *)g_stUserMng.pstUserContext[pstMotionsensorDataInfo->id]->pReadPointer[pstMotionsensorDataInfo->data_type][DATA_TYPE_PTS]))
        //{
            //osal_printk("wrong!begin_pts = %llu, readpts=%llu\n",pstMotionsensorDataInfo->begin_pts,
            //    *((unsigned long long *)g_stUserMng.pstUserContext[pstMotionsensorDataInfo->id]->pReadPointer[pstMotionsensorDataInfo->data_type][DATA_TYPE_PTS]));
        //}
    }

    ///osal_printk("l=%d\n",__LINE__);

    if (pstMSensorDataInfo->data[1].num > 0)
    {
        //osal_printk("l=%d\n",__LINE__);
        pstMSensorDataInfo->data[1].x_phy_addr    =  (hi_void *)((hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_X].pStartAddr    + g_s64Offset);
        pstMSensorDataInfo->data[1].y_phy_addr    =  (hi_void *)((hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr    + g_s64Offset);
        pstMSensorDataInfo->data[1].z_phy_addr    =  (hi_void *)((hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Z].pStartAddr    + g_s64Offset);
        pstMSensorDataInfo->data[1].temp_phy_addr =  (hi_void *)((hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP].pStartAddr + g_s64Offset);
        pstMSensorDataInfo->data[1].pts_phy_addr  =  (hi_void *)((hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr  + g_s64Offset);
    }

    if (0 == pstMSensorDataInfo->data[0].num && 0 == pstMSensorDataInfo->data[1].num)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR, "No data! BgPts:%12lld EdPts:%12lld\n",pstMSensorDataInfo->begin_pts, pstMSensorDataInfo->end_pts);
        //HI_ASSERT(0);
        g_bAlreadyReleased[pstMSensorDataInfo->id] = HI_TRUE;
        g_stMotionsensorProcInfo.au32BufDataUnmatch[pstMSensorDataInfo->data_type]++;
        g_stMotionsensorProcInfo.as32BufDataUnmatchID[pstMSensorDataInfo->data_type] = pstMSensorDataInfo->id;
        u32Interval = ((hi_u8 *)pPtsReadPointer - (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_PTS].pStartAddr) >> 1;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_X]    = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_X].pStartAddr    + u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Y]    = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Y].pStartAddr    + u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_Z]    = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_Z].pStartAddr    + u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP] = (hi_u8 *)g_astBufInfo[pstMSensorDataInfo->data_type][DATA_TYPE_TEMP].pStartAddr + u32Interval;
        g_stUserMng.pstUserContext[pstMSensorDataInfo->id]->pReadPointer[pstMSensorDataInfo->data_type][DATA_TYPE_PTS]  = pPtsReadPointer;
    }
    else
    {
        //osal_printk("l=%d\n",__LINE__);
        g_bAlreadyReleased[pstMSensorDataInfo->id] = HI_FALSE;
    }

    ///osal_printk("l=%d\n",__LINE__);

    pstMSensorDataInfo->addr_offset = g_s64Offset;

   osal_spin_unlock(&g_stUserMng.stUserSync.read_lock[pstMSensorDataInfo->id]);

    //osal_spin_unlock_irqrestore(&g_stUserMng.msensormng_lock, &mngflags);

    osal_memcpy(pstMSensorData,pstMSensorDataInfo,sizeof(hi_msensor_data_info));

    MOTIONSENSOR_BUF_ReleaseData(pstMSensorDataInfo);

    return HI_SUCCESS;
}


hi_s32 MOTIONSENSOR_BUF_SyncInit(hi_void)
{
    hi_s32 s32Ret = HI_SUCCESS;
    hi_s32 i=0;

    for(i=0;i<MAX_USER_NUM;i++)
    {
        s32Ret = osal_spin_lock_init(&g_stUserMng.stUserSync.read_lock[i]);
        if(HI_SUCCESS!=s32Ret)
        {
            HI_TRACE_MSENSOR(HI_DBG_ERR,"spin_lock_init failed!!!!\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_BUF_SyncDeInit(hi_void)
{
    hi_s32 i=0;

    for(i=0;i<MAX_USER_NUM;i++)
    {
        //osal_printk("UUUUUU fun:%s line:%d UUUUUU\n",__func__,__LINE__);
        osal_spin_lock_destory(&g_stUserMng.stUserSync.read_lock[i]);
    }

    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_BUF_LockInit(hi_void)
{
    hi_s32 s32Ret = HI_SUCCESS;

    osal_memset(&g_stUserMng, 0, sizeof(MSENSOR_BUF_USER_MNG_S));

    s32Ret = osal_spin_lock_init(&g_stUserMng.msensormng_lock);

    if (HI_SUCCESS != s32Ret)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"spin_lock_init failed!!!!\n");
        return HI_FAILURE;
    }

    MOTIONSENSOR_BUF_SyncInit();

    osal_mutex_init(&g_stUserMng.buf_mng_mutex);

    return HI_SUCCESS;
}

hi_void MOTIONSENSOR_BUF_LockDeInit(hi_void)
{
    osal_mutex_destory(&g_stUserMng.buf_mng_mutex);

    MOTIONSENSOR_BUF_SyncDeInit();

    osal_spin_lock_destory(&g_stUserMng.msensormng_lock);
}

