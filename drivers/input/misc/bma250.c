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

#undef ACCELEROMETER_IRQ_USED
#define ACCELEROMETER_IRQ_USED

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
#include <linux/log2.h>

#define BMA250_DRIVER_NAME "bma250"
#define BMA250_INTERRUPT 28

static u8 BMA250_PMU_BW_CAMMAND;
static unsigned long originalTime;

DEFINE_MUTEX(Bma250_global_lock);

#define	BMA250_BGW_CHIPID		0X00
#define BMA250_ACCD_X_LSB		0X02
#define BMA250_ACCD_X_MSB		0X03
#define BMA250_ACCD_Y_LSB		0X04
#define BMA250_ACCD_Y_MSB		0X05
#define BMA250_ACCD_Z_LSB		0X06
#define BMA250_ACCD_Z_MSB		0X07
#define BMA250_RESERVED			0X08
#define BMA250_INT_STATUS_0		0X09
#define BMA250_INT_STATUS_1		0X0A
#define BMA250_INT_STATUS_2		0X0B
#define BMA250_INT_STATUS_3		0X0C
#define BMA250_FIFO_STATUS		0X0E
#define BMA250_PMU_RANGE		0X0F
#define BMA250_PMU_BW			0X10
#define BMA250_PMU_LPW			0X11
#define BMA250_PMU_LOW_NOISE	0X12
#define BMA250_ACCD_HBW			0X13
#define BMA250_BGW_SOFTRESET	0X14
#define BMA250_INT_EN_0			0X16
#define BMA250_INT_EN_1			0X17
#define BMA250_INT_EN_2			0X18
#define BMA250_INT_MAP_0		0X19
#define BMA250_INT_MAP_1		0X1A
#define BMA250_INT_MAP_2		0X1B
#define BMA250_INT_SRC			0X1E
#define BMA250_INT_OUT_CTRL		0X20
#define BMA250_INT_RST_LATCH	0X21
#define BMA250_INT_0			0X22
#define BMA250_INT_1			0X23
#define BMA250_INT_2			0X24
#define BMA250_INT_3			0X25
#define BMA250_INT_4			0X26
#define BMA250_INT_5			0X27
#define BMA250_INT_6			0X28
#define BMA250_INT_7			0X29
#define BMA250_INT_8			0X2A
#define BMA250_INT_9			0X2B
#define BMA250_INT_A			0X2C
#define BMA250_INT_B			0X2D
#define BMA250_INT_C			0X2E
#define BMA250_INT_D			0X2F
#define BMA250_FIFO_CONFIG_0	0X30
#define BMA250_PMU_SELF_TEST	0X32
#define BMA250_TRIM_NVM_CTRL	0X33
#define BMA250_BGW_SPI3_WDT		0X34
#define BMA250_OFC_CTRL			0X36
#define BMA250_OFC_SETTING		0X37
#define BMA250_OFC_OFFSET_X		0X38
#define BMA250_OFC_OFFSET_Y		0X39
#define BMA250_OFC_OFFSET_Z		0X3A
#define BMA250_TRIM_GP0			0X3B
#define BMA250_TRIM_GP1			0X3C
#define BMA250_FIFO_CONFIG_1	0X3E
#define BMA250_FIFO_DATA		0X3F

/* ---------------------------------------------------------------------------------------- *
   Input device interface
 * ---------------------------------------------------------------------------------------- */

#ifdef ACCELEROMETER_IRQ_USED
static irqreturn_t bma250_irq(int irq, void* handle)
{
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("%s ++\n", __func__);
	#endif
	disable_irq_nosync(data->irq);
	if(WorkMode == SPECIAL_MODE){
		queueIndex = ignoreCount = 0;
		memset(queueData, 0, sizeof(queueData));
	}
	queue_delayed_work(Accelerometer_WorkQueue, &data->dw, 0);
	#if debug
	pr_info("%s --\n", __func__);
	#endif

	return IRQ_HANDLED;
}

static void bma250_interrupt_release(Accelerometer* data)
{
	if(data->irq != -1){
		mutex_lock(&data->mutex);
		free_irq(data->irq, data);
		data->irq = -1;
		mutex_unlock(&data->mutex);
		gpio_free(BMA250_INTERRUPT);
	}
}

