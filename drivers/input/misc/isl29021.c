/* Proximity-sensor
 *
 * Copyright (c) 2011-2014, HuizeWeng@Arimacomm Crop. All rights reserved.
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

#undef PROXIMITY_IRQ_USED
#define PROXIMITY_IRQ_USED

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
#include <linux/proximity_common.h>

#define ISL29021_INTERRUPT 17

#define ISL29021_DRIVER_NAME "isl29021"

#define THRESHOLD_LEVEL 700
#define THRESHOLD_RANGE (THRESHOLD_LEVEL << 1)

DEFINE_MUTEX(Isl29021_global_lock);

enum{
	PROXIMITY_REG_COMMANDI		= 0x00,
	PROXIMITY_REG_COMMANDII		= 0x01,
	PROXIMITY_REG_DATA_LSB		= 0x02,
	PROXIMITY_REG_DATA_MSB		= 0x03,
	PROXIMITY_REG_INT_LT_LSB	= 0x04,
	PROXIMITY_REG_INT_LT_MSB	= 0x05,
	PROXIMITY_REG_INT_HT_LSB	= 0x06,
	PROXIMITY_REG_INT_HT_MSB	= 0x07,
};

/* ---------------------------------------------------------------------------------------- *
   Input device interface
 * ---------------------------------------------------------------------------------------- */

static void isl29021_setThreshold(u16 LOW, u16 HIGH)
{
	u8 tmp[4] = {LOW & 0XFF, LOW >> 8, HIGH & 0XFF, HIGH >> 8};
	i2c_smbus_write_i2c_block_data(this_client, PROXIMITY_REG_INT_LT_LSB, 4, tmp);
}

static int isl29021_enable(void)
{
	Proximity* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("ISL29021: %s ++\n", __func__);
	#endif
	
	if(data->enabled == false){
		mutex_lock(&data->mutex);
		//enable sensor
		data->enabled = true;
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		mutex_unlock(&data->mutex);
		isl29021_setThreshold(0, 65535);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDII, 0xA0);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDI, 0xE0);
		input_report_abs(data->input, ABS_DISTANCE, PROXIMITY_UNDETECTED);
		input_sync(data->input);
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
	}

	return 0;
}

static int isl29021_disable(void)
{
	Proximity* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("ISL29021: %s\n", __func__);
	#endif

	if(data->enabled == true){
		mutex_lock(&data->mutex);
		data->enabled = false;
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		mutex_unlock(&data->mutex);
		cancel_delayed_work_sync(&data->dw);
		flush_workqueue(Proximity_WorkQueue);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDII, 0x00);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDI, 0x00);
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
	}

	return 0;
}

static int isl29021_open(struct inode* inode, struct file* file)
{
	int rc = 0;
	#if debug
	pr_info("ISL29021: %s\n", __func__);
	#endif

	mutex_lock(&Isl29021_global_lock);
	if(Proxmity_sensor_opened){
		pr_err("%s: already opened\n", __func__);
		rc = -EBUSY;
	}
	Proxmity_sensor_opened = 1;
	mutex_unlock(&Isl29021_global_lock);
	
	return rc;
}

static int isl29021_release(struct inode* inode, struct file* file)
{
	#if debug
	pr_info("ISL29021: %s\n", __func__);
	#endif
	mutex_lock(&Isl29021_global_lock);
	Proxmity_sensor_opened = 0;
	mutex_unlock(&Isl29021_global_lock);
	return 0;
}

static ssize_t isl29021_read(struct file* f, char* str, size_t length, loff_t* f_pos)
{
	Proximity* data = i2c_get_clientdata(this_client);
	ssize_t error = 0L;
	#if debug
	pr_info("ISL29021: %s\n", __func__);
	#endif

	mutex_lock(&Isl29021_global_lock);
	if(data->enabled && !data->suspend){
		error = copy_to_user(str, (char *)&(data->sdata), sizeof(ProximityInfor));
	}else{
		ProximityInfor pi = {
			.Value			= -1,
			.State			= PROXIMITY_STATE_UNKNOWN,
			.Threshold_L	= DEFAULT_THRESHOLD,
			.Threshold_H	= DEFAULT_THRESHOLD,
		};
		error = copy_to_user(str, &pi, sizeof(ProximityInfor));
	}
	mutex_unlock(&Isl29021_global_lock);

	return error;
}

