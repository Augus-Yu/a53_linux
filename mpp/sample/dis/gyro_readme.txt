1、Before using the gyro sensor,Modify file smp/a53_linux/drv/interdrv/sysconfig/sys_config.c some function
1.1 spi1_pin_mux
#if 1
static void spi1_pin_mux(void)
{
    SYS_WRITEL(reg_iocfg_base+0x0064, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x0068, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x006C, 0x000014f1);
    SYS_WRITEL(reg_iocfg_base+0x0070, 0x000004f1);
    //printk( "============spi1_pin_mux done=============\n");
}
#endif

1.2 gyro_pin_mode
#if 1
static void gyro_pin_mode(void)
{
    spi1_pin_mux();
    SYS_WRITEL(reg_iocfg1_base+0x0008, 0x000014F0);//gpio, for interrupt pin
}
#endif

1.3 pinmux
static int pinmux(void)
{
    sensor_pin_mux();

    //i2c
    i2c1_pin_mux();
    i2c2_pin_mux();
    //i2c3_pin_mux();
    //i2c4_pin_mux();
    i2c5_pin_mux();
    i2c6_pin_mux();

    //spi
    //spi1_pin_mux();
    //spi2_pin_mux();
    //spi4_pin_mux();

    //icm20690
    gyro_pin_mode();

    //sil9022/adv7179
    i2c0_pin_mux();
	......

Rebuild and get the sys_config.ko

2、update the file: smp/a53_linux/mpp/ko/load3519av100
2.1  insert_gyro
2.2  rmmod_gyro

3、modify the makefile parameter: smp/a53_linux/mpp/sample/Makefile.param. 
    ################ open GYRO_DIS sample ########################
	GYRO_DIS ?= y

Rebuild the sample and get the sample_dis.
