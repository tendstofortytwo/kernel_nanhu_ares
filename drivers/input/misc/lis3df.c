/* Accelerometer-sensor
 *
 * Copyright (c) 2011-2014, HuizeWeng@Arimacomm Corp. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <mach/vreg.h>
#include <linux/accelerometer_common.h>

#define LIS3DF_DRIVER_NAME "lis3df"

DEFINE_MUTEX(Lis3df_global_lock);

#define		CTRL_REG1_XEN			0x01
#define		CTRL_REG1_YEN			0x02
#define		CTRL_REG1_ZEN			0x04
#define 	CTRL_REG1_POWER_DOWN	0x00
#define 	CTRL_REG1_1HZ			0x10
#define 	CTRL_REG1_10HZ			0x20
#define 	CTRL_REG1_25HZ			0x30
#define 	CTRL_REG1_50HZ			0x40
#define 	CTRL_REG1_100HZ			0x50
#define 	CTRL_REG1_200HZ			0x60
#define 	CTRL_REG1_400HZ			0x70

enum {
	ACCELEROMETER_REG_WHOAMI 		= 0x0F,
	ACCELEROMETER_REG_CTRL_REG1		= 0x20,
	ACCELEROMETER_REG_CTRL_REG2		= 0x21,
	ACCELEROMETER_REG_CTRL_REG3		= 0x22,
	ACCELEROMETER_REG_CTRL_REG4		= 0x23,
	ACCELEROMETER_REG_CTRL_REG5		= 0x24,
	ACCELEROMETER_REG_CTRL_REG6		= 0x25,
	ACCELEROMETER_REG_STATUS_REG	= 0x27,
	ACCELEROMETER_REG_OUT_X			= 0X29,
	ACCELEROMETER_REG_OUT_Y			= 0X2B,
	ACCELEROMETER_REG_OUT_Z			= 0x2D,
	ACCELEROMETER_REG_INT1_CFG		= 0X30,
	ACCELEROMETER_REG_INT1_SRC		= 0X31,
	ACCELEROMETER_REG_INT1_THS		= 0X32,
	ACCELEROMETER_REG_INT1_DURATION	= 0X33,
	ACCELEROMETER_REG_INT2_CFG		= 0X34,
	ACCELEROMETER_REG_INT2_SRC		= 0X35,
	ACCELEROMETER_REG_INT2_THS		= 0X36,
	ACCELEROMETER_REG_INT2_DURATION	= 0X37,
};

/* ---------------------------------------------------------------------------------------- *
   Input device interface
 * ---------------------------------------------------------------------------------------- */

static int lis3df_type_setting(bool open)
{
	int rc = 0;
	queueIndex = ignoreCount = 0;
	memset(queueData, 0, sizeof(queueData));
	if(open){
		//Setting Operation mode
		rc = i2c_smbus_write_byte_data(this_client, ACCELEROMETER_REG_CTRL_REG1, POWER_MODE_CAMMAND);
	}else{
		//Setting Suspend mode
		rc = i2c_smbus_write_byte_data(this_client, ACCELEROMETER_REG_CTRL_REG1, CTRL_REG1_POWER_DOWN);
	}
	return rc;
}

static int lis3df_enable(void)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("LIS3DF: %s ++\n", __func__);
	#endif

	if(data->enabled == false){
		//enable sensor
		lis3df_type_setting(true);
		mutex_lock(&data->mutex);
		data->enabled = true;
		mutex_unlock(&data->mutex);	
		rc = 1;
	}

	rc = (rc == 1) ? queue_delayed_work(Accelerometer_WorkQueue, &data->dw, msecs_to_jiffies(20)) : -1;
	return 0;
}

static int lis3df_disable(void)
{
	Accelerometer* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("LIS3DF: %s \n", __func__);
	#endif

	if(data->enabled == true){
		mutex_lock(&data->mutex);
		data->enabled = false;
		mutex_unlock(&data->mutex);
		cancel_delayed_work_sync(&data->dw);
		flush_workqueue(Accelerometer_WorkQueue);

		//Setting Suspend mode
		lis3df_type_setting(false);
		WorkMode = NORMAL_MODE;
		SleepTime = msecs_to_jiffies(200);
		POWER_MODE_CAMMAND = CTRL_REG1_XEN | CTRL_REG1_YEN | CTRL_REG1_ZEN | CTRL_REG1_100HZ;
	}

	return 0;
}