static ssize_t isl29021_write(struct file* f, const char* str, size_t length, loff_t* f_pos)
{
	Proximity* data = i2c_get_clientdata(this_client);
	ssize_t error = 0L;
	#if debug
	pr_info("ISL29021: %s\n", __func__);
	#endif

	mutex_lock(&Isl29021_global_lock);
	if(data->enabled && !data->suspend){
		int result = -1;
		error = copy_from_user(&result, str, sizeof(int));
		error = proximity_resetThreshold(result + THRESHOLD_LEVEL, THRESHOLD_LEVEL);
	}else{
		error = -1;
	}
	mutex_unlock(&Isl29021_global_lock);

	return error;
}

static long isl29021_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	#if debug
	pr_info("%s cmd:%d, arg:%ld\n", __func__, _IOC_NR(cmd), arg);
	#endif

	mutex_lock(&Isl29021_global_lock);
	switch(cmd){
		case PROXIMITY_IOCTL_SET_STATE:
			rc = arg ? isl29021_enable() : isl29021_disable();
			break;
		case PROXIMITY_IOCTL_GET_DEVICE_INFOR:
		{
			struct device_infor infor = {
				.name		= "Proximity Sensor",
				.vendor		= "Intersil Americas Inc.",
				.maxRange	= PROXIMITY_UNDETECTED,
				.resolution	= 2500,//
				.power		= 100,// uA
			};
			rc = copy_to_user((unsigned long __user *)arg, (char *)&(infor), sizeof(struct device_infor));
			break;
		}
		default:
			pr_err("%s: invalid cmd %d\n", __func__, _IOC_NR(cmd));
			rc = -EINVAL;
	}
	mutex_unlock(&Isl29021_global_lock);

	return rc;
}

static struct file_operations isl29021_fops = {
	.owner			= THIS_MODULE,
	.read			= isl29021_read,
	.write			= isl29021_write,
	.open			= isl29021_open,
	.release		= isl29021_release,
	.unlocked_ioctl = isl29021_ioctl
};

struct miscdevice isl29021_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "proximity",
	.fops	= &isl29021_fops
};

static irqreturn_t isl29021_irq(int irq, void* handle)
{
	Proximity* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("%s ++\n", __func__);
	#endif
	disable_irq_nosync(data->irq);
	queue_delayed_work(Proximity_WorkQueue, &data->dw, 0);
	#if debug
	pr_info("%s --\n", __func__);
	#endif

	return IRQ_HANDLED;
}

static int isl29021_reportData(Proximity* data){
	u8 State = PROXIMITY_STATE_UNKNOWN;
	int Value = (i2cData[1] << 8) | i2cData[0];

	int Threshold = (data->sdata.State != PROXIMITY_STATE_TRUE) ? data->sdata.Threshold_H : data->sdata.Threshold_L;
	State = (Value > Threshold) ? PROXIMITY_STATE_TRUE : PROXIMITY_STATE_FALSE;

	mutex_lock(&data->mutex);
	data->sdata.Value = Value;
	if(State != data->sdata.State){
		if(need2ChangeState == true){
			data->sdata.State = State;
			input_report_abs(data->input, ABS_DISTANCE, (State) ? PROXIMITY_DETECTED : PROXIMITY_UNDETECTED);
			input_sync(data->input);
		}
		need2ChangeState = !need2ChangeState;
	}else{
		need2ChangeState = false;
	}
	mutex_unlock(&data->mutex);

	#if debug
	pr_info("ISL29021: Proximity : %d\n", Value);
	#endif
	return Value;
}

static void isl29021_autoCalibrate(Proximity* data, int Value){
	// Auto-calibrate
	if(!i2cIsFine || !stateIsFine || Value < 0 || ((Value + THRESHOLD_RANGE) > data->sdata.Threshold_H)){
		need2Reset = false;
	}else{
		(need2Reset) ? proximity_resetThreshold(Value + THRESHOLD_LEVEL, THRESHOLD_LEVEL) : 0;
		need2Reset = !need2Reset;
	}
}

