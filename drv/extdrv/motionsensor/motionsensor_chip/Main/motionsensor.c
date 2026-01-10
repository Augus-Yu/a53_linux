#include "motionsensor.h"
#include "motionsensordev.h"
//#include "bmm050.h"
#ifdef ICM20690_PARAM_PROC
#include "motionsensor_proc.h"
#endif
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>

#include "sys_ext.h"

#include "motionsensor_ext.h"
#include "motionsensor_chip_cmd.h"
#include "hi_comm_motionsensor.h"

static osal_dev_t* s_pstMotionsensorDev = NULL;

#define MNGBUFF_ENABLE

static hi_s32           g_s32MotionSensorInit;
static hi_bool          g_bMotionSensorStart = HI_FALSE;
hi_msensor_param*       MotionSensorStatus = HI_NULL;
//static hi_u8          g_u8FlagConfigInit;  	  //bit[0] mode init ? bit[1] gyro odr init ?

//bit[2] gyro FSR init? bit[3] accel odr init ?
//bit[4] accel FSR init
#ifdef MNGBUFF_ENABLE
hi_s32 MotionSensorDev_IntCallBack(hi_msensor_data* pstMSensorData)
{
    hi_s32 s32Ret = HI_SUCCESS;
    msensor_mng_export_func*  pfnMotionsensorMngExpFunc = HI_NULL;


    if (HI_NULL == pstMSensorData)
    {
        print_info( "pstMotionsensorChipData is NULL!(s32Ret:0x%x)\n", s32Ret);
        return s32Ret;

    }

    pfnMotionsensorMngExpFunc = FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR);

    if (HI_NULL == pfnMotionsensorMngExpFunc)
    {
        print_info( "pfnMotionsensorMngExpFunc is NULL!(s32Ret:0x%x)\n", s32Ret);
        return s32Ret;

    }

    s32Ret = pfnMotionsensorMngExpFunc->pfn_chip_write_data_to_mng_buff(pstMSensorData);

    if (HI_SUCCESS != s32Ret)
    {
        print_info( "pfn_chip_write_data_to_mng_buff Failed!(s32Ret:0x%x)\n", s32Ret);
        return s32Ret;

    }

    return s32Ret;
}
hi_s32 MotionSensorDev_InitMngBuff(hi_msensor_attr pstMSensorAttr,
    hi_msensor_buf_attr* pstMotionsensorBufAttr,
    hi_msensor_config* pconfig)
{
    hi_s32 s32Ret = HI_SUCCESS;
    msensor_mng_export_func*  pfnMotionsensorMngExpFunc = HI_NULL;

    if ((HI_NULL == pstMotionsensorBufAttr) || (HI_NULL == pconfig))
    {
        print_info( "Init Mng Buff Failed!(s32Ret:0x%x)\n", s32Ret);
        return s32Ret;
    }

    pfnMotionsensorMngExpFunc = FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR);

    if ((HI_NULL == pfnMotionsensorMngExpFunc) || (HI_NULL == pfnMotionsensorMngExpFunc->pfn_init))
    {
        print_info( "Init Mng Buff Failed!(pfnMotionsensorMngExpFunc:0x%p)\n", pfnMotionsensorMngExpFunc);
        return s32Ret;

    }

    s32Ret = pfnMotionsensorMngExpFunc->pfn_init(&pstMSensorAttr, pstMotionsensorBufAttr, pconfig);

    if (HI_SUCCESS != s32Ret)
    {
        print_info( "Init Mng Buff Failed!(s32Ret:0x%x)\n", s32Ret);
        return s32Ret;

    }

    return s32Ret;
}


hi_s32 HI_MOTIONSENSOR_DeInitMngBuff(hi_void)
{

    hi_s32 s32Ret = HI_SUCCESS;
    msensor_mng_export_func*  pfnMotionsensorMngExpFunc = HI_NULL;

    pfnMotionsensorMngExpFunc = FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR);

    if ((HI_NULL == pfnMotionsensorMngExpFunc) || (HI_NULL == pfnMotionsensorMngExpFunc->pfn_deinit))
    {
        print_info( "pfn_chip_write_data_to_mng_buff Failed!(pfnMotionsensorMngExpFunc:0x%p)\n", pfnMotionsensorMngExpFunc);
        return s32Ret;
    }

    s32Ret = pfnMotionsensorMngExpFunc->pfn_deinit();

    if (HI_SUCCESS != s32Ret)
    {
        print_info( "pfn_chip_write_data_to_mng_buff Failed!(s32Ret:0x%x)\n", s32Ret);
        return s32Ret;

    }

    return s32Ret;
}