static int lis3df_open(struct inode *inode, struct file *file)
{
	int rc = 0;
	#if debug
	pr_info("LIS3DF: %s\n", __func__);
	#endif

	mutex_lock(&Lis3df_global_lock);
	if(Accelerometer_sensor_opened){
		pr_err("%s: already opened\n", __func__);
		rc = -EBUSY;
	}
	Accelerometer_sensor_opened = 1;
	mutex_unlock(&Lis3df_global_lock);

	return rc;
}

static int lis3df_release(struct inode *inode, struct file *file)
{
	#if debug
	pr_info("LIS3DF: %s\n", __func__);
	#endif
	mutex_lock(&Lis3df_global_lock);
	Accelerometer_sensor_opened = 0;
	mutex_unlock(&Lis3df_global_lock);
	return 0;
}

static long lis3df_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("%s cmd:%d, arg:%ld\n", __func__, _IOC_NR(cmd), arg);
	#endif

	mutex_lock(&Lis3df_global_lock);
	switch(cmd){
		case ACCELEROMETER_IOCTL_SET_STATE:
			rc = arg ? lis3df_enable() : lis3df_disable();
			break;
		case ACCELEROMETER_IOCTL_GET_STATE:
			mutex_lock(&data->mutex);
			put_user(data->enabled, (unsigned long __user *) arg);
			mutex_unlock(&data->mutex);
			break;
		case ACCELEROMETER_IOCTL_GET_DEVICE_INFOR:
		{
			struct device_infor infor = {
				.name		= "Accelerometer Sensor",
				.vendor		= "ST Microelectionics",
				.maxRange	= 2,// 2G
				.resolution	= 128,// 2G / 128
				.power		= 700,// uA
			};
			rc = copy_to_user((unsigned long __user *)arg, (char *)&(infor), sizeof(struct device_infor));
			break;
		}
		case ACCELEROMETER_IOCTL_SET_DELAY:
		{
			arg = (arg >= 10) ? arg : 10;
			SleepTime = msecs_to_jiffies(--arg);// To makeure timer is exactly.
			break;
		}
		case ACCELEROMETER_IOCTL_SET_AXIS_OFFSET:
		{
			AccelerometerAxisOffset* offset = kzalloc(sizeof(AccelerometerAxisOffset), GFP_KERNEL);
			rc = copy_from_user(offset, (unsigned long __user *) arg, sizeof(AccelerometerAxisOffset));
			mutex_lock(&data->mutex);
			offset->X += data->odata.X;
			offset->Y += data->odata.Y;
			offset->Z += data->odata.Z;
			mutex_unlock(&data->mutex);
			rc = (accelerometer_resetAxisOffset(offset->X, offset->Y, offset->Z)) ? 1 : -1;
			kfree(offset);
			break;
		}
		case ACCELEROMETER_IOCTL_SET_AXIS_OFFSET_INIT:
		{
			rc = (accelerometer_resetAxisOffset(0, 0, 0)) ? 1 : -1;
			break;
		}
		default:
			pr_err("%s: invalid cmd %d\n", __func__, _IOC_NR(cmd));
			rc = -EINVAL;
	}
	mutex_unlock(&Lis3df_global_lock);

	return rc;
}

static struct file_operations lis3df_fops = {
	.owner = THIS_MODULE,
	.open = lis3df_open,
	.release = lis3df_release,
	.unlocked_ioctl = lis3df_ioctl
};

struct miscdevice lis3df_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "accelerometer",
	.fops = &lis3df_fops
};

