#include "motionsensordev.h"
#include <linux/fs.h>
#include <linux/slab.h>
#ifndef __HuaweiLite__
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <asm/dma.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#else

#include "linux/kernel.h"
#include "asm/dma.h"
#include "linux/delay.h"
#include "linux/interrupt.h"
#include <linux/module.h>
#include <linux/kernel.h>
#endif

#ifdef TRANSFER_I2C
#include "i2c_dev.h"
#elif defined TRANSFER_SPI
#include "spi_dev.h"
#endif

#include "hi_comm_motionsensor.h"
#include "motionsensor_ext.h"
#include "sys_ext.h"
//#include "vi_ext.h"
#include "motionsensorgpio.h"



//#define TEST_DEBUG (1)
#define BUS_NUM 1
hi_u32 u32MotionSensorSpiNum = 1;
//static hi_u64 timerecord[51000];
hi_u64 time_num = 0;


static ICM20690_DEV_INFO* ICM20690_dev = NULL;
static hi_msensor_data stIMUdata;

//static struct workqueue_struct*  my_wq;
//static osal_mutex_t mutexGetData;
static osal_spinlock_t lockGetData;

static hi_u8  su8GyroAccDataLen;
static hi_msensor_attr stMSensorMode;
static hi_u64 saTimeBackup[TIME_RECORD_CNT] = {0};
static hi_u32 saEveryDataCntBackup[DATA_RECORD_CNT] = {0};
static hi_u8  su8Timecnt;
static hi_u8  su8datacount;

#define SCALE_TIMES                 (100)

#define THREAD_EXIT 0
#define THREAD_CTRL 1

volatile static unsigned int hithread_state;
#ifdef __HuaweiLite__
static gpio_groupbit_info     group_bit_info;
#endif


typedef struct hiMOTION_SENSOR_OUT_DATA_S
{
    hi_msensor_sample_data        stGyroData;
    hi_msensor_sample_data        stAccData;
    //hi_msensor_sample_data        stMagnData;
} MOTION_SENSOR_OUT_DATA_S;

#ifdef TEST_DEBUG
static MOTION_SENSOR_OUT_DATA_S test_fifo[11000] = {0};
#endif
hi_u32 su32DataCount = 0, i_thread = 0;
hi_u64 u64PTSNow;

extern hi_s32 MotionSensorDev_IntCallBack(hi_msensor_data* pstMSensorData);

extern unsigned short hi_motionsensor_ssp_read_alt(unsigned int ssp_no, hi_u8 u8reg_addr, hi_u8* u8reg_data, hi_u32 u32cnt, hi_bool bFifoMode);
extern int hi_motionsensor_ssp_write_alt(unsigned int ssp_no, hi_u8 u8reg_addr, hi_u8* data);


hi_s32 HI_ICM20690_Transfer_read(hi_u8 u8reg_addr, hi_u8* u8reg_data, hi_u32 u32cnt, hi_bool bFifoMode)
{
    hi_s32 s32Ret;
#ifndef __HuaweiLite__
#ifdef TRANSFER_I2C
    s32Ret = MotionSersor_I2C_read(ICM20690_dev->client, u8reg_addr, u8reg_data, u32cnt);
#elif defined TRANSFER_SPI
    //s32Ret = MotionSersor_SPI_read(ICM20690_dev->hi_spi, u8reg_addr, u8reg_data, u32cnt);
    s32Ret = hi_motionsensor_ssp_read_alt(BUS_NUM, u8reg_addr, u8reg_data, u32cnt, bFifoMode);
#endif
#else
    s32Ret = MotionSersor_SPI_read(u8reg_addr, u8reg_data, u32cnt, u32MotionSensorSpiNum);
    //s32Ret = hi_motionsensor_ssp_read_alt(BUS_NUM, u8reg_addr, u8reg_data, u32cnt, bFifoMode);
#endif
    return s32Ret;
}

hi_s32 HI_ICM20690_Transfer_write(hi_u8 u8reg_addr, hi_u8* u8reg_data, hi_u32 u32cnt)
{
    hi_s32 s32Ret;
#ifndef __HuaweiLite__
#ifdef TRANSFER_I2C
    s32Ret = MotionSersor_I2C_write(ICM20690_dev->client, u8reg_addr, u8reg_data, u32cnt);
#elif defined TRANSFER_SPI
    //s32Ret = MotionSersor_SPI_write(ICM20690_dev->hi_spi, u8reg_addr, u8reg_data, u32cnt);
    s32Ret = hi_motionsensor_ssp_write_alt(BUS_NUM, u8reg_addr, u8reg_data);
#endif
#else
    s32Ret = MotionSersor_SPI_write(u8reg_addr, u8reg_data, u32cnt, u32MotionSensorSpiNum);
     //s32Ret = hi_motionsensor_ssp_write_alt(BUS_NUM, u8reg_addr, u8reg_data);
#endif
    return s32Ret;
}


static hi_u64 inline HI_ICM20690_GetCurPts(void)
{
    hi_u64 u64TimeNow;

    extern hi_u64 sched_clock(void);
    u64TimeNow = sched_clock();
    do_div(u64TimeNow, 1000);
    return u64TimeNow;
}

#ifdef TRANSFER_SPI
static hi_s32 HI_I2C_Disable(void)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;
    u8RegisterValue = 0x10;
    u8Ret = HI_ICM20690_Transfer_write(USER_CONTROL_REGISTER_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("disable i2c failed(%d)\n", u8Ret);
        return -EAGAIN;
    }

    return HI_SUCCESS;
}
#endif

