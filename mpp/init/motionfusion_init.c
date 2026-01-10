/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description : motionfusion_init.c, init draft
 * Author : ISP SW
 * Create : 2018-12-22
 * Version : Initial Draft
 */

#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/of_platform.h>

#include "hi_defines.h"
#include "hi_type.h"
#include "hi_osal.h"

extern hi_s32 mfusion_drv_mod_init(hi_void);
extern hi_void mfusion_drv_mod_exit(hi_void);

hi_s32 motionfusion_init(hi_void)
{
    return mfusion_drv_mod_init();
}

hi_void motionfusion_exit(hi_void)
{
    mfusion_drv_mod_exit();
}

module_init(motionfusion_init);
module_exit(motionfusion_exit);

MODULE_DESCRIPTION("motionfusion driver");
MODULE_LICENSE("Proprietary");