// Return true if data ready to report
static bool lis3df_readData(void){
	memset(i2cData, 0, sizeof(i2cData));
	memset(&rawData, 0, sizeof(AccelerometerData));
	rawData.Y = i2c_smbus_read_byte_data(this_client, ACCELEROMETER_REG_OUT_X);
	rawData.Y = (rawData.Y < 128) ? rawData.Y : (rawData.Y - 256);
	rawData.Y = (rawData.Y * 10000) >> 6;
	rawData.X = i2c_smbus_read_byte_data(this_client, ACCELEROMETER_REG_OUT_Y);
	rawData.X = (rawData.X < 128) ? rawData.X : (rawData.X - 256);
	rawData.X = (rawData.X * 10000) >> 6;
	rawData.Z = i2c_smbus_read_byte_data(this_client, ACCELEROMETER_REG_OUT_Z);
	rawData.Z = (rawData.Z < 128) ? rawData.Z : (rawData.Z - 256);
	rawData.Z = (rawData.Z * 10000) >> 6;
	memcpy(&(queueData[queueIndex]), &rawData, sizeof(AccelerometerData));
	queueIndex = (queueIndex < FILTER_INDEX) ? queueIndex + 1 : 0;
	ignoreCount = (ignoreCount < FILTER_SIZE) ? ignoreCount + 1 : FILTER_SIZE;
	return (ignoreCount == FILTER_SIZE);
}

static void lis3df_reportData(Accelerometer* data){
	u8 i = 0;
	memset(&averageData, 0, sizeof(AccelerometerData));
	for( ; i < FILTER_SIZE ; ++i){
		averageData.X += queueData[i].X >> FILTER_SIZEBIT;
		averageData.Y += queueData[i].Y >> FILTER_SIZEBIT;
		averageData.Z += queueData[i].Z >> FILTER_SIZEBIT;
	}
	mutex_lock(&data->mutex);
	memcpy(&(data->sdata), &averageData, sizeof(AccelerometerData));
	mutex_unlock(&data->mutex);
	input_report_abs(data->input, ABS_X, (0 - data->sdata.X) - data->odata.X);
	input_report_abs(data->input, ABS_Y, data->sdata.Y - data->odata.Y);
	input_report_abs(data->input, ABS_Z, data->sdata.Z - data->odata.Z);
	input_sync(data->input);
}

static void lis3df_work_func(struct work_struct *work)
{
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("LIS3DF: %s ++\n", __func__);
	#endif

	if(data->enabled && !data->suspend){
		(lis3df_readData()) ? lis3df_reportData(data) : 0;
		#if debug
		printk("LIS3DF: ACCELEROMETER X: %d, Y: %d, Z: %d\n", data->sdata.X / 1000, data->sdata.Y  / 1000, data->sdata.Z  / 1000);
		#endif
	}

	if(data->enabled && !data->suspend){
		queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime);
	}

	#if debug
	pr_info("LIS3DF: %s --\n", __func__);
	#endif
}