static void isl29021_work_func(struct work_struct* work)
{
	int Value = 0;
	Proximity* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("ISL29021: %s ++\n", __func__);
	#endif
	
	memset(i2cData, 0, sizeof(i2cData));
	if(data->enabled && !data->suspend && 
		(i2cIsFine = (i2c_smbus_read_i2c_block_data(this_client, PROXIMITY_REG_DATA_LSB, 2, &i2cData[0]) == 2))){
		i2c_smbus_read_byte_data(this_client, PROXIMITY_REG_COMMANDI);
		Value = isl29021_reportData(data);
		#if debug
		pr_info("ISL29021: Proximity : %d\n", Value);
		#endif
	}else if(!i2cIsFine){
		// Varify transaction, 
		// reset chip if it is failure.
		mutex_lock(&data->mutex);
		data->sdata.Value = ERROR_TRANSACTION;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		mutex_unlock(&data->mutex);
		pr_err("ISL29021: ERROR_TRANSACTION, try to reset chip \n");
	}


	isl29021_autoCalibrate(data, Value);

	// Closed sensor
	if(data->enabled && !data->suspend){
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDII, 0x00);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDI, 0x00);
		msleep(150);
	}

	// Restart Sensor, 
	// it needs to check current state for avoiding async, so lock it.
	mutex_lock(&data->mutex);
	if(data->enabled && !data->suspend){
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDII, 0xA0);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDI, 0xE0);
	}
	mutex_unlock(&data->mutex);

	enable_irq(data->irq);
	#if debug
	pr_info("ISL29021: %s --\n", __func__);
	#endif
}

static int isl29021_suspend(struct i2c_client* client, pm_message_t state)
{
	Proximity* data = i2c_get_clientdata(client);
	#if debug
	pr_info("ISL29021: %s ++\n", __func__);
	#endif

	mutex_lock(&data->mutex);
	data->suspend = true;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Proximity_WorkQueue);

	if(data->enabled){
		i2c_smbus_write_byte_data(client, PROXIMITY_REG_COMMANDII, 0x00);
		i2c_smbus_write_byte_data(client, PROXIMITY_REG_COMMANDI, 0x00);
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
	}

	#if debug
	pr_info("ISL29021: %s --\n", __func__);
	#endif
	return 0;// It needs to return 0, non-zero means has fault.
}

static int isl29021_resume(struct i2c_client* client)
{
	Proximity* data = i2c_get_clientdata(client);
	#if debug
	pr_info("ISL29021: %s ++\n", __func__);
	#endif

	mutex_lock(&data->mutex);
	data->suspend = false;
	mutex_unlock(&data->mutex);
	cancel_delayed_work_sync(&data->dw);
	flush_workqueue(Proximity_WorkQueue);

	if(data->enabled){
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDII, 0xA0);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDI, 0xE0);
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
	}

	#if debug
	pr_info("ISL29021: %s --\n", __func__);
	#endif
	return 0;// It needs to return 0, non-zero means has fault.
}

