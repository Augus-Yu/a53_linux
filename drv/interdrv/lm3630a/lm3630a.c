/*
 * lm3630a.c - TI LM3630A backlight LED driver
 *
 * Copyright (c) 2023, www.veye.cc, TIANJIN DATA IMAGING TECHNOLOGY CO.,LTD
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/uaccess.h>

#include "lm3630a.h"

/* Register map */
#define LM3630A_REG_CONTROL		0x00
#define LM3630A_REG_CONFIGURATION	0x01
#define LM3630A_REG_BOOST_CONTROL	0x02
#define LM3630A_REG_BRIGHTNESS_A	0x03
#define LM3630A_REG_BRIGHTNESS_B	0x04
#define LM3630A_REG_CURRENT_A		0x05
#define LM3630A_REG_CURRENT_B		0x06
#define LM3630A_REG_ON_OFF_RAMP		0x07
#define LM3630A_REG_RUN_RAMP		0x08
#define LM3630A_REG_INT_STATUS		0x09
#define LM3630A_REG_INT_ENABLE		0x0A
#define LM3630A_REG_FAULT_STATUS	0x0B
#define LM3630A_REG_SW_RESET		0x0F
#define LM3630A_REG_FILTER_STRENGTH	0x50

/* Control bits */
#define LM3630A_CTRL_LED_A_EN		BIT(2)
#define LM3630A_CTRL_LED_B_EN		BIT(1)
#define LM3630A_CTRL_LED2_ON_A		BIT(0)

/* Configuration bits */
#define LM3630A_CFG_FB_EN_A		BIT(3)
#define LM3630A_CFG_FB_EN_B		BIT(4)
#define LM3630A_CFG_PWM_EN_A		BIT(0)
#define LM3630A_CFG_PWM_EN_B		BIT(1)

/* Boost control helpers */
#define LM3630A_BOOST_500KHZ		0
#define LM3630A_BOOST_1MHZ		BIT(0)
#define LM3630A_BOOST_SHIFT		BIT(1)
#define LM3630A_BOOST_SLOW_START	BIT(2)

#define LM3630A_OVP_16V			(0x0 << 5)
#define LM3630A_OVP_24V			(0x1 << 5)
#define LM3630A_OVP_32V			(0x2 << 5)
#define LM3630A_OVP_40V			(0x3 << 5)

#define LM3630A_OCP_600MA		(0x0 << 3)
#define LM3630A_OCP_800MA		(0x1 << 3)
#define LM3630A_OCP_1A			(0x2 << 3)
#define LM3630A_OCP_1A2			(0x3 << 3)

static int major;
static struct class *lm3630a_class;
static struct lm3630a_priv *g_priv;
static DEFINE_MUTEX(lm3630a_mutex);

const struct regmap_config lm3630a_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x50,
};

/* 亮度等级表：0~5 级，对应 0/20/40/60/80/100% (线性近似) */
static const u8 lm3630a_brt_table[] = {
	0x00,	/* 0: off */
	0x33,	/* 1: ~20%  ( 51/255) */
	0x66,	/* 2: ~40%  (102/255) */
	0x99,	/* 3: ~60%  (153/255) */
	0xCC,	/* 4: ~80%  (204/255) */
	0xFF,	/* 5: 100%  (255/255) */
};

/*------------------------------------------------------------------------------
 * Low-level I2C access
 *----------------------------------------------------------------------------*/

static int lm3630a_write(const struct lm3630a_priv *priv, unsigned int reg,
			 unsigned int val)
{
	int err = regmap_write(priv->regmap, reg, val);

	if (err)
		dev_err(&priv->client->dev,
			"write reg 0x%02X = 0x%02X failed (%d)\n", reg, val, err);
	else
		dev_dbg(&priv->client->dev,
			"write reg 0x%02X = 0x%02X\n", reg, val);
	return err;
}

/*------------------------------------------------------------------------------
 * Character device interface
 *----------------------------------------------------------------------------*/

static int lm3630a_open_led(struct inode *node, struct file *file)
{
	mutex_lock(&lm3630a_mutex);
	file->private_data = g_priv;
	mutex_unlock(&lm3630a_mutex);

	if (!g_priv)
		return -ENODEV;

	return 0;
}

static ssize_t lm3630a_read_led(struct file *fp, char __user *buf,
				size_t count, loff_t *ppos)
{
	return 0;	/* 可扩展为读取当前亮度 */
}

