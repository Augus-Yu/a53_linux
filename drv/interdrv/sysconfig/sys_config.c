#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/version.h>
#include <linux/of_platform.h>
#include <asm/io.h>

#define SENSOR_LIST_CMDLINE_LEN     256
#define SENSOR_NAME_LEN             64

#define SYS_WRITEL(Addr, Value) ((*(volatile unsigned int *)(Addr)) = (Value))
#define SYS_READ(Addr)          (*((volatile int *)(Addr)))

static void  *reg_crg_base=0;
static void  *reg_sys_base=0;
static void  *reg_ddr_base=0;
static void  *reg_misc_base=0;
static void  *reg_iocfg_base=0;
static void  *reg_iocfg1_base=0;
static void  *reg_iocfg2_base=0;
static void  *reg_iocfg3_base=0;
static void  *reg_gpio_base=0;

static int g_bt1120_flag = 0;
static char sensor_list[SENSOR_LIST_CMDLINE_LEN]= "sns0=ov9734,sns1=ov6946,sns2=imx334,sns3=imx334,sns4=imx334";

module_param(g_bt1120_flag, int, S_IRUGO);

module_param_string(sensors, sensor_list, SENSOR_LIST_CMDLINE_LEN, 0600);

MODULE_PARM_DESC(sensors,"sns0=ov9734,sns1=ov6946,sns2=imx334,sns3=imx334,sns4=imx334");

typedef enum {
    BUS_TYPE_I2C = 0,
    BUS_TYPE_SPI = 1,
} BUS_TYPE;

static int parse_sensor_index(char *s)
{
    char tmp[8] = {0};
    int i;
    char* line = NULL;
    int index = -1;

    line = strsep(&s, "=");

    if (NULL == line)
    {
        printk("FUNC:%s line:%d  err sensor index: [%s] \n", __FUNCTION__, __LINE__, s);
        return index;
    }

    for (i = 0; i < 5; i++)
    {
        snprintf(tmp, sizeof(tmp), "sns%d", i);

        if (0 == strncmp(tmp, line, 8))
        {
            index = i;
            return index;
        }
    }

    printk("FUNC:%s line:%d  SNS prefix:[%s] is not supported !\n", __FUNCTION__,__LINE__, line);

    return index;
}

static int parse_sensor_name(char *s, char *name)
{
    unsigned int len;
    char* line = NULL;

    line = strsep(&s, "=");
    if(NULL == line)
    {
        return -1;
    }

    line=strsep(&s, "=");
    if(NULL != line)
    {
        len = strlen(line);

        if(len >= SENSOR_NAME_LEN)
        {
            printk("FUNC:%s line:%d  name:[%s] is too long, can not longer than %d\n", __FUNCTION__,__LINE__,  line, SENSOR_NAME_LEN);

            return -1;
        }   

        strncpy(name, line, SENSOR_NAME_LEN - 1);

        return 0;
    }

    return -1;
}

static inline void reg_write32(unsigned long value, unsigned long mask, unsigned long addr)
{
    unsigned long t;

    t = SYS_READ((const volatile void *)addr);
    t &= ~mask;
    t |= value & mask;
    SYS_WRITEL((volatile void *)addr, t);
}

static void sensor_clock_config(int index, unsigned int clock)
{
    /* Notice: sensor0 use clock0, sensor1 and sensor2 share clock1, sensor3 and sensor4 share clock2 */
    int clock_index = (index + 1) / 2;

    reg_write32(clock << (8 * clock_index), 0xF << (8 * clock_index), (unsigned long)reg_crg_base+0x0114);
}

#if 0
static BUS_TYPE parse_sensor_bus_type(char *name)
{
    unsigned int len;
    BUS_TYPE bus_type = BUS_TYPE_I2C;

    len = SENSOR_NAME_LEN;

    if (   (0 == strncmp("imx377", name, len))
        || (0 == strncmp("imx334", name, len))
        || (0 == strncmp("imx477", name, len))
        || (0 == strncmp("imx290", name, len))
        || (0 == strncmp("imx290_slave", name, len)))
    {
        bus_type = BUS_TYPE_I2C;
    }
    else if ((0 == strncmp("imx277_slvs", name, len))
          || (0 == strncmp("imx226", name, len)))
    {
        bus_type = BUS_TYPE_SPI;
    }
    else
    {
        printk("FUNC:%s line:%d  SNS:[%s] is not supported !\n", __FUNCTION__,__LINE__, name);
        bus_type = BUS_TYPE_I2C;
    }

    return bus_type;
}
#endif