static int isl29021_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
	Proximity* Sensor_device = NULL;
	struct input_dev* input_dev = NULL;
	int err = 0;

	#if debug
	pr_info("ISL29021: %s ++\n", __func__);
	#endif

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)){
		return -EIO;
	}

	Sensor_device = kzalloc(sizeof(Proximity), GFP_KERNEL);

	input_dev = input_allocate_device();

	if(!Sensor_device || !input_dev){
		err = -ENOMEM;
		goto err_free_mem;
	}

	INIT_DELAYED_WORK(&Sensor_device->dw, isl29021_work_func);
	i2c_set_clientdata(client, Sensor_device);

	err = gpio_request(ISL29021_INTERRUPT, "isl29021_sensor_init");
	if(err){
		goto err_free_data;
	}

	err = gpio_direction_input(ISL29021_INTERRUPT);
	
	if(err < 0){
		printk(KERN_ERR"ISL29021_SENSOR: failed to config the interrupt pin\n");
	}

	Sensor_device->irq = gpio_to_irq(ISL29021_INTERRUPT);

	err = request_irq(Sensor_device->irq, isl29021_irq, IRQF_TRIGGER_LOW, "ISL29021_SENSOR", Sensor_device);

	if(err < 0){
		dev_err(&client->dev, "irq %d busy?\n", Sensor_device->irq);
		goto err_free_mem;
	}

	gpio_tlmm_config(GPIO_CFG(ISL29021_INTERRUPT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	#if debug
	pr_info("ISL29021: isl29021_irq:request_irq finished\n");
	#endif

	input_dev->name = "proximity";
	input_dev->id.bustype = BUS_I2C;

	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 65535, 0, 0);
	input_set_drvdata(input_dev, Sensor_device);	

	err = input_register_device(input_dev);
	if(err){
		pr_err("ISL29021: input_register_device error\n");
		goto err_free_irq;
	}

	i2c_smbus_write_byte_data(client, PROXIMITY_REG_COMMANDII, 0x00);
	i2c_smbus_write_byte_data(client, PROXIMITY_REG_COMMANDI, 0x00);

	err = misc_register(&isl29021_misc);
    if(err < 0){
		pr_err("ISL29021: sensor_probe: Unable to register misc device: %s\n", input_dev->name);
		goto err;
	}
	
	Sensor_device->input 				= input_dev;
	Sensor_device->enabled 				= false;
	Sensor_device->suspend				= false;
	Sensor_device->sdata.Value			= -1;
	Sensor_device->sdata.State 			= PROXIMITY_STATE_UNKNOWN;
	Sensor_device->sdata.Threshold_L 	= DEFAULT_THRESHOLD;
	Sensor_device->sdata.Threshold_H	= DEFAULT_THRESHOLD;

	mutex_init(&Sensor_device->mutex);

	Proxmity_sensor_opened = 0;
	i2cIsFine = true;
	stateIsFine = true;
	need2Reset = false;
	need2ChangeState = false;
	#if debug
	dev_info(&client->dev, "registered with irq (%d)\n", Sensor_device->irq);
	#endif

	#if debug
	pr_info("ISL29021: %s --\n", __func__);
	#endif

	this_client = client;
	Proximity_WorkQueue = create_singlethread_workqueue(input_dev->name);
	proximity_readThreshold(THRESHOLD_LEVEL);// It needs to avoid deadlock.
	SleepTime = 0;

	return 0;

 	err:
		misc_deregister(&isl29021_misc);
	err_free_irq:
		free_irq(Sensor_device->irq, Sensor_device);
 	err_free_mem:
		input_free_device(input_dev);
	err_free_data:
		kfree(Sensor_device);
	return err;
}

static int isl29021_remove(struct i2c_client* client)
{
	Proximity* data = i2c_get_clientdata(client);

	destroy_workqueue(Proximity_WorkQueue);
	free_irq(data->irq, data);
	input_unregister_device(data->input);
	misc_deregister(&isl29021_misc);
	kfree(data);

	return 0;
}

static void isl29021_shutdown(struct i2c_client* client)
{
	Proximity* data = i2c_get_clientdata(client);
	free_irq(data->irq, data);
	isl29021_disable();
}

static struct i2c_device_id isl29021_idtable[] = {
	{"isl29021", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, isl29021_idtable);

static struct i2c_driver isl29021_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= ISL29021_DRIVER_NAME
	},
	.id_table	= isl29021_idtable,
	.probe		= isl29021_probe,
	.remove		= isl29021_remove,
	.suspend  	= isl29021_suspend,
	.resume   	= isl29021_resume,
	.shutdown	= isl29021_shutdown,
};

static int __init isl29021_init(void)
{
	return i2c_add_driver(&isl29021_driver);
}

static void __exit isl29021_exit(void)
{
	i2c_del_driver(&isl29021_driver);
}

module_init(isl29021_init);
module_exit(isl29021_exit);

MODULE_AUTHOR("HuizeWeng@Arimacomm");
MODULE_DESCRIPTION("Proxmity Sensor ISL29021");
MODULE_LICENSE("GPLv2");