static hi_s32 ICM20690_Reset(void)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    u8Ret = HI_ICM20690_Transfer_read(POWER_MANAGEMENT_REGISTER_1_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("reset ICM20690 failed\n");
        return -EAGAIN;
    }

    u8RegisterValue |= TRUE_REGISTER_VALUE << RESET_OFFSET;
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(POWER_MANAGEMENT_REGISTER_1_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("reset ICM20690 failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}

static hi_s32 ICM20690_SetClk(void)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue ;

    u8Ret = HI_ICM20690_Transfer_read(POWER_MANAGEMENT_REGISTER_1_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("reset ICM20690 failed  HI_S8\n");
        return -EAGAIN;
    }

    u8RegisterValue = CLKSET_VALUE;
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(POWER_MANAGEMENT_REGISTER_1_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("reset ICM20690 failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}

static hi_s32 ICM20690_SetAxisMode(hi_u32 u32DevMode)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue = 0xFF;

    if (u32DevMode & MSENSOR_DEVICE_GYRO)
    {
        u8RegisterValue &= 0x38;
    }

    if (u32DevMode & MSENSOR_DEVICE_ACC)
    {
        u8RegisterValue &= 0x07;
    }

    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(POWER_MANAGEMENT_REGISTER_2_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_AXIS_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}

static hi_s32 ICM20690_SetSampleRate(hi_u64 u64SampleRate)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    if (u64SampleRate == 0 || osal_div64_u64_rem(1000 , u64SampleRate))
    {
        print_info("u8SAMPLE_RATE must be  is divisible by 1000,  %lld  \n", u64SampleRate);
        return -EAGAIN;
    }

    u8RegisterValue = osal_div64_u64(1000, u64SampleRate) - 1;
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(SMPLRT_DIV, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_AXIS_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}
static hi_s32 ICM20690_SetGyroDLFP_CFG(hi_u8 dlfp_cfg)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    if (dlfp_cfg < 0 || dlfp_cfg > 7)
    {
        print_info("ICM20690_GYRO_DLFP_CFG is invalid\n");
        return -EINVAL;
    }

    u8Ret = HI_ICM20690_Transfer_read(CONFIGURATION_REGISTER_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_DLFP_CFG_SET failed\n");
        return -EAGAIN;
    }

    u8RegisterValue &= ~0x7;
    u8RegisterValue |= dlfp_cfg;
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(CONFIGURATION_REGISTER_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_DLFP_CFG_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}

static hi_s32 ICM20690_SetAccelDLFP_CFG(hi_u8 dlfp_cfg)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    if (dlfp_cfg < 0 || dlfp_cfg > 7)
    {
        print_info("ICM20690_GYRO_DLFP_CFG is invalid\n");
        return -EINVAL;
    }

    u8Ret = HI_ICM20690_Transfer_read(ACCEL_CONFIG_REGISTER_2_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_DLFP_CFG_SET failed\n");
        return -EAGAIN;
    }

    u8RegisterValue &= ~0x7;
    u8RegisterValue |= dlfp_cfg;
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(ACCEL_CONFIG_REGISTER_2_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_DLFP_CFG_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}



static hi_s32 ICM20690_SetGryoFchoice_B(hi_u8 fchoice_b)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    if (fchoice_b < 0 || fchoice_b > 3)
    {
        print_info("ICM20690_GYRO_FCHOICE_B is invalid\n");
        return -EINVAL;
    }

    u8Ret = HI_ICM20690_Transfer_read(GYRO_CONFIG_REGISTER_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FCHOICE_B_SET failed\n");
        return -EAGAIN;
    }

    u8RegisterValue &= ~0x3;
    u8RegisterValue |= fchoice_b;
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(GYRO_CONFIG_REGISTER_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FCHOICE_B_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}


static hi_s32 ICM20690_SetAccelFchoice_B(hi_u8 fchoice_b)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    if (fchoice_b < 0 || fchoice_b > 1)
    {
        print_info("ICM20690_GYRO_FCHOICE_B is invalid\n");
        return -EINVAL;
    }

    u8Ret = HI_ICM20690_Transfer_read(ACCEL_CONFIG_REGISTER_2_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FCHOICE_B_SET failed\n");
        return -EAGAIN;
    }

    u8RegisterValue &= ~(0x1 << 3);
    u8RegisterValue |= fchoice_b;
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(ACCEL_CONFIG_REGISTER_2_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FCHOICE_B_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}


static hi_s32 ICM20690_Gyro_SetFullScaleRange(hi_u8 fs_sel)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    u8Ret = HI_ICM20690_Transfer_read(GYRO_CONFIG_REGISTER_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FCHOICE_B_SET failed\n");
        return -EAGAIN;
    }

    u8RegisterValue &= ~(0x3 << 2);
    u8RegisterValue |= (fs_sel << 2);
    print_info("u8RegisterValue = %x\n", u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write( GYRO_CONFIG_REGISTER_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FULL_SCALE_RANGE_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}


static hi_s32 ICM20690_UI_SetAccelFullScaleRange(hi_u8 fs_sel)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue;

    u8Ret = HI_ICM20690_Transfer_read(ACCEL_CONFIG_REGISTER_1_ADDR, &u8RegisterValue, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FCHOICE_B_SET failed\n");
        return -EAGAIN;
    }

    //print_info("fs_sel = %x u8RegisterValue = %x\n", fs_sel << 3, u8RegisterValue);
    u8RegisterValue &= ~(0x7 << 3);
    u8RegisterValue |= (fs_sel << 3);
    //print_info("fs_sel = %x u8RegisterValue = %x\n", fs_sel << 3, u8RegisterValue);

    u8Ret = HI_ICM20690_Transfer_write(ACCEL_CONFIG_REGISTER_1_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("ICM20690_GYRO_FULL_SCALE_RANGE_SET failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}


static hi_s32 ICM20690_SetGyroAttr(GYRO_STATUS_S stGyroStatus)
{
    //hi_u8 u8Ret;
    //hi_u8 u8RegisterValue;
    /*set gyro odr and BW*/

    if (stGyroStatus.gyro_config.odr == GYRO_OUTPUT_DATA_RATE_32KHZ)
    {
        if (stGyroStatus.u32BandWidth == GYRO_BAND_WIDTH_8800HZ)
        {
            ICM20690_SetGryoFchoice_B(0x01);
        }
        else if (stGyroStatus.u32BandWidth == GYRO_BAND_WIDTH_3600HZ)
        {
            ICM20690_SetGryoFchoice_B(0x02);
        }
        else
        {
            print_info("BandWidth and ODR is not match\n");
            return -EINVAL;
        }

    }
    else if (stGyroStatus.gyro_config.odr == GYRO_OUTPUT_DATA_RATE_8KHZ)
    {
        if (stGyroStatus.u32BandWidth == GYRO_BAND_WIDTH_250HZ)
        {
            ICM20690_SetGyroDLFP_CFG(0x0);
            ICM20690_SetGryoFchoice_B(0x0);
        }
        else if (stGyroStatus.u32BandWidth == GYRO_BAND_WIDTH_3600HZ)
        {
            ICM20690_SetGyroDLFP_CFG(0x7);
            ICM20690_SetGryoFchoice_B(0x0);
        }
        else
        {
            print_info("BandWidth and ODR is not match\n");
            return -EINVAL;
        }
    }
    else if (stGyroStatus.gyro_config.odr <= 1000)
    {
        switch (stGyroStatus.u32BandWidth)
        {
            case GYRO_BAND_WIDTH_5HZ:
                ICM20690_SetGyroDLFP_CFG(0x6);
                break;

            case GYRO_BAND_WIDTH_10HZ:
                ICM20690_SetGyroDLFP_CFG(0x5);
                break;

            case GYRO_BAND_WIDTH_20HZ:
                ICM20690_SetGyroDLFP_CFG(0x4);
                break;

            case GYRO_BAND_WIDTH_41HZ:
                ICM20690_SetGyroDLFP_CFG(0x3);
                break;

            case GYRO_BAND_WIDTH_92HZ:
                ICM20690_SetGyroDLFP_CFG(0x2);
                break;

            case GYRO_BAND_WIDTH_184HZ:
                ICM20690_SetGyroDLFP_CFG(0x1);
                break;

            default:
                print_info("BandWidth and ODR is not match\n");
                break;
        }
    }

    /*set gyro FSR */
    print_info("gyro_status.range is %lld\n", stGyroStatus.gyro_config.fsr);

    switch (stGyroStatus.gyro_config.fsr)
    {
        case GYRO_FULL_SCALE_RANGE_250DPS:
            ICM20690_Gyro_SetFullScaleRange(ICM20690_GYRO_FULL_SCALE_SET_250DPS);
            break;

        case GYRO_FULL_SCALE_RANGE_500DPS:
            ICM20690_Gyro_SetFullScaleRange(ICM20690_GYRO_FULL_SCALE_SET_500DPS);
            break;

        case GYRO_FULL_SCALE_RANGE_1KDPS:
            ICM20690_Gyro_SetFullScaleRange(ICM20690_GYRO_FULL_SCALE_SET_1000DPS);
            break;

        case GYRO_FULL_SCALE_RANGE_2KDPS:
            ICM20690_Gyro_SetFullScaleRange(ICM20690_GYRO_FULL_SCALE_SET_2000DPS);
            break;

        case GYRO_FULL_SCALE_RANGE_31DPS:
            ICM20690_Gyro_SetFullScaleRange(ICM20690_GYRO_FULL_SCALE_SET_31DPS);
            break;

        case GYRO_FULL_SCALE_RANGE_62DPS:
            ICM20690_Gyro_SetFullScaleRange(ICM20690_GYRO_FULL_SCALE_SET_62DPS);
            break;

        case GYRO_FULL_SCALE_RANGE_125DPS:
            ICM20690_Gyro_SetFullScaleRange(ICM20690_GYRO_FULL_SCALE_SET_125DPS);
            break;

        default:
            print_info("ICM20690_GYRO_ATTR is invalid\n");
            return -EINVAL;
    }

    return HI_SUCCESS;
}



static hi_s32 ICM20690_SetAccelAttr(ACC_STATUS_S stAccStatus)
{
    //hi_u8 u8Ret;
    //hi_u8 u8RegisterValue;
    /*set ACCEL odr and BW*/

    if (stAccStatus.acc_config.odr == ACCEL_OUTPUT_DATA_RATE_4KHZ)
    {
        ICM20690_SetAccelFchoice_B(0x01);
    }
    else if (stAccStatus.acc_config.odr == ACCEL_OUTPUT_DATA_RATE_1KHZ)
    {
        ICM20690_SetAccelFchoice_B(0x0);
        ICM20690_SetAccelDLFP_CFG(0x7);
    }
    else if (stAccStatus.acc_config.odr < 1000)
    {
        switch (stAccStatus.u32BandWidth)
        {
            case ACCEL_BAND_WIDTH_5HZ:
                ICM20690_SetAccelDLFP_CFG(0x6);
                break;

            case ACCEL_BAND_WIDTH_10HZ:
                ICM20690_SetAccelDLFP_CFG(0x5);
                break;

            case ACCEL_BAND_WIDTH_21HZ:
                ICM20690_SetAccelDLFP_CFG(0x4);
                break;

            case ACCEL_BAND_WIDTH_44HZ:
                ICM20690_SetAccelDLFP_CFG(0x3);
                break;

            case ACCEL_BAND_WIDTH_99HZ:
                ICM20690_SetAccelDLFP_CFG(0x2);
                break;

            case ACCEL_BAND_WIDTH_218HZ:
                ICM20690_SetAccelDLFP_CFG(0x1);
                break;

            default:
                print_info("BandWidth and ODR is not match\n");
                break;
        }
    }


    /*set ACCEL FSR */
    print_info("accel_status.u32Range is %lld\n", stAccStatus.acc_config.fsr);

    switch (stAccStatus.acc_config.fsr)
    {
        case ACCEL_UI_FULL_SCALE_SET_2G:
            ICM20690_UI_SetAccelFullScaleRange(ICM20690_ACCEL_UI_FULL_SCALE_SET_2G);
            break;

        case ACCEL_UI_FULL_SCALE_SET_4G:
            ICM20690_UI_SetAccelFullScaleRange(ICM20690_ACCEL_UI_FULL_SCALE_SET_4G);
            break;

        case ACCEL_UI_FULL_SCALE_SET_8G:
            ICM20690_UI_SetAccelFullScaleRange(ICM20690_ACCEL_UI_FULL_SCALE_SET_8G);
            break;

        case ACCEL_UI_FULL_SCALE_SET_16G:
            ICM20690_UI_SetAccelFullScaleRange(ICM20690_ACCEL_UI_FULL_SCALE_SET_16G);
            break;

        default:
            print_info("ICM20690_ACCEL_ATTR is invalid\n");
            return -EINVAL;
    }

    return HI_SUCCESS;
}


static hi_s32 ICM20690_GyroLowPowerModeDisable(void)
{
    hi_u8 u8Ret;
    hi_u8 u8RegisterValue = 0x00;

    u8Ret = HI_ICM20690_Transfer_write(LP_MODE_CONFIG_REGISTER_ADDR, &u8RegisterValue, 1);

    if (u8Ret)
    {
        print_info("RESET_FIFO failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}



hi_s32 MotionSensor_GetTrigerConfig(TRIGER_CONFIG_S* pstTrigerConfig)
{
    hi_s32 s32Ret = HI_SUCCESS;
    pstTrigerConfig->eTrigerMode =  ICM20690_dev->stTrigerConfig.eTrigerMode;
    pstTrigerConfig->uTrigerInfo.stTimerConfig.u32interval = ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval;
    return s32Ret;
}

#if 0
static hi_s32 All_Register_Read(void)
{
    hi_s32 i;
    HI_S8 ret, ch;

    for (i = 0; i < 127; i++)
    {

        ret = HI_ICM20690_Transfer_read(i, &ch, 1, 0);

        if (ret)
        {
            print_info("register_read failed\n");
            return -EAGAIN;
        }

        printk("register[0x%x] value is 0x%x\n", i, ch);
    }

    return HI_SUCCESS;
}
#endif
static hi_s32 inline Fifo_Is_Overflow(void)
{
    hi_u8 ret, ch;

    ret = HI_ICM20690_Transfer_read(0x3a, &ch, 1, HI_FALSE);

    if (ret)
    {
        print_info("register_read failed\n");
        return -EAGAIN;
    }

    return (ch & (0x1 << 4));
}

static hi_s32 inline Reset_Fifo(void)
{
    //REset FIFO , in case the FIFO is overflow //00000100 -> 0x6A 	Set 00000100 to USER_CTRL (Reset the FIFO)
    hi_u8 u8ch, u8Ret;

    u8Ret = HI_ICM20690_Transfer_read(USER_CONTROL_REGISTER_ADDR, &u8ch, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("RESET_FIFO failed\n");
        return -EAGAIN;
    }

    u8ch |= 0x04;

    u8Ret = HI_ICM20690_Transfer_write( USER_CONTROL_REGISTER_ADDR, &u8ch, 1);

    if (u8Ret)
    {
        print_info("RESET_FIFO failed\n");
        return -EAGAIN;
    }

    ICM20690_dev->b_FLAG_FIFOIncomming = 0;
    return HI_SUCCESS;
}


static hi_s32 inline ICM20690_GetFifoLengthAndCount(void)
{
    hi_u32 recordLength;
    hi_u8  ret, buff[2];
    memset(buff, 0 , sizeof(buff));
#if 1
    ret = HI_ICM20690_Transfer_read(FIFO_COUNTH, buff, 2, HI_FALSE);

    if (ret)
    {
        print_info("get_ICM20690_fifo_count_Length failed\n");
        return -EAGAIN;
    }
#else
    ret = HI_ICM20690_Transfer_read(FIFO_COUNTH, &buff[0], 1);

    if (ret)
    {
        print_info("get_ICM20690_fifo_count_Length failed\n");
        return -EAGAIN;
    }

     ret = HI_ICM20690_Transfer_read(FIFO_COUNTL, &buff[1], 1);

    if (ret)
    {
        print_info("get_ICM20690_fifo_count_Length failed\n");
        return -EAGAIN;
    }
#endif

    ///osal_printk("buff[0]:%d buff[1]:%d\n",buff[0], buff[1]);

    ICM20690_dev->recordNum = (short)(((short)buff[0]) << 8 | buff[1]);

    recordLength = ((ICM20690_dev->b_FLAG_ACC_FIFO_Enabled + ICM20690_dev->b_FLAG_GYRO_FIFO_Enabled) * 6);
    ICM20690_dev->fifoLength = ICM20690_dev->recordNum * recordLength;

    //print_info("recordNum:%d,fifoLength:%d,recordLength:%d\n",ICM20690_dev->recordNum,ICM20690_dev->fifoLength,recordLength);
    //osal_printk("++++fun:%s recordNum:%d\n",__func__,ICM20690_dev->recordNum);
    //osal_printk("+recordNum:%d\n",ICM20690_dev->recordNum);
    return HI_SUCCESS;
}

static hi_s32 inline ICM20690_UI_FifoSaveData(void)
{
    hi_s32 s32Ret, i;

#if 0
    ret = FIFO_IS_OVERFLOW();

    if (s32Ret)
    {
        mpu_print_info("FIFO is over flow !!\n");
        RESET_FIFO(); //?? FIFO reset , over flow
        //  	    return -EMSGSIZE;
    }

#else


    if (ICM20690_dev->fifoLength > 1024)
    {
        print_info("FIFO is over flow !!, NUM  = %d\n\n", ICM20690_dev->recordNum);
        s32Ret = Reset_Fifo(); //?? FIFO reset , over flow

        if (s32Ret)
        {
            print_info("reset fifo failed\n");
            return -EAGAIN;
        }

        return HI_SUCCESS;
    }

#endif

    //begin to read FIFO in several Sections

    for (i = 0; i < ICM20690_dev->fifoLength / ICM20690_FIFO_R_MAX_SIZE; i++)
    {

        s32Ret = HI_ICM20690_Transfer_read(FIFO_R_W, ICM20690_dev->FIFO_buf + i * ICM20690_FIFO_R_MAX_SIZE, ICM20690_FIFO_R_MAX_SIZE, HI_TRUE);

        if (s32Ret)
        {
            print_info("read FIFO in several Sections failed\n");
            return -EAGAIN;
        }
    }

    s32Ret = HI_ICM20690_Transfer_read(FIFO_R_W, ICM20690_dev->FIFO_buf + i * ICM20690_FIFO_R_MAX_SIZE, ICM20690_dev->fifoLength - \
                                       ICM20690_FIFO_R_MAX_SIZE * (ICM20690_dev->fifoLength / ICM20690_FIFO_R_MAX_SIZE), HI_TRUE);

    if (s32Ret)
    {
        print_info("read FIFO in last_data failed\n");
        return -EAGAIN;
    } //FIFO_R_W(0x74)

    ICM20690_dev->b_FLAG_FIFOIncomming = 1;

    return HI_SUCCESS;
}


hi_s32 FIFO_DATA_UPDATE(void)
{
    return ICM20690_UI_FifoSaveData();
}

hi_s32 FIFO_DATA_RESET(void)
{
    hi_s32 s32Ret;

    if (ICM20690_dev->u8FifoEn)
    {
        s32Ret = Reset_Fifo();

        if (s32Ret)
        {
            print_info("reset fifo failed\n");
            return -EAGAIN;
        }

    }

    return HI_SUCCESS;
}

static hi_s32 ICM20690_UI_FifoModeEnable(hi_u32 u32DevMode)
{
    hi_u8 ch, u8Ret;
    u8Ret = Reset_Fifo();

    if (u8Ret)
    {
        print_info("reset fifo failed\n");
        return -EAGAIN;
    }

    //i2c_write_1B(0x6A,0x40);                                     //Enable the FIFO Operation mode
    ch = 0x40;

    u8Ret = HI_ICM20690_Transfer_write(USER_CONTROL_REGISTER_ADDR, &ch, 1);

    if (u8Ret)
    {
        print_info("Enable the FIFO Operation mode failed(0x%x)\n", u8Ret);
        return -EAGAIN;
    }

    //i2c_read_1B(0x1D,&ch); ch|=0xC0; i2c_write_1B(0x1D,ch);     //11000000 -> 0x1D	Get and Set 11000000 to ACCEL _CONFIG2 (Enable the FIFO size to 1024)

    u8Ret = HI_ICM20690_Transfer_read(ACCEL_CONFIG_REGISTER_2_ADDR, &ch, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("Enable the FIFO size to 1024 failed\n");
        return -EAGAIN;
    }

    ch |= 0xC0;
    print_info("register = %x\n", ch);
    u8Ret = HI_ICM20690_Transfer_write(ACCEL_CONFIG_REGISTER_2_ADDR, &ch, 1);

    if (u8Ret)
    {
        print_info("Enable the FIFO size to 1024 failed\n");
        return -EAGAIN;
    }

    u8Ret = HI_ICM20690_Transfer_read(CONFIGURATION_REGISTER_ADDR, &ch, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("Enable FIFO mode and record mode failed\n");
        return -EAGAIN;
    }

    ch |= 0x80;
    print_info("CONFIGURATION_REGISTER_ADDR:%x\n", ch);
    u8Ret = HI_ICM20690_Transfer_write( CONFIGURATION_REGISTER_ADDR, &ch, 1);

    if (u8Ret)
    {
        print_info("Enable FIFO mode and record mode failed\n");
        return -EAGAIN;
    }

    u8Ret = HI_ICM20690_Transfer_read(FIFO_ENABLE_REGISTER_ADDR, &ch, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("Enable ACC and GYRO in FIFO failed\n");
        return -EAGAIN;
    }

    if (MSENSOR_DEVICE_ACC & u32DevMode)
    {
        ch |= 0x08;
        ICM20690_dev->b_FLAG_ACC_FIFO_Enabled = 1;
    }

    if (MSENSOR_DEVICE_GYRO & u32DevMode)
    {
        ch |= 0x70;
        ICM20690_dev->b_FLAG_GYRO_FIFO_Enabled = 1;
    }

    print_info("FIFO_ENABLE_REGISTER_ADDR:0x%x\n", ch);

    u8Ret = HI_ICM20690_Transfer_write( FIFO_ENABLE_REGISTER_ADDR, &ch, 1);

    if (u8Ret)
    {
        print_info("Enable ACC and GYRO in FIFO failed\n");
        return -EAGAIN;
    }

    //ICM20690_dev->b_FLAG_ACC_FIFO_Enabled = 1;
    //ICM20690_dev->b_FLAG_GYRO_FIFO_Enabled = 1;
    ch = ICM20690_dev->stTrigerConfig.uTrigerInfo.stExternInterruptConfig.u32Interrupt_num / (ICM20690_dev->b_FLAG_ACC_FIFO_Enabled + ICM20690_dev->b_FLAG_GYRO_FIFO_Enabled);
    u8Ret = HI_ICM20690_Transfer_write(0x61, &ch, 1);

    if (u8Ret)
    {
        print_info("Enable the FIFO Operation mode failed\n");
        return -EAGAIN;
    }

    //set fifo data len
    su8GyroAccDataLen = (ICM20690_dev->b_FLAG_ACC_FIFO_Enabled + ICM20690_dev->b_FLAG_GYRO_FIFO_Enabled) * 6;
    u8Ret = Reset_Fifo();

    if (u8Ret)
    {
        print_info("reset fifo failed\n");
        return -EAGAIN;
    }

    return HI_SUCCESS;
}

static int ICM20690_INTConfig(void)
{
    hi_u8 ch, u8Ret;

    /*set interrupt config.. bit7:INT_LEVEL. bit6:INT_OPEN.bit5:LATCH_INT_EN.bit4:INT_RD_CLEAR*/
    ch = 0xA0;
    u8Ret = HI_ICM20690_Transfer_write(INT_PIN_CONFIGURATION, &ch, 1);

    if (u8Ret)
    {
        print_info("Set INT config failed\n");
        return -EAGAIN;
    }

    /*set interrupt type  fifo watermark INT or data ready INT*/
    if (HI_TRUE == ICM20690_dev->u8FifoEn)
    {
        u8Ret = HI_ICM20690_Transfer_read(FIFO_WM_INT_STATUS, &ch, 1, HI_FALSE);
        //ch = 0x40;
        //u8Ret = HI_ICM20690_Transfer_write(FIFO_WM_INT_STATUS, &ch, 1);

        if (u8Ret)
        {
            print_info("Set INT enable failed\n");
            return -EAGAIN;
        }

    }
    else
    {
        /*data ready interrupt enable set*/
        ch = 0x01;

        u8Ret = HI_ICM20690_Transfer_write(INTERRUPT_ENABLE, &ch, 1);

        if (u8Ret)
        {
            print_info("Set INT enable failed\n");
            return -EAGAIN;
        }

        HI_ICM20690_Transfer_read(INTERRUPT_ENABLE, &ch, 1, HI_FALSE);
        print_info("INTERRUPT_ENABLE = %x\n", ch);
    }

    return u8Ret;
}


static hi_s32 ICM20690_GetTemperature(hi_s32* s32Temperature)
{
    hi_u32 u32Ret;
    hi_u8 u8RegisterValue[2];

    u32Ret = HI_ICM20690_Transfer_read(DEV_TEMPERATURE_LSB_ADDR, &u8RegisterValue[0], 2, HI_FALSE);

    if (u32Ret)
    {
        print_info("ICM20690_GetTemperature failed\n");
        return -EAGAIN;
    }

    *s32Temperature = ((hi_s32)((HI_S8) (u8RegisterValue[0]) << 8)) | u8RegisterValue[1];
    /*1024 times of Celsius temperature magnification*/
    *s32Temperature = ROOMTEMP_OFFSET + ((*s32Temperature) * GRADIENT_TEMP / TEMP_SENSITIVITY);
    return HI_SUCCESS;
}

hi_s32 ICM20690_ReadAccelData_XYZ(hi_msensor_sample_data* stAccData)
{
    hi_u8 buff[6] = {0, 0, 0, 0, 0, 0};
    hi_s32 s32Ret = HI_SUCCESS;
#ifdef Hi3559A_DMA
    /*6 registers continuously read, with a first address of accel_xout_h*/
    s32Ret = i2cdev_dma_read(ACCEL_XOUT_H, 6, buff);

    if (s32Ret < 0)
    {
        print_info("ICM20690_read_accel_xyz failed\n");
        return -EAGAIN;
    }

#else

    s32Ret = HI_ICM20690_Transfer_read(ACCEL_XOUT_H , &buff[0], 6, HI_FALSE);

    if (s32Ret)
    {
        print_info("ICM20690_read_accel_xyz failed\n");
        return -EAGAIN;
    }

    stAccData->x = ((buff[0] << 8) & 0xff00) | (buff[1] & 0xff);
    stAccData->y = ((buff[2] << 8) & 0xff00) | (buff[3] & 0xff);
    stAccData->z = ((buff[4] << 8) & 0xff00) | (buff[5] & 0xff);
    stAccData->temp  = ICM20690_dev->s32temperature;
    stAccData->pts   = u64PTSNow;

#endif
    return HI_SUCCESS;
}

hi_s32 ICM20690_ReadGyroData_XYZ(hi_msensor_sample_data* stGyroData)
{
    hi_u8 buff[6] = {0, 0, 0, 0, 0, 0};
    hi_s32 res;
#ifdef Hi3559A_DMA
    /* read out 6 registers in a row with a first address of gyro_xout_h*/
    res = i2cdev_dma_read(0x43, 6, buff);

    if (res < 0)
    {
        print_info("ICM20690_read_gyro_xyz failed\n");
        return -EAGAIN;
    }

#else

    res = HI_ICM20690_Transfer_read(GYRO_XOUT_H , &buff[0], 6, HI_FALSE);

    if (res)
    {
        print_info("ICM20690_read_accel_xH failed\n");
        return -EAGAIN;
    }

    stGyroData->x = ((buff[0] << 8) & 0xff00) | (buff[1] & 0xff);
    stGyroData->y = ((buff[2] << 8) & 0xff00) | (buff[3] & 0xff);
    stGyroData->z = ((buff[4] << 8) & 0xff00) | (buff[5] & 0xff);
    stGyroData->temp  = ICM20690_dev->s32temperature;
    stGyroData->pts   = u64PTSNow;
#endif
    return HI_SUCCESS;
}

hi_s32 ICM20690_ReadData(hi_msensor_data* pstIMUdata)
{
    hi_s32 s32Ret;
    memset(&ICM20690_dev->accel_cur_data, 0x0, sizeof(hi_msensor_sample_data));
    memset(&ICM20690_dev->gyro_cur_data, 0x0, sizeof(hi_msensor_sample_data));

    if (i_thread % 50 == 0)
    {
        s32Ret = ICM20690_GetTemperature(&ICM20690_dev->s32temperature);

        if (HI_SUCCESS != s32Ret)
        {
            print_info("bmi160_get_temp failed! ret=%x\n", s32Ret);
            return s32Ret;
        }

    }

    if (stMSensorMode.device_mask & MSENSOR_DEVICE_ACC)
    {
        s32Ret = ICM20690_ReadAccelData_XYZ(&ICM20690_dev->accel_cur_data);

        if (s32Ret)
        {
            print_info("ICM20690_read_accel_data_XYZ failed\n");
            return -ENODATA;
        }
    }

    if (stMSensorMode.device_mask & MSENSOR_DEVICE_GYRO)
    {
        s32Ret = ICM20690_ReadGyroData_XYZ(&ICM20690_dev->gyro_cur_data);

        if (s32Ret)
        {
            print_info("ICM20690_read_gyro_data_XYZ failed\n");
            return -ENODATA;
        }
    }

    memcpy(&pstIMUdata->attr, &stMSensorMode, sizeof(hi_msensor_attr));

    if (stMSensorMode.device_mask == (MSENSOR_DEVICE_GYRO | MSENSOR_DEVICE_ACC))
    {
        /*accel data handler, pls fix this*/
        //pstIMUdata->u32AccelCount = ICM20690_dev->recordNum;
        pstIMUdata->acc_buffer.data_num = 1;
        //pst_IMUdata->u32AccelValid = ACCEL_DATA_VALID | ACCEL_TEMPERATURE_VALID;

        pstIMUdata->acc_buffer.acc_data[0].x = ICM20690_dev->accel_cur_data.x;
        pstIMUdata->acc_buffer.acc_data[0].y = ICM20690_dev->accel_cur_data.y;
        pstIMUdata->acc_buffer.acc_data[0].z = ICM20690_dev->accel_cur_data.z;
        pstIMUdata->acc_buffer.acc_data[0].temp  = ICM20690_dev->accel_cur_data.temp;
        pstIMUdata->acc_buffer.acc_data[0].pts   = ICM20690_dev->accel_cur_data.pts;
        //ICM20690_dev->stAccStatus.u64LastPts = u64PTSNow;

        /*Gyro data handler*, pls fix this*/
        pstIMUdata->gyro_buffer.data_num = 1;

        pstIMUdata->gyro_buffer.gyro_data[0].x  =  ICM20690_dev->gyro_cur_data.x;
        pstIMUdata->gyro_buffer.gyro_data[0].y  =  ICM20690_dev->gyro_cur_data.y;
        pstIMUdata->gyro_buffer.gyro_data[0].z  =  ICM20690_dev->gyro_cur_data.z;
        pstIMUdata->gyro_buffer.gyro_data[0].temp   =  ICM20690_dev->gyro_cur_data.temp;
        pstIMUdata->gyro_buffer.gyro_data[0].pts    =  ICM20690_dev->gyro_cur_data.pts;

        /*PTS  handle*/
        ICM20690_dev->stGyroStatus.u64LastPts = u64PTSNow;

    }
    else if (stMSensorMode.device_mask == MSENSOR_DEVICE_GYRO)
    {
        /*Gyro data handler*, pls fix this*/
        pstIMUdata->gyro_buffer.data_num = 1;
        //pst_IMUdata->u32GyroValid = GYRO_DATA_VALID  | GYRO_TEMPERATURE_VALID;

        pstIMUdata->gyro_buffer.gyro_data[0].x  =  ICM20690_dev->gyro_cur_data.x;
        pstIMUdata->gyro_buffer.gyro_data[0].y  =  ICM20690_dev->gyro_cur_data.y;
        pstIMUdata->gyro_buffer.gyro_data[0].z  =  ICM20690_dev->gyro_cur_data.z;
        pstIMUdata->gyro_buffer.gyro_data[0].temp   =  ICM20690_dev->gyro_cur_data.temp;
        pstIMUdata->gyro_buffer.gyro_data[0].pts    =  ICM20690_dev->gyro_cur_data.pts;

        /*PTS  handle*/
        //ICM20690_dev->stGyroStatus.u64LastPts = u64PTSNow;
    }
    else if (stMSensorMode.device_mask == MSENSOR_DEVICE_ACC)
    {
        pstIMUdata->acc_buffer.data_num = 1;
        //pst_IMUdata->u32AccelValid = ACCEL_DATA_VALID | ACCEL_TEMPERATURE_VALID;

        pstIMUdata->acc_buffer.acc_data[0].x =  ICM20690_dev->accel_cur_data.x;
        pstIMUdata->acc_buffer.acc_data[0].y = ICM20690_dev->accel_cur_data.y;
        pstIMUdata->acc_buffer.acc_data[0].z =  ICM20690_dev->accel_cur_data.z;
        pstIMUdata->acc_buffer.acc_data[0].temp = ICM20690_dev->accel_cur_data.temp;
        pstIMUdata->acc_buffer.acc_data[0].pts = ICM20690_dev->accel_cur_data.pts;
        //ICM20690_dev->stAccStatus.u64LastPts = u64PTSNow;
    }

#ifdef TEST_DEBUG
    test_fifo[su32DataCount].stGyroData.x	 = ICM20690_dev->gyro_cur_data.x;
    test_fifo[su32DataCount].stGyroData.y	 = ICM20690_dev->gyro_cur_data.y;
    test_fifo[su32DataCount].stGyroData.z    = ICM20690_dev->gyro_cur_data.z;
    test_fifo[su32DataCount].stGyroData.temp = ICM20690_dev->s32temperature;
    test_fifo[su32DataCount].stGyroData.pts  = u64PTSNow;

    test_fifo[su32DataCount].stAccData.x	 = ICM20690_dev->accel_cur_data.x;
    test_fifo[su32DataCount].stAccData.y	 = ICM20690_dev->accel_cur_data.y;
    test_fifo[su32DataCount].stAccData.z     = ICM20690_dev->accel_cur_data.z;
    test_fifo[su32DataCount].stAccData.temp  = ICM20690_dev->s32temperature;
    test_fifo[su32DataCount].stAccData.pts 	 = u64PTSNow;

    su32DataCount++;
#endif

    //	msleep(1000);
    i_thread++;
    return HI_SUCCESS;
}

static hi_u64 inline HI_MotionSensor_GetCurPts(void)
{
    hi_u64 u64TimeNow;
#if 0
    extern hi_u64 sched_clock(void);
    u64TimeNow = sched_clock();
    do_div(u64TimeNow, 1000);
#else
    u64TimeNow = call_sys_get_time_stamp();
#endif
    return u64TimeNow;
}

static hi_u64 u64Time_last;

static hi_u8  s8TimerFristFlag = 0;

static hi_s32 ICM20690_SetParam(hi_msensor_param* stMSensorParam)
{
    hi_s32 s32Ret = HI_SUCCESS;
    hi_u32 u32SampleRate, u32value;

    if (MSENSOR_DEVICE_GYRO & stMSensorParam->attr.device_mask)
    {
        /*set GyroODRConfig*/
        u32value = stMSensorParam->config.gyro_config.odr / GRADIENT;
        u32SampleRate = u32value;

        if (u32SampleRate != GYRO_OUTPUT_DATA_RATE_32KHZ && u32SampleRate != GYRO_OUTPUT_DATA_RATE_8KHZ)
        {
            if ((1000 % u32value) != 0 || u32value > 1000)
            {
                u32value = GYRO_OUTPUT_DATA_RATE_BUTT;
            }
            else
            {
                u32value = GYRO_OUTPUT_DATA_RATE_UNDER_1KHZ;
            }
        }

        switch (u32value)
        {
            case GYRO_OUTPUT_DATA_RATE_UNDER_1KHZ:
            case GYRO_OUTPUT_DATA_RATE_32KHZ:
            case GYRO_OUTPUT_DATA_RATE_8KHZ:
                stMSensorParam->config.gyro_config.odr = u32SampleRate;
                break;

            case GYRO_OUTPUT_DATA_RATE_BUTT:
                print_info("not support Gyro ODR!\n");
                s32Ret = HI_FAILURE;
                break;

            default:
                print_info("out of gyro range!!!\n");
                s32Ret = HI_FAILURE;
                break;
        }

        /*set gyro valid data bit*/
        stMSensorParam->config.gyro_config.data_width = ICM20690_VALID_DATA_BIT;
    }

    if (MSENSOR_DEVICE_ACC & stMSensorParam->attr.device_mask)
    {
        print_info("set acc odr range!!\n");
        /*set AccelODRConfig*/
        u32value = stMSensorParam->config.acc_config.odr / GRADIENT;
        u32SampleRate = u32value;

        if (u32SampleRate != ACCEL_OUTPUT_DATA_RATE_4KHZ && u32SampleRate != ACCEL_OUTPUT_DATA_RATE_1KHZ)
        {
            if ((1000 % u32value) != 0 || u32value > 4000)
            {
                u32value = ACCEL_OUTPUT_DATA_RATE_BUTT;
            }
            else
            {
                u32value = ACCEL_OUTPUT_DATA_RATE_UNDER_1KHZ;
            }
        }

        switch (u32value)
        {
            case ACCEL_OUTPUT_DATA_RATE_UNDER_1KHZ:

            case ACCEL_OUTPUT_DATA_RATE_4KHZ:

            case ACCEL_OUTPUT_DATA_RATE_1KHZ:
                stMSensorParam->config.acc_config.odr = u32SampleRate;
                break;

            default:
                print_info("not support ACCEL ODR!\n");
                s32Ret = HI_FAILURE;
                break;
        }

        /*set accel valid data bit*/
        stMSensorParam->config.acc_config.data_width = ICM20690_VALID_DATA_BIT;
    }

    //print_info("gyro u32Odr:%lld,range:%lld\n",stMSensorParam->config.gyro_config.odr,
    //stMSensorParam->config.gyro_config.fsr);
    //print_info("acc u32Odr:%lld,range:%lld\n",stMSensorParam->config.acc_config.odr,
    //stMSensorParam->config.acc_config.fsr);
    osal_memcpy(&stMSensorMode, &stMSensorParam->attr, sizeof(hi_msensor_attr));
    return s32Ret;
}

static hi_s32 ICM20690_AxisFifoSensorInit(hi_u32 u32DevMode)
{
    //hi_u8 tmp_val;
    hi_s32 s32Ret;

    /*1. reset ICM20690 */
    s32Ret = ICM20690_Reset();

    if (s32Ret)
    {
        print_info("reset ICM20690 failed\n");
        return -EAGAIN;
    }

    msleep(100);
#ifdef TRANSFER_SPI
    //#ifdef TRANSFER_TYPE_SPI
    //disable i2c transfer
    s32Ret = HI_I2C_Disable();

    if (s32Ret)
    {
        print_info("disable i2c failed\n");
        return -EAGAIN;
    }

#endif

    /*2. enable  PLL, CLKSEL =  1*/
    s32Ret = ICM20690_SetClk();

    if (s32Ret)
    {
        print_info("ICM20690_SetClk failed\n");
        return -EAGAIN;
    }

    msleep(30);

    /*3. enable  gyro, accel*/
    s32Ret = ICM20690_SetAxisMode(u32DevMode);

    if (s32Ret)
    {
        print_info("ICM20690_SetAxisMode failed\n");
        return -EAGAIN;
    }

    msleep(30);

    /*4. set sample rate to 1KHz,1000/(1+0) */
    if (MSENSOR_DEVICE_GYRO & u32DevMode)
    {
        if (ICM20690_dev->stGyroStatus.gyro_config.odr <= 1000)
        {
            print_info("ICM20690_dev->stGyroStatus.u32Odr = %lld\n", ICM20690_dev->stGyroStatus.gyro_config.odr);
            s32Ret = ICM20690_SetSampleRate(ICM20690_dev->stGyroStatus.gyro_config.odr);

            if (s32Ret)
            {
                print_info("ICM20690_SetSampleRate failed\n");
                return -EAGAIN;
            }

            msleep(5);
        }
    }
    else
    {
        if (ICM20690_dev->stAccStatus.acc_config.odr <= 1000)
        {
            //print_info("ICM20690_dev->stGyroStatus.u32Odr = %d\n", ICM20690_dev->stAccStatus.acc_config.odr);
            s32Ret = ICM20690_SetSampleRate(ICM20690_dev->stAccStatus.acc_config.odr);

            if (s32Ret)
            {
                print_info("ICM20690_SetSampleRate failed\n");
                return -EAGAIN;
            }

            msleep(5);
        }
    }

    if (ICM20690_dev->u8FifoEn)
    {
        /*5.disable FIFO_MODE, DLFP_CFG = 0*/
        s32Ret = ICM20690_UI_FifoModeEnable(u32DevMode);

        if (s32Ret)
        {
            print_info("ICM20690_UI_6FifoModeEnable failed(%d)\n", s32Ret);
            return -EAGAIN;
        }

        msleep(5);

        ICM20690_dev->FIFO_buf = osal_kmalloc(1024, osal_gfp_kernel);

        if (!ICM20690_dev->FIFO_buf) {
            print_info("kzalloc FIFO_buf failed\n");
            return -ENOMEM;
        }

        osal_memset(ICM20690_dev->FIFO_buf,0,1024);
    }

    if (TRIGER_EXTERN_INTERRUPT == ICM20690_dev->stTrigerConfig.eTrigerMode)
    {
        /*interrupt config*/
        s32Ret = ICM20690_INTConfig();

        if (s32Ret)
        {
            print_info("INT config failed\n");
            return -EAGAIN;
        }
    }

    if (MSENSOR_DEVICE_GYRO & u32DevMode)
    {
        /*6. 250dps, 8K ODR, 250Hz BW for gyro*/
        s32Ret = ICM20690_SetGyroAttr(ICM20690_dev->stGyroStatus);

        if (s32Ret)
        {
            print_info("ICM20690_SetGyroAttr failed\n");
            goto err_kzalloc;
        }

        msleep(5);
    }

    if (MSENSOR_DEVICE_ACC & u32DevMode)
    {
        /*7. accel FSR setting: UI accel to 4G, OIS accel to 2G*/
        s32Ret = ICM20690_SetAccelAttr(ICM20690_dev->stAccStatus);

        if (s32Ret)
        {
            print_info("ICM20690_SetAccelAttr failed\n");
            goto err_kzalloc;
        }

        msleep(5);
    }

    /*9. disable gyro low power mode*/
    s32Ret = ICM20690_GyroLowPowerModeDisable();

    if (s32Ret)
    {
        print_info("ICM20690_GyroLowPowerModeDisable failed\n");
        goto err_kzalloc;
    }

    return HI_SUCCESS;
err_kzalloc:

    if (ICM20690_dev->FIFO_buf)
    {
        kfree(ICM20690_dev->FIFO_buf);
        ICM20690_dev->FIFO_buf = NULL;
    }

    return -EAGAIN;
}

static hi_void ICM20690_AxisFifoSensorDeInit(void)
{
    if (ICM20690_dev->FIFO_buf != NULL)
    {
        kfree(ICM20690_dev->FIFO_buf);
        ICM20690_dev->FIFO_buf = NULL;
    }

}

static hi_s32 ICM20690_SensorInit(hi_u32 u32DevMode)
{
    hi_s32 s32Ret = HI_SUCCESS;

    if ((u32DevMode & MSENSOR_DEVICE_GYRO) || (u32DevMode & MSENSOR_DEVICE_ACC))
    {
        s32Ret = ICM20690_AxisFifoSensorInit(u32DevMode);

        if (HI_SUCCESS != s32Ret)
        {
            print_info("ICM20690_SensorInit failed! ret=%x\n", s32Ret);
        }
    }
    else
    {
        print_info("ICM20690_SensorInit not support this mode : %d\n", u32DevMode);
        s32Ret = HI_FAILURE;
    }

    return s32Ret;
}

hi_s32 HI_MOTIONSENSOR_SaveData_ModeDofFifo(hi_msensor_attr attr)
{
    hi_s32 i;
    hi_s32 s32Ret = HI_SUCCESS;
    hi_ulong  time_inter = 0;
	hi_u32 u32Adddatacnt = 0;

    if (NULL == MotionSensorStatus)
    {
        print_info("MotionSensorStatus is NULL!!!! MotionSensorStatus(%p)\n", MotionSensorStatus);
        return HI_FAILURE;
    }

    //print_info("axis_mode:%d\n",axis_mode);

    HI_ASSERT(HI_NULL != MotionSensorStatus);

    //osal_printk("recordNum:%d\n", ICM20690_dev->recordNum);

	if(su8datacount < DATA_RECORD_CNT)
	{
		saEveryDataCntBackup[su8datacount] = ICM20690_dev->recordNum;
	}
	else
	{
		for(i=0; i < DATA_RECORD_CNT - 1;i++)
		{
			saEveryDataCntBackup[i]          = saEveryDataCntBackup[i+1];
		}
		saEveryDataCntBackup[DATA_RECORD_CNT - 1] = ICM20690_dev->recordNum;
	}
    if(su8Timecnt < TIME_RECORD_CNT)
	{
		saTimeBackup[su8Timecnt] = u64PTSNow;
		//saEveryDataCntBackup[su8Timecnt] = ICM20690_dev->recordNum;

		for(i=0; i <= su8datacount;i++)
		{
			u32Adddatacnt += saEveryDataCntBackup[i];
		}
		//osal_printk("time:%lld,%lld,num:%d,%d\n",saTimeBackup[su8Timecnt], saTimeBackup[0],u32Adddatacnt,su8Timecnt);
		time_inter = osal_div_u64(saTimeBackup[su8Timecnt] - saTimeBackup[0],DIV_0_TO_1(u32Adddatacnt));
		//osal_printk("inter:%ld,last:%lld\n",time_inter,ICM20690_dev->stGyroStatus.u64LastPts);

	}
	else
	{
		for(i=0; i < TIME_RECORD_CNT - 1;i++)
		{
			saTimeBackup[i] = saTimeBackup[i+1];

		}

		saTimeBackup[TIME_RECORD_CNT - 1] = u64PTSNow;
		for(i=1; i < DATA_RECORD_CNT;i++)
		{
			u32Adddatacnt += saEveryDataCntBackup[i];
		}
		//osal_printk("(%d)time:%lld,%lld,num:%d,now:%lld\n",__LINE__,saTimeBackup[TIME_RECORD_CNT - 1], saTimeBackup[0],u32Adddatacnt);
		time_inter = osal_div_u64(saTimeBackup[TIME_RECORD_CNT - 1] - saTimeBackup[0],DIV_0_TO_1(u32Adddatacnt));
        //osal_printk("line:%d,%lld\n",__LINE__,saTimeBackup[TIME_RECORD_CNT - 1] - saTimeBackup[0]);
	}
    if(su8datacount < DATA_RECORD_CNT)
    {
        su8datacount++;
    }
    //time_inter = osal_div_u64((u64PTSNow - ICM20690_dev->stGyroStatus.u64LastPts) * SCALE_TIMES, DIV_0_TO_1(ICM20690_dev->recordNum));

    osal_memcpy(&stIMUdata.attr, &attr, sizeof(hi_msensor_attr));

	//osal_printk("inter:%ld,last:%lld\n",time_inter,ICM20690_dev->stGyroStatus.u64LastPts);
    for (i = 0; i < ICM20690_dev->recordNum; i++)
    {
        if (attr.device_mask == (MSENSOR_DEVICE_GYRO | MSENSOR_DEVICE_ACC))
        {


            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].x =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 0]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 1] & 0xff);
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].y = (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 2]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 3] & 0xff);
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].z =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 4]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 5] & 0xff);
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].temp = ICM20690_dev->s32temperature;
            //stIMUdata.acc_buffer.acc_data[i].pts = u64PTSNow - (ICM20690_dev->recordNum - i - 1) * time_inter / SCALE_TIMES;
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].pts = ICM20690_dev->stGyroStatus.u64LastPts + time_inter;
			ICM20690_dev->stAccStatus.u64LastPts = stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].pts;
			stIMUdata.acc_buffer.data_num++;

            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].x  =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 6]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 7] & 0xff);
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].y  = (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 8]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 9] & 0xff);
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].z  =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 10]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 11] & 0xff);
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].temp = ICM20690_dev->s32temperature;

            //stIMUdata.gyro_buffer.gyro_data[i].pts = u64PTSNow - (ICM20690_dev->recordNum - i - 1) * time_inter / SCALE_TIMES;
             stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].pts = ICM20690_dev->stGyroStatus.u64LastPts + time_inter;

            ICM20690_dev->stGyroStatus.u64LastPts = stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].pts;
             stIMUdata.gyro_buffer.data_num++;

        }
        else if (attr.device_mask == MSENSOR_DEVICE_GYRO)
        {
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].x  =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 0]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 1] & 0xff);
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].y  = (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 2]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 3] & 0xff);
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].z  =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 4]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 5] & 0xff);
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].temp = ICM20690_dev->s32temperature;
            stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].pts = u64PTSNow - (ICM20690_dev->recordNum - i - 1) * time_inter;
            /*PTS  handle*/
            //ICM20690_dev->stGyroStatus.u64LastPts = u64PTSNow;
            ICM20690_dev->stGyroStatus.u64LastPts = stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num].pts;
            stIMUdata.gyro_buffer.data_num++;
        }
        else if (attr.device_mask == MSENSOR_DEVICE_ACC)
        {
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].x =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 0]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 1] & 0xff);
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].y = (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 2]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 3] & 0xff);
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].z =  (short)(((short)(ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 4]) << 8) & 0xff00) \
                    | (ICM20690_dev->FIFO_buf[i * su8GyroAccDataLen + 5] & 0xff);
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].temp = ICM20690_dev->s32temperature;
            stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].pts = u64PTSNow - (ICM20690_dev->recordNum - i - 1) * time_inter;
            //ICM20690_dev->stAccStatus.u64LastPts = u64PTSNow;
            ICM20690_dev->stAccStatus.u64LastPts = stIMUdata.acc_buffer.acc_data[stIMUdata.acc_buffer.data_num].pts;
            stIMUdata.acc_buffer.data_num++;
        }