static bool bma250_interrupt_request(Accelerometer* data)
{
	gpio_tlmm_config(GPIO_CFG(BMA250_INTERRUPT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if(gpio_request(BMA250_INTERRUPT, "bma250_gpio_request") != 0){
		printk(KERN_ERR"BMA250_SENSOR: failed to request gpio\n");
		return false;
	}
	if(gpio_direction_input(BMA250_INTERRUPT) < 0){
		printk(KERN_ERR"BMA250_SENSOR: failed to config the interrupt pin\n");
		gpio_free(BMA250_INTERRUPT);
		return false;
	}

	mutex_lock(&data->mutex);
	WorkMode = INTERRUPT_MODE;
	data->irq = gpio_to_irq(BMA250_INTERRUPT);
	if(request_irq(data->irq, bma250_irq, IRQF_TRIGGER_RISING, "BMA250_SENSOR", data) < 0){
		printk(KERN_ERR"BMA250_SENSOR: irq %d busy\n?", data->irq);
		WorkMode = NORMAL_MODE;
		data->irq = -1;
		free_irq(data->irq, data);
	}
	mutex_unlock(&data->mutex);

	if(data->irq == -1) return false;

	#if debug
	pr_info("BMA250: BMA250_irq:request_irq finished\n");
	#endif
	return true;
}

static void bma250_interrupt_duration(unsigned long arg)
{
	switch(ilog2(arg))
	{
		case 3:
			POWER_MODE_CAMMAND = (arg > 10) ? 0X52 : 0X4C;
			BMA250_PMU_BW_CAMMAND = 0X0B;//Setting Bandwith 62.5 HZ, updated time 8ms
		break;
		case 4:
			POWER_MODE_CAMMAND = (arg > 24) ? 0X54 : 0X4C;
			BMA250_PMU_BW_CAMMAND = 0X0A;//Setting Bandwith 31.25 HZ, updated time 16ms
		break;
		case 5:
			POWER_MODE_CAMMAND = (arg > 48) ? 0X56 : 0X4C;
			BMA250_PMU_BW_CAMMAND = 0X09;//Setting Bandwith 15.63 HZ, updated time 32ms
		break;
		case 6:
			POWER_MODE_CAMMAND = 0X00;
			BMA250_PMU_BW_CAMMAND = 0X08;//Setting Bandwith 7.81 HZ, updated time 64ms
		break;
		case 14:
			WorkMode = SPECIAL_MODE;
			POWER_MODE_CAMMAND = 0X00;
			BMA250_PMU_BW_CAMMAND = 0X0A;//Setting Bandwith 31.25 HZ, updated time 16ms
		break;	
		default :
			WorkMode = NORMAL_MODE;
			bma250_interrupt_release(i2c_get_clientdata(this_client));
		break;
	}
}

static int bma250_irqSetting(u8 en0, u8 en1, u8 map0, u8 map1)
{
	int rc = 0;
	if(WorkMode == NORMAL_MODE) return rc;
	rc |= i2c_smbus_write_byte_data(this_client, BMA250_INT_EN_0, en0);
	rc |= i2c_smbus_write_byte_data(this_client, BMA250_INT_EN_1, en1);
	rc |= i2c_smbus_write_byte_data(this_client, BMA250_INT_MAP_0, map0);
	rc |= i2c_smbus_write_byte_data(this_client, BMA250_INT_MAP_1, map1);
	//Reset Interrupt
	rc |= i2c_smbus_write_byte_data(this_client, BMA250_INT_RST_LATCH, 0X10);
	return rc;
}
#endif

static int bma250_type_setting(bool open)
{
	int rc = 0;
	queueIndex = ignoreCount = 0;
	memset(queueData, 0, sizeof(queueData));
	if(open){
		#ifdef ACCELEROMETER_IRQ_USED
		u8 irq_en[2] = {0, 0};
		u8 irq_map[2] = {0, 0};
		#endif
		//Setting Operation mode
		rc |= i2c_smbus_write_byte_data(this_client, BMA250_PMU_LPW, POWER_MODE_CAMMAND);
		//Setting bandwith
		rc |= i2c_smbus_write_byte_data(this_client, BMA250_PMU_BW, BMA250_PMU_BW_CAMMAND);
		#ifdef ACCELEROMETER_IRQ_USED
		switch(WorkMode){
			case SPECIAL_MODE:
				irq_en[0] = 0X47;
				irq_map[0] = 0X44;
				break;
			case INTERRUPT_MODE:
				irq_en[1] = 0X10;
				irq_map[1] = 0X01;
				break;
		}
		rc |= bma250_irqSetting(irq_en[0], irq_en[1], irq_map[0], irq_map[1]);
		rc |= i2c_smbus_write_byte_data(this_client, BMA250_INT_6, (WorkMode != SPECIAL_MODE) ? 0X14 : 0X02);
		#endif
	}else{
		#ifdef ACCELEROMETER_IRQ_USED
		rc |= bma250_irqSetting(0, 0, 0, 0);
		#endif
		//Setting Suspend mode
		rc |= i2c_smbus_write_byte_data(this_client, BMA250_PMU_LPW, 0X80);
	}
	return rc;
}

static int bma250_enable(void)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("BMA250: %s ++\n", __func__);
	#endif

	if(data->enabled == false){
		//enable sensor
		bma250_type_setting(true);
		mutex_lock(&data->mutex);
		data->enabled = true;
		mutex_unlock(&data->mutex);	
		rc = 1;
	}

	rc = (rc == 1 && WorkMode == NORMAL_MODE) ? queue_delayed_work(Accelerometer_WorkQueue, &data->dw, msecs_to_jiffies(20)) : -1;
	return 0;
}

static int bma250_disable(void)
{
	Accelerometer* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("BMA250: %s \n", __func__);
	#endif

	if(data->enabled == true){
		mutex_lock(&data->mutex);
		data->enabled = false;
		mutex_unlock(&data->mutex);
		cancel_delayed_work_sync(&data->dw);
		flush_workqueue(Accelerometer_WorkQueue);

		#ifdef ACCELEROMETER_IRQ_USED
		bma250_interrupt_release(data);
		#endif

		//Setting Suspend mode
		bma250_type_setting(false);
		WorkMode = NORMAL_MODE;
		SleepTime = msecs_to_jiffies(200);
		originalTime = 200;
		POWER_MODE_CAMMAND = 0X4C;//Sleep duration 1ms
		BMA250_PMU_BW_CAMMAND = 0X0B;//Setting Bandwith 62.5 HZ, updated time 8ms
	}

	return 0;
}

static int bma250_open(struct inode *inode, struct file *file)
{
	int rc = 0;
	#if debug
	pr_info("BMA250: %s\n", __func__);
	#endif

	mutex_lock(&Bma250_global_lock);
	if(Accelerometer_sensor_opened){
		pr_err("%s: already opened\n", __func__);
		rc = -EBUSY;
	}
	Accelerometer_sensor_opened = 1;
	mutex_unlock(&Bma250_global_lock);

	return rc;
}

static int bma250_release(struct inode *inode, struct file *file)
{
	#if debug
	pr_info("BMA250: %s\n", __func__);
	#endif
	mutex_lock(&Bma250_global_lock);
	Accelerometer_sensor_opened = 0;
	mutex_unlock(&Bma250_global_lock);
	return 0;
}

static void bma250_commandSet(Accelerometer* data, unsigned long time)
{
	#ifdef ACCELEROMETER_IRQ_USED
	if((time == 16384 || time < 70) && bma250_interrupt_request(data) == true){
		bma250_interrupt_duration(time);
	}
	#endif
	if(WorkMode == NORMAL_MODE){
		POWER_MODE_CAMMAND = 0X4C;//lower power enable, sleep duration 1ms
		BMA250_PMU_BW_CAMMAND = 0X0B;//Setting Bandwith 62.5 HZ, updated time 8ms
	}
	SleepTime = msecs_to_jiffies(time);
}

static long bma250_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("%s cmd:%d, arg:%ld\n", __func__, _IOC_NR(cmd), arg);
	#endif

	mutex_lock(&Bma250_global_lock);
	switch(cmd){
		case ACCELEROMETER_IOCTL_SET_STATE:
			rc = arg ? bma250_enable() : bma250_disable();
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
				.power		= 139,// uA
			};
			rc = copy_to_user((unsigned long __user *)arg, (char *)&(infor), sizeof(struct device_infor));
			break;
		}
		case ACCELEROMETER_IOCTL_SET_DELAY:
		{
			bool enabled = data->enabled;
			arg = (arg >= 10) ? arg : 10;
			(data->enabled) ? bma250_disable() : 0;
			originalTime = arg;
			bma250_commandSet(data, arg);
			(enabled) ? bma250_enable() : 0;
	
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
	mutex_unlock(&Bma250_global_lock);

	return rc;
}