static ssize_t lm3630a_write_led(struct file *fp, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct lm3630a_priv *priv = fp->private_data;
	u8 level;
	int ret;

	if (!priv)
		return -ENODEV;

	if (count < 1)
		return -EINVAL;

	ret = copy_from_user(&level, buf, sizeof(level));
	if (ret) {
		dev_err(&priv->client->dev,
			"%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	if (level >= ARRAY_SIZE(lm3630a_brt_table)) {
		dev_err(&priv->client->dev,
			"%s: invalid brightness level %u (max %zu)\n",
			__func__, level, ARRAY_SIZE(lm3630a_brt_table) - 1);
		return -EINVAL;
	}

	mutex_lock(&lm3630a_mutex);

	ret = lm3630a_write(priv, LM3630A_REG_BRIGHTNESS_A,
			    lm3630a_brt_table[level]);

	mutex_unlock(&lm3630a_mutex);

	return ret ? ret : count;
}

static int lm3630a_close_led(struct inode *node, struct file *file)
{
	return 0;
}

static struct file_operations lm3630a_fops = {
	.owner   = THIS_MODULE,
	.open    = lm3630a_open_led,
	.read    = lm3630a_read_led,
	.write   = lm3630a_write_led,
	.release = lm3630a_close_led,
};

/*------------------------------------------------------------------------------
 * Hardware control & chip init
 *----------------------------------------------------------------------------*/

static void lm3630a_hw_enable(struct lm3630a_priv *priv)
{
	if (priv->hwen_gpio) {
		gpiod_set_value_cansleep(priv->hwen_gpio, 1);
		/* tWAIT = 1 ms (Datasheet 7.3.4) */
		usleep_range(1500, 2500);
	}
}

static void lm3630a_hw_disable(struct lm3630a_priv *priv)
{
	if (priv->hwen_gpio)
		gpiod_set_value_cansleep(priv->hwen_gpio, 0);
}

static int lm3630a_parse_dt(struct lm3630a_priv *priv)
{
	struct device *dev = &priv->client->dev;
	struct device_node *np = dev->of_node;
	u32 val;
	int ret;

	if (!np)
		goto defaults;

	ret = of_property_read_u32(np, "ti,default-brightness", &val);
	priv->default_brightness = (ret == 0 && val <= 255) ? (u8)val : 0x99; /* 60% */

	ret = of_property_read_u32(np, "ti,fullscale-current-code", &val);
	/* 5-bit, 0~31 -> 5mA ~ 28.25mA. 默认 0x14 = 20mA，避免烧灯 */
	priv->fullscale_code = (ret == 0 && val <= 31) ? (u8)val : 0x14;

	ret = of_property_read_u32(np, "ti,boost-ctrl", &val);
	if (ret == 0) {
		priv->boost_ctrl = (u8)val;
	} else {
		/* 默认：OVP=24V, OCP=1A, 500kHz.
		 * 对2颗LED+470Ω电阻，输出约15.7V，24V足够。
		 * 若LED颗数增加，请设备树改OVP到32V/40V。
		 */
		priv->boost_ctrl = LM3630A_OVP_24V |
				   LM3630A_OCP_1A |
				   LM3630A_BOOST_500KHZ;
	}

	return 0;

defaults:
	priv->default_brightness = 0x99; /* 60% = level 3 */
	priv->fullscale_code   = 0x14;	/* 20mA */
	priv->boost_ctrl       = LM3630A_OVP_24V | LM3630A_OCP_1A | LM3630A_BOOST_500KHZ;
	return 0;
}

static int lm3630a_chip_init(struct lm3630a_priv *priv)
{
	struct device *dev = &priv->client->dev;
	int err = 0, ret;

	dev_info(dev, "%s: initializing LM3630A\n", __func__);

	/* 1. 等待芯片退出 reset / tWAIT */
	lm3630a_hw_enable(priv);

	/* 2. 解析设备树参数 */
	lm3630a_parse_dt(priv);

	/* 3. 推荐初始化序列（单串 LED1 场景，LED2 悬空） */
	/* 3.1 PWM采样器滤波强度（即使不用PWM，也保持推荐值） */
	ret = lm3630a_write(priv, LM3630A_REG_FILTER_STRENGTH, 0x03);
	if (ret) err = ret;

	/* 3.2 Configuration: 只使能 Bank A 反馈，禁用 PWM */
	ret = lm3630a_write(priv, LM3630A_REG_CONFIGURATION,
			    LM3630A_CFG_FB_EN_A);
	if (ret) err = ret;

	/* 3.3 Boost: OVP / OCP / 频率 */
	ret = lm3630a_write(priv, LM3630A_REG_BOOST_CONTROL, priv->boost_ctrl);
	if (ret) err = ret;

	/* 3.4 Current A: 设定全标度电流。
	 * 注意：硬件有 470Ω 串联电阻，Boost 必须额外提供 ~9.4V@20mA
	 * 压降来克服该电阻，效率会降低，但电流调节仍有效。
	 */
	ret = lm3630a_write(priv, LM3630A_REG_CURRENT_A,
			    priv->fullscale_code);
	if (ret) err = ret;

	/* 3.5 Control: 使能 Bank A，LED2 也映射到 A（虽然未接） */
	ret = lm3630a_write(priv, LM3630A_REG_CONTROL,
			    LM3630A_CTRL_LED_A_EN | LM3630A_CTRL_LED2_ON_A);
	if (ret) err = ret;

	/* 3.6 初始亮度 */
	ret = lm3630a_write(priv, LM3630A_REG_BRIGHTNESS_A,
			    priv->default_brightness);
	if (ret) err = ret;

	if (err)
		dev_err(dev, "%s: init completed with errors\n", __func__);
	else
		dev_info(dev,
			 "%s: OK (boost=0x%02X, current=0x%02X, brt=0x%02X)\n",
			 __func__, priv->boost_ctrl, priv->fullscale_code,
			 priv->default_brightness);

	return err;
}

static int lm3630a_chip_shutdown(struct lm3630a_priv *priv)
{
	int err = 0, ret;

	/* 先灭灯，再进入 sleep */
	ret = lm3630a_write(priv, LM3630A_REG_BRIGHTNESS_A, 0x00);
	if (ret) err = ret;

	/* SLEEP_CMD = 1 (bit7)，进入低功耗模式 (350uA) */
	ret = lm3630a_write(priv, LM3630A_REG_CONTROL, 0x80);
	if (ret) err = ret;

	return err;
}

/*------------------------------------------------------------------------------
 * Probe / Remove
 *----------------------------------------------------------------------------*/

static int lm3630a_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct lm3630a_priv *priv;
	struct device *dev = &client->dev;
	struct device *cls_dev;
	int err;

	dev_info(dev, "%s: start\n", __func__);

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->client = client;
	i2c_set_clientdata(client, priv);

	mutex_lock(&lm3630a_mutex);
	g_priv = priv;
	mutex_unlock(&lm3630a_mutex);

	/* regmap */
	priv->regmap = devm_regmap_init_i2c(client, &lm3630a_regmap_config);
	if (IS_ERR(priv->regmap)) {
		err = PTR_ERR(priv->regmap);
		dev_err(dev, "regmap init failed (%d)\n", err);
		return err;
	}

	/* 可选 HWEN GPIO（你的硬件常开，获取失败也不报错） */
	priv->hwen_gpio = devm_gpiod_get_optional(dev, "hwen", GPIOD_OUT_HIGH);
	if (IS_ERR(priv->hwen_gpio)) {
		err = PTR_ERR(priv->hwen_gpio);
		dev_err(dev, "failed to get HWEN gpio (%d)\n", err);
		return err;
	}

	/* 芯片初始化 */
	err = lm3630a_chip_init(priv);
	if (err) {
		dev_err(dev, "chip init failed (%d)\n", err);
		return err;
	}

	/* 注册字符设备 */
	major = register_chrdev(0, "lm3630a", &lm3630a_fops);
	if (major < 0) {
		dev_err(dev, "register_chrdev failed (%d)\n", major);
		return major;
	}

	lm3630a_class = class_create(THIS_MODULE, "lm3630a");
	if (IS_ERR(lm3630a_class)) {
		err = PTR_ERR(lm3630a_class);
		dev_err(dev, "class_create failed (%d)\n", err);
		goto err_unregister;
	}

	cls_dev = device_create(lm3630a_class, dev, MKDEV(major, 0), priv,
				"lm3630a");
	if (IS_ERR(cls_dev)) {
		err = PTR_ERR(cls_dev);
		dev_err(dev, "device_create failed (%d)\n", err);
		goto err_class;
	}

	dev_info(dev, "%s: done\n", __func__);
	return 0;

err_class:
	class_destroy(lm3630a_class);
err_unregister:
	unregister_chrdev(major, "lm3630a");
	return err;
}

