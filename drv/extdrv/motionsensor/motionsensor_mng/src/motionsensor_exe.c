#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include "motionsensor_exe.h"
#include "motionsensor_ext.h"
#include "motionsensor_proc.h"
#include "motionsensor_buf.h"
#include "motionsensor_mng_cmd.h"
#include "sys_ext.h"
#include "hi_osal.h"

static osal_dev_t* s_pstMotionsensorDev = NULL;
static msensor_mng_callback g_stMotionsensorMngCallback = {HI_NULL};

#define USER_SEND_DATA 1

#define MOTIONSENSOR_DEV_NAME "motionsensor_mng"
extern  MSENSOR_BUF_USER_MNG_S g_stUserMng;

hi_s32 MOTIONSENSOR_WriteDataToMngBuff(hi_msensor_data* pstMSensorData);

hi_s32 HI_MOTIONSENSOR_GetProcInfo(hi_void)
{
    osal_memcpy(g_stMotionsensorProcInfo.aszGyroName, "ICM20690", sizeof("ICM20690"));
    osal_memcpy(g_stMotionsensorProcInfo.aszAccelName, "ICM20690", sizeof("ICM20690"));

    return HI_SUCCESS;
}

static long motionsensor_exe_ioctl(unsigned int cmd, unsigned long arg, void* private_data)
{
    hi_s32 s32Ret = HI_SUCCESS;
    #ifdef USER_SEND_DATA
    hi_msensor_data *pstMSensorData;
    #endif

    switch (cmd)
    {
        case MSENSOR_CMD_RELEASE_BUF:
        {
            s32Ret= MOTIONSENSOR_BUF_ReleaseData((hi_msensor_data_info*)arg);

            if (HI_SUCCESS != s32Ret)
            {
                HI_TRACE_MSENSOR(HI_DBG_ERR,"HI_MOTIONSENSOR_BufRelease failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

            break;
        }

        case MSENSOR_CMD_GET_DATA:
        {

            s32Ret = MOTIONSENSOR_BUF_GetData((hi_msensor_data_info*)arg);

            if (HI_SUCCESS != s32Ret)
            {
                HI_TRACE_MSENSOR(HI_DBG_ERR,"HI_MOTIONSENSOR_BufReadData failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

            break;
        }

        #ifdef USER_SEND_DATA
        case MSENSOR_CMD_SEND_DATA:
        {
            pstMSensorData = (hi_msensor_data*)arg;

            s32Ret = MOTIONSENSOR_WriteDataToMngBuff(pstMSensorData);

            if (HI_SUCCESS != s32Ret)
            {
                HI_TRACE_MSENSOR(HI_DBG_ERR,"HI_MOTIONSENSOR_BufReadData failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

            break;
        }
        #endif

        case MSENSOR_CMD_ADD_USER:
        {
            s32Ret = MOTIONSENSOR_BUF_AddUser((hi_s32 *)arg);

            if (HI_SUCCESS != s32Ret)
            {
                HI_TRACE_MSENSOR(HI_DBG_ERR,"MOTIONSENSOR_BUF_AddUser failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

            break;
        }

        case MSENSOR_CMD_DELETE_USER:
        {
            s32Ret = MOTIONSENSOR_BUF_DeleteUser((hi_s32 *)arg);

            if (HI_SUCCESS != s32Ret)
            {
                HI_TRACE_MSENSOR(HI_DBG_ERR,"MOTIONSENSOR_BUF_DeleteUser failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

            break;
        }

        default :
        {
            HI_TRACE_MSENSOR(HI_DBG_ERR,"ioctl cmd 0x%x does not exist!\n", cmd);
            break;
        }
    }

    return s32Ret;
}

static hi_s32 motionsensor_open(hi_void* private_data)
{
    return HI_SUCCESS;
}

static hi_s32 motionsensor_release(hi_void* private_data)
{
    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_MNG_Init(hi_void* pArgs)
{
    return HI_SUCCESS;
}
static hi_void MOTIONSENSOR_MNG_Exit(hi_void)
{
    return ;
}

static hi_void MOTIONSENSOR_MNG_QueryState(mod_state* pstState)
{
    *pstState = MOD_STATE_FREE;
    return ;
}


static hi_void MOTIONSENSOR_MNG_Notify(mod_notice_id enNotice)
{
    return ;
}

static hi_u32 MOTIONSENSOR_MNG_GetVerMagic(hi_void)
{
    return VERSION_MAGIC;
}


hi_s32 MOTIONSENSOR_WriteDataToMngBuff(hi_msensor_data* pstMSensorData)
{
    hi_u32 i = 0;
    hi_s32 s32Ret = HI_SUCCESS;
    //unsigned long mngflags;

    osal_spin_lock(&g_stUserMng.msensormng_lock);
    #if 1
    if (MSENSOR_DEVICE_GYRO == (pstMSensorData->attr.device_mask & MSENSOR_DEVICE_GYRO ))
    {
        //HI_TRACE_MSENSOR(HI_DBG_ERR,"### device_mask:%d u32GyroCount:%d++\n",pstMSensorData->attr.device_mask,pstMSensorData->gyro_buffer.data_num);
        ////osal_printk("### device_mask:%d u32GyroCount:%d++\n",pstMSensorData->attr.device_mask,pstMSensorData->gyro_buffer.data_num);
        for (i = 0; i < pstMSensorData->gyro_buffer.data_num; i++)
        {
            if(MSENSOR_TEMP_GYRO != (pstMSensorData->attr.temperature_mask & MSENSOR_TEMP_GYRO))
            {
                pstMSensorData->gyro_buffer.gyro_data[i].temp= 0xffffffff;
            }

            //Debug Set
            #if 0
            pstMSensorData->gyro_buffer.gyro_data[i].x = 100;
            pstMSensorData->gyro_buffer.gyro_data[i].y = 200;
            pstMSensorData->gyro_buffer.gyro_data[i].z = 300;
            pstMSensorData->gyro_buffer.gyro_data[i].temp  = 400;
            #endif

            #if 0
            if(i%300 == 0)
            {
                osal_printk("Gyro:u32X:%d u32Y:%d u32Z:%d u32Temprature:%d pts:%lld  !\n",pstMSensorData->gyro_buffer.gyro_data[i].x, pstMSensorData->gyro_buffer.gyro_data[i].y,
                                       pstMSensorData->gyro_buffer.gyro_data[i].z, pstMSensorData->gyro_buffer.gyro_data[i].temp, pstMSensorData->gyro_buffer.gyro_data[i].pts);
            }
            #endif

            HI_TRACE_MSENSOR(HI_DBG_DEBUG,"Gyro:u32X:%8d u32Y:%8d u32Z:%8d u32Temprature:%8d pts:%10lld!\n",pstMSensorData->gyro_buffer.gyro_data[i].x, pstMSensorData->gyro_buffer.gyro_data[i].y,
                                   pstMSensorData->gyro_buffer.gyro_data[i].z, pstMSensorData->gyro_buffer.gyro_data[i].temp, pstMSensorData->gyro_buffer.gyro_data[i].pts);

            s32Ret = MOTIONSENSOR_BUF_WriteData(MSENSOR_DATA_GYRO, pstMSensorData->gyro_buffer.gyro_data[i].x, pstMSensorData->gyro_buffer.gyro_data[i].y,
                                       pstMSensorData->gyro_buffer.gyro_data[i].z, pstMSensorData->gyro_buffer.gyro_data[i].temp, pstMSensorData->gyro_buffer.gyro_data[i].pts);
        }

    }

    if (MSENSOR_DEVICE_ACC == (pstMSensorData->attr.device_mask & MSENSOR_DEVICE_ACC))
    {
        ////osal_printk("+++ fun:%s line:%d u32AccelValid:%d u32AccelCount:%d++\n",__func__,__LINE__,pstMotionsensorChipData->u32AccelValid,pstMotionsensorChipData->u32AccelCount);

        for (i = 0; i < pstMSensorData->acc_buffer.data_num; i++)
        {
            if(MSENSOR_TEMP_ACC != (pstMSensorData->attr.temperature_mask & MSENSOR_TEMP_ACC))
            {
                pstMSensorData->acc_buffer.acc_data[i].temp = 0xffffffff;
            }

            //Debug Set
            #if 0
            pstMotionsensorChipData->stMotionsensorAccelData[i].u32X = 1000;
            pstMotionsensorChipData->stMotionsensorAccelData[i].u32Y = 2000;
            pstMotionsensorChipData->stMotionsensorAccelData[i].u32Z = 3000;
            pstMotionsensorChipData->stMotionsensorAccelData[i].u32Temprature = 4000;
            #endif

            #if 0
            if(intwrite_exe_cnt%300 == 1)
            {
                osal_printk("Accel:u32X:%d u32Y:%d u32Z:%d u32Temprature:%d pts:%lld!\n",pstMSensorData->acc_buffer.acc_data[i].x, pstMSensorData->acc_buffer.acc_data[i].y,
                                       pstMSensorData->acc_buffer.acc_data[i].z, pstMSensorData->acc_buffer.acc_data[i].temp, pstMSensorData->acc_buffer.acc_data[i].pts);

            }
            #endif

            s32Ret = MOTIONSENSOR_BUF_WriteData(MSENSOR_DATA_ACC, pstMSensorData->acc_buffer.acc_data[i].x, pstMSensorData->acc_buffer.acc_data[i].y,
                                       pstMSensorData->acc_buffer.acc_data[i].z, pstMSensorData->acc_buffer.acc_data[i].temp, pstMSensorData->acc_buffer.acc_data[i].pts);
        }
    }

    if (MSENSOR_DEVICE_MAGN == (pstMSensorData->attr.device_mask & MSENSOR_DEVICE_MAGN))
    {
        for (i = 0; i < pstMSensorData->magn_buffer.data_num; i++)
        {
            if(MSENSOR_TEMP_MAGN != (pstMSensorData->attr.temperature_mask & MSENSOR_TEMP_MAGN))
            {
                 pstMSensorData->magn_buffer.magn_data[i].temp = 0xffffffff;
            }

            s32Ret = MOTIONSENSOR_BUF_WriteData(MSENSOR_DATA_MAGN,
                pstMSensorData->magn_buffer.magn_data[i].x,
                pstMSensorData->magn_buffer.magn_data[i].y,
                pstMSensorData->magn_buffer.magn_data[i].z,
                pstMSensorData->magn_buffer.magn_data[i].temp,
                pstMSensorData->magn_buffer.magn_data[i].pts);
        }
    }
    #endif
    osal_spin_unlock(&g_stUserMng.msensormng_lock);

    return s32Ret;
}

hi_s32 MOTIONSENSOR_MngInitBuf(hi_msensor_attr* pstMotionAttr,
    hi_msensor_buf_attr* pstMSensorBufAttr,
    hi_msensor_config * pstMSensorConfig)
{
    hi_u32 u32GyroOdr = 0;
    hi_u32 u32AccOdr = 0;
    hi_u32 u32MagnOdr = 0;
    hi_s32 s32Ret = HI_SUCCESS;

    switch (pstMotionAttr->device_mask)
    {
        case MSENSOR_DEVICE_GYRO|MSENSOR_DEVICE_ACC:
        {

            /*Only For American Present*/
            u32GyroOdr = pstMSensorConfig->gyro_config.odr;
            u32AccOdr  = pstMSensorConfig->acc_config.odr;
            u32MagnOdr = 0;

            HI_TRACE_MSENSOR(HI_DBG_DEBUG,"ODR:u32GyroOdr:%d u32AccOdr:%d u32MagnOdr:%d\n",u32GyroOdr,u32AccOdr,u32MagnOdr);

            s32Ret = MOTIONSENSOR_BUF_Init(pstMSensorBufAttr,  u32GyroOdr,  u32AccOdr,  u32MagnOdr);

            break;
        }

        case MSENSOR_DEVICE_GYRO:
        {
            /*Only For American Present*/
            u32GyroOdr = pstMSensorConfig->gyro_config.odr;
            u32AccOdr  = 0;
            u32MagnOdr = 0;

           HI_TRACE_MSENSOR(HI_DBG_DEBUG,"u32GyroOdr:%d u32AccOdr:%d u32MagnOdr:%d\n",u32GyroOdr,u32AccOdr,u32MagnOdr);

            s32Ret = MOTIONSENSOR_BUF_Init(pstMSensorBufAttr,  u32GyroOdr,  u32AccOdr,  u32MagnOdr);
            break;
        }
        case MSENSOR_DEVICE_ALL:
        {
            /*Only For American Present*/
            u32GyroOdr = pstMSensorConfig->gyro_config.odr;
            u32AccOdr  = pstMSensorConfig->acc_config.odr;
            u32MagnOdr = pstMSensorConfig->acc_config.odr;

            HI_TRACE_MSENSOR(HI_DBG_DEBUG,"u32GyroOdr:%d u32AccOdr:%d u32MagnOdr:%d\n",u32GyroOdr,u32AccOdr,u32MagnOdr);

            s32Ret = MOTIONSENSOR_BUF_Init(pstMSensorBufAttr,  u32GyroOdr,  u32AccOdr,  u32MagnOdr);

            break;
        }
        default:
        {
            HI_TRACE_MSENSOR(HI_DBG_ERR,"MOTIONSENSOR_MngInitBuf(u32GyroOdr:%d u32AccOdr:%d u32MagnOdr:%d) err!\n",u32GyroOdr,u32AccOdr,u32MagnOdr);
            break;
        }
    }

    return s32Ret;
}

hi_s32 MOTIONSENSOR_MngDeInitBuf(hi_void)
{
    hi_s32 s32Ret = HI_SUCCESS;

    MOTIONSENSOR_BUF_Deinit();

    return s32Ret;
}

hi_s32 MOTIONSENSOR_BUF_GetMotionsensorConfig(hi_msensor_param* pstMSensorParam)
{
    g_stMotionsensorMngCallback.pfn_get_config_from_chip(pstMSensorParam);

    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_BUF_WriteData2Buf(hi_void)
{
    g_stMotionsensorMngCallback.pfn_write_data_to_buf();

    return HI_SUCCESS;
}

hi_s32 MOTIONSENSOR_MNG_RegisterMotionsensorCallBack (msensor_mng_callback* pstCallback)
{
    CHECK_NULL_PTR(pstCallback);

    g_stMotionsensorMngCallback.pfn_get_config_from_chip = pstCallback->pfn_get_config_from_chip;
    g_stMotionsensorMngCallback.pfn_write_data_to_buf    = pstCallback->pfn_write_data_to_buf;
    return HI_SUCCESS;
}

hi_void MOTIONSENSOR_MNG_UnRegisterMotionsensorCallBack (hi_void)
{
    g_stMotionsensorMngCallback.pfn_get_config_from_chip = HI_NULL;

}

static msensor_mng_export_func s_stExportFuncs =
{
    .pfn_add_msensor_user            = MOTIONSENSOR_BUF_AddUser,
    .pfn_delete_msensor_user         = MOTIONSENSOR_BUF_DeleteUser,
    .pfn_get_data                    = MOTIONSENSOR_BUF_GetData,
    .pfn_release_data                = MOTIONSENSOR_BUF_ReleaseData,
    .pfn_get_msensor_config          = MOTIONSENSOR_BUF_GetMotionsensorConfig,

    //CallBack2Chip
    .pfn_chip_write_data_to_mng_buff = MOTIONSENSOR_WriteDataToMngBuff,
    .pfn_init                        = MOTIONSENSOR_MngInitBuf,
    .pfn_deinit                      = MOTIONSENSOR_MngDeInitBuf,

    //CallBackToChip
    .pfn_register_call_back          = MOTIONSENSOR_MNG_RegisterMotionsensorCallBack,
    .pfn_unregister_call_back        = MOTIONSENSOR_MNG_UnRegisterMotionsensorCallBack,
};

static umap_module s_stModule =
{
    .mod_id            = HI_ID_MOTIONSENSOR,
    .mod_name          = "motionsensor",

    .pfn_init          = MOTIONSENSOR_MNG_Init,
    .pfn_exit          = MOTIONSENSOR_MNG_Exit,
    .pfn_query_state   = MOTIONSENSOR_MNG_QueryState,
    .pfn_notify        = MOTIONSENSOR_MNG_Notify,
    .pfn_ver_checker   = MOTIONSENSOR_MNG_GetVerMagic,

    .export_funcs      = &s_stExportFuncs,
    .data = HI_NULL,
};



static osal_fileops_t motionsensor_fops =
{
    .open           = motionsensor_open,
    .release        = motionsensor_release,
    .unlocked_ioctl = motionsensor_exe_ioctl,
};


hi_s32 motionsensor_init(hi_void)
{
    hi_s32 s32Ret;

    if (cmpi_register_module(&s_stModule))
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"RegisterModule FAILURE(Chip:%s)!\n", CHIP_NAME);
        return HI_FAILURE;
    }

    MOTIONSENSOR_PROC_Init();

    s_pstMotionsensorDev = osal_createdev(MOTIONSENSOR_DEV_NAME);

    if (NULL == s_pstMotionsensorDev)
    {
        HI_TRACE_MSENSOR(HI_DBG_ERR,"motionsensor: create device failed\n");
        return HI_FAILURE;
    }

    s_pstMotionsensorDev->fops  = &motionsensor_fops;
    s32Ret = osal_registerdevice(s_pstMotionsensorDev);

    if (s32Ret)
    {
        osal_destroydev(s_pstMotionsensorDev);
        s_pstMotionsensorDev = NULL;
        HI_TRACE_MSENSOR(HI_DBG_ERR,"register motionsensor device failed!\n");
        return HI_FAILURE;
    }

    s32Ret = MOTIONSENSOR_BUF_LockInit();
    if (s32Ret)
    {
        osal_deregisterdevice(s_pstMotionsensorDev);
        osal_destroydev(s_pstMotionsensorDev);
        s_pstMotionsensorDev = NULL;
        HI_TRACE_MSENSOR(HI_DBG_ERR,"register MOTIONSENSOR_BUF_LockInit failed!\n");
        return HI_FAILURE;
    }

    osal_printk("load motionsensor_mng.ko for %s...OK!\n",CHIP_NAME);

    return HI_SUCCESS;
}

hi_void motionsensor_exit(hi_void)
{
    cmpi_unregister_module(HI_ID_MOTIONSENSOR);
    MOTIONSENSOR_PROC_Exit();
    MOTIONSENSOR_BUF_LockDeInit();
    osal_deregisterdevice(s_pstMotionsensorDev);
    osal_destroydev(s_pstMotionsensorDev);
    s_pstMotionsensorDev = NULL;
    osal_printk("Unload motionsensor_mng.ko for %s...OK!\n",CHIP_NAME);
}

module_init(motionsensor_init);
module_exit(motionsensor_exit);

MODULE_DESCRIPTION("motionsensor driver");
MODULE_LICENSE("GPL");