static struct file_operations bma250_fops = {
	.owner = THIS_MODULE,
	.open = bma250_open,
	.release = bma250_release,
	.unlocked_ioctl = bma250_ioctl
};

struct miscdevice bma250_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "accelerometer",
	.fops = &bma250_fops
};

static void bma250_swReset(bool reStart, u16 gRange){
	pr_err("%s: reStart: %d, gRange: %d\n", __func__, reStart, gRange);
	bma250_type_setting(false);
	accelerometer_readAxisOffset();
	i2c_smbus_write_byte_data(this_client, BMA250_BGW_SOFTRESET, 0XB6);
	msleep(20);
	//Setting Range
	i2c_smbus_write_byte_data(this_client, BMA250_PMU_RANGE, gRange);
	msleep(10);
	bma250_type_setting(reStart);
}

// Return true if data ready to report
static bool bma250_readData(void){
	memset(i2cData, 0, sizeof(i2cData));
	memset(&rawData, 0, sizeof(AccelerometerData));
	if(i2c_smbus_read_i2c_block_data(this_client, BMA250_ACCD_X_LSB, 6, &i2cData[0]) != 6){
		bma250_swReset(true, 0x08);
		return false;
	}
	rawData.Y = (i2cData[1] < 128) ? i2cData[1] : (i2cData[1] - 256);
	rawData.Y = ((rawData.Y << 2) + (i2cData[0] >> 6)) * 10000 >> 6;
	rawData.X = (i2cData[3] < 128) ? i2cData[3] : (i2cData[3] - 256);
	rawData.X = ((rawData.X << 2) + (i2cData[2] >> 6)) * 10000 >> 6;
	rawData.Z = (i2cData[5] < 128) ? i2cData[5] : (i2cData[5] - 256);
	rawData.Z = ((rawData.Z << 2) + (i2cData[4] >> 6)) * 10000 >> 6;
	memcpy(&(queueData[queueIndex]), &rawData, sizeof(AccelerometerData));
	queueIndex = (queueIndex < FILTER_INDEX) ? queueIndex + 1 : 0;
	ignoreCount = (ignoreCount < FILTER_SIZE) ? ignoreCount + 1 : FILTER_SIZE;
	#ifdef ACCELEROMETER_IRQ_USED
	if(WorkMode == SPECIAL_MODE){
		while(queueIndex <= FILTER_INDEX){
			memcpy(&(queueData[queueIndex]), &rawData, sizeof(AccelerometerData));
			++queueIndex;
		}	
		ignoreCount = FILTER_SIZE;
		queueIndex = 0;
	}
	#endif
	return (ignoreCount == FILTER_SIZE);
}

