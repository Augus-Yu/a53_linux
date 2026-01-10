#ifndef __MOTIONSENSOR_BUF_H__
#define __MOTIONSENSOR_BUF_H__

#include "motionsensor_ext.h"
#include "hi_osal.h"

#define HIALIGN(x, a) ((a) * (((x) + (a) - 1) / (a)))
//#define BUF_BLOCK_NUM 6
#define BUF_BLOCK_NUM 6
#define MAX_USER_NUM 10
#define WR_GAP 100 //minmotionsensorm gap between reader pointer and write pointer, to prevent new datas overlap with reading/processing datas

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

typedef struct
{
    hi_void * pStartAddr;      //start address
    hi_void * pWritePointer;   //write pointer
}MSENSOR_BUF_INFO_S;

typedef enum
{
    DATA_TYPE_X,
    DATA_TYPE_Y,
    DATA_TYPE_Z,
    DATA_TYPE_TEMP,
    DATA_TYPE_PTS,
    DATA_TYPE_BUTT
}MOTIONSENSOR_BUF_DATA_TYPE_E;

typedef struct
{
    hi_void * pReadPointer[MSENSOR_DATA_BUTT][DATA_TYPE_BUTT];
    hi_s32 s32Reverd3[4];
    //osal_spinlock_t read_lock;
}MSENSOR_BUF_USER_CONTEXT_S;

#define RESERVE_NUM 100

typedef struct
{
    osal_spinlock_t read_lock[MAX_USER_NUM];
}MSENSOR_USER_SYNC_S;

typedef struct
{
    hi_u32 u32UserCnt;
    osal_spinlock_t              msensormng_lock;
    MSENSOR_USER_SYNC_S          stUserSync;
    MSENSOR_BUF_USER_CONTEXT_S * pstUserContext[MAX_USER_NUM];

    osal_mutex_t buf_mng_mutex;
}MSENSOR_BUF_USER_MNG_S;

hi_s32 MOTIONSENSOR_BUF_LockInit(hi_void);

hi_void MOTIONSENSOR_BUF_LockDeInit(hi_void);

hi_s32 MOTIONSENSOR_BUF_Init(hi_msensor_buf_attr* pstBufAttr,
    hi_u32 u32GyroFreq,
    hi_u32 u32AccelFreq,
    hi_u32 u32MagFreq);

hi_s32 MOTIONSENSOR_BUF_Deinit(void);

hi_s32 MOTIONSENSOR_BUF_WriteData(hi_msensor_data_type data_type, hi_s32 x, hi_s32 y, hi_s32 z, hi_s32 temp, hi_u64 pts);

//int MOTIONSENSOR_BUF_WriteUsrData(MOTIONSENSOR_USRDATA_S* pstUsrData);
hi_s32 MOTIONSENSOR_BUF_GetData(hi_void* pstMotionsensorData);

hi_s32 MOTIONSENSOR_BUF_ReleaseData(hi_void* pstMotionsensorDataInfo);

hi_s32 MOTIONSENSOR_BUF_AddUser(hi_s32* ps32Id);

hi_s32 MOTIONSENSOR_BUF_DeleteUser(hi_s32* ps32Id);

hi_s32 MOTIONSENSOR_BUF_SyncInit(hi_void);

hi_s32 MOTIONSENSOR_BUF_SyncDeInit(hi_void);

#endif

