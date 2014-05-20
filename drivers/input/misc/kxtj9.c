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

#define KXTJ9_DRIVER_NAME "kxtj9"

DEFINE_MUTEX(Kxtj9_global_lock);

#define	KXTJ9_XOUT_L			0X06
#define	KXTJ9_XOUT_H			0X07
#define	KXTJ9_YOUT_L			0X08
#define	KXTJ9_YOUT_H			0X09
#define	KXTJ9_ZOUT_L			0X0A
#define	KXTJ9_ZOUT_H			0X0B
#define	KXTJ9_DCST_RESP			0X0C
#define	KXTJ9_WHO_AM_I			0X0F
#define	KXTJ9_INT_SOURCE1		0X16
#define	KXTJ9_INT_SOURCE2		0X17
#define	KXTJ9_STATUS_REG		0X18
#define	KXTJ9_INT_REL			0X1A
#define	KXTJ9_CTRL_REG1			0X1B
#define	KXTJ9_CTRL_REG2			0X1D
#define	KXTJ9_INT_CTRL_REG1		0X1E
#define	KXTJ9_INT_CTRL_REG2		0X1F
#define	KXTJ9_DATA_CTRL_REG		0X21
#define	KXTJ9_WAKEUP_TIMER		0X29
#define	KXTJ9_WAKEUP_THREHOLD	0X6A

/* ---------------------------------------------------------------------------------------- *
   Input device interface
 * ---------------------------------------------------------------------------------------- */

static int kxtj9_type_setting(bool open)
{
	int rc = 0;
	queueIndex = ignoreCount = 0;
	memset(queueData, 0, sizeof(queueData));
	if(open){
		//Setting Operation mode
		rc = i2c_smbus_write_byte_data(this_client, KXTJ9_CTRL_REG1, POWER_MODE_CAMMAND);
	}else{
		//Setting Suspend mode
		rc = i2c_smbus_write_byte_data(this_client, KXTJ9_CTRL_REG1, 0X00);
	}
	return rc;
}

static int kxtj9_enable(void)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("KXTJ9: %s ++\n", __func__);
	#endif

	if(data->enabled == false){
		//enable sensor
		kxtj9_type_setting(true);
		mutex_lock(&data->mutex);
		data->enabled = true;
		mutex_unlock(&data->mutex);	
		rc = 1;
	}

	rc = (rc == 1) ? queue_delayed_work(Accelerometer_WorkQueue, &data->dw, msecs_to_jiffies(20)) : -1;
	return 0;
}

static int kxtj9_disable(void)
{
	Accelerometer* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("KXTJ9: %s \n", __func__);
	#endif

	if(data->enabled == true){
		mutex_lock(&data->mutex);
		data->enabled = false;
		mutex_unlock(&data->mutex);
		cancel_delayed_work_sync(&data->dw);
		flush_workqueue(Accelerometer_WorkQueue);

		//Setting Suspend mode
		kxtj9_type_setting(false);
		WorkMode = NORMAL_MODE;
		SleepTime = msecs_to_jiffies(200);
		POWER_MODE_CAMMAND = 0XD0;
	}

	return 0;
}

static int kxtj9_open(struct inode *inode, struct file *file)
{
	int rc = 0;
	#if debug
	pr_info("KXTJ9: %s\n", __func__);
	#endif

	mutex_lock(&Kxtj9_global_lock);
	if(Accelerometer_sensor_opened){
		pr_err("%s: already opened\n", __func__);
		rc = -EBUSY;
	}
	Accelerometer_sensor_opened = 1;
	mutex_unlock(&Kxtj9_global_lock);

	return rc;
}

static int kxtj9_release(struct inode *inode, struct file *file)
{
	#if debug
	pr_info("KXTJ9: %s\n", __func__);
	#endif
	mutex_lock(&Kxtj9_global_lock);
	Accelerometer_sensor_opened = 0;
	mutex_unlock(&Kxtj9_global_lock);
	return 0;
}