static void bma250_reportData(Accelerometer* data){
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

static void bma250_work_func(struct work_struct *work)
{
	Accelerometer* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("BMA250: %s ++\n", __func__);
	#endif

	if(data->enabled && !data->suspend){
		(bma250_readData()) ? bma250_reportData(data) : 0;
		#if debug
		pr_err("BMA250: ACCELEROMETER X: %d, Y: %d, Z: %d\n", data->sdata.X / 1000, data->sdata.Y  / 1000, data->sdata.Z  / 1000);
		#endif
	}

	switch(WorkMode){
		case NORMAL_MODE:
			(data->enabled && !data->suspend) ? 
			queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime) : -1;
			break;
		#ifdef ACCELEROMETER_IRQ_USED
		case SPECIAL_MODE:
		case INTERRUPT_MODE:
			enable_irq(data->irq);
			break;
		#endif
	}

	#if debug
	pr_info("BMA250: %s --\n", __func__);
	#endif
}

static int bma250_suspend(struct i2c_client *client, pm_message_t state)
{
	Accelerometer* data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = true;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("BMA250: %s ++\n", __func__);
	#endif

	if(data->enabled){
		//Setting Operation mode
		bma250_type_setting(false);
		#ifdef ACCELEROMETER_IRQ_USED
		if(WorkMode != NORMAL_MODE){
			bma250_interrupt_release(data);
		}
		#endif
	}

	#if debug
	pr_info("BMA250: %s--\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int bma250_resume(struct i2c_client *client)
{
	Accelerometer *data = i2c_get_clientdata(client);
	mutex_lock(&data->mutex);
	data->suspend = false;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Accelerometer_WorkQueue);

	#if debug
	pr_info("BMA250: %s ++\n", __func__);
	#endif

	if(data->enabled){
		bma250_commandSet(data, originalTime);
		bma250_type_setting(true);
		(WorkMode == NORMAL_MODE) ? queue_delayed_work(Accelerometer_WorkQueue, &data->dw, SleepTime) : -1;
	}

	#if debug
	pr_info("BMA250: %s--\n", __func__);
	#endif
	return 0;// It's need to return 0, non-zero means has fault.
}

static int bma250_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	Accelerometer* Sensor_device = NULL;
	struct input_dev* input_dev = NULL;
	int err = 0;

	#if debug
	pr_info("BMA250: %s ++\n", __func__);
	#endif

	msleep(5);

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)){
		return -EIO;
	}

	if((err = i2c_smbus_read_byte_data(client, BMA250_BGW_CHIPID)) != 0X03){
		return -ENODEV;
	}
	#if debug
	printk(KERN_INFO"BMA250_BGW_CHIPID value = %d\n", err);
	#endif

	Sensor_device = kzalloc(sizeof(Accelerometer), GFP_KERNEL);

	input_dev = input_allocate_device();

	if(!Sensor_device || !input_dev){
		err = -ENOMEM;
		goto err_free_mem;
	}

	INIT_DELAYED_WORK(&Sensor_device->dw, bma250_work_func);
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
		pr_err("BMA250: input_register_device error\n");
		goto err_free_mem;
	}

	//Setting Range +/- 8G
	i2c_smbus_write_byte_data(client, BMA250_PMU_RANGE, 0X08);
	//Setting Suspend mode
	i2c_smbus_write_byte_data(client, BMA250_PMU_LPW, 0X80);

	err = misc_register(&bma250_misc);
    if(err < 0){
		pr_err("BMA250: sensor_probe: Unable to register misc device: %s\n", input_dev->name);
		goto err;
	}

	#ifdef ACCELEROMETER_IRQ_USED
	Sensor_device->irq		= -1;
	#endif
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
	originalTime = 200;

	POWER_MODE_CAMMAND = 0X4C;//lower power enable, sleep duration 1ms
	BMA250_PMU_BW_CAMMAND = 0X0B;//Setting Bandwith 62.5 HZ, updated time 8ms
	WorkMode = NORMAL_MODE;

	#if debug
	pr_info("BMA250: %s --\n", __func__);
	#endif
	accelerometer_readAxisOffset();

	return 0;

	err:
		misc_deregister(&bma250_misc);
	err_free_mem:
		input_free_device(input_dev);
		kfree(Sensor_device);
	return err;
}

