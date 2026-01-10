/*
 * thcv242a.c - Thine THCV242A deserializer and THCV241A serializer driver
 *
 * Copyright (c) 2023, www.veye.cc, TIANJIN DATA IMAGING TECHNOLOGY CO.,LTD
 *
 * This program is for the THCV242A V-by-ONE deserializer in connection
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

#include "thcv241a.h"
#if 0
const struct regmap_config thcv242a_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
};
#endif
const struct regmap_config thcv241a_regmap_config_orig = {
	.reg_bits = 16,
	.val_bits = 8,
};

const struct regmap_config thcv241a_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};


/*------------------------------------------------------------------------------
 * THCV241A FUNCTIONS
 *----------------------------------------------------------------------------*/
static int thcv241a_write(const struct thcv241a_priv *priv, unsigned int reg,
			   unsigned int val)
{
	int err;
	err = regmap_write(priv->regmap, reg, val);
	if(err) {
		dev_err(&priv->client->dev,
			"Cannot write subdev 0x%02x register 0x%02x (%d)!\n",
			priv->client->addr, reg, err);
	}
    //dev_err(&priv->client->dev,"W241 0x%x : 0x%x\n", reg,val);
	return err;
}
/*
void print_i2c_client_info(struct i2c_client *client) {
    if (client) {
        printk("i2c bus: %s\n", dev_name(&client->dev));

        // Additional information you want to print
    } else {
        printk("Invalid i2c_client structure.\n");
    }
}*/

/*
void print_regmap_info(struct regmap *regmap) {
    if (regmap) {
        unsigned int reg_stride = regmap_get_reg_stride(regmap);
        unsigned int val_bytes = regmap_get_val_bytes(regmap);
        
        printk("Register address length: %u bytes\n", reg_stride);
        printk("Register value length: %u bytes\n", val_bytes);

        // Additional information you want to print
    } else {
        printk("Invalid regmap structure.\n");
    }
}*/

static int thcv241a_regmap_update(struct thcv241a_priv *priv);

static int thcv241a_init(struct thcv241a_priv *priv)
{
    int err = 0;

    struct device *dev = &priv->client->dev;
    dev_info(dev, "%s: begin \n", __func__);
    
    
    err |= thcv241a_write(priv,0x0010, 0x00);
    err |= thcv241a_write(priv,0x101d, 0x01);
    err |= thcv241a_write(priv,0x101e, 0x10);
    err |= thcv241a_write(priv,0x1076, 0x10);
    err |= thcv241a_write(priv,0x1000, 0x00);
    err |= thcv241a_write(priv,0x1001, 0x00);
    err |= thcv241a_write(priv,0x100f, 0x01);
    err |= thcv241a_write(priv,0x1011, 0x1c);
    err |= thcv241a_write(priv,0x1012, 0x6c);
    err |= thcv241a_write(priv,0x1013, 0x80);
    err |= thcv241a_write(priv,0x1014, 0x00);
    err |= thcv241a_write(priv,0x1015, 0x77);
    err |= thcv241a_write(priv,0x1016, 0x01);
    err |= thcv241a_write(priv,0x102b, 0x05);
    err |= thcv241a_write(priv,0x102f, 0x00);
    err |= thcv241a_write(priv,0x102d, 0x10);
    err |= thcv241a_write(priv,0x102c, 0x01);
    err |= thcv241a_write(priv,0x1005, 0x01);
    err |= thcv241a_write(priv,0x1006, 0x01);

    dev_info(dev, "%s:  successfully \n", __func__);
    
   // err |= thcv241a_read(serpriv,0x3F, &val);
   // dev_info(dev, "thcv241a_read 0x3F val is %x ,should be 0xF\n",val );
    return err;
}


static int thcv241a_free(struct thcv241a_priv *priv)
{   
    printk("%d:[YDY]thcv241a_free\n",__LINE__);
    printk("[YDY]priv->client->addr:0x%x\n",priv->client->addr);
    printk("[YDY]priv->client->name:%s\n",priv->client->name);
    printk("[YDY]priv->client->dev.of_node.name:%s\n",priv->client->dev.of_node->name);
    //devm_kfree(&priv->client->dev,priv);
	i2c_unregister_device(priv->client);
    return 0;
}

//update to addr,8bit val
static int thcv241a_regmap_update(struct thcv241a_priv *priv)
{
    struct regmap *new_regmap = NULL;
	struct device *dev = &priv->client->dev;
	int err = 0;
    
    if(priv->regmap){
        regmap_exit(priv->regmap);
    }
	/* setup now regmap */
	new_regmap = devm_regmap_init_i2c(priv->client,
					  &thcv241a_regmap_config);
	if(IS_ERR_VALUE(priv->regmap)) {
		err = PTR_ERR(priv->regmap);
		dev_err(dev, "regmap init of subdevice failed (%d)\n", err);
		return err;
	}
	priv->regmap = new_regmap;
	dev_info(dev, "%s regmap done\n", __func__);
	return err;
}