hi_s32 MOTIONSENSOR_GetConfigFromChip(hi_msensor_param* pstMSensorParam)
{
    if (!MotionSensorStatus)
    {
        print_info("MotionSensorStatus is NULL\n");
        return HI_FAILURE;
    }

    osal_memcpy(pstMSensorParam, MotionSensorStatus, sizeof(hi_msensor_param));

    return HI_SUCCESS;
}

#endif

#if 1
//hi_s32 MOTIONSENSOR_GetData(hi_msensor_attr attr)
//{
//    hi_s32 s32Ret = HI_SUCCESS;

//    if ((attr.device_mask & MSENSOR_DEVICE_GYRO) || (attr.device_mask & MSENSOR_DEVICE_ACC))
//    {
//        s32Ret = HI_MOTIONSENSOR_SaveData_ModeDofFifo(attr);
//    }
//    else
//    {
//        print_info("[error]Not support MODE\n");
//        s32Ret = HI_FAILURE;
//    }

//#ifdef MNGBUFF_ENABLE
//    //Debug Only
//    MotionSensorDev_IntCallBack(pstIMUdata);
//#endif

//    return s32Ret;
//}
hi_s32 HI_MOTIONSENSOR_GetData(hi_void)
{
    hi_s32 s32Ret = HI_SUCCESS;
    static hi_msensor_data stIMUdataTemp;

    s32Ret = ICM20690_Data_TransferIMU(&stIMUdataTemp);

    if (HI_SUCCESS != s32Ret)
    {
        print_info("[error]Not support MODE\n");
    }

    //HI_TRACE_MSENSOR(HI_DBG_ERR,"~~~~~device_mask:%d u32GyroCount:%d~~~\n",stIMUdata.attr.device_mask,stIMUdata.gyro_buffer.data_num);
    //#ifdef MNGBUFF_ENABLE
    /////MotionSensorDev_IntCallBack(&stIMUdata);
    //#endif

    return s32Ret;

}
#endif

static hi_s32 MotionSensor_open(hi_void* private_data)
{
    //print_info("MotionSensor_open\n");

    return HI_SUCCESS;
}

static hi_s32 MotionSensor_release(hi_void* private_data)
{
    print_info("MotionSensor_close\n");

    return HI_SUCCESS;
}

