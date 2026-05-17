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
#include <linux/uaccess.h>

#include "lm3630a.h"

static int major = 0;
static struct class *lm3630a_class;
static struct lm3630a_priv *g_priv;
static DEFINE_MUTEX(lm3630a_mutex);

const struct regmap_config lm3630a_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

/*------------------------------------------------------------------------------
 * lm3630a FUNCTIONS
 *----------------------------------------------------------------------------*/

static int lm3630a_write(const struct lm3630a_priv *priv, unsigned int reg,
			   unsigned int val)
{
	int err;

	err = regmap_write(priv->regmap, reg, val);
	if (err) {
		dev_err(&priv->client->dev,
			"Cannot write register 0x%02x (%d)!\n", reg, err);
	}
	return err;
}

static int lm3630a_open_led(struct inode *node, struct file *file)
{
	return 0;
}

static ssize_t lm3630a_read_led(struct file *fp, char __user *buf,
				size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t lm3630a_write_led(struct file *fp, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	int err = 0;
	int ret;
	char status;

	ret = copy_from_user(&status, buf, sizeof(status));
	if (ret != 0) {
		pr_err("%s copy_from_user failed (%d)!\n", __func__, ret);
		return -EFAULT;
	}

	mutex_lock(&lm3630a_mutex);

	switch (status) {
	case 0:
		ret = lm3630a_write(g_priv, 0x03, 0x00);
		break;
	case 1:
		ret = lm3630a_write(g_priv, 0x03, 0xc1);
		break;
	case 2:
		ret = lm3630a_write(g_priv, 0x03, 0xd8);
		break;
	case 3:
		ret = lm3630a_write(g_priv, 0x03, 0xe7);
		break;
	case 4:
		ret = lm3630a_write(g_priv, 0x03, 0xf1);
		break;
	case 5:
		ret = lm3630a_write(g_priv, 0x03, 0xf8);
		break;
	default:
		pr_err("%s: unsupported brightness level %d\n", __func__, status);
		mutex_unlock(&lm3630a_mutex);
		return -EINVAL;
	}

	mutex_unlock(&lm3630a_mutex);

	if (ret)
		err = ret;

	return err;
}

static int lm3630a_close_led(struct inode *node, struct file *file)
{
	return 0;
}

static struct file_operations lm3630a_ops = {
	.owner = THIS_MODULE,
	.open  = lm3630a_open_led,
	.read  = lm3630a_read_led,
	.write = lm3630a_write_led,
	.release = lm3630a_close_led,
};

static int lm3630a_free(struct lm3630a_priv *priv)
{
	int err = 0;
	int ret;

	struct device *dev = &priv->client->dev;
	dev_info(dev, "%s: begin\n", __func__);

	ret = lm3630a_write(priv, 0x00, 0x0);
	if (ret)
		err = ret;
	ret = lm3630a_write(priv, 0x03, 0x0);
	if (ret)
		err = ret;
	return err;
}

static int lm3630a_init(struct lm3630a_priv *priv)
{
	int err = 0;
	int ret;
	struct device *dev = &priv->client->dev;
	dev_info(dev, "%s: begin\n", __func__);

	ret = lm3630a_write(priv, 0x00, 0x04);
	if (ret)
		err = ret;
	ret = lm3630a_write(priv, 0x03, 0xd8);
	if (ret)
		err = ret;

	dev_info(dev, "%s: successfully\n", __func__);

	return err;
}

/*------------------------------------------------------------------------------
 * PROBE FUNCTION
 *----------------------------------------------------------------------------*/

static int lm3630a_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct lm3630a_priv *priv;
	struct device *dev = &client->dev;
	int err;

	dev_info(dev, "%s: start\n", __func__);

	priv = devm_kzalloc(dev, sizeof(struct lm3630a_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->client = client;
	i2c_set_clientdata(client, priv);

	mutex_lock(&lm3630a_mutex);
	g_priv = priv;
	mutex_unlock(&lm3630a_mutex);

	priv->regmap = devm_regmap_init_i2c(client, &lm3630a_regmap_config);
	if (IS_ERR_VALUE(priv->regmap)) {
		err = PTR_ERR(priv->regmap);
		dev_err(dev, "%s: regmap init failed (%d)\n", __func__, err);
		goto err_regmap;
	}

	err = lm3630a_init(priv);
	if (unlikely(err)) {
		dev_err(dev, "%s: error initializing lm3630a\n", __func__);
		goto err_regmap;
	}

	/* register chrdev */
	major = register_chrdev(0, "lm3630a", &lm3630a_ops);
	if (major < 0) {
		dev_err(dev, "%s: register_chrdev failed (%d)\n", __func__, major);
		err = major;
		goto err_regmap;
	}

	lm3630a_class = class_create(THIS_MODULE, "lm3630a_class");
	if (IS_ERR(lm3630a_class)) {
		err = PTR_ERR(lm3630a_class);
		dev_err(dev, "%s: class_create failed (%d)\n", __func__, err);
		goto err_class;
	}

	device_create(lm3630a_class, NULL, MKDEV(major, 0), NULL, "lm3630a");
	return 0;

err_class:
	unregister_chrdev(major, "lm3630a");
err_regmap:
	mutex_lock(&lm3630a_mutex);
	g_priv = NULL;
	mutex_unlock(&lm3630a_mutex);
	return err;
}

static int lm3630a_remove(struct i2c_client *client)
{
	int err;
	struct lm3630a_priv *priv = dev_get_drvdata(&client->dev);

	mutex_lock(&lm3630a_mutex);
	g_priv = NULL;
	mutex_unlock(&lm3630a_mutex);

	device_destroy(lm3630a_class, MKDEV(major, 0));
	class_destroy(lm3630a_class);
	unregister_chrdev(major, "lm3630a");

	err = lm3630a_free(priv);
	if (err < 0) {
		dev_info(&client->dev, "lm3630a removed with error %d\n", err);
		return err;
	}

	dev_info(&client->dev, "lm3630a removed\n");
	return 0;
}

static const struct i2c_device_id lm3630a_id[] =
{
	{ "lm3630a", 0 },
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(i2c, lm3630a_id);

static const struct of_device_id lm3630a_dt_ids[] = {
	{ .compatible = "ti,lm3630a" },
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(of, lm3630a_dt_ids);

static struct i2c_driver lm3630a_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "lm3630a",
		.of_match_table = lm3630a_dt_ids,
	},
	.probe = lm3630a_probe,
	.remove = lm3630a_remove,
	.id_table = lm3630a_id,
};

module_i2c_driver(lm3630a_driver);

MODULE_AUTHOR("Yu daoyang");
MODULE_DESCRIPTION("TI LM3630A backlight LED driver");
MODULE_LICENSE("GPL");
