/*
 * lm3630a.h - TI LM3630A backlight LED driver
 *
 * Copyright (c) 2023, www.veye.cc, TIANJIN DATA IMAGING TECHNOLOGY CO.,LTD
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#ifndef I2C_LM3630A_H
#define I2C_LM3630A_H

#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/types.h>

struct lm3630a_priv {
	struct i2c_client *client;
	struct regmap *regmap;
	struct gpio_desc *hwen_gpio;
	u8 default_brightness;
	u8 fullscale_code;
	u8 boost_ctrl;
};

#endif /* I2C_LM3630A_H */
