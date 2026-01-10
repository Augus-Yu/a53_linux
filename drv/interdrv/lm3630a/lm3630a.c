/*
 * lm3630a.c - Thine lm3630a deserializer and THCV241A serializer driver
 *
 * Copyright (c) 2023, www.veye.cc, TIANJIN DATA IMAGING TECHNOLOGY CO.,LTD
 *
 * This program is for the lm3630a V-by-ONE deserializer in connection
 * with the SHA241 serializer from Thine
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
 
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/mod_devicetable.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/property.h>
#include <linux/acpi.h>
#include <linux/nvmem-provider.h>
#include <linux/pm_runtime.h>
#include <linux/gpio/consumer.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
//#include <linux/unistd.h>
#include "lm3630a.h"

static int major = 0;
static struct class *lm3630a_class;
struct lm3630a_priv *g_priv;

const struct regmap_config lm3630a_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

/*------------------------------------------------------------------------------
 * lm3630a FUNCTIONS
 *----------------------------------------------------------------------------*/
/*
static int lm3630a_read(struct lm3630a_priv *priv, unsigned int reg,
			  unsigned int *val)
{
	int err;
	err = regmap_read(priv->regmap, reg, val);
	if(err) {
		dev_err(&priv->client->dev,
			"Cannot read register 0x%02x (%d)!\n", reg, err);
	}
	return err;
}
*/

static int lm3630a_write(const struct lm3630a_priv *priv, unsigned int reg,
			   unsigned int val)
{
	int err;

	err = regmap_write(priv->regmap, reg, val);
	if(err) {
		dev_err(&priv->client->dev,
			"Cannot write register 0x%02x (%d)!\n", reg, err);
	}
    dev_err(&priv->client->dev,"Wlm3630 0x%x : 0x%x\n", reg,val);
	return err;
}

static ssize_t lm3630a_open_led(struct inode *node, struct file *file)
{
	printk("open lm3630a successfully\n");
	return 0;
}

static ssize_t lm3630a_read_led(struct file *fp,char *buf, size_t count,loff_t *ppos)
{
	return 0;
}

static ssize_t lm3630a_write_led(struct file *fp,const char *buf, size_t count,loff_t *ppos)
{
	int err;
	char status;
	

	err = copy_from_user(&status, buf, sizeof(status));
	if(err != 0)
	{
		printk("%s %s line %d,copy_from_user failed!!!\n", __FILE__, __FUNCTION__, __LINE__);
	}

	switch(status)
	{
	case 0:
		err |= lm3630a_write(g_priv,0x03,0x0);
		break;
	case 1:
		err |= lm3630a_write(g_priv,0x03,0xc1);
		break;
	case 2:
		err |= lm3630a_write(g_priv,0x03,0xd8);
		break;
	case 3:
		err |= lm3630a_write(g_priv,0x03,0xe7);
		break;
	case 4:
		err |= lm3630a_write(g_priv,0x03,0xf1);
		break;
	case 5:
		err |= lm3630a_write(g_priv,0x03,0xf8);
		break;
	default:
		printk("%s %s line %d,Unsupported level!!!\n", __FILE__, __FUNCTION__, __LINE__);
		break;	
	}

	return err;
}

static int lm3630a_close_led (struct inode *node, struct file *file)
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

#if 0
static int lm3630a_update(struct lm3630a_priv *priv,
			  unsigned int reg, unsigned int mask,
			  unsigned int data)
{
	return regmap_update_bits(priv->regmap, reg, mask, data);
}
#endif
 static int lm3630a_free(struct lm3630a_priv *priv)
 {

	int err = 0;

    struct device *dev = &priv->client->dev;
    dev_info(dev, "%s: begin \n", __func__);

	err |= lm3630a_write(priv,0x00,0x0); 
	err |= lm3630a_write(priv,0x03,0x0);  
	return err;
 }