static int lis3df_suspend(struct i2c_client *client, pm_message_t state)
{
	Accelerometer* data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = true;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("LIS3DF: %s ++\n", __func__);
	#endif

	if(data->enabled){
		//Setting Operation mode
		lis3df_type_setting(false);
	}

	#if debug
	pr_info("LIS3DF: %s --\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int lis3df_resume(struct i2c_client *client)
{
	Accelerometer *data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = false;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("LIS3DF: %s ++\n", __func__);
	#endif

	if(data->enabled){
		lis3df_type_setting(true);
		queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime);
	}

	#if debug
	pr_info("LIS3DF: %s --\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int lis3df_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	Accelerometer* Sensor_device = NULL;
	struct input_dev* input_dev = NULL;
	int err = 0;

	#if debug
	pr_info("LIS3DF: %s ++\n", __func__);
	#endif

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)){
		return -EIO;
	}

	if((err = i2c_smbus_read_byte_data(client, ACCELEROMETER_REG_WHOAMI)) != 0X33){
		return -ENODEV;
	}
	#if debug
	printk(KERN_INFO"REG_WHOAMI value = %d\n", err);
	#endif

	Sensor_device = kzalloc(sizeof(Accelerometer), GFP_KERNEL);

	input_dev = input_allocate_device();

	if(!Sensor_device || !input_dev){
		err = -ENOMEM;
		goto err_free_mem;
	}

	INIT_DELAYED_WORK(&Sensor_device->dw, lis3df_work_func);
	i2c_set_clientdata(client, Sensor_device);

	input_dev->name = "accelerometer";
	input_dev->id.bustype = BUS_I2C;

	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_X, -1280000, 1280000, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, -1280000, 1280000, 0, 0);
	input_set_abs_params(input_dev, ABS_Z, -1280000, 1280000, 0, 0);
	input_set_drvdata(input_dev, Sensor_device);	

	err = input_register_device(input_dev);
	if(err){
		pr_err("LIS3DF: input_register_device error\n");
		goto err_free_mem;
	}

	err = misc_register(&lis3df_misc);
    if(err < 0){
		pr_err("LIS3DF: sensor_probe: Unable to register misc device: %s\n", input_dev->name);
		goto err;
	}
	
	Sensor_device->input	= input_dev;
	Sensor_device->enabled	= false;
	Sensor_device->suspend	= false;
	memset(&(Sensor_device->sdata), 0 , sizeof(AccelerometerData));
	memset(&(Sensor_device->odata), 0 , sizeof(AccelerometerAxisOffset));

	mutex_init(&Sensor_device->mutex);

	Accelerometer_sensor_opened = 0;

	i2c_smbus_write_byte_data(client, ACCELEROMETER_REG_CTRL_REG1, 0x00);
	i2c_smbus_write_byte_data(client, ACCELEROMETER_REG_CTRL_REG2, 0x00);
	i2c_smbus_write_byte_data(client, ACCELEROMETER_REG_CTRL_REG3, 0x00);
	i2c_smbus_write_byte_data(client, ACCELEROMETER_REG_CTRL_REG4, 0x00);
	i2c_smbus_write_byte_data(client, ACCELEROMETER_REG_CTRL_REG5, 0x00);
	i2c_smbus_write_byte_data(client, ACCELEROMETER_REG_CTRL_REG6, 0x00);

	this_client = client;
	Accelerometer_WorkQueue = create_singlethread_workqueue(input_dev->name);
	SleepTime = msecs_to_jiffies(200);

	POWER_MODE_CAMMAND = CTRL_REG1_XEN | CTRL_REG1_YEN | CTRL_REG1_ZEN | CTRL_REG1_100HZ;
	WorkMode = NORMAL_MODE;

	#if debug
	pr_info("LIS3DF: %s --\n", __func__);
	#endif
	accelerometer_readAxisOffset();

	return 0;

	err:
		misc_deregister(&lis3df_misc);
	err_free_mem:
		input_free_device(input_dev);
		kfree(Sensor_device);
	return err;
}

static int lis3df_remove(struct i2c_client *client)
{
	Accelerometer* data = i2c_get_clientdata(client);

	destroy_workqueue(Accelerometer_WorkQueue);
	input_unregister_device(data->input);
	misc_deregister(&lis3df_misc);
	kfree(data);

	return 0;
}

static void lis3df_shutdown(struct i2c_client *client)
{
	lis3df_disable();
}

static struct i2c_device_id lis3df_idtable[] = {
	{"lis3df", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, lis3df_idtable);

static struct i2c_driver lis3df_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= LIS3DF_DRIVER_NAME
	},
	.id_table	= lis3df_idtable,
	.probe		= lis3df_probe,
	.remove		= lis3df_remove,
	.suspend  	= lis3df_suspend,
	.resume   	= lis3df_resume,
	.shutdown	= lis3df_shutdown,
};

static int __init lis3df_init(void)
{
	return i2c_add_driver(&lis3df_driver);
}

static void __exit lis3df_exit(void)
{
	i2c_del_driver(&lis3df_driver);
}

module_init(lis3df_init);
module_exit(lis3df_exit);

MODULE_AUTHOR("HuizeWeng@Arimacomm");
MODULE_DESCRIPTION("Accelerometer Sensor LIS3DF");
MODULE_LICENSE("GPLv2");
