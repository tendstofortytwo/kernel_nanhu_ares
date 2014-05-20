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

#define BMA250E_DRIVER_NAME "bma250e"

static u8 BMA250E_PMU_BW_CAMMAND;

DEFINE_MUTEX(Bma250e_global_lock);

#define	BMA250E_BGW_CHIPID			0X00
#define BMA250E_ACCD_X_LSB			0X02
#define BMA250E_ACCD_X_MSB			0X03
#define BMA250E_ACCD_Y_LSB			0X04
#define BMA250E_ACCD_Y_MSB			0X05
#define BMA250E_ACCD_Z_LSB			0X06
#define BMA250E_ACCD_Z_MSB			0X07
#define BMA250E_RESERVED			0X08
#define BMA250E_INT_STATUS_0		0X09
#define BMA250E_INT_STATUS_1		0X0A
#define BMA250E_INT_STATUS_2		0X0B
#define BMA250E_INT_STATUS_3		0X0C
#define BMA250E_FIFO_STATUS			0X0E
#define BMA250E_PMU_RANGE			0X0F
#define BMA250E_PMU_BW				0X10
#define BMA250E_PMU_LPW				0X11
#define BMA250E_PMU_LOW_NOISE		0X12
#define BMA250E_ACCD_HBW			0X13
#define BMA250E_BGW_SOFTRESET		0X14
#define BMA250E_INT_EN_0			0X16
#define BMA250E_INT_EN_1			0X17
#define BMA250E_INT_EN_2			0X18
#define BMA250E_INT_MAP_0			0X19
#define BMA250E_INT_MAP_1			0X1A
#define BMA250E_INT_MAP_2			0X1B
#define BMA250E_INT_SRC				0X1E
#define BMA250E_INT_OUT_CTRL		0X20
#define BMA250E_INT_RST_LATCH		0X21
#define BMA250E_INT_0				0X22
#define BMA250E_INT_1				0X23
#define BMA250E_INT_2				0X24
#define BMA250E_INT_3				0X25
#define BMA250E_INT_4				0X26
#define BMA250E_INT_5				0X27
#define BMA250E_INT_6				0X28
#define BMA250E_INT_7				0X29
#define BMA250E_INT_8				0X2A
#define BMA250E_INT_9				0X2B
#define BMA250E_INT_A				0X2C
#define BMA250E_INT_B				0X2D
#define BMA250E_INT_C				0X2E
#define BMA250E_INT_D				0X2F
#define BMA250E_FIFO_CONFIG_0		0X30
#define BMA250E_PMU_SELF_TEST		0X32
#define BMA250E_TRIM_NVM_CTRL		0X33
#define BMA250E_BGW_SPI3_WDT		0X34
#define BMA250E_OFC_CTRL			0X36
#define BMA250E_OFC_SETTING			0X37
#define BMA250E_OFC_OFFSET_X		0X38
#define BMA250E_OFC_OFFSET_Y		0X39
#define BMA250E_OFC_OFFSET_Z		0X3A
#define BMA250E_TRIM_GP0			0X3B
#define BMA250E_TRIM_GP1			0X3C
#define BMA250E_FIFO_CONFIG_1		0X3E
#define BMA250E_FIFO_DATA			0X3F

/* ---------------------------------------------------------------------------------------- *
   Input device interface
 * ---------------------------------------------------------------------------------------- */

static int bma250e_type_setting(bool open)
{
	int rc = 0;
	queueIndex = ignoreCount = 0;
	memset(queueData, 0, sizeof(queueData));
	if(open){
		//Setting Operation mode
		rc |= i2c_smbus_write_byte_data(this_client, BMA250E_PMU_LPW, POWER_MODE_CAMMAND);
		//Setting bandwith
		rc |= i2c_smbus_write_byte_data(this_client, BMA250E_PMU_BW, BMA250E_PMU_BW_CAMMAND);
	}else{
		//Setting Suspend mode
		rc = i2c_smbus_write_byte_data(this_client, BMA250E_PMU_LPW, 0X80);
	}
	return rc;
}

static int bma250e_enable(void)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("BMA250E: %s ++\n", __func__);
	#endif

	if(data->enabled == false){
		//enable sensor
		bma250e_type_setting(true);
		mutex_lock(&data->mutex);
		data->enabled = true;
		mutex_unlock(&data->mutex);	
		rc = 1;
	}

	rc = (rc == 1) ? queue_delayed_work(Accelerometer_WorkQueue, &data->dw, msecs_to_jiffies(20)) : -1;
	return 0;
}