static long kxtj9_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("%s cmd:%d, arg:%ld\n", __func__, _IOC_NR(cmd), arg);
	#endif

	mutex_lock(&Kxtj9_global_lock);
	switch(cmd){
		case ACCELEROMETER_IOCTL_SET_STATE:
			rc = arg ? kxtj9_enable() : kxtj9_disable();
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
				.vendor		= "Kionix",
				.maxRange	= 8,// 8G
				.resolution	= 2048,// 8G / 2048
				.power		= 250,// uA
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
	mutex_unlock(&Kxtj9_global_lock);

	return rc;
}

static struct file_operations kxtj9_fops = {
	.owner = THIS_MODULE,
	.open = kxtj9_open,
	.release = kxtj9_release,
	.unlocked_ioctl = kxtj9_ioctl
};

struct miscdevice kxtj9_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "accelerometer",
	.fops = &kxtj9_fops
};

// Return true if data ready to report
static bool kxtj9_readData(void){
	memset(i2cData, 0, sizeof(i2cData));
	memset(&rawData, 0, sizeof(AccelerometerData));
	i2c_smbus_read_i2c_block_data(this_client, KXTJ9_XOUT_L, 6, &i2cData[0]);
	rawData.Y = (i2cData[1] < 128) ? i2cData[1] : (i2cData[1] - 256);
	rawData.Y = ((rawData.Y << 4) + (i2cData[0] >> 4)) * 10000 >> 8;
	rawData.X = (i2cData[3] < 128) ? i2cData[3] : (i2cData[3] - 256);
	rawData.X = ((rawData.X << 4) + (i2cData[2] >> 4)) * 10000 >> 8;
	rawData.Z = (i2cData[5] < 128) ? i2cData[5] : (i2cData[5] - 256);
	rawData.Z = ((rawData.Z << 4) + (i2cData[4] >> 4)) * 10000 >> 8;
	memcpy(&(queueData[queueIndex]), &rawData, sizeof(AccelerometerData));
	queueIndex = (queueIndex < FILTER_INDEX) ? queueIndex + 1 : 0;
	ignoreCount = (ignoreCount < FILTER_SIZE) ? ignoreCount + 1 : FILTER_SIZE;
	return (ignoreCount == FILTER_SIZE);
}

static void kxtj9_reportData(Accelerometer* data){
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

static void kxtj9_work_func(struct work_struct *work)
{
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("KXTJ9: %s ++\n", __func__);
	#endif

	if(data->enabled && !data->suspend){
		(kxtj9_readData()) ? kxtj9_reportData(data) : 0;
		#if debug
		printk("KXTJ9: ACCELEROMETER X: %d, Y: %d, Z: %d\n", data->sdata.X / 1000, (0 - data->sdata.Y)  / 1000, (0 - data->sdata.Z)  / 1000);
		#endif
	}

	if(data->enabled && !data->suspend){
		queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime);
	}

	#if debug
	pr_info("KXTJ9: %s --\n", __func__);
	#endif
}

