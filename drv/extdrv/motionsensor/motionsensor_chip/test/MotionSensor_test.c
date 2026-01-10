#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h> 
#include <string.h>
#include <unistd.h>
#include <linux/spi/spidev.h>

#include "motionsensor_chip_cmd.h"
#include "hi_comm_motionsensor.h"


hi_s32 HI_MOTIONSENSOR_GET_PARAM(int fd)
{
	hi_u32 u32Ret;
	hi_msensor_param stMSensorParamGet;

	u32Ret =  ioctl(fd, MSENSOR_CMD_GET_PARAM, &stMSensorParamGet);
	if(u32Ret)
	{
		perror("IOCTL_CMD_SET_MODE");
		return -1;
	}

	if(stMSensorParamGet.attr.device_mask & MSENSOR_DEVICE_ACC)
	{
		printf("------gyro parameter------\n");
		printf("%24s\n","##ICM20690##");
		printf("%24s %24s %24s %24s %24s\n","SampleRate","Full-scale-range", "Datawidth", "Max-Chip-Temperature", "Min-Chip-Temperature");
		printf("%24lld %24lld %24d %24d %24d\n", stMSensorParamGet.config.gyro_config.odr,
			stMSensorParamGet.config.gyro_config.fsr, stMSensorParamGet.config.gyro_config.data_width,
			stMSensorParamGet.config.gyro_config.temp_max, stMSensorParamGet.config.gyro_config.temp_min);
	}
	if(stMSensorParamGet.attr.device_mask & MSENSOR_DEVICE_ACC)
	{
		printf("------accelerometer parameter------\n");
		printf("%24s\n","##ICM20690##");
		printf("%24s %24s %24s %24s %24s\n","SampleRate","Full-scale-range", "Datawidth", "Max-Chip-Temperature", "Min-Chip-Temperature");
		printf("%24lld %24lld %24d %24d %24d\n", stMSensorParamGet.config.acc_config.odr,
			stMSensorParamGet.config.acc_config.fsr, stMSensorParamGet.config.acc_config.data_width,
			stMSensorParamGet.config.acc_config.temp_max, stMSensorParamGet.config.acc_config.temp_min);
	}

	return u32Ret;
	
}

int SPI_Param_init(void)
{
	hi_s32  fd, s32Ret;
	hi_s32 s32mode = SPI_MODE_3/* | SPI_LSB_FIRST | SPI_LOOP | SPI_CS_HIGH*/;
	hi_s32 s32bits = 8;
	hi_u64 u64speed = 10000000;
	//hi_bool endianmode = 0;

	
	fd = open("/dev/spidev1.0", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		return HI_FAILURE;
	}
	/*set spi mode */
	s32Ret = ioctl(fd, SPI_IOC_WR_MODE32, &s32mode);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't set spi mode");
		close(fd);
		return HI_FAILURE;
	}

	s32Ret = ioctl(fd, SPI_IOC_RD_MODE32, &s32mode);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't get spi mode");
		close(fd);
		return HI_FAILURE;
		
	}

	/*
	 * bits per word
	 */
	s32Ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &s32bits);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't set bits per word");
		close(fd);
		return HI_FAILURE;
	}

	s32Ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &s32bits);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't get bits per word");
		close(fd);
		return HI_FAILURE;
	}

#if 0
	/*set spi endian mode*/
	s32Ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &endianmode);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't set bits max speed HZ");
		close(fd);
		return HI_FAILURE;
	}
	
	s32Ret = ioctl(fd, SPI_IOC_RD_LSB_FIRST, &endianmode);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't set bits max speed HZ");
		close(fd);
		return HI_FAILURE;
	}
#endif
	

	/*set spi max speed*/
	s32Ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &u64speed);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't set bits max speed HZ");
		close(fd);
		return HI_FAILURE;
	}
	
	s32Ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &u64speed);
	if (s32Ret == HI_FAILURE)
	{
		perror("can't set bits max speed HZ");
		close(fd);
		return HI_FAILURE;
	}
	//printf("spi mode: 0x%x\n", s32mode);
	//printf("bits per word: %d\n", s32bits);
	//printf("max speed: %lld Hz (%lld KHz)\n", u64speed, u64speed/1000);

	close(fd);
	return 0;
}


int main(void)
{
	hi_s32 fd, s32Ret;
	
	SPI_Param_init();
	
	fd = open("/dev/MotionSensor", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		return -1;
	}
#if 1

    hi_msensor_param stMSensorParamSet;
	//set device work mode
	stMSensorParamSet.attr.device_mask = MSENSOR_DEVICE_GYRO | MSENSOR_DEVICE_ACC;
	//stMSensorParamSet.attr.device_mask = MSENSOR_DEVICE_GYRO;
	stMSensorParamSet.attr.temperature_mask = MSENSOR_TEMP_GYRO | MSENSOR_TEMP_ACC;
	//set gyro samplerate and full scale range
    stMSensorParamSet.config.gyro_config.odr = 1000 * GRADIENT ;
	stMSensorParamSet.config.gyro_config.fsr = 250 * GRADIENT;
	//set accel samplerate and full scale range
	stMSensorParamSet.config.acc_config.odr = 1000 * GRADIENT ;
	stMSensorParamSet.config.acc_config.fsr = 4 * GRADIENT;

	s32Ret =  ioctl(fd, MSENSOR_CMD_INIT, &stMSensorParamSet);
	if(s32Ret)
	{
		perror("MSENSOR_CMD_INIT");
		return -1;
	}
//#if 0
	s32Ret =  ioctl(fd, MSENSOR_CMD_START, NULL);
	if(s32Ret)
	{
		perror("MSENSOR_CMD_START");
		return -1;
	}
	//sleep(2);
	getchar();	

	s32Ret =  ioctl(fd, MSENSOR_CMD_STOP, NULL);
	if(s32Ret)
	{
		perror("MSENSOR_CMD_STOP");
		return -1;
	}
	//#endif
	//HI_MOTIONSENSOR_GET_PARAM(fd);
	//sleep(2);
	s32Ret =  ioctl(fd, MSENSOR_CMD_DEINIT, NULL);
	
	if(s32Ret)
	{
		perror("MSENSOR_CMD_DEINIT");
		return -1;
	}
#endif
//	while(1);
	close(fd);
	return 0;
}