#ifdef TEST_DEBUG

        test_fifo[su32DataCount].stGyroData.x	= stIMUdata.gyro_buffer.gyro_data[i].x;
        test_fifo[su32DataCount].stGyroData.y	= stIMUdata.gyro_buffer.gyro_data[i].y;
        test_fifo[su32DataCount].stGyroData.z   = stIMUdata.gyro_buffer.gyro_data[i].z;
        test_fifo[su32DataCount].stGyroData.temp = stIMUdata.gyro_buffer.gyro_data[i].temp;
        test_fifo[su32DataCount].stGyroData.pts 	= stIMUdata.gyro_buffer.gyro_data[i].pts;

        if (attr.device_mask & MSENSOR_DEVICE_ACC)
        {
            test_fifo[su32DataCount].stAccData.x = stIMUdata.acc_buffer.acc_data[i].x;
            test_fifo[su32DataCount].stAccData.y = stIMUdata.acc_buffer.acc_data[i].y;
            test_fifo[su32DataCount].stAccData.z = stIMUdata.acc_buffer.acc_data[i].z;
            test_fifo[su32DataCount].stAccData.temp = stIMUdata.acc_buffer.acc_data[i].temp;
            test_fifo[su32DataCount].stAccData.pts = stIMUdata.acc_buffer.acc_data[i].pts;
        }

        su32DataCount++;