#if 0
static void sensor_fpga_clock_config(char *name)
{
    void *fpga_sensor_clk_reg = (void*)ioremap(0x04A50000, 0x8000);
    void *fpga2_reg = (void*)ioremap(0x047F0000, 0x4);
    unsigned int len = SENSOR_NAME_LEN;

    if (NULL == fpga_sensor_clk_reg)
    {
        printk("fpga_sensor_clk_reg remap fail. \n");
        return;
    }

    if ( (0 == strncmp("imx377", name, len))
        || (0 == strncmp("imx334", name, len))
        || (0 == strncmp("imx477", name, len)))
    {
    	SYS_WRITEL(fpga_sensor_clk_reg+0x1000, 0x0);
    	SYS_WRITEL(fpga_sensor_clk_reg+0x3000, 0x0);
    }
    else if (0 == strncmp("imx290", name, len))
    {
        SYS_WRITEL(fpga_sensor_clk_reg+0x1000, 0x0);
        SYS_WRITEL(fpga_sensor_clk_reg+0x3000, 0x300);
    }
    else if (0 == strncmp("imx226", name, len))
    {
        SYS_WRITEL(fpga_sensor_clk_reg+0x1000, 0x1);
        SYS_WRITEL(fpga_sensor_clk_reg+0x3000, 0x400);
        SYS_WRITEL(fpga2_reg+0x0, 0x1);
    }
    else
    {
        printk("sensor not support.\n");
    }

    iounmap(fpga2_reg);
    iounmap(fpga_sensor_clk_reg);
    return;
}
#endif


/*
0x0: 74.25MHz; 0x1: 72MHz;0x2: 54MHz;0x3: 50MHz;0x4: 24MHz;0x6: 32.4MHz;
0x8: 37.125MHz;0x9: 36MHz;0xA: 27MHz;0xB: 25MHz;0xC: 12MHz;
*/
static unsigned int parse_sensor_clock(char *name)
{
    unsigned int clock = 0x0;
    unsigned int len;

    len = SENSOR_NAME_LEN;

    if ( (0 == strncmp("imx377", name, len))
         || (0 == strncmp("imx334", name, len))
         || (0 == strncmp("imx477", name, len))
         || (0 == strncmp("ov9734", name, len))
         || (0 == strncmp("ov6946", name, len))
        )
    {
        clock = 0x4;
    }
    else if (0 == strncmp("imx290", name, len))
    {
        clock = 0x8;
    }
    else if (0 == strncmp("imx290_slave", name, len))
    {
        clock = 0x0;
    }
    else if ((0 == strncmp("imx277_slvs", name, len))
              || (0 == strncmp("imx226", name, len)))
    {
        clock = 0x1;
    }
    else
    {
        printk("FUNC:%s line:%d  SNS:[%s] is not supported !\n", __FUNCTION__, __LINE__, name);
        return clock;
    }

    return clock;
}

