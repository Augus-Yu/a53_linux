#ifndef __SPI_DEV_H__
#define __SPI_DEV_H__

#include "hi_type.h"
#include "hi_osal.h"

#ifdef __HuaweiLite__
#include <spi.h>
#include "fcntl.h"
#else
#include <linux/spi/spi.h>
#endif

#ifndef __HuaweiLite__
hi_s32 MotionSersor_SPI_write(struct spi_device *hi_spi, hi_u8 addr, hi_u8 *data, hi_u32 u32cnt);
hi_s32 MotionSersor_SPI_read(struct spi_device *hi_spi, hi_u8 addr, hi_u8 *data, hi_u32 u32cnt);
hi_s32 MotionSersor_SPI_init(struct spi_device **hi_spi);
hi_s32 MotionSersor_SPI_deinit(struct spi_device *spi_device);
#else
hi_s32 MotionSersor_SPI_write(hi_u8 addr, hi_u8 *data, hi_u32 u32cnt, hi_u32 u32SpiNum);
hi_s32 MotionSersor_SPI_read(hi_u8 addr, hi_u8 *data, hi_u32 u32cnt, hi_u32 u32SpiNum);
hi_s32 MotionSersor_SPI_init(hi_void);
hi_s32 MotionSersor_SPI_deinit(hi_void);
#endif

#endif