static int lm3630a_remove(struct i2c_client *client)
{
	struct lm3630a_priv *priv = dev_get_drvdata(&client->dev);
	int err;

	if (!priv)
		return -EINVAL;

	mutex_lock(&lm3630a_mutex);
	g_priv = NULL;
	mutex_unlock(&lm3630a_mutex);

	/* 删除 sysfs 节点 */
	device_destroy(lm3630a_class, MKDEV(major, 0));
	class_destroy(lm3630a_class);
	unregister_chrdev(major, "lm3630a");

	/* 关 LED 并进入 sleep */
	err = lm3630a_chip_shutdown(priv);
	if (err)
		dev_warn(&client->dev, "shutdown error %d\n", err);

	/* 拉低 HWEN（如果有 GPIO） */
	lm3630a_hw_disable(priv);

	dev_info(&client->dev, "lm3630a removed\n");
	return 0;
}

static const struct i2c_device_id lm3630a_id[] = {
	{ "lm3630a", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lm3630a_id);

static const struct of_device_id lm3630a_dt_ids[] = {
	{ .compatible = "ti,lm3630a" },
	{ }
};
MODULE_DEVICE_TABLE(of, lm3630a_dt_ids);

static struct i2c_driver lm3630a_driver = {
	.driver = {
		.name = "lm3630a",
		.of_match_table = lm3630a_dt_ids,
	},
	.probe    = lm3630a_probe,
	.remove   = lm3630a_remove,
	.id_table = lm3630a_id,
};

module_i2c_driver(lm3630a_driver);

MODULE_AUTHOR("Yu daoyang");
MODULE_DESCRIPTION("TI LM3630A backlight LED driver (single string)");
MODULE_LICENSE("GPL");