static int kxtj9_suspend(struct i2c_client *client, pm_message_t state)
{
	Accelerometer* data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = true;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("KXTJ9: %s ++\n", __func__);
	#endif

	if(data->enabled){
		//Setting Operation mode
		kxtj9_type_setting(false);
	}

	#if debug
	pr_info("KXTJ9: %s --\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int kxtj9_resume(struct i2c_client *client)
{
	Accelerometer *data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = false;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("KXTJ9: %s ++\n", __func__);
	#endif

	if(data->enabled){
		kxtj9_type_setting(true);
		queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime);
	}

	#if debug
	pr_info("KXTJ9: %s --\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int kxtj9_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	Accelerometer* Sensor_device = NULL;
	struct input_dev* input_dev = NULL;
	int err = 0;

	#if debug
	pr_info("KXTJ9: %s ++\n", __func__);
	#endif

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)){
		return -EIO;
	}

	if((err = i2c_smbus_read_byte_data(client, KXTJ9_WHO_AM_I)) != 0X07){
		return -ENODEV;
	}
	#if debug
	printk(KERN_INFO"KXTJ9_WHO_AM_I value = %d\n", err);
	#endif

	Sensor_device = kzalloc(sizeof(Accelerometer), GFP_KERNEL);

	input_dev = input_allocate_device();

	if(!Sensor_device || !input_dev){
		err = -ENOMEM;
		goto err_free_mem;
	}

	INIT_DELAYED_WORK(&Sensor_device->dw, kxtj9_work_func);
	i2c_set_clientdata(client, Sensor_device);

	input_dev->name = "accelerometer";
	input_dev->id.bustype = BUS_I2C;

	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_X, -20480000, 20480000, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, -20480000, 20480000, 0, 0);
	input_set_abs_params(input_dev, ABS_Z, -20480000, 20480000, 0, 0);
	input_set_drvdata(input_dev, Sensor_device);	

	err = input_register_device(input_dev);
	if(err){
		pr_err("KXTJ9: input_register_device error\n");
		goto err_free_mem;
	}

	//Software reset
	i2c_smbus_write_byte_data(client, KXTJ9_CTRL_REG2, 0X80);
	msleep(10);
	//Setting Bandwith 50 HZ, updated time 10ms
	i2c_smbus_write_byte_data(client, KXTJ9_DATA_CTRL_REG, 0X03);

	err = misc_register(&kxtj9_misc);
    if(err < 0){
		pr_err("KXTJ9: sensor_probe: Unable to register misc device: %s\n", input_dev->name);
		goto err;
	}
	
	Sensor_device->input	= input_dev;
	Sensor_device->enabled	= false;
	Sensor_device->suspend	= false;
	memset(&(Sensor_device->sdata), 0 , sizeof(AccelerometerData));
	memset(&(Sensor_device->odata), 0 , sizeof(AccelerometerAxisOffset));

	mutex_init(&Sensor_device->mutex);

	Accelerometer_sensor_opened = 0;

	this_client = client;
	Accelerometer_WorkQueue = create_singlethread_workqueue(input_dev->name);
	SleepTime = msecs_to_jiffies(200);

	POWER_MODE_CAMMAND = 0XD0;
	WorkMode = NORMAL_MODE;

	#if debug
	pr_info("KXTJ9: %s --\n", __func__);
	#endif
	accelerometer_readAxisOffset();

	return 0;

	err:
		misc_deregister(&kxtj9_misc);
	err_free_mem:
		input_free_device(input_dev);
		kfree(Sensor_device);
	return err;
}

static int kxtj9_remove(struct i2c_client *client)
{
	Accelerometer* data = i2c_get_clientdata(client);

	destroy_workqueue(Accelerometer_WorkQueue);
	input_unregister_device(data->input);
	misc_deregister(&kxtj9_misc);
	kfree(data);

	return 0;
}

static void kxtj9_shutdown(struct i2c_client *client)
{
	kxtj9_disable();
}

static struct i2c_device_id kxtj9_idtable[] = {
	{"kxtj9", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, kxtj9_idtable);

static struct i2c_driver kxtj9_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= KXTJ9_DRIVER_NAME
	},
	.id_table	= kxtj9_idtable,
	.probe		= kxtj9_probe,
	.remove		= kxtj9_remove,
	.suspend  	= kxtj9_suspend,
	.resume   	= kxtj9_resume,
	.shutdown	= kxtj9_shutdown,
};

static int __init kxtj9_init(void)
{
	return i2c_add_driver(&kxtj9_driver);
}

static void __exit kxtj9_exit(void)
{
	i2c_del_driver(&kxtj9_driver);
}

module_init(kxtj9_init);
module_exit(kxtj9_exit);

MODULE_AUTHOR("HuizeWeng@Arimacomm");
MODULE_DESCRIPTION("Accelerometer Sensor KXTJ9");
MODULE_LICENSE("GPLv2");
