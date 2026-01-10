#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "motionsensor.h"
#include "motionsensor_proc.h"

#define  ICN20690_INFO "mpu_info"

#define  MPU_VERSION_INFO "MPU debug 0.0.0.1"
//MPU_PROC_INFO_S g_stMpuProcInfo;

static hi_s32 MPU_PROC_Open(struct inode *inode, struct file *filp);
static hi_s32 MPU_PROC_Show(struct seq_file *s, void *v);
static hi_s32 MPU_PROC_Release(struct inode *inode, struct file *filp);

static const struct file_operations mpu_proc_fops = {
    .owner   = THIS_MODULE,
    .open    = MPU_PROC_Open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    //.release = seq_release,
    .release = MPU_PROC_Release,
};

static hi_s32 MPU_PROC_Open(struct inode *inode, struct file *filp)
{
    hi_s32 s32Ret = 0;

    s32Ret = single_open(filp, MPU_PROC_Show, NULL);
	//seq_open
    return s32Ret;
}

static hi_s32 MPU_PROC_Release(struct inode *inode, struct file *filp)
{
	return single_release(inode, filp);
}



static hi_s32 MPU_PROC_Show(struct seq_file *s, void *v)
{

    hi_s32 s32Ret;

	if(MotionSensorStatus->attr.device_mask & MSENSOR_DEVICE_GYRO)
	{
		seq_printf(s,"------gyro parameter------\n");
		seq_printf(s,"%24s\n","##ICM20690##");
		seq_printf(s,"%24s %24s %24s %24s %24s\n","SampleRate","Full-scale-range", "Datawidth", "Max-Chip-Temperature", "Min-Chip-Temperature");
		seq_printf(s,"%24d %24d %24d %24d %24d\n", MotionSensorStatus->config.gyro_config.odr,
			MotionSensorStatus->config.gyro_config.fsr, MotionSensorStatus->config.gyro_config.data_width,
			MotionSensorStatus->config.gyro_config.temp_max, MotionSensorStatus->config.gyro_config.temp_min);
	}
	if(MotionSensorStatus->attr.device_mask & MSENSOR_DEVICE_ACC)
	{
		seq_printf(s,"------accelerometer parameter------\n");
		seq_printf(s,"%24s\n","##ICM20690##");
		seq_printf(s,"%24s %24s %24s %24s %24s\n","SampleRate","Full-scale-range", "Datawidth", "Max-Chip-Temperature", "Min-Chip-Temperature");
		seq_printf(s,"%24d %24d %24d %24d %24d\n", MotionSensorStatus->config.acc_config.odr,
			MotionSensorStatus->config.acc_config.fsr, MotionSensorStatus->config.acc_config.data_width,
			MotionSensorStatus->config.acc_config.temp_max, MotionSensorStatus->config.acc_config.temp_min);
	}
    return 0;
}


hi_s32 MPU_PROC_Init(void)
{
    struct proc_dir_entry *pentry;
    //memset(&g_stMpuProcInfo, 0, sizeof(g_stMpuProcInfo));

    pentry = proc_create(ICN20690_INFO, 0444, NULL, &mpu_proc_fops);
    if(pentry == NULL)
    {
        return -ENOMEM;
    }

    return 0;
}

void  MPU_PROC_Exit(void)
{
     remove_proc_entry(ICN20690_INFO, 0);
}