#endif
    }

    if(su8Timecnt < TIME_RECORD_CNT)
    {
        saTimeBackup[su8Timecnt] = ICM20690_dev->stGyroStatus.u64LastPts;
        su8Timecnt++;
    }
    else
    {
    	saTimeBackup[TIME_RECORD_CNT - 1] = ICM20690_dev->stGyroStatus.u64LastPts;
    }

    return s32Ret;
}

hi_s32 MOTIONSENSOR_GetData(hi_msensor_attr attr)
{
    hi_s32 s32Ret = HI_SUCCESS;
//
    if ((attr.device_mask & MSENSOR_DEVICE_GYRO) || (attr.device_mask & MSENSOR_DEVICE_ACC))
    {
        s32Ret = HI_MOTIONSENSOR_SaveData_ModeDofFifo(attr);
    }
    else
    {
        print_info("[error]Not support MODE\n");
        s32Ret = HI_FAILURE;
    }
    return s32Ret;
}


static hi_void ICM20690_SensorDeInit(hi_s32 axis_mode)
{
    if ((axis_mode & MSENSOR_DEVICE_GYRO) || (axis_mode & MSENSOR_DEVICE_ACC))
    {
        ICM20690_AxisFifoSensorDeInit();
    }
    else
    {
        print_info("ICM20690_SensorInit not support this mode : %d\n", axis_mode);
    }

    //mutex_destroy(&ICM20690_dev->mutex);
    return;
}