static HI_SL MotionSensor_ioctl(hi_u32 u32cmd, hi_ulong arg, void* private_data)
{
    hi_s32 s32Ret = HI_SUCCESS;
    //   HI_S16 s16Temperature;
    hi_msensor_buf_attr* pstMotionsensorBufAttr;
    static TRIGER_CONFIG_S stTrigerConfig = {0};

    switch (u32cmd)
    {
        case MSENSOR_CMD_START:
        {
            if (HI_FALSE == g_s32MotionSensorInit)
            {
                print_info("MotionSensor is not init!\n");
                return HI_FAILURE;
            }

            if (HI_TRUE == g_bMotionSensorStart)
            {
                print_info("MotionSensor is already start!\n");
                return HI_SUCCESS;
            }

            FIFO_DATA_RESET();

            if (stTrigerConfig.eTrigerMode == TRIGER_TIMER)
            {
                HI_MotionSensor_TimerRun();
            }
            else if (stTrigerConfig.eTrigerMode == TRIGER_EXTERN_INTERRUPT)
            {
                HI_MotionSensor_INTERRUPTRun();
            }
            else
            {
                print_info("ERROR TrigerMode!!\n");
                return HI_FAILURE;
            }

            g_bMotionSensorStart = HI_TRUE;

            break;
        }

        case MSENSOR_CMD_STOP:
        {
            if (HI_FALSE == g_s32MotionSensorInit)
            {
                print_info("MotionSensor is not init!\n");
                return HI_FAILURE;
            }

            if (HI_FALSE == g_bMotionSensorStart)
            {
                print_info("MotionSensor is already stop!\n");
                return HI_SUCCESS;
            }

            if (stTrigerConfig.eTrigerMode == TRIGER_TIMER)
            {
                HI_MotionSensor_TimerStop();
            }
            else if (stTrigerConfig.eTrigerMode == TRIGER_EXTERN_INTERRUPT)
            {
                HI_MotionSensor_INTERRUPTStop();
                //return HI_FAILURE;
            }
            else
            {
                print_info("ERROR TrigerMode!!\n");
                return HI_FAILURE;
            }

            g_bMotionSensorStart = HI_FALSE;
            //print_info("****fun:%s line:%d\n",__func__,__LINE__);

            break;
        }

        case MSENSOR_CMD_INIT:
        {
            //sprint_info("start to init!!\n");
            if (HI_TRUE == g_s32MotionSensorInit)
            {
                print_info("MotionSensor is already inited!\n");
                return HI_FAILURE;
            }

            osal_memcpy(MotionSensorStatus, (void*)arg, sizeof(hi_msensor_param));

            MotionSensorStatus->config.gyro_config.temp_max = MOTIONSENSOR_MAX_TEMP;
            MotionSensorStatus->config.gyro_config.temp_min = MOTIONSENSOR_MIN_TEMP;
            MotionSensorStatus->config.acc_config.temp_max = MOTIONSENSOR_MAX_TEMP;
            MotionSensorStatus->config.acc_config.temp_min = MOTIONSENSOR_MIN_TEMP;
            /*init senser*/
            s32Ret = HI_MotionSensor_DevInit(MotionSensorStatus);

            if (HI_SUCCESS != s32Ret)
            {
                print_info("HI_MotionSensor_ParamInit failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

            s32Ret = MotionSensor_GetTrigerConfig(&stTrigerConfig);

            if (HI_SUCCESS != s32Ret)
            {
                print_info("IMU_GetTrigerConfig failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

#ifdef ICM20690_PARAM_PROC
            /*proc info init*/
            MPU_PROC_Init();
#endif
            ///print_info("MotionSensor init finshed\n");

            /*proc info init*/

            /*buff init*/
#ifdef MNGBUFF_ENABLE
            /*fix in this*/
            pstMotionsensorBufAttr = &MotionSensorStatus->buf_attr;
            MotionSensorDev_InitMngBuff(MotionSensorStatus->attr, pstMotionsensorBufAttr, &MotionSensorStatus->config);
#endif
            g_s32MotionSensorInit = HI_TRUE;
            break;
        }

        case MSENSOR_CMD_DEINIT:
        {
            if (HI_FALSE == g_s32MotionSensorInit)
            {
                print_info("MotionSensor has not inited!\n");
                return HI_FAILURE;
            }

#if 0
            pfnMngDeInitBufBuff();
#endif
#ifdef ICM20690_PARAM_PROC
            MPU_PROC_Exit();
#endif
            g_s32MotionSensorInit = HI_FALSE;
            HI_MotionSensor_DevDeInit(MotionSensorStatus);
#ifdef MNGBUFF_ENABLE

            s32Ret = HI_MOTIONSENSOR_DeInitMngBuff();

            if (HI_SUCCESS != s32Ret)
            {
                print_info("HI_MOTIONSENSOR_ParamInit failed! ret=%x\n", s32Ret);
                return HI_FAILURE;
            }

#endif
            break;
        }

        case MSENSOR_CMD_GET_PARAM:
        {
            osal_memcpy((hi_msensor_param*)arg, MotionSensorStatus, sizeof(hi_msensor_param));

            break;
        }

        default :
        {
            print_info("*******MotionSensor_ioctl***IOCTL_CMD is not found******* \n ");
            break;
        }
    }

    return (HI_SL)s32Ret;
}


#ifdef MNGBUFF_ENABLE
static hi_s32 HI_CHIP_RegisterDataMngCallback(void)
{
    msensor_mng_callback stCallback = {0};

    if (HI_FALSE  == ckfn_sys_entry())
    {
        printk("sys is not ready, please check it\n");
        return HI_FAILURE;
    }

    stCallback.pfn_get_config_from_chip = MOTIONSENSOR_GetConfigFromChip;
    stCallback.pfn_write_data_to_buf    = HI_MOTIONSENSOR_GetData;

    if ((NULL != FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR)) && (ckfn_msensor_mng_register_call_back()))
    {
        call_msensor_mng_register_call_back(&stCallback);
    }
    else
    {
        printk("register motionsensor callback failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static hi_s32 HI_CHIP_UnRegisterDataMngCallback(void)
{
    if ((NULL != FUNC_ENTRY(msensor_mng_export_func, HI_ID_MOTIONSENSOR)) && (ckfn_msensor_mng_unregister_call_back()))
    {
        call_msensor_mng_unregister_call_back();
    }

    return HI_SUCCESS;
}

#endif

static osal_fileops_t motionsensor_fops =
{
    .open           = MotionSensor_open,
    .release        = MotionSensor_release,
    .unlocked_ioctl = MotionSensor_ioctl,
};

hi_s32 MotionSensorInit(hi_void)
{
    hi_s32 s32Ret = HI_SUCCESS;

    MotionSensorStatus = (hi_msensor_param*)osal_kmalloc(sizeof(hi_msensor_param), osal_gfp_kernel);

    if (!MotionSensorStatus)
    {
        print_info("Could not allocate memory\n");
        return -ENOMEM;
    }

    osal_memset(MotionSensorStatus, 0, sizeof(hi_msensor_param));

    g_bMotionSensorStart = HI_FALSE;

    s32Ret = HI_MotionSensor_SensorInit();

    if (HI_SUCCESS != s32Ret)
    {
        print_info("init failed! ret=%x\n", s32Ret);
        osal_kfree(MotionSensorStatus);
        //osal_kfree(MSensorStatus);
        return HI_FAILURE;
    }

#ifdef MNGBUFF_ENABLE
    s32Ret = HI_CHIP_RegisterDataMngCallback();

    if (HI_SUCCESS != s32Ret)
    {
        print_info("HI_CHIP_RegisterDataMngCallback failed! ret=%x\n", s32Ret);
        osal_kfree(MotionSensorStatus);
        //osal_kfree(MSensorStatus);
        return HI_FAILURE;
    }

#endif

    s_pstMotionsensorDev = osal_createdev(MSENSOR_DEV_NAME);

    if (NULL == s_pstMotionsensorDev)
    {
        osal_printk( "motionsensor: create device failed\n");
        return HI_FAILURE;
    }

    s_pstMotionsensorDev->fops  = &motionsensor_fops;
    s32Ret = osal_registerdevice(s_pstMotionsensorDev);

    if (s32Ret)
    {
        osal_destroydev(s_pstMotionsensorDev);
        s_pstMotionsensorDev = NULL;
        osal_printk("register motionsensor device failed!\n");
        return HI_FAILURE;
    }

    osal_printk("load motionsensor_chip.ko for %s...OK!\n",CHIP_NAME);

    return HI_SUCCESS;
}

hi_void MotionSensorExit(hi_void)
{
#ifdef MNGBUFF_ENABLE
    HI_CHIP_UnRegisterDataMngCallback();
#endif

    if (MotionSensorStatus)

    {
        HI_MotionSensor_SensorDeInit(MotionSensorStatus);
    }
    else
    {
        print_info("nothing to exit \n");
    }


    if (MotionSensorStatus == NULL)
    {
        print_info("MotionSensorStatus == NULL\n");
    }
    else
    {
        kfree(MotionSensorStatus);
        MotionSensorStatus = NULL;
    }

    osal_deregisterdevice(s_pstMotionsensorDev);
    osal_destroydev(s_pstMotionsensorDev);
    s_pstMotionsensorDev = NULL;
    osal_printk("Unload motionsensor_chip.ko for %s...OK!\n",CHIP_NAME);
    //misc_deregister(&HI_MotionSensor_dev);
}

module_init(MotionSensorInit);
module_exit(MotionSensorExit);

MODULE_AUTHOR("hisilion");
MODULE_DESCRIPTION("MotionSensor driver");
MODULE_LICENSE("GPL");