static int bma250e_disable(void)
{
	Accelerometer* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("BMA250E: %s\n", __func__);
	#endif

	if(data->enabled == true){
		mutex_lock(&data->mutex);
		data->enabled = false;
		mutex_unlock(&data->mutex);
		cancel_delayed_work_sync(&data->dw);
		flush_workqueue(Accelerometer_WorkQueue);

		//Setting Suspend mode
		bma250e_type_setting(false);
		WorkMode = NORMAL_MODE;
		SleepTime = msecs_to_jiffies(200);
		POWER_MODE_CAMMAND = 0X4C;//Sleep duration 1ms
		BMA250E_PMU_BW_CAMMAND = 0X0B;//Setting Bandwith 62.5 HZ, updated time 8ms
	}

	return 0;
}

static int bma250e_open(struct inode *inode, struct file *file)
{
	int rc = 0;
	#if debug
	pr_info("BMA250E: %s\n", __func__);
	#endif

	mutex_lock(&Bma250e_global_lock);
	if(Accelerometer_sensor_opened){
		pr_err("%s: already opened\n", __func__);
		rc = -EBUSY;
	}
	Accelerometer_sensor_opened = 1;
	mutex_unlock(&Bma250e_global_lock);

	return rc;
}

static int bma250e_release(struct inode *inode, struct file *file)
{
	#if debug
	pr_info("BMA250E: %s\n", __func__);
	#endif
	mutex_lock(&Bma250e_global_lock);
	Accelerometer_sensor_opened = 0;
	mutex_unlock(&Bma250e_global_lock);
	return 0;
}

static long bma250e_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("%s cmd:%d, arg:%ld\n", __func__, _IOC_NR(cmd), arg);
	#endif

	mutex_lock(&Bma250e_global_lock);
	switch(cmd){
		case ACCELEROMETER_IOCTL_SET_STATE:
			rc = arg ? bma250e_enable() : bma250e_disable();
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
				.vendor		= "Bosch Sensortec",
				.maxRange	= 8,// 8G
				.resolution	= 512,// 8G / 512
				.power		= 130,// uA
			};
			rc = copy_to_user((unsigned long __user *)arg, (char *)&(infor), sizeof(struct device_infor));
			break;
		}
		case ACCELEROMETER_IOCTL_SET_DELAY:
		{
			arg = (arg >= 10) ? arg : 10;
			SleepTime = msecs_to_jiffies(--arg);// To makeure timer is exactly.
			POWER_MODE_CAMMAND = 0X4C;//lower power enable, sleep duration 1ms
			BMA250E_PMU_BW_CAMMAND = 0X0B;//Setting Bandwith 62.5 HZ, updated time 8ms
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
	mutex_unlock(&Bma250e_global_lock);

	return rc;
}

static struct file_operations bma250e_fops = {
	.owner = THIS_MODULE,
	.open = bma250e_open,
	.release = bma250e_release,
	.unlocked_ioctl = bma250e_ioctl
};

struct miscdevice bma250e_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "accelerometer",
	.fops = &bma250e_fops
};

// Return true if data ready to report
static bool bma250e_readData(void){
	memset(i2cData, 0, sizeof(i2cData));
	memset(&rawData, 0, sizeof(AccelerometerData));
	i2c_smbus_read_i2c_block_data(this_client, BMA250E_ACCD_X_LSB, 6, &i2cData[0]);
	rawData.Y = (i2cData[1] < 128) ? i2cData[1] : (i2cData[1] - 256);
	rawData.Y = ((rawData.Y << 2) + (i2cData[0] >> 6)) * 10000 >> 6;
	rawData.X = (i2cData[3] < 128) ? i2cData[3] : (i2cData[3] - 256);
	rawData.X = ((rawData.X << 2) + (i2cData[2] >> 6)) * 10000 >> 6;
	rawData.Z = (i2cData[5] < 128) ? i2cData[5] : (i2cData[5] - 256);
	rawData.Z = ((rawData.Z << 2) + (i2cData[4] >> 6)) * 10000 >> 6;
	memcpy(&(queueData[queueIndex]), &rawData, sizeof(AccelerometerData));
	queueIndex = (queueIndex < FILTER_INDEX) ? queueIndex + 1 : 0;
	ignoreCount = (ignoreCount < FILTER_SIZE) ? ignoreCount + 1 : FILTER_SIZE;
	return (ignoreCount == FILTER_SIZE);
}

static void bma250e_reportData(Accelerometer* data){
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
	input_report_abs(data->input, ABS_X, data->sdata.X - data->odata.X);
	input_report_abs(data->input, ABS_Y, (0 - data->sdata.Y) - data->odata.Y);
	input_report_abs(data->input, ABS_Z, data->sdata.Z - data->odata.Z);
	input_sync(data->input);
}

