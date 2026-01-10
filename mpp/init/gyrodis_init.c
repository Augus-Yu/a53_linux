#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/version.h>

#include "hi_common.h"
#include "hi_osal.h"

#define DIS_DEV_NAME_LENGTH 10

extern int gyrodis_mod_init(void);
extern void gyrodis_mod_exit(void);

#include <linux/of_platform.h>

extern void * pDisReg[2];
extern unsigned int dis_irq;

static int hi35xx_gyrodis_probe(struct platform_device *pdev)
{
    gyrodis_mod_init();

    return 0;
}

static int hi35xx_gyrodis_remove(struct platform_device *pdev)
{
    gyrodis_mod_exit();

    return 0;
}

static const struct of_device_id hi35xx_gyrodis_match[] = {
        { .compatible = "hisilicon,hisi-gyro-dis" },
        {},
};
MODULE_DEVICE_TABLE(of, hi35xx_gyrodis_match);

static struct platform_driver hi35xx_gyrodis_driver = {
        .probe          = hi35xx_gyrodis_probe,
        .remove         = hi35xx_gyrodis_remove,
        .driver         = {
                .name   = "hi35xx_gyrodis",
                .of_match_table = hi35xx_gyrodis_match,
        },
};

osal_module_platform_driver(hi35xx_gyrodis_driver);

MODULE_LICENSE("Proprietary");