static hi_s32 ICM20690_ParamInit(hi_msensor_param stMSensorParam)
{
    hi_s32 s32Ret = HI_SUCCESS;

    if (MSENSOR_DEVICE_GYRO & stMSensorParam.attr.device_mask)
    {
        /*gyro attr set*/
        if (stMSensorParam.config.gyro_config.odr == GYRO_OUTPUT_DATA_RATE_32KHZ)
        {
            //default :GYRO_BAND_WIDTH_3600HZ
            ICM20690_dev->stGyroStatus.u32BandWidth = GYRO_BAND_WIDTH_3600HZ;
            ICM20690_dev->stGyroStatus.gyro_config.odr = stMSensorParam.config.gyro_config.odr;
        }
        else if (stMSensorParam.config.gyro_config.odr == GYRO_OUTPUT_DATA_RATE_8KHZ)
        {
            //default :GYRO_BAND_WIDTH_250HZ
            ICM20690_dev->stGyroStatus.u32BandWidth = GYRO_BAND_WIDTH_250HZ;
            ICM20690_dev->stGyroStatus.gyro_config.odr = stMSensorParam.config.gyro_config.odr;
        }
        else if (stMSensorParam.config.gyro_config.odr <= 100)
        {
            ICM20690_dev->stGyroStatus.u32BandWidth = GYRO_BAND_WIDTH_20HZ;
            ICM20690_dev->stGyroStatus.gyro_config.odr = stMSensorParam.config.gyro_config.odr;
        }
        else if (stMSensorParam.config.gyro_config.odr <= 300)
        {
            ICM20690_dev->stGyroStatus.u32BandWidth = GYRO_BAND_WIDTH_41HZ;
            ICM20690_dev->stGyroStatus.gyro_config.odr = stMSensorParam.config.gyro_config.odr;
        }
        else if (stMSensorParam.config.gyro_config.odr <= 500)
        {
            ICM20690_dev->stGyroStatus.u32BandWidth = GYRO_BAND_WIDTH_92HZ;
            ICM20690_dev->stGyroStatus.gyro_config.odr = stMSensorParam.config.gyro_config.odr;
        }
        else if (stMSensorParam.config.gyro_config.odr <= 1000)
        {
            ICM20690_dev->stGyroStatus.u32BandWidth = GYRO_BAND_WIDTH_184HZ;
            ICM20690_dev->stGyroStatus.gyro_config.odr = stMSensorParam.config.gyro_config.odr;
        }
        else
        {
            print_info("ICM20690_ParamInit failed! gyro_odr:%lld not found !\n", stMSensorParam.config.gyro_config.odr);
            s32Ret = HI_FAILURE;
        }

        switch (stMSensorParam.config.gyro_config.fsr)
        {
            case GYRO_FULL_SCALE_SET_2KDPS:

            case GYRO_FULL_SCALE_SET_1KDPS:

            case GYRO_FULL_SCALE_SET_500DPS:

            case GYRO_FULL_SCALE_SET_250DPS:

            case GYRO_FULL_SCALE_SET_31DPS:

            case GYRO_FULL_SCALE_SET_62DPS:

            case GYRO_FULL_SCALE_SET_125DPS:
                ICM20690_dev->stGyroStatus.gyro_config.fsr = stMSensorParam.config.gyro_config.fsr;
                break;

            default:
                print_info("ICM20690_ParamInit failed! gyro_fsr:%lld not found !\n", stMSensorParam.config.gyro_config.fsr);
                s32Ret = HI_FAILURE;
                break;
        }
    }

    /*accel attr set*/
    if (MSENSOR_DEVICE_ACC & stMSensorParam.attr.device_mask)
    {
        if (stMSensorParam.config.acc_config.odr == ACCEL_OUTPUT_DATA_RATE_4KHZ)
        {
            //default :ACCEL_BAND_WIDTH_1046HZ
            ICM20690_dev->stAccStatus.u32BandWidth = ACCEL_BAND_WIDTH_1046HZ;
            ICM20690_dev->stAccStatus.acc_config.odr = stMSensorParam.config.acc_config.odr;
        }
        else if (stMSensorParam.config.acc_config.odr == ACCEL_OUTPUT_DATA_RATE_1KHZ)
        {
            //default :ACCEL_BAND_WIDTH_420HZ
            ICM20690_dev->stAccStatus.u32BandWidth = ACCEL_BAND_WIDTH_420HZ;
            ICM20690_dev->stAccStatus.acc_config.odr = stMSensorParam.config.acc_config.odr;
        }
        else if (stMSensorParam.config.acc_config.odr < 100)
        {
            //default :ACCEL_BAND_WIDTH_218HZ
            ICM20690_dev->stAccStatus.u32BandWidth = ACCEL_BAND_WIDTH_21HZ;
            ICM20690_dev->stAccStatus.acc_config.odr = stMSensorParam.config.acc_config.odr;
        }
        else if (stMSensorParam.config.acc_config.odr < 300)
        {
            //default :ACCEL_BAND_WIDTH_218HZ
            ICM20690_dev->stAccStatus.u32BandWidth = ACCEL_BAND_WIDTH_44HZ;
            ICM20690_dev->stAccStatus.acc_config.odr = stMSensorParam.config.acc_config.odr;
        }
        else if (stMSensorParam.config.acc_config.odr < 500)
        {
            //default :ACCEL_BAND_WIDTH_218HZ
            ICM20690_dev->stAccStatus.u32BandWidth = ACCEL_BAND_WIDTH_99HZ;
            ICM20690_dev->stAccStatus.acc_config.odr = stMSensorParam.config.acc_config.odr;
        }
        else if (stMSensorParam.config.acc_config.odr < 1000)
        {
            //default :ACCEL_BAND_WIDTH_218HZ
            ICM20690_dev->stAccStatus.u32BandWidth = ACCEL_BAND_WIDTH_218HZ;
            ICM20690_dev->stAccStatus.acc_config.odr = stMSensorParam.config.acc_config.odr;
        }
        else
        {
            print_info("ICM20690_ParamInit failed! gyro_odr:%lld not found !\n", stMSensorParam.config.acc_config.odr);
            s32Ret = HI_FAILURE;
        }

        switch (stMSensorParam.config.acc_config.fsr)
        {
            case ACCEL_UI_FULL_SCALE_SET_2G:

            case ACCEL_UI_FULL_SCALE_SET_4G:

            case ACCEL_UI_FULL_SCALE_SET_8G:

            case ACCEL_UI_FULL_SCALE_SET_16G:
                ICM20690_dev->stAccStatus.acc_config.fsr = stMSensorParam.config.acc_config.fsr;
                break;

            default:
                print_info("ICM20690_ParamInit failed! accel_range:%lld not found !\n", stMSensorParam.config.acc_config.fsr);
                s32Ret = HI_FAILURE;
                break;
        }
    }

    print_info("gyro odr:%lld,acc odr:%lld\n", stMSensorParam.config.gyro_config.odr,
               stMSensorParam.config.acc_config.odr);
    return s32Ret;

}

