/*
 * lm3630a.c - TI LM3630A backlight LED driver
 *
 * Copyright (c) 2023, www.veye.cc, TIANJIN DATA IMAGING TECHNOLOGY CO.,LTD
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef I2C_LM3630A_H
#define I2C_LM3630A_H

#include <linux/i2c.h>

/*------------------------------------------------------------------------------
 * DEFINES
 *----------------------------------------------------------------------------*/

struct lm3630a_priv {
	struct i2c_client *client;
	struct regmap *regmap;
};

#endif /* I2C_LM3630A_H */