static int thcv241a_parse_dt(struct i2c_client *client,
			      struct thcv241a_priv *priv)
{
	struct device *dev = &client->dev;
	struct device_node *des = dev->of_node;
	//struct thcv241a_priv *thcv241a = priv; 

	u32 val = 0;
	int err = 0;

	dev_info(dev, "%s: parsing serializers device tree:\n", __func__);
    err = of_property_read_u32(des, "csi-lane-count", &val);
    if(err) {
        dev_info(dev, "%s: - csi-lane-count property not found\n",
             __func__);
        /* default value: 1 */
        priv->csi_lane_count = 1;
        dev_info(dev, "%s: csi-lane-count set to default val: 1\n",
             __func__);
    } else {
        /* set csi-lane-count*/ 
        priv->csi_lane_count = val;
        dev_info(dev, "%s: - csi-lane-count %i\n", __func__, val);
    }

    err = of_property_read_u32(des, "csi-lane-speed", &val);
    if(err) {
        dev_info(dev, "%s: - csi-lane-speed property not found\n",
             __func__);
        priv->csi_lane_speed = 1200;
        dev_info(dev, "%s: csi-lane-speed set to default val: 2\n",
             __func__);
    } else {
        /* set csi-lane-count*/
        priv->csi_lane_speed = val;
        dev_info(dev, "%s: - csi-lane-speed %i\n", __func__, val);
    }
    if(priv->csi_lane_speed != 1200 && priv->csi_lane_speed != 1188)
    {
        dev_err(dev, "%s: - csi-lane-speed %i not supported,will exit!\n", __func__, val);
        goto ERR;
    }
#if 0
    err = thcv241a_i2c_client(thcv241a,thcv241a->client->addr);
    if(err) {
        dev_info(dev, "%s: - thcv241a_i2c_client failed\n",
             __func__);
        goto ERR;
    }

    err = thcv241a_regmap_init(thcv241a, 0);
    if(err) {
        dev_info(dev, "%s: - thcv241a_regmap_init failed\n",
             __func__);
        goto ERR;
    }

    /* all initialization of this serializer complete */
    thcv241a->initialized = 1;
    dev_info(dev, "%s: serializer %i successfully parsed\n", __func__,0);
#endif
	return 0;
ERR:
	return -1;
}

/*------------------------------------------------------------------------------
 * PROBE FUNCTION
 *----------------------------------------------------------------------------*/

static int thcv241a_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct thcv241a_priv *priv;
	struct device *dev = &client->dev;
	int err;
   
	dev_info(dev, "%s: start\n", __func__);

	priv = devm_kzalloc(dev, sizeof(struct thcv241a_priv), GFP_KERNEL);
	if(!priv)
		return -ENOMEM;

	priv->client = client;
	i2c_set_clientdata(client, priv);
#if 0
	err = thcv241a_parse_dt(client, priv);
	if(unlikely(err < 0)) {
		dev_err(dev, "%s: error parsing device tree\n", __func__);
		goto err_parse_dt;
	}
#endif
	priv->regmap = devm_regmap_init_i2c(client, &thcv241a_regmap_config_orig);
	if(IS_ERR_VALUE(priv->regmap)) {
		err = PTR_ERR(priv->regmap);
		dev_err(dev, "%s: regmap init failed (%d)\n", __func__, err);
		goto err_regmap;
	}


    /*init thcv241a*/
    err = thcv241a_init(priv);

	if(unlikely(err)) {
		dev_err(dev, "%s: error initializing thcv241a\n", __func__);
		goto err_regmap;
	}
 

	return 0;

err_regmap:
	thcv241a_free(priv);
err_parse_dt:
	devm_kfree(dev, priv);
	return err;
}

static int thcv241a_remove(struct i2c_client *client)
{   

	struct thcv241a_priv *priv = dev_get_drvdata(&client->dev);
    printk("%d:[YDY]priv_addr:%p\n",__LINE__,priv);
    printk("%d:[YDY]&client->dev:%p\n",__LINE__,&client->dev);
    if(priv == NULL)
    {
        printk("priv is NULL!\n");
    }
    printk("%d:[YDY]thcv241a_remove\n",__LINE__);
    
	thcv241a_free(priv);

	dev_info(&client->dev, "thcv241a removed\n");

    return 0;
}

static const struct i2c_device_id thcv241a_id[] = 
{
	//{ "thcv241a", 0 },
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(i2c,thcv241a_id);

static const struct of_device_id thcv241a_dt_ids[] = {
	{ .compatible = "weigao,thcv241a" },
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(of, thcv241a_dt_ids);

static struct i2c_driver thcv241a_driver = {
	.driver = {
        .owner = THIS_MODULE,
		.name = "thcv241a",
		.of_match_table = thcv241a_dt_ids,
	},
	.probe = thcv241a_probe,
	.remove = thcv241a_remove,
    .id_table	= thcv241a_id,
};

module_i2c_driver(thcv241a_driver);

MODULE_AUTHOR("Yu daoyang");
MODULE_DESCRIPTION("V-by-ONE driver from VEYE IMAGING");
MODULE_LICENSE("GPL");