static hi_s32 ICM20690_ConfigToParam(hi_msensor_param* stMSensorParam)
{
    hi_s32 s32Ret = HI_SUCCESS;

    if (!(stMSensorParam))
    {
        print_info("MotionSensor_status is NULL\n");
        return HI_FAILURE;
    }

    if ((stMSensorParam->attr.device_mask & MSENSOR_DEVICE_GYRO) || (stMSensorParam->attr.device_mask & MSENSOR_DEVICE_ACC))
    {
        s32Ret = ICM20690_SetParam(stMSensorParam);

        if (HI_SUCCESS != s32Ret)
        {
            print_info("ICM20690_SetParam failed! ret=%x\n", s32Ret);
        }
    }
    else
    {
        print_info("not support mode!\n");
        s32Ret = HI_FAILURE;
    }

    return s32Ret;
}


hi_s32 HI_MotionSensor_DataHandle(void* data)
{
    hi_msensor_attr attr = *(hi_msensor_attr*)data;
    //static hi_msensor_data stMotionSensorData;
    hi_s32 s32Ret;

    //hi_u32 time_inter;
    //MOTION_SENSOR_OUT_DATA_S tmp_data;
    //print_info("start to get data!!\n");
    while (!kthread_should_stop())
    {

        if ((attr.temperature_mask & MSENSOR_TEMP_GYRO) ||
            (attr.temperature_mask & MSENSOR_TEMP_ACC))
        {
            if (i_thread % 50 == 0)
            {
                s32Ret = ICM20690_GetTemperature(&ICM20690_dev->s32temperature);

                if (HI_SUCCESS != s32Ret)
                {
                    print_info("get_temp failed! ret=%x\n", s32Ret);
                    return s32Ret;
                }

            }
        }

        //osal_printk("***fun:%s line:%d***\n",__func__,__LINE__);
        osal_down_interruptible(&ICM20690_dev->g_sem);


#if 0

        if (kthread_should_stop())
        {
            break;
        }

#endif

        s32Ret = FIFO_DATA_UPDATE();

        if (HI_SUCCESS != s32Ret)
        {
            print_info("ICM20690_UI_FIFO_DATA_UPDATE failed\n");
            continue;
            //return HI_FAILURE;
        }


        s32Ret = MOTIONSENSOR_GetData(attr);

        if (HI_SUCCESS != s32Ret)
        {
            print_info("MOTIONSENSOR_GetData failed! ret=%x\n", s32Ret);
            Reset_Fifo();
            continue;
            //return HI_FAILURE;
        }

        i_thread ++;

    }

    return HI_SUCCESS;
}


hi_void HI_MotionSensor_TimerStart(hi_void)
{
#ifndef __HuaweiLite__
    ktime_t stime;
    stime = ktime_set(0, ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval * NSEC_PER_USEC);
#else
    union ktime stime;

    stime.tv.sec = 0;
    stime.tv.usec = ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval;
#endif
    hrtimer_start(&ICM20690_dev->hrtimer, stime, HRTIMER_MODE_REL);
}
#ifdef TEST_DEBUG
volatile static hi_bool print_debug = HI_FALSE;
#endif
static void HI_MotionSensor_GetData_ATTR(void)
{
    hi_s32 s32Ret;

    if (ICM20690_dev->u8EnableKthread == HI_TRUE && su32DataCount < 1000)
    {

        /* get PTS*/
        if (!s8TimerFristFlag)
        {

            hi_u64 u64TimeNow = HI_MotionSensor_GetCurPts();
            ICM20690_dev->stGyroStatus.u64LastPts = u64TimeNow;
            ICM20690_dev->stAccStatus.u64LastPts = u64TimeNow;
            ICM20690_dev->stMagnStatus.u64LastPts = u64TimeNow;
            u64Time_last = u64TimeNow;
            s8TimerFristFlag = HI_TRUE;
            u64PTSNow = u64TimeNow;
			saTimeBackup[su8Timecnt] = u64PTSNow;
			su8Timecnt++;

            FIFO_DATA_RESET();
            return;
        }

        if (ICM20690_dev->u8FifoEn)
        {
            /*get fifo count*/
            s32Ret = ICM20690_GetFifoLengthAndCount();

            if (HI_SUCCESS != s32Ret)
            {
                print_info("get_ICM20690_fifo_length_and_count failed\n");
                return;
            }

        }

        u64PTSNow = HI_MotionSensor_GetCurPts();

        //i_hande ++;
        /*wakeup thread*/
        //if(ICM20690_dev->u8FifoEn)
        //{
        //    osal_up(&ICM20690_dev->g_sem);
        //}
    }

#ifdef TEST_DEBUG
    //test
    else if (!print_debug)
    {

        print_info("time = %llu , data_count = %d\n", u64PTSNow - u64Time_last, su32DataCount);

        for (i = 0; i < su32DataCount; i++)
        {
            /*print_info("stGyroData.temperature = %d, stGyroData.x = %d, stGyroData.y = %d, stGyroData.z = %d,pts:%lld\n",
                       test_fifo[i].stGyroData.temp, test_fifo[i].stGyroData.x, test_fifo[i].stGyroData.y,
                       test_fifo[i].stGyroData.z,test_fifo[i].stGyroData.pts);
            print_info("stAccData.temperature = %d, stAccData.x = %d, stAccData.y = %d, stAccData.z = %d, pts:%lld\n",
                       test_fifo[i].stAccData.temp, test_fifo[i].stAccData.x, test_fifo[i].stAccData.y,
                       test_fifo[i].stAccData.z,test_fifo[i].stAccData.pts);*/
            printk("%d, %d, %d, %d,%lld\n",
                   test_fifo[i].stGyroData.x, test_fifo[i].stGyroData.y,
                   test_fifo[i].stGyroData.z, test_fifo[i].stGyroData.temp, test_fifo[i].stGyroData.pts);
        }
        print_debug = HI_TRUE;

    }

#endif
}
hi_u64 su64testtime;
static hi_s32 HI_GetData_Handle(void);