static void bma250e_work_func(struct work_struct *work)
{
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("BMA250E: %s ++\n", __func__);
	#endif

	if(data->enabled && !data->suspend){
		(bma250e_readData()) ? bma250e_reportData(data) : 0;
		#if debug
		printk("BMA250E: ACCELEROMETER X: %d, Y: %d, Z: %d\n", data->sdata.X / 1000, data->sdata.Y  / 1000, data->sdata.Z  / 1000);
		#endif
	}

	if(data->enabled && !data->suspend){
		queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime);
	}

	#if debug
	pr_info("BMA250E: %s --\n", __func__);
	#endif
}

static int bma250e_suspend(struct i2c_client *client, pm_message_t state)
{
	Accelerometer* data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = true;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("BMA250E: %s++\n", __func__);
	#endif

	if(data->enabled){
		//Setting Operation mode
		bma250e_type_setting(false);
	}

	#if debug
	pr_info("BMA250E: %s--\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int bma250e_resume(struct i2c_client *client)
{
	Accelerometer *data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = false;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("BMA250E: %s ++\n", __func__);
	#endif

	if(data->enabled){
		bma250e_type_setting(true);
		queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime);
	}

	#if debug
	pr_info("BMA250E: %s --\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int bma250e_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	Accelerometer* Sensor_device = NULL;
	struct input_dev* input_dev = NULL;
	int err = 0;

	#if debug
	pr_info("BMA250E: %s ++\n", __func__);
	#endif

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)){
		return -EIO;
	}

	if((err = i2c_smbus_read_byte_data(client, BMA250E_BGW_CHIPID)) != 0XF9){
		return -ENODEV;
	}
	#if debug
	printk(KERN_INFO"BMA250E_BGW_CHIPID value = %d\n", err);
	#endif

	Sensor_device = kzalloc(sizeof(Accelerometer), GFP_KERNEL);

	input_dev = input_allocate_device();

	if(!Sensor_device || !input_dev){
		err = -ENOMEM;
		goto err_free_mem;
	}

	INIT_DELAYED_WORK(&Sensor_device->dw, bma250e_work_func);
	i2c_set_clientdata(client, Sensor_device);

	input_dev->name = "accelerometer";
	input_dev->id.bustype = BUS_I2C;

	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_X, -5120000, 5120000, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, -5120000, 5120000, 0, 0);
	input_set_abs_params(input_dev, ABS_Z, -5120000, 5120000, 0, 0);
	input_set_drvdata(input_dev, Sensor_device);	

	err = input_register_device(input_dev);
	if(err){
		pr_err("BMA250E: input_register_device error\n");
		goto err_free_mem;
	}

	//Setting Range +/- 8G
	i2c_smbus_write_byte_data(client, BMA250E_PMU_RANGE, 0X08);
	//Setting Suspend mode
	i2c_smbus_write_byte_data(client, BMA250E_PMU_LPW, 0X80);

	err = misc_register(&bma250e_misc);
    if(err < 0){
		pr_err("BMA250E: sensor_probe: Unable to register misc device: %s\n", input_dev->name);
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

	POWER_MODE_CAMMAND = 0X4C;//lower power enable, sleep duration 1ms
	BMA250E_PMU_BW_CAMMAND = 0X0B;//Setting Bandwith 62.5 HZ, updated time 8ms
	WorkMode = NORMAL_MODE;

	#if debug
	pr_info("BMA250E: %s --\n", __func__);
	#endif
	accelerometer_readAxisOffset();

	return 0;

	err:
		misc_deregister(&bma250e_misc);
	err_free_mem:
		input_free_device(input_dev);
		kfree(Sensor_device);
	return err;
}

static int bma250e_remove(struct i2c_client *client)
{
	Accelerometer* data = i2c_get_clientdata(client);

	destroy_workqueue(Accelerometer_WorkQueue);
	input_unregister_device(data->input);
	misc_deregister(&bma250e_misc);
	kfree(data);

	return 0;
}

static void bma250e_shutdown(struct i2c_client *client)
{
	bma250e_disable();
}

static struct i2c_device_id bma250e_idtable[] = {
	{"bma250e", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma250e_idtable);

static struct i2c_driver bma250e_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= BMA250E_DRIVER_NAME
	},
	.id_table	= bma250e_idtable,
	.probe		= bma250e_probe,
	.remove		= bma250e_remove,
	.suspend  	= bma250e_suspend,
	.resume   	= bma250e_resume,
	.shutdown	= bma250e_shutdown,
};

static int __init bma250e_init(void)
{
	return i2c_add_driver(&bma250e_driver);
}

static void __exit bma250e_exit(void)
{
	i2c_del_driver(&bma250e_driver);
}

module_init(bma250e_init);
module_exit(bma250e_exit);

MODULE_AUTHOR("HuizeWeng@Arimacomm");
MODULE_DESCRIPTION("Accelerometer Sensor BMA250E");
MODULE_LICENSE("GPLv2");