static int bma250_remove(struct i2c_client *client)
{
	Accelerometer* data = i2c_get_clientdata(client);

	destroy_workqueue(Accelerometer_WorkQueue);
	#ifdef ACCELEROMETER_IRQ_USED
	bma250_interrupt_release(data);
	#endif
	input_unregister_device(data->input);
	misc_deregister(&bma250_misc);
	kfree(data);

	return 0;
}

static void bma250_shutdown(struct i2c_client *client)
{
	bma250_disable();
}

static struct i2c_device_id bma250_idtable[] = {
	{"bma250", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma250_idtable);

static struct i2c_driver bma250_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= BMA250_DRIVER_NAME
	},
	.id_table	= bma250_idtable,
	.probe		= bma250_probe,
	.remove		= bma250_remove,
	.suspend  	= bma250_suspend,
	.resume   	= bma250_resume,
	.shutdown	= bma250_shutdown,
};

static int __init bma250_init(void)
{
	return i2c_add_driver(&bma250_driver);
}

static void __exit bma250_exit(void)
{
	i2c_del_driver(&bma250_driver);
}

module_init(bma250_init);
module_exit(bma250_exit);

MODULE_AUTHOR("HuizeWeng@Arimacomm");
MODULE_DESCRIPTION("Accelerometer Sensor BMA250");
MODULE_LICENSE("GPLv2");