static int lm3630a_init(struct lm3630a_priv *priv)
{
    int err = 0;
    //unsigned int val;
    struct device *dev = &priv->client->dev;
    dev_info(dev, "%s: begin \n", __func__);
    

	err |= lm3630a_write(priv,0x00,0x04);    //0x04:enable A    0x0:disable A
	err |= lm3630a_write(priv,0x03,0xe7);     //brighness A leve0:0x0 leve1:0xc1 leve2:0xd8 leve3:0xe7 leve4:0xf1 leve5:0xf8

#if 0
	err |= lm3630a_update(priv,0x03,0x07,0x11);
    err |= lm3630a_write(priv,0x01,0x01);
    err |= lm3630a_write(priv,0x02,0x02);
    err |= lm3630a_write(priv,0x04,0xa1);
    err |= lm3630a_write(priv,0x05,0x00);    //current A
    err |= lm3630a_write(priv,0x06,0x60);
    err |= lm3630a_write(priv,0x07,0x01);
    err |= lm3630a_write(priv,0x08,0x31);
    err |= lm3630a_write(priv,0x09,0x00);
    err |= lm3630a_write(priv,0x0a,0x00);
    err |= lm3630a_write(priv,0x0b,0x00);
    err |= lm3630a_write(priv,0x0f,0x07);
    err |= lm3630a_write(priv,0x12,0x01);
    err |= lm3630a_write(priv,0x13,0x01);
    err |= lm3630a_write(priv,0x1f,0x01);
    err |= lm3630a_write(priv,0x50,0x03);
 #endif   

    dev_info(dev, "%s:  successfully \n", __func__);
    
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
    printk("[YDY]Into lm3630a_probe\n");
	dev_info(dev, "%s: start\n", __func__);

	priv = devm_kzalloc(dev, sizeof(struct lm3630a_priv), GFP_KERNEL);
	if(!priv)
		return -ENOMEM;

	priv->client = client;
	i2c_set_clientdata(client, priv);
	g_priv = priv;
	priv->regmap = devm_regmap_init_i2c(client, &lm3630a_regmap_config);
	if(IS_ERR_VALUE(priv->regmap)) {
		err = PTR_ERR(priv->regmap);
		dev_err(dev, "%s: regmap init failed (%d)\n", __func__, err);
		goto err_regmap;
	}
	err = lm3630a_init(priv);
	if(unlikely(err)) {
		dev_err(dev, "%s: error initializing lm3630a\n", __func__);
		goto err_regmap;
	}
		/* register_chrdev */
	major = register_chrdev(0, "lm3630a", &lm3630a_ops);

	lm3630a_class = class_create(THIS_MODULE, "lm3630a_class");
	device_create(lm3630a_class, NULL, MKDEV(major, 0), NULL, "lm3630a");
	return 0;

err_regmap:
	//thcv241a_free(priv);
	//lm3630a_pwr_disable(priv);
	//lm3630a_free_gpio(priv);

	return err;
}

static int lm3630a_remove(struct i2c_client *client)
{
	int err = 0;
	struct lm3630a_priv *priv = dev_get_drvdata(&client->dev);

	device_destroy(lm3630a_class, MKDEV(major, 0));
	class_destroy(lm3630a_class);
	
	/* unregister_chrdev */
	unregister_chrdev(major, "lm3630a");

	err = lm3630a_free(priv);
	if(err < 0)
	{
		dev_info(&client->dev, "lm3630a removed failed!!!\n");
	}
	else
	{
		dev_info(&client->dev, "lm3630a removed\n");
	}
	
    return 0;
}
static const struct i2c_device_id lm3630a_id[] = 
{
	{ "lm3630a", 0 },
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(i2c,lm3630a_id);

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
    .id_table	= lm3630a_id,
};

module_i2c_driver(lm3630a_driver);

MODULE_AUTHOR("Yu daoyang");
MODULE_DESCRIPTION("V-by-ONE driver from VEYE IMAGING");
MODULE_LICENSE("GPL");