static int is_coms(char *name)
{
    unsigned int len;

    len = SENSOR_NAME_LEN;

    if (   (0 == strncmp("bt1120", name, len))
        || (0 == strncmp("bt656", name, len))
        || (0 == strncmp("bt601", name, len))
        || (0 == strncmp("ov6946", name, len)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void coms_clock_config(int index)
{
    if (1 == index)
    {
        reg_write32(0x5 << 9, 0x7 << 9, (unsigned long)reg_crg_base+0x104);
    }
    else if (0 == index)
    {
        reg_write32(0x5 << 12, 0x7 << 12, (unsigned long)reg_crg_base+0x104);
    }
}

static int clkcfg(void)
{
    SYS_WRITEL(reg_crg_base+0x00dc, 0x06080008);

    SYS_WRITEL(reg_crg_base+0x00EC, 0x00007fff);

    SYS_WRITEL(reg_crg_base+0x0104, 0x000036db);

    SYS_WRITEL(reg_crg_base+0x0108, 0x000036db);

    SYS_WRITEL(reg_crg_base+0x010C, 0x0001b000);

	SYS_WRITEL(reg_crg_base+0x011C, 0x00002011);
	SYS_WRITEL(reg_crg_base+0x0120, 0x2815e4c3);
	SYS_WRITEL(reg_crg_base+0x0140, 0x2);
	SYS_WRITEL(reg_crg_base+0x0150, 0x2);

    /*VEDU clk	000:568MHz, 010:750MHz*/
    SYS_WRITEL(reg_crg_base + 0x0164, 0x00000000);
	SYS_WRITEL(reg_crg_base+0x0168, 0x00000010);
	SYS_WRITEL(reg_crg_base+0x016c, 0x00000362);
    SYS_WRITEL(reg_crg_base+0x0194, 0x009aaa80);
    SYS_WRITEL(reg_crg_base+0x0198, 0x68ff0000);
    SYS_WRITEL(reg_crg_base+0x01a0, 0x33ff0000);

    return 0;
}

void sys_ctl(int online_flag)
{
    if (3 == online_flag) /*vi_online_vpss_online*/
    {
                                                         //#each module 3bit:          30:28           26:24           22:20          18:16        14:12         10:8         6:4         2:0
        SYS_WRITEL( reg_misc_base+0x2000, 0x66476444); //#each module 3bit:           vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x2004, 0x34344445); //#each module 3bit:           jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2008, 0x55130033); //#each module 3bit:           cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x200c, 0x30333333); //#each module 3bit:           avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2010, 0x00000333); //#each module 3bit:           ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x2014, 0x00000000); //#each module 3bit:           emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
        SYS_WRITEL( reg_misc_base+0x2018, 0x66476444); //#each module 3bit:           vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x201c, 0x34344445); //#each module 3bit:           jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2020, 0x55130033); //#each module 3bit:           cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x2024, 0x30333375); //#each module 3bit:           avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2028, 0x00000333); //#each module 3bit:           ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x202c, 0x00000000); //#each module 3bit:           emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
    }
    else if (2 == online_flag) /*vi_online_vpss_offline*/
    {
                                                         //#each module 3bit:          30:28           26:24           22:20          18:16        14:12         10:8         6:4         2:0
        SYS_WRITEL( reg_misc_base+0x2000, 0x66474444); //#each module 3bit:           vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x2004, 0x34344445); //#each module 3bit:           jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2008, 0x55130033); //#each module 3bit:           cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x200c, 0x30333333); //#each module 3bit:           avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2010, 0x00000333); //#each module 3bit:           ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x2014, 0x00000000); //#each module 3bit:           emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
        SYS_WRITEL( reg_misc_base+0x2018, 0x66474444); //#each module 3bit:           vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x201c, 0x34344445); //#each module 3bit:           jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2020, 0x55130033); //#each module 3bit:           cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x2024, 0x30333375); //#each module 3bit:           avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2028, 0x00000333); //#each module 3bit:           ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x202c, 0x00000000); //#each module 3bit:           emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
    }
    else if (1 == online_flag)  /* vi_offline_vpss_online */
    {
                                                         //#each module 3bit:          30:28           26:24           22:20          18:16        14:12         10:8         6:4         2:0
        SYS_WRITEL( reg_misc_base+0x2000, 0x66476444);    //#each module 3bit:        vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x2004, 0x34344445);    //#each module 3bit:        jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2008, 0x55130033);    //#each module 3bit:        cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x200c, 0x30333333);    //#each module 3bit:        avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2010, 0x00000333);    //#each module 3bit:        ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x2014, 0x00000000);    //#each module 3bit:        emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
        SYS_WRITEL( reg_misc_base+0x2018, 0x66476444);    //#each module 3bit:        vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x201c, 0x34344445);    //#each module 3bit:        jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2020, 0x55130033);    //#each module 3bit:        cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x2024, 0x30333375);    //#each module 3bit:        avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2028, 0x00000333);    //#each module 3bit:        ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x202c, 0x00000000);    //#each module 3bit:        emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
    }
    else /* vi_offline_vpss_offline */
    {
                                                         //#each module 3bit:          30:28           26:24           22:20          18:16        14:12         10:8         6:4         2:0
        SYS_WRITEL( reg_misc_base+0x2000, 0x64474444);    //#each module 3bit:        vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x2004, 0x34344445);    //#each module 3bit:        jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2008, 0x55130033);    //#each module 3bit:        cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x200c, 0x30333333);    //#each module 3bit:        avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2010, 0x00000333);    //#each module 3bit:        ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x2014, 0x00000000);    //#each module 3bit:        emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
        SYS_WRITEL( reg_misc_base+0x2018, 0x64474444);    //#each module 3bit:        vicap_wqos   viproc_wqos      aiao_wqos      vdp_wqos     vpss_wqos     vedu_wqos    scd_wqos     vgs_wqos
        SYS_WRITEL( reg_misc_base+0x201c, 0x34344445);    //#each module 3bit:        jpgd_wqos    jpge_wqos        tde_wqos       gdc_wqos     gme_wqos      dpu_wqos     vdma_wqos    a53up0_wqos
        SYS_WRITEL( reg_misc_base+0x2020, 0x55130033);    //#each module 3bit:        cci_wqos     a53up1_wqos      coresight_wqos dsp_m0_wqos  ddrt0_wqos    gzip_wqos    avsp_m0_wqos avsp_m1_wqos
        SYS_WRITEL( reg_misc_base+0x2024, 0x30333375);    //#each module 3bit:        avsp_m2_wqos fd_wqos          ive_wqos       wukong_wqos  dsp_m1_wqos   pcie_wqos    usb3_wqos    usb2_wqos
        SYS_WRITEL( reg_misc_base+0x2028, 0x00000333);    //#each module 3bit:        ddrt1_wqos   fmc_wqos         spacc_wqos     edma0_wqos   edma1_wqos    sdio0_wqos   sdio1_wqos   gsf_wqos
        SYS_WRITEL( reg_misc_base+0x202c, 0x00000000);    //#each module 3bit:        emmc_wqos    reserved         reserved       reserved     reserved      reserved     reserved     reserved
    }
}

EXPORT_SYMBOL(sys_ctl);

static void sensor_pin_mux(void)
{
    /* CLK0 CLK1 CLK2 */
    SYS_WRITEL(reg_iocfg1_base+0x0018 ,0x000004f1);
    SYS_WRITEL(reg_iocfg1_base+0x0020 ,0x000004f4);
    SYS_WRITEL(reg_iocfg1_base+0x0028 ,0x000004f5);

    /* RST0 RST1 RST2 */
    SYS_WRITEL(reg_iocfg1_base+0x001C ,0x000004f1);
    SYS_WRITEL(reg_iocfg1_base+0x0024 ,0x000004f4);
    SYS_WRITEL(reg_iocfg1_base+0x002C ,0x000004f5);

    /* HS VS */
    SYS_WRITEL(reg_iocfg1_base+0x0000 ,0x000004f1);
    SYS_WRITEL(reg_iocfg1_base+0x0004 ,0x000004f1);

    SYS_WRITEL(reg_iocfg1_base+0x0008 ,0x000004f4);
    SYS_WRITEL(reg_iocfg1_base+0x000C ,0x000004f4);

    SYS_WRITEL(reg_iocfg1_base+0x0010 ,0x000004f5);
    SYS_WRITEL(reg_iocfg1_base+0x0014 ,0x000004f5);

    //printk( "============vicap_pin_mux done=============\n");
}

//#i2c0 -> sil9022/adv7179
static void i2c0_pin_mux(void)
{
    //#SCL SDA
    SYS_WRITEL(reg_iocfg_base+0x0020 ,0x000014f0);
    SYS_WRITEL(reg_iocfg_base+0x0024 ,0x000014f0);

    //printk( "============i2c0_pin_mux done=============\n");
}

//#i2c1 -> sensor
static void i2c1_pin_mux(void)
{
    //#SCL SDA
    SYS_WRITEL(reg_iocfg_base+0x0054 ,0x000014f3);
    SYS_WRITEL(reg_iocfg_base+0x0058 ,0x000014f3);

    //printk( "============i2c1_pin_mux done=============\n");
}


//#i2c2 -> sensor
static void i2c2_pin_mux(void)
{
    //#SCL SDA
    SYS_WRITEL(reg_iocfg_base+0x005C ,0x000014f3);
    SYS_WRITEL(reg_iocfg_base+0x0060 ,0x000014f3);

    //printk( "============i2c2_pin_mux done=============\n");
}

//#i2c3 -> sensor
static void i2c3_pin_mux(void)
{
    //#SCL SDA
    SYS_WRITEL(reg_iocfg_base+0x0064 ,0x000014f3);
    SYS_WRITEL(reg_iocfg_base+0x0068 ,0x000014f3);

    //printk( "============i2c3_pin_mux done=============\n");
}


//#i2c4 -> sensor
static void i2c4_pin_mux(void)
{
    //#SCL SDA
    SYS_WRITEL(reg_iocfg_base+0x006C ,0x000014f3);
    SYS_WRITEL(reg_iocfg_base+0x0070 ,0x000014f3);

    //printk( "============i2c4_pin_mux done=============\n");
}


//#i2c5 -> sensor
static void i2c5_pin_mux(void)
{
    //#SCL SDA
    SYS_WRITEL(reg_iocfg_base+0x00c4 ,0x000014f3);
    SYS_WRITEL(reg_iocfg_base+0x00C8 ,0x000014f3);

    //printk( "============i2c5_pin_mux done=============\n");
}

//#i2c6 -> sensor
static void i2c6_pin_mux(void)
{
    //#SCL SDA
    SYS_WRITEL(reg_iocfg_base+0x00CC ,0x000014f3);
    SYS_WRITEL(reg_iocfg_base+0x00D0 ,0x000014f3);
    //printk( "============i2c6_pin_mux done=============\n");
}

#if 0
static void spi0_pin_mux(void)
{
    SYS_WRITEL(reg_iocfg_base+0x0054, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x0058, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x005C, 0x000014f1);
    SYS_WRITEL(reg_iocfg_base+0x0060, 0x000004f1);
    //printk( "============spi0_pin_mux done=============\n");
}
#endif

#if 0
static void spi1_pin_mux(void)
{
    SYS_WRITEL(reg_iocfg_base+0x0064, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x0068, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x006C, 0x000014f1);
    SYS_WRITEL(reg_iocfg_base+0x0070, 0x000004f1);
    //printk( "============spi1_pin_mux done=============\n");
}
#endif

#if 0
static void spi2_pin_mux(void)
{
    SYS_WRITEL(reg_iocfg_base+0x00C4, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x00C8, 0x000004f1);
    SYS_WRITEL(reg_iocfg_base+0x00CC, 0x000014f1);
    SYS_WRITEL(reg_iocfg_base+0x00D0, 0x000004f1);
    //printk( "============spi2_pin_mux done=============\n");
}

static void spi3_pin_mux(void)
{
    SYS_WRITEL(reg_iocfg2_base+0x0038, 0x000004f3);
    SYS_WRITEL(reg_iocfg2_base+0x0044, 0x000004f3);
    SYS_WRITEL(reg_iocfg2_base+0x005C, 0x000014f3);
    SYS_WRITEL(reg_iocfg2_base+0x0060, 0x000004f3);
    //printk( "============spi3_pin_mux done=============\n");
}

static void spi4_pin_mux(void)
{
    SYS_WRITEL(reg_iocfg_base+0x007C, 0x000004f2);
    SYS_WRITEL(reg_iocfg_base+0x0080, 0x000004f2);
    SYS_WRITEL(reg_iocfg_base+0x0084, 0x000014f2);
    SYS_WRITEL(reg_iocfg_base+0x0088, 0x000004f2);
    //printk( "============spi4_pin_mux done=============\n");
}
#endif

#if 0

static void vo_bt656_mode(void)
{
    SYS_WRITEL(reg_iocfg2_base+0x20, 0x000000f5); //1A19 VOU656_CLK

    SYS_WRITEL(reg_iocfg2_base+0x00, 0x00000455); // VOU656_DATA0
    SYS_WRITEL(reg_iocfg2_base+0x04, 0x00000455); // VOU656_DATA1
    SYS_WRITEL(reg_iocfg2_base+0x08, 0x00000455); // VOU656_DATA2
    SYS_WRITEL(reg_iocfg2_base+0x0c, 0x00000455); // VOU656_DATA3
    SYS_WRITEL(reg_iocfg2_base+0x10, 0x00000455); // VOU656_DATA4
    SYS_WRITEL(reg_iocfg2_base+0x14, 0x00000455); // VOU656_DATA5
    SYS_WRITEL(reg_iocfg2_base+0x18, 0x00000455); // VOU656_DATA6
    SYS_WRITEL(reg_iocfg2_base+0x1c, 0x00000455); // VOU656_DATA7

}


static void vo_bt1120_mode(int bt1120_mode)
{
    SYS_WRITEL(reg_iocfg2_base+0x20, 0x000000f4); //1A19 VOU1120_CLK

    SYS_WRITEL(reg_iocfg2_base+0x1c, 0x00000454); //1C17 VOU1120_DATA0
    SYS_WRITEL(reg_iocfg2_base+0x18, 0x00000454); //1B17 VOU1120_DATA1
    SYS_WRITEL(reg_iocfg2_base+0x14, 0x00000454); //1A17 VOU1120_DATA2
    SYS_WRITEL(reg_iocfg2_base+0x10, 0x00000454); //1B16 VOU1120_DATA3
    SYS_WRITEL(reg_iocfg2_base+0x0c, 0x00000454); //1A16 VOU1120_DATA4
    SYS_WRITEL(reg_iocfg2_base+0x08, 0x00000454); //1B15 VOU1120_DATA5
    SYS_WRITEL(reg_iocfg2_base+0x04, 0x00000454); //1A15 VOU1120_DATA6
    SYS_WRITEL(reg_iocfg2_base+0x00, 0x00000454); //1C15 VOU1120_DATA7

#if 0
    //DEBM board: VOU1120_8~VOU1120_15
    SYS_WRITEL(reg_iocfg2_base+0x60, 0x00000454); //1F19 VOU1120_DATA8
    SYS_WRITEL(reg_iocfg2_base+0x5c, 0x00000454); //1F18 VOU1120_DATA9
    SYS_WRITEL(reg_iocfg2_base+0x54, 0x00000454); //1E18 VOU1120_DATA10
    SYS_WRITEL(reg_iocfg2_base+0x58, 0x00000454); //1B19 VOU1120_DATA11
    SYS_WRITEL(reg_iocfg2_base+0x28, 0x00000454); //1D19 VOU1120_DATA12
    SYS_WRITEL(reg_iocfg2_base+0x24, 0x00000454); //1C19 VOU1120_DATA13
    SYS_WRITEL(reg_iocfg2_base+0x30, 0x00000454); //C36    VOU1120_DATA14
    SYS_WRITEL(reg_iocfg2_base+0x2c, 0x00000454); //B36    VOU1120_DATA15

#else
    //DMEBLITE board: VOU1120_8~VOU1120_15
    SYS_WRITEL(reg_iocfg2_base+0x6c, 0x00000454); //A31  VOU1120_DATA8
    SYS_WRITEL(reg_iocfg2_base+0x68, 0x00000454); //B30  VOU1120_DATA9
    SYS_WRITEL(reg_iocfg2_base+0x78, 0x00000454); //B29  VOU1120_DATA10
    SYS_WRITEL(reg_iocfg2_base+0x74, 0x00000454); //A29  VOU1120_DATA11
    SYS_WRITEL(reg_iocfg2_base+0x64, 0x00000454); //B28  VOU1120_DATA12
    SYS_WRITEL(reg_iocfg2_base+0x80, 0x00000454); //B27  VOU1120_DATA13
    SYS_WRITEL(reg_iocfg2_base+0x7c, 0x00000454); //A27  VOU1120_DATA14
    SYS_WRITEL(reg_iocfg2_base+0x70, 0x00000454); //B26  VOU1120_DATA15
#endif

}

static void vo_lcd6bit_mode(void)
{
    SYS_WRITEL(reg_iocfg2_base+0x0020 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0048 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x004C ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0034 ,0x4f3);

    SYS_WRITEL(reg_iocfg2_base+0x0000 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0004 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0008 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x000C ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0010 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0014 ,0x4f3);
}

static void vo_lcd8bit_mode(void)
{
    SYS_WRITEL(reg_iocfg2_base+0x0020 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0048 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x004C ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0034 ,0x4f3);

    SYS_WRITEL(reg_iocfg2_base+0x0000 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0004 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0008 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x000C ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0010 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0014 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x0018 ,0x4f3);
    SYS_WRITEL(reg_iocfg2_base+0x001C ,0x4f3);
}

static void vo_lcd16bit_mode(void)
{
    SYS_WRITEL(reg_iocfg2_base+0x0020 ,0x4f3);  //1A19 LCD_CLK
    SYS_WRITEL(reg_iocfg2_base+0x0048 ,0x4f3);  //D37  LCD_DE
    SYS_WRITEL(reg_iocfg2_base+0x004c ,0x4f3);  //D36  LCD_HSYNC
    SYS_WRITEL(reg_iocfg2_base+0x0034 ,0x4f3);  //1C18 LCD_VSYNC

    SYS_WRITEL(reg_iocfg2_base+0x0000 ,0x4f3);  //1C15 LCD_DATA0
    SYS_WRITEL(reg_iocfg2_base+0x0004 ,0x4f3);  //1A15 LCD_DATA1
    SYS_WRITEL(reg_iocfg2_base+0x0008 ,0x4f3);  //1B15 LCD_DATA2
    SYS_WRITEL(reg_iocfg2_base+0x000C ,0x4f3);  //1A16 LCD_DATA3
    SYS_WRITEL(reg_iocfg2_base+0x0010 ,0x4f3);  //1B16 LCD_DATA4
    SYS_WRITEL(reg_iocfg2_base+0x0014 ,0x4f3);  //1A17 LCD_DATA5
    SYS_WRITEL(reg_iocfg2_base+0x0018 ,0x4f3);  //1B17 LCD_DATA6
    SYS_WRITEL(reg_iocfg2_base+0x001C ,0x4f3);  //1C17 LCD_DATA7

    SYS_WRITEL(reg_iocfg2_base+0x0050 ,0x4f3);  //1B18 LCD_DATA8
    SYS_WRITEL(reg_iocfg2_base+0x0058 ,0x4f3);  //1B19 LCD_DATA9
    SYS_WRITEL(reg_iocfg2_base+0x0028 ,0x4f3);  //1D19 LCD_DATA10
    SYS_WRITEL(reg_iocfg2_base+0x003c ,0x4f3);  //1D18 LCD_DATA11
    SYS_WRITEL(reg_iocfg2_base+0x002c ,0x4f3);  //B36  LCD_DATA12
    SYS_WRITEL(reg_iocfg2_base+0x0044 ,0x4f3);  //1E18 LCD_DATA13
    SYS_WRITEL(reg_iocfg2_base+0x0030 ,0x4f3);  //C36  LCD_DATA14
    SYS_WRITEL(reg_iocfg2_base+0x0040 ,0x4f3);  //1F18 LCD_DATA15
}

static void vo_lcd24bit_mode(void)
{
    SYS_WRITEL(reg_iocfg2_base+0x0020 ,0x4f3);  //1A19 LCD_CLK
    SYS_WRITEL(reg_iocfg2_base+0x0058 ,0x4f6);  //1B19 LCD_DE
    SYS_WRITEL(reg_iocfg2_base+0x0080 ,0x4f7);  //B27 LCD_HSYNC
    SYS_WRITEL(reg_iocfg2_base+0x007c ,0x4f7);  //A27 LCD_VSYNC

    SYS_WRITEL(reg_iocfg2_base+0x0000 ,0x4f3);  //1C15 LCD_DATA0
    SYS_WRITEL(reg_iocfg2_base+0x0004 ,0x4f3);  //1A15 LCD_DATA1
    SYS_WRITEL(reg_iocfg2_base+0x0008 ,0x4f3);  //1B15 LCD_DATA2
    SYS_WRITEL(reg_iocfg2_base+0x000C ,0x4f3);  //1A16 LCD_DATA3
    SYS_WRITEL(reg_iocfg2_base+0x0010 ,0x4f3);  //1B16 LCD_DATA4
    SYS_WRITEL(reg_iocfg2_base+0x0014 ,0x4f3);  //1A17 LCD_DATA5
    SYS_WRITEL(reg_iocfg2_base+0x0018 ,0x4f3);  //1B17 LCD_DATA6
    SYS_WRITEL(reg_iocfg2_base+0x001C ,0x4f3);  //1C17 LCD_DATA7

    SYS_WRITEL(reg_iocfg2_base+0x0024 ,0x4f7);  //1C19 LCD_DATA8
    SYS_WRITEL(reg_iocfg2_base+0x0028 ,0x4f7);  //1D19 LCD_DATA9
    SYS_WRITEL(reg_iocfg2_base+0x002c ,0x4f7);  //B36 LCD_DATA10
    SYS_WRITEL(reg_iocfg2_base+0x0030 ,0x4f7);  //C36 LCD_DATA11
    SYS_WRITEL(reg_iocfg2_base+0x0070 ,0x4f7);  //B26 LCD_DATA12
    SYS_WRITEL(reg_iocfg2_base+0x0064 ,0x4f7);  //B28 LCD_DATA13
    SYS_WRITEL(reg_iocfg2_base+0x0074 ,0x4f7);  //A29 LCD_DATA14
    SYS_WRITEL(reg_iocfg2_base+0x0078 ,0x4f7);  //B29 LCD_DATA15
    SYS_WRITEL(reg_iocfg2_base+0x0068 ,0x4f7);  //B30 LCD_DATA16
    SYS_WRITEL(reg_iocfg2_base+0x006c ,0x4f7);  //A31 LCD_DATA17
    SYS_WRITEL(reg_iocfg2_base+0x0094 ,0x4f7);  //B32 LCD_DATA18
    SYS_WRITEL(reg_iocfg2_base+0x0098 ,0x4f7);  //B33 LCD_DATA19
    SYS_WRITEL(reg_iocfg2_base+0x0088 ,0x4f7);  //A33 LCD_DATA20
    SYS_WRITEL(reg_iocfg2_base+0x008c ,0x4f7);  //B34 LCD_DATA21
    SYS_WRITEL(reg_iocfg2_base+0x0090 ,0x4f7);  //B35 LCD_DATA22
    SYS_WRITEL(reg_iocfg2_base+0x0084 ,0x4f7);  //A35 LCD_DATA23
}

#endif

static void hmdi_pin_mode(void)
{
    SYS_WRITEL(reg_iocfg1_base+0x005C ,0x00003C00);
    SYS_WRITEL(reg_iocfg1_base+0x0060 ,0x00001C00);
    SYS_WRITEL(reg_iocfg1_base+0x0064 ,0x00003C00);
    SYS_WRITEL(reg_iocfg1_base+0x0068 ,0x00003C00);
}

#if 0
static void gyro_pin_mode(void)
{
    spi1_pin_mux();
    SYS_WRITEL(reg_iocfg1_base+0x0008, 0x000014F0);//gpio, for interrupt pin
}
#endif

#if 0
static void i2s_pin_mux(void)
{
    SYS_WRITEL(reg_iocfg_base+0x0030 ,0x00000ef2);
    SYS_WRITEL(reg_iocfg_base+0x0040 ,0x00001ef2);
    SYS_WRITEL(reg_iocfg_base+0x0044 ,0x00001ef2);
    SYS_WRITEL(reg_iocfg_base+0x0048 ,0x00000ef2);
    SYS_WRITEL(reg_iocfg_base+0x004C ,0x00001ef2);

    //printk( "============i2s_pin_mux done=============\n");
}
#endif

#if 0
static void sensor_bus_pin_mux(int index, BUS_TYPE bus_type)
{
    if(BUS_TYPE_I2C == bus_type)
    {
        if(0 == index)
        {
            i2c1_pin_mux();
        }
        else if(1 == index)
        {
            i2c2_pin_mux();
        }
        else if(2 == index)
        {
            i2c3_pin_mux();
        }
        else if(3 == index)
        {
            i2c4_pin_mux();
        }
        else if(4 == index)
        {
            i2c5_pin_mux();
        }
    }
    else if(BUS_TYPE_SPI == bus_type)
    {
        if(0 == index)
        {
            spi0_pin_mux();
        }
        else if(1 == index)
        {
            spi1_pin_mux();
        }
        else if(2 == index)
        {
            spi2_pin_mux();
        }
        else if(3 == index)
        {
            //spi3_pin_mux();
        }
        else if(4 == index)
        {
            spi4_pin_mux();
        }
    }
}
#endif

static int pinmux(void)
{
    sensor_pin_mux();

    //i2c
    i2c1_pin_mux();
    i2c2_pin_mux();
    i2c3_pin_mux();
    i2c4_pin_mux();
    i2c5_pin_mux();
    i2c6_pin_mux();

    //spi
    //spi1_pin_mux();
    //spi2_pin_mux();
    //spi4_pin_mux();

    //icm20690
    //gyro_pin_mode();

    //sil9022/adv7179
    i2c0_pin_mux();

    //vo_bt1120_mode(g_bt1120_flag);

    //vo_bt656_mode();

    //spi3_pin_mux();

    //vo_lcd8bit_mode();
    //vo_lcd6bit_mode();
    //vo_lcd16bit_mode();
    //vo_lcd24bit_mode();

    hmdi_pin_mode();

    //i2s_pin_mux();

    return 0;
}

static int sensor_config(char *s)
{
    int ret;
    int index;
    unsigned int clock;
    char* line;
    //BUS_TYPE bus_type;
    char sensor_name[SENSOR_NAME_LEN];

    while ((line = strsep(&s, ":")) != NULL)
    {
        int i;
        char* argv[8];

        for (i = 0; (argv[i] = strsep(&line, ",")) != NULL;)
        {
            ret = parse_sensor_name(argv[i], sensor_name);
            if(ret >= 0)
            {
                index = parse_sensor_index(argv[i]);

                if (index >= 0)
                {
                    printk("==========sensr%d: %s==========\n", index, sensor_name);
                #ifndef HI_FPGA
                    clock = parse_sensor_clock(sensor_name);
                    //bus_type = parse_sensor_bus_type(sensor_name);

                    if (is_coms(sensor_name))
                    {
                        coms_clock_config(index);
                    }
                    else
                    {
                        //sensor_bus_pin_mux(index, bus_type);
                        sensor_clock_config(index, clock);
                    }
               #else
                    sensor_fpga_clock_config(sensor_name);
               #endif
                }
            }

            if (++i == ARRAY_SIZE(argv))
            {
                break;
            }
        }
    }

    return 0;
}

#if 1
static int ampunmute(void)
{
    SYS_WRITEL(reg_iocfg1_base+0x38 ,0x00000cf0);

    SYS_WRITEL(reg_gpio_base+0x8100 ,0x00000040);
    SYS_WRITEL(reg_gpio_base+0x8400 ,0x00000040);
    SYS_WRITEL(reg_gpio_base+0x8100 ,0x00000040);

    return 0;
}
#endif

static int __init hi_sysconfig_init(void)
{
    int online_flag;

    reg_crg_base = (void*)ioremap(0x04510000, 0x10000);
    if (NULL == reg_crg_base)
    {
        goto out;
    }
    reg_sys_base = (void*)ioremap(0x04520000, 0x8000);
    if (NULL == reg_sys_base)
    {
        goto out;
    }
    reg_misc_base = (void*)ioremap(0x04528000, 0x8000);
    if (NULL == reg_misc_base)
    {
        goto out;
    }
    reg_ddr_base = (void*)ioremap(0x04600000, 0x10000);
    if (NULL == reg_ddr_base)
    {
        goto out;
    }
    reg_iocfg_base = (void*)ioremap(0x04058000, 0x8000);
    if (NULL == reg_iocfg_base)
    {
        goto out;
    }
    reg_iocfg1_base = (void*)ioremap(0x047B8000, 0x8000);
    if (NULL == reg_iocfg1_base)
    {
        goto out;
    }
    reg_iocfg2_base = (void*)ioremap(0x047E0000, 0x8000);
    if (NULL == reg_iocfg2_base)
    {
        goto out;
    }
    reg_iocfg3_base = (void*)ioremap(0x047E8000, 0x8000);
    if (NULL == reg_iocfg3_base)
    {
        goto out;
    }
    reg_gpio_base = (void*)ioremap(0x045F0000, 0x10000);
    if (NULL == reg_gpio_base)
    {
        goto out;
    }

    clkcfg();

    /*
    0:cap/proc/vpss(offline);
    1:cap-proc/vpss(cap-proc online, vpss offline);
    2:cap-proc-vpss(cap-proc online, vpss online);
    */
    online_flag = 0;
    sys_ctl(online_flag);

    pinmux();

    ampunmute();

	sensor_config(sensor_list);

out:
    if (NULL != reg_crg_base)
    {
        iounmap(reg_crg_base);
        reg_crg_base = 0;
    }
    if (NULL != reg_sys_base)
    {
        iounmap(reg_sys_base);
        reg_sys_base = 0;
    }

    if (NULL != reg_ddr_base)
    {
        iounmap(reg_ddr_base);
        reg_ddr_base = 0;
    }
    if (NULL != reg_iocfg_base)
    {
        iounmap(reg_iocfg_base);
        reg_iocfg_base = 0;
    }
    if (NULL != reg_iocfg1_base)
    {
        iounmap(reg_iocfg1_base);
        reg_iocfg1_base = 0;
    }
    if (NULL != reg_iocfg2_base)
    {
        iounmap(reg_iocfg2_base);
        reg_iocfg2_base = 0;
    }
    if (NULL != reg_iocfg3_base)
    {
        iounmap(reg_iocfg3_base);
        reg_iocfg3_base = 0;
    }
    if (NULL != reg_gpio_base)
    {
        iounmap(reg_gpio_base);
        reg_gpio_base = 0;
    }
    return 0;
}

static void __exit hi_sysconfig_exit(void)
{
    if (NULL != reg_crg_base)
    {
        iounmap(reg_crg_base);
        reg_crg_base = 0;
    }
    if (NULL != reg_sys_base)
    {
        iounmap(reg_sys_base);
        reg_sys_base = 0;
    }
    if (NULL != reg_ddr_base)
    {
        iounmap(reg_ddr_base);
        reg_ddr_base = 0;
    }
    if (NULL != reg_misc_base)
    {
        iounmap(reg_misc_base);
        reg_misc_base = 0;
    }
    if (NULL != reg_iocfg_base)
    {
        iounmap(reg_iocfg_base);
        reg_iocfg_base = 0;
    }

    if (NULL != reg_iocfg1_base)
    {
        iounmap(reg_iocfg1_base);
        reg_iocfg1_base = 0;
    }

    if (NULL != reg_iocfg2_base)
    {
        iounmap(reg_iocfg2_base);
        reg_iocfg2_base = 0;
    }

    if (NULL != reg_iocfg3_base)
    {
        iounmap(reg_iocfg3_base);
        reg_iocfg3_base = 0;
    }

    if (NULL != reg_gpio_base)
    {
        iounmap(reg_gpio_base);
        reg_gpio_base = 0;
    }

    return;
}

module_init(hi_sysconfig_init);
module_exit(hi_sysconfig_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon");