static enum hrtimer_restart timer_hr_interrupt(struct hrtimer* timer)
{
    //hi_ulong  flags;
    hi_s32 s32Ret = HI_SUCCESS;

#ifndef __HuaweiLite__
    ktime_t stime;
    stime = ktime_set(0, ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval * NSEC_PER_USEC);
    hrtimer_forward_now(timer, stime);
#endif
    //HI_MotionSensor_GetData_ATTR();
    s32Ret = osal_schedule_work(&ICM20690_dev->work);
    if(s32Ret == HI_FALSE) {
        return HI_FAILURE;
    }

    ICM20690_dev->s32WorkqueueCallTimes++;

    return HRTIMER_RESTART;
}


static hi_s32 HI_MotionSensor_TimerInit(void)
{
#ifdef __HuaweiLite__
    hi_s32 s32Ret;
    union ktime time;

    time.tv.sec = 0;

    time.tv.usec = ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval;

    s32Ret = hrtimer_create(&ICM20690_dev->hrtimer, time, timer_hr_interrupt);

    if (HI_SUCCESS != s32Ret)
    {
        print_info("create tiemr failed!(%d)\n", s32Ret);
    }

#else
    hrtimer_init(&ICM20690_dev->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ICM20690_dev->hrtimer.function = timer_hr_interrupt;
#endif
    return HI_SUCCESS;
}

static hi_s32 HI_MotionSensor_TimerDeInit(void)
{
    hrtimer_cancel(&ICM20690_dev->hrtimer);
    return HI_SUCCESS;
}

static hi_s32 HI_GetData_Handle(void)
{
    //static hi_msensor_data stMotionSensorData;
    hi_s32 s32Ret;

    //print_info("start to get data!!\n");
	if(!ICM20690_dev->recordNum)
		return HI_SUCCESS;

    if ((stMSensorMode.temperature_mask & MSENSOR_TEMP_GYRO) ||
        (stMSensorMode.temperature_mask & MSENSOR_TEMP_ACC))
    {
        if (i_thread % 50 == 0)
        {
            s32Ret = ICM20690_GetTemperature(&ICM20690_dev->s32temperature);

            if (HI_SUCCESS != s32Ret)
            {
                print_info("get_temp failed! ret=%x\n", s32Ret);
                return s32Ret;
            }

        }
    }

    s32Ret = FIFO_DATA_UPDATE();

    if (HI_SUCCESS != s32Ret)
    {
        print_info("ICM20690_UI_FIFO_DATA_UPDATE failed\n");

        return HI_FAILURE;
    }


    s32Ret = MOTIONSENSOR_GetData(stMSensorMode);

    if (HI_SUCCESS != s32Ret)
    {
        print_info("MOTIONSENSOR_GetData failed! ret=%x\n", s32Ret);
        Reset_Fifo();

        return HI_FAILURE;
    }

    i_thread ++;

    return HI_SUCCESS;

}
hi_u64 lastime;

hi_s32 ICM20690_Data_TransferIMU(hi_msensor_data *pstMotiondata)
{
    hi_s32 s32Ret;
    hi_ulong  flags;
#ifndef __HuaweiLite__
    //ktime_t stime;
    //stime = ktime_set(0, ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval * NSEC_PER_USEC);
#else
    union ktime stime;

    stime.tv.sec = 0;
    stime.tv.usec = ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval;
#endif

    osal_spin_lock_irqsave(&lockGetData,&flags);
    HI_MotionSensor_GetData_ATTR();

    if (HI_FALSE == ICM20690_dev->u8FifoEn)
    {
        s32Ret = ICM20690_ReadData(&stIMUdata);

        if (s32Ret)
        {
            print_info("read Data failed: %d\n", s32Ret);
            return HI_FAILURE;
        }

    }
    else
    {
            s32Ret = HI_GetData_Handle();
            if (s32Ret)
            {
                print_info("read Data failed: %d\n", s32Ret);
                return HI_FAILURE;
            }


    }

    //osal_printk("~~~~~device_mask:%d u32GyroCount:%d~~~\n",stIMUdata.attr.device_mask,stIMUdata.gyro_buffer.data_num);

//       osal_printk("num:%d,lastpst:%lld,pts now:%lld\n",stIMUdata.gyro_buffer.data_num,
//       stIMUdata.gyro_buffer.gyro_data[stIMUdata.gyro_buffer.data_num -1].pts,u64PTSNow);

//#ifdef MNGBUFF_ENABLE
    osal_memcpy(pstMotiondata,&stIMUdata,sizeof(hi_msensor_data));

    MotionSensorDev_IntCallBack(&stIMUdata);

//#endif
    osal_memset(&stIMUdata, 0, sizeof(hi_msensor_data));
    osal_spin_unlock_irqrestore(&lockGetData, &flags);
#ifndef __HuaweiLite__
    //hrtimer_forward_now(&ICM20690_dev->hrtimer, stime);
#else
    hrtimer_forward(&ICM20690_dev->hrtimer, stime);
#endif


    return HI_SUCCESS;


}
static hi_s32 ICM20690_clear_irq(void)
{
    hi_u8 ch, u8Ret;
    /*read INT status*/
    u8Ret = HI_ICM20690_Transfer_read(INT_STATUS, &ch, 1, HI_FALSE);

    if (u8Ret)
    {
        print_info("read INT status failed\n");
        return HI_FAILURE;
    }

    /*read watermark status*/
    if (ICM20690_dev->u8FifoEn)
    {
        u8Ret = HI_ICM20690_Transfer_read(FIFO_WM_INT_STATUS, &ch, 1, HI_FALSE);

        if (u8Ret)
        {
            print_info("read INT status failed\n");
            return HI_FAILURE;
        }
    }

    //print_info("INT_STATUS:%d\n",ch);

    return HI_SUCCESS;
}



static void ICM20690_work(struct osal_work_struct* work)
{
    hi_s32 s32Ret = HI_SUCCESS;
    hi_ulong  flags;
    //static hi_msensor_data stIMUdata;
    //VI_PIPE ViPipe = 0;
    //hi_u64 nowtime = HI_MotionSensor_GetCurPts();
    //osal_printk("work time:%lld\n",nowtime - lastime);
    //lastime = nowtime;

    osal_spin_lock_irqsave(&lockGetData,&flags);
    //osal_printk("timer_hr_interrupt\n");
    if(ICM20690_dev->u8EnableKthread)
    {
        HI_MotionSensor_GetData_ATTR();
        if ((HI_FALSE == ICM20690_dev->u8FifoEn) && (su32DataCount < 1000))
        {

            //osal_down_interruptible(&ICM20690_dev->g_sem);
            s32Ret = ICM20690_ReadData(&stIMUdata);

            if (s32Ret)
            {
                print_info("ReadData failed: %d\n", s32Ret);
                goto EXIT;
            }

            //#ifdef MNGBUFF_ENABLE
            //Debug Only
            //MotionSensorDev_IntCallBack(&stIMUdata);
            //#endif
        }
        else
        {
            s32Ret = HI_GetData_Handle();

            if (s32Ret)
            {
                print_info("GetData_Handle failed: %d\n", s32Ret);
                goto EXIT;
            }
        }
    }

    if(stIMUdata.acc_buffer.data_num> 40 || stIMUdata.gyro_buffer.data_num > 40)
    {
        MotionSensorDev_IntCallBack(&stIMUdata);
        osal_memset(&stIMUdata, 0, sizeof(hi_msensor_data));
    }

EXIT:
    ICM20690_dev->s32WorkqueueCallTimes--;
    osal_wakeup(&ICM20690_dev->stWaitCallStopWorking);
    osal_spin_unlock_irqrestore(&lockGetData, &flags);

}

#ifndef __HuaweiLite__
static irqreturn_t ICM20690_IRQ(int irq, void* data)
#else
static void ICM20690_IRQ(hi_u32 irq, void* data)
#endif
{
    hi_s32 s32Ret = 0;
    //hi_u64 u64FramPTS = 0;
    hi_u64 u64CurrPTS = 0;
    static hi_u64 u64Last1s = 0;
    //static hi_u64 u64LastPTS = 0;
    static hi_u32 int_cnt = 0;

    //static hi_msensor_data stIMUdata;

    int_cnt++;
    u64CurrPTS = call_sys_get_time_stamp();
    s32Ret = ICM20690_clear_irq();

    if (s32Ret)
    {
        print_info("Clear irq status failed\n");
        return HI_FAILURE;
    }

    HI_MotionSensor_GetData_ATTR();
    //schedule_work(&ICM20690_dev->work);
    osal_schedule_work(&ICM20690_dev->work);

    if (!u64Last1s)
    { u64Last1s = u64CurrPTS; }

#if 0

    if (u64FramPTS)
    {
        hi_u32 u32Diff = u64CurrPTS - u64FramPTS;
        HI_TRACE_MSENSOR(HI_DBG_ERR, "u64FramPTS:%llu gyroPTS:%llu,diff:%d\n", u64FramPTS, u64CurrPTS, u32Diff);
    }

#endif

    if (u64Last1s && (u64CurrPTS - u64Last1s > 1000000))
    {
        //HI_TRACE_MSENSOR(HI_DBG_ERR, "1s int_cnt: %d\n", int_cnt);
        u64Last1s = u64CurrPTS;
        int_cnt = 0;
    }

    //if (u64LastPTS)
    //        HI_TRACE_MSENSOR(HI_DBG_ERR,"gyro diff:%d\n", (u64CurrPTS - u64LastPTS));
    //u64LastPTS = u64CurrPTS;

#ifndef __HuaweiLite__
    return IRQ_HANDLED;
#endif
}

hi_s32 HI_MotionSensor_INTERRUPTRun(void)
{
    hi_s32 s32Ret = 0;

	ICM20690_dev->u8EnableKthread = HI_TRUE;
    s32Ret = osal_init_work(&ICM20690_dev->work, ICM20690_work);

    //INIT_WORK(&ICM20690_dev->work, ICM20690_work);
#ifndef __HuaweiLite__
    ICM20690_dev->s32IRQNum = gpio_to_irq(gpio_num(INT_GPIO_CHIP, INT_GPIO_OFFSET));
    //print_info("IRQNum:%d\n",ICM20690_dev->s32IRQNum);

    s32Ret = request_threaded_irq(ICM20690_dev->s32IRQNum, NULL, ICM20690_IRQ, /*IRQF_TRIGGER_LOW*/IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "MotionSensor", ICM20690_dev->client);
#else
    group_bit_info.groupnumber   = ICM20690_dev->gd.group_num;
    group_bit_info.bitnumber     = ICM20690_dev->gd.bit_num;
    group_bit_info.direction     = GPIO_DIR_IN;
    group_bit_info.irq_type      = IRQ_TYPE_EDGE_FALLING;
    group_bit_info.irq_handler   = ICM20690_IRQ;
    group_bit_info.irq_enable    = GPIO_IRQ_ENABLE;
    group_bit_info.data          = &stMSensorMode;

    print_info("gpio:%d_%d\n", group_bit_info.groupnumber, group_bit_info.bitnumber);
    s32Ret =  gpio_direction_input(&group_bit_info);
    s32Ret |= gpio_irq_register(&group_bit_info);
    s32Ret |= gpio_set_irq_type(&group_bit_info);
    s32Ret |= gpio_irq_enable(&group_bit_info);
#endif

    if (s32Ret)
    {
        print_info("request irq failed: %d\n", s32Ret);
        return s32Ret;
    }



    return HI_SUCCESS;
}

hi_s32 HI_MotionSensor_INTERRUPTStop(void)
{
#ifndef __HuaweiLite__

    if (ICM20690_dev->s32IRQNum)
    {
        free_irq(ICM20690_dev->s32IRQNum, ICM20690_dev->client);
        ICM20690_dev->u8EnableKthread = HI_FALSE;
    }

#else

    if (group_bit_info.irq_enable)
    {
        group_bit_info.irq_enable = GPIO_IRQ_DISABLE;
        gpio_irq_enable(&group_bit_info);
        ICM20690_dev->u8EnableKthread = HI_FALSE;
    }

#endif
    else
    { print_info("irq has already free!!\n"); }

    osal_destroy_work(&ICM20690_dev->work);
    return HI_SUCCESS;
}

hi_s32 ICM20690_WaitStopWorkingCallBack(const hi_void* pParam)
{
    if (ICM20690_dev->s32WorkqueueCallTimes == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



hi_s32 HI_MotionSensor_TimerRun(void)
{
    hi_s32 s32Ret;
    ICM20690_dev->u8EnableKthread = HI_TRUE;
    s32Ret = HI_MotionSensor_TimerInit();

    if (HI_SUCCESS != s32Ret)
    {
        print_info("HI_MotionSensor_TimerInit failed\n");
        return HI_FAILURE;
    }
    //INIT_WORK(&ICM20690_dev->work, ICM20690_work);
    osal_init_work(&ICM20690_dev->work, ICM20690_work);
    su8Timecnt = 0;
    su8datacount = 0;
    s8TimerFristFlag = HI_FALSE;
	ICM20690_dev->recordNum = 0;

    HI_MotionSensor_TimerStart();

    return HI_SUCCESS;
}
hi_s32 HI_MotionSensor_TimerStop(void)
{
    hi_ulong  flags;
    hi_s32 s32Ret;

#ifdef TEST_DEBUG

    while (!print_debug && su32DataCount != 0)
    {
        msleep(5);
    }

#endif

    s32Ret = HI_MotionSensor_TimerDeInit();

    if (HI_SUCCESS != s32Ret)
    {
        print_info("HI_MotionSensor_TimerInit failed\n");
        return HI_FAILURE;
    }

    osal_wait_event_uninterruptible(&ICM20690_dev->stWaitCallStopWorking, ICM20690_WaitStopWorkingCallBack, HI_NULL);

    osal_spin_lock_irqsave(&lockGetData,&flags);
    ICM20690_dev->u8EnableKthread = HI_FALSE;
    osal_destroy_work(&ICM20690_dev->work);
    osal_spin_unlock_irqrestore(&lockGetData, &flags);
    //su8Timecnt = 0;
    //   su8datacount = 0;
    //   s8TimerFristFlag = HI_FALSE;
    //ICM20690_dev->recordNum = 0;
	osal_memset(saTimeBackup,0,sizeof(saTimeBackup));
	osal_memset(saEveryDataCntBackup,0,sizeof(saEveryDataCntBackup));
    return HI_SUCCESS;
}
static hi_void ICM20690_TrigerMode_init(hi_msensor_param* pstMSensorParam)
{
    ICM20690_dev->stTrigerConfig.eTrigerMode = TRIGER_TIMER; //TRIGER_TIMER;TRIGER_EXTERN_INTERRUPT;
    //ICM20690_dev->u8FifoEn = HI_FALSE;
    ICM20690_dev->u8FifoEn = HI_TRUE;

    if (TRIGER_TIMER == ICM20690_dev->stTrigerConfig.eTrigerMode)
    {
        if (pstMSensorParam->config.gyro_config.odr <= 50)
        {
            ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval = 1400000;
        }
        else if (pstMSensorParam->config.gyro_config.odr <= 200)
        {
            ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval = 350000;
        }
        else if (pstMSensorParam->config.gyro_config.odr <= 500)
        {
            ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval = 140000;
        }
        else if (pstMSensorParam->config.gyro_config.odr <= 1000)
        {
            ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval = 50000;//70000;
        }
        else if (pstMSensorParam->config.gyro_config.odr == 8000)
        {
            ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval = 8750;
        }
        else if (pstMSensorParam->config.gyro_config.odr == 32000)
        {
            ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval = 2188;
        }
        else
        {
            ICM20690_dev->stTrigerConfig.uTrigerInfo.stTimerConfig.u32interval = 70000;
        }
    }
    else if (TRIGER_EXTERN_INTERRUPT == ICM20690_dev->stTrigerConfig.eTrigerMode)
    {
        ICM20690_dev->stTrigerConfig.uTrigerInfo.stExternInterruptConfig.u32Interrupt_num = ICM20690_FIFO_MAX_RECORD;
    }
}

static hi_s32 ICM20690Dev_init(hi_msensor_param* stMSensorParam)
{
    hi_s32 s32Ret;

    osal_printk("######fun:%s line:%d MotionSensor_status:%p######\n", __func__, __LINE__, stMSensorParam);

    s32Ret = ICM20690_ConfigToParam(stMSensorParam);

    if (HI_SUCCESS != s32Ret)
    {
        print_info("ICM20690_ConfigToParam failed! ret=%x\n", s32Ret);
        goto ERR_INIT;
    }

    osal_printk("######fun:%s line:%d ######\n", __func__, __LINE__);

    s32Ret = ICM20690_ParamInit(*stMSensorParam);

    if (HI_SUCCESS != s32Ret)
    {
        print_info("ICM20690_ParamInit failed! ret=%x\n", s32Ret);
        goto ERR_INIT;
    }

    ICM20690_TrigerMode_init(stMSensorParam);
    s32Ret = ICM20690_SensorInit(stMSensorParam->attr.device_mask);

    if (HI_SUCCESS != s32Ret)
    {
        print_info("ICM20690_SensorInit failed! ret=%x\n", s32Ret);
        goto ERR_INIT;
    }

    //mutex_init(&ICM20690_dev->mutex);
    return HI_SUCCESS;

ERR_INIT:
#ifndef __HuaweiLite__
#ifdef TRANSFER_I2C
    MotionSersor_i2c_exit(&ICM20690_dev->client);
#elif defined TRANSFER_SPI
    MotionSersor_SPI_deinit(ICM20690_dev->hi_spi);
#endif
#endif

    return HI_FAILURE;

}

void ICM20690Dev_exit(hi_msensor_param* pstMSensorParam)
{
    /*Exit may need to be forced out when Deinit is not called*/
    ICM20690_SensorDeInit(pstMSensorParam->attr.device_mask);
}
hi_u8 HI_MotionSensor_DevID_Read(void)
{
    hi_s32 s32Ret;
    hi_u8 u8DevID = 0;

    s32Ret = HI_ICM20690_Transfer_read(0x75, &u8DevID, 1, HI_FALSE);

    if (s32Ret)
    {
        print_info("read dev  failed\n");
        return -EAGAIN;
    }

    osal_printk("func:%s,dev info :0x%x\n",__func__, u8DevID);
    return u8DevID;

}

hi_s32 HI_MotionSensor_DevInit(hi_msensor_param* pstMSensorParam)
{
    hi_s32 s32Ret;
    //hi_s32 s32Err;
    //hi_u32 cpu = 2;
    hi_u8 u8DevID = 0;

    u8DevID = HI_MotionSensor_DevID_Read();
    if(ICM20690_SELFID != u8DevID)
    {
        print_info("ICM20690 Device Abnormal!!\n");
        return -ENODEV;
    }
    s32Ret = ICM20690Dev_init(pstMSensorParam);


    if (HI_SUCCESS != s32Ret)
    {
        print_info("ICM20690_init failed\n");
        return -ENODEV;
    }

#if 0

    if (ICM20690_dev->u8FifoEn)
    {
#ifndef __HuaweiLite__
        //ICM20690_dev->read_data_task = kthread_run(HI_MotionSensor_DataHandle, (void*)&pstMSensorParam->attr,
        //    "MotionSensor_read_data_task");
        ICM20690_dev->read_data_task = kthread_create(HI_MotionSensor_DataHandle, (void*)&pstMSensorParam->attr,
                                       "MotionSensor_read_data_task");
#else
        hithread_state = THREAD_CTRL;
        ICM20690_dev->read_data_task = osal_kthread_create(HI_MotionSensor_DataHandle, (void*)&pstMSensorParam->attr,
                                       "motionsensor_thread");
#endif

        if (IS_ERR(ICM20690_dev->read_data_task))
        {
            print_info("Unable to start kernel thread.\n");
            s32Err = PTR_ERR(ICM20690_dev->read_data_task);
            ICM20690_dev->read_data_task = NULL;
            return s32Err;
        }


        //kthread_bind(ICM20690_dev->read_data_task, cpu);
        //wake_up_process(ICM20690_dev->read_data_task);
    }

#endif
    s8TimerFristFlag = HI_FALSE;
	su8Timecnt = 0;
    su8datacount = 0;
#ifdef TEST_DEBUG
    print_debug = HI_FALSE;
    su32DataCount = 0;
    osal_memset(test_fifo, 0, sizeof(test_fifo));
#endif
    return HI_SUCCESS;
}
#ifdef TRANSFER_I2C
static struct i2c_board_info hi_icm20690_info =
{
    I2C_BOARD_INFO("ICM20690", ICM20690_DEV_ADDR),
};
#endif


//hi_s32 HI_MotionSensor_SensorInit(MOTIONSENSOR_STATUS_S MotionSensor_status)
hi_s32 HI_MotionSensor_SensorInit(hi_void)
{
    hi_s32 s32Ret;

    /*1.malloc a ICM20690 dev*/
    ICM20690_dev = osal_kmalloc(sizeof(ICM20690_DEV_INFO), osal_gfp_kernel);

    if (!ICM20690_dev)
    {
        print_info("Could not allocate memory\n");
        return -ENOMEM;
    }

    osal_memset(ICM20690_dev, 0, sizeof(ICM20690_DEV_INFO));
    //memset(ICM20690_dev, 0, sizeof(ICM20690_DEV_INFO));
#ifndef __HuaweiLite__
#ifdef TRANSFER_I2C
    s32Ret = MotionSersor_i2c_init(&ICM20690_dev->client, hi_icm20690_info, I2C_DEV_NUM);

    if (s32Ret)
    {
        print_info("i2cdev_init failed\n");
        goto ERR_KZALLOC;
    }

#elif defined TRANSFER_SPI
    s32Ret = MotionSersor_SPI_init(&ICM20690_dev->hi_spi);

    if (s32Ret)
    {
        print_info("spidev_init failed\n");
        goto ERR_KZALLOC;
    }

#endif
    //osal_sema_init(&ICM20690_dev->g_sem, 1);
    gpio_init();
#else
    s32Ret = MotionSersor_SPI_init();

    if (s32Ret)
    {
        print_info("spidev_init failed\n");
        goto ERR_KZALLOC;
    }

    gpio_init(&ICM20690_dev->gd);
#endif

    //my_wq = create_workqueue("my_wq");
    //osal_mutex_init(&mutexGetData);
    osal_spin_lock_init(&lockGetData);
    osal_wait_init(&ICM20690_dev->stWaitCallStopWorking);

    return HI_SUCCESS;

ERR_KZALLOC:
    osal_kfree(ICM20690_dev);
    ICM20690_dev = NULL;

    return HI_FAILURE;
}


void HI_MotionSensor_DevDeInit(hi_msensor_param* MotionSensor_status)
{
    //osal_up(&ICM20690_dev->g_sem);
    //    if (ICM20690_dev->read_data_task)
    //    {
    //        //osal_up(&ICM20690_dev->g_sem);
    //        kthread_stop(ICM20690_dev->read_data_task);
    //        ICM20690_dev->read_data_task = NULL;
    //    }

    ICM20690Dev_exit(MotionSensor_status);
}


void HI_MotionSensor_SensorDeInit(hi_msensor_param* MotionSensor_status)
{
    //osal_sema_destory(&ICM20690_dev->g_sem);
    osal_wait_destory(&ICM20690_dev->stWaitCallStopWorking);
#ifndef __HuaweiLite__
#ifdef TRANSFER_I2C
    MotionSersor_i2c_exit(&ICM20690_dev->client);
#elif defined TRANSFER_SPI
    MotionSersor_SPI_deinit(ICM20690_dev->hi_spi);
#endif
    gpio_deinit();
#else
    gpio_deinit(&ICM20690_dev->gd);
    MotionSersor_SPI_deinit();
#endif
    //osal_mutex_destory(&mutexGetData);
    osal_spin_lock_destory(&lockGetData);

    //	if(my_wq)
    //	{
    //		flush_workqueue( my_wq );
    //		destroy_workqueue(my_wq);
    //	}

    if (ICM20690_dev != NULL)
    {
        kfree(ICM20690_dev);
        ICM20690_dev = NULL;
    }
}

