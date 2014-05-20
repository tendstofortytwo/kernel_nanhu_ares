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

#define EPL6803_DRIVER_NAME "epl6803"

#define THRESHOLD_LEVEL 350
#define THRESHOLD_RANGE (THRESHOLD_LEVEL << 1)

DEFINE_MUTEX(Epl6803_global_lock);

enum{
	PROXIMITY_REG_COMMANDI			= 0x00 << 3,
	PROXIMITY_REG_COMMANDII			= 0x01 << 3,
	PROXIMITY_REG_INT_HT_LSB		= 0x02 << 3,
	PROXIMITY_REG_INT_HT_MSB		= 0x03 << 3,
	PROXIMITY_REG_INT_LT_LSB		= 0x04 << 3,
	PROXIMITY_REG_INT_LT_MSB		= 0x05 << 3,
	PROXIMITY_REG_THRESHOLD_LIMIT	= 0x06 << 3,
	PROXIMITY_REG_CONTROL_SETTINGI	= 0x07 << 3,
	PROXIMITY_REG_CONTROL_SETTINGII = 0x09 << 3,
	PROXIMITY_REG_GOMID_SETTING		= 0x0A << 3,
	PROXIMITY_REG_GOLOW_SETTING		= 0x0B << 3,
	PROXIMITY_REG_CHIP_STATE		= 0x0D << 3,
	PROXIMITY_REG_CH0DATA_LSB		= 0x0E << 3,
	PROXIMITY_REG_CH0DATA_MSB		= 0x0F << 3,
	PROXIMITY_REG_CH1DATA_LSB		= 0x10 << 3,
	PROXIMITY_REG_CH1DATA_MSB		= 0x11 << 3,
	PROXIMITY_REG_REVISIONCODE_LSB	= 0x13 << 3,
	PROXIMITY_REG_REVISIONCODE_MSB	= 0x14 << 3,
};

/* ---------------------------------------------------------------------------------------- *
   Input device interface
 * ---------------------------------------------------------------------------------------- */

static int epl6803_enable(void)
{
	int rc = 0;
	Proximity* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("EPL6803: %s ++\n", __func__);
	#endif
	
	if(data->enabled == false){
		mutex_lock(&data->mutex);
		//enable sensor
		data->enabled = true;
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		mutex_unlock(&data->mutex);
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_CONTROL_SETTINGI, 0x04);
		input_report_abs(data->input, ABS_DISTANCE, PROXIMITY_UNDETECTED);
		input_sync(data->input);
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
		rc = 1;
	}

	rc = (rc == 1) ? queue_delayed_work(Proximity_WorkQueue, &data->dw, SleepTime) : -1;
	return 0;
}

static int epl6803_disable(void)
{
	Proximity* data = i2c_get_clientdata(this_client);
	#if debug
	pr_info("EPL6803: %s \n", __func__);
	#endif

	if(data->enabled == true){
		mutex_lock(&data->mutex);
		data->enabled = false;
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		mutex_unlock(&data->mutex);
		cancel_delayed_work_sync(&data->dw);
		flush_workqueue(Proximity_WorkQueue);
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
	}

	return 0;
}

static int epl6803_open(struct inode* inode, struct file* file)
{
	int rc = 0;
	#if debug
	pr_info("EPL6803: %s\n", __func__);
	#endif

	mutex_lock(&Epl6803_global_lock);
	if(Proxmity_sensor_opened){
		pr_err("%s: already opened\n", __func__);
		rc = -EBUSY;
	}
	Proxmity_sensor_opened = 1;
	mutex_unlock(&Epl6803_global_lock);
	
	return rc;
}

static int epl6803_release(struct inode* inode, struct file* file)
{
	#if debug
	pr_info("EPL6803: %s\n", __func__);
	#endif
	mutex_lock(&Epl6803_global_lock);
	Proxmity_sensor_opened = 0;
	mutex_unlock(&Epl6803_global_lock);
	return 0;
}

static ssize_t epl6803_read(struct file* f, char* str, size_t length, loff_t* f_pos)
{
	Proximity* data = i2c_get_clientdata(this_client);
	ssize_t error = 0L;
	#if debug
	pr_info("EPL6803: %s\n", __func__);
	#endif

	mutex_lock(&Epl6803_global_lock);
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
	mutex_unlock(&Epl6803_global_lock);

	return error;
}

static ssize_t epl6803_write(struct file* f, const char* str, size_t length, loff_t* f_pos)
{
	Proximity* data = i2c_get_clientdata(this_client);
	ssize_t error = 0L;
	#if debug
	pr_info("EPL6803: %s\n", __func__);
	#endif

	mutex_lock(&Epl6803_global_lock);
	if(data->enabled && !data->suspend){
		int result = -1;
		error = copy_from_user(&result, str, sizeof(int));
		error = proximity_resetThreshold(result + THRESHOLD_LEVEL, THRESHOLD_LEVEL);
	}else{
		error = -1;
	}
	mutex_unlock(&Epl6803_global_lock);

	return error;
}

static long epl6803_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	#if debug
	pr_info("%s cmd:%d, arg:%ld\n", __func__, _IOC_NR(cmd), arg);
	#endif

	mutex_lock(&Epl6803_global_lock);
	switch(cmd){
		case PROXIMITY_IOCTL_SET_STATE:
			rc = arg ? epl6803_enable() : epl6803_disable();
			break;
		case PROXIMITY_IOCTL_GET_DEVICE_INFOR:
		{
			struct device_infor infor = {
				.name		= "Proximity Sensor",
				.vendor		= "ELAN Microelectronics Corp.",
				.maxRange	= PROXIMITY_UNDETECTED,
				.resolution	= 2500,//
				.power		= 350,// uA
			};
			rc = copy_to_user((unsigned long __user *)arg, (char *)&(infor), sizeof(struct device_infor));
			break;
		}
		default:
			pr_err("%s: invalid cmd %d\n", __func__, _IOC_NR(cmd));
			rc = -EINVAL;
	}
	mutex_unlock(&Epl6803_global_lock);

	return rc;
}

static struct file_operations epl6803_fops = {
	.owner			= THIS_MODULE,
	.read			= epl6803_read,
	.write			= epl6803_write,
	.open			= epl6803_open,
	.release		= epl6803_release,
	.unlocked_ioctl = epl6803_ioctl
};

struct miscdevice epl6803_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "proximity",
	.fops	= &epl6803_fops
};

static int epl6803_reportData(Proximity* data){
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
	pr_info("EPL6803: Proximity : %d\n", Value);
	#endif
	return Value;
}

static void epl6803_autoCalibrate(Proximity* data, int Value){
	// Auto-calibrate
	if(!i2cIsFine || !stateIsFine || Value < 0 || ((Value + THRESHOLD_RANGE) > data->sdata.Threshold_H)){
		need2Reset = false;
	}else{
		(need2Reset) ? proximity_resetThreshold(Value + THRESHOLD_LEVEL, THRESHOLD_LEVEL) : 0;
		need2Reset = !need2Reset;
	}
}

static void epl6803_work_func(struct work_struct* work)
{
	int Value = 0;
	Proximity* data = i2c_get_clientdata(this_client);

	#if debug
	pr_info("EPL6803: %s ++\n", __func__);
	#endif
	
	memset(i2cData, 0, sizeof(i2cData));
	if(data->enabled && !data->suspend && 
		(i2cIsFine = (i2c_smbus_read_i2c_block_data(this_client, PROXIMITY_REG_CH1DATA_LSB | 0x01, 2, &i2cData[0]) == 2)) && 
		(stateIsFine = !(i2c_smbus_read_byte_data(this_client, PROXIMITY_REG_CHIP_STATE) & 0x02))){
		#if debug
		printk("Reg: %x, %x\n", PROXIMITY_REG_CHIP_STATE >> 3, i2c_smbus_read_byte_data(this_client, PROXIMITY_REG_CHIP_STATE));
		#endif

		Value = epl6803_reportData(data);

	}else if(!i2cIsFine || !stateIsFine){
		// Varify transaction, 
		// reset chip if it is failure.
		mutex_lock(&data->mutex);
		data->sdata.Value = ERROR_TRANSACTION;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		mutex_unlock(&data->mutex);
		pr_err("EPL6803: ERROR_TRANSACTION, try to reset chip \n");
		// reset chip
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_CONTROL_SETTINGI, 0x00);
	}
	epl6803_autoCalibrate(data, Value);

	// Restart Sensor, 
	// it needs to check current state for avoiding async, so lock it.
	mutex_lock(&data->mutex);
	Value = 0;
	if(data->enabled && !data->suspend){
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_CONTROL_SETTINGI, 0x04);
		Value = 1;
	}
	mutex_unlock(&data->mutex);

	(Value == 1) ? queue_delayed_work(Proximity_WorkQueue, &data->dw, SleepTime) : -1;
	#if debug
	pr_info("EPL6803: %s --\n", __func__);
	#endif
}

static int epl6803_suspend(struct i2c_client* client, pm_message_t state)
{
	Proximity* data = i2c_get_clientdata(client);
	int rc = (data->enabled) ? cancel_delayed_work_sync(&data->dw) : -1;
	flush_workqueue(Proximity_WorkQueue);

	#if debug
	pr_info("EPL6803: %s rc: %d++\n", __func__, rc);
	#endif

	mutex_lock(&data->mutex);
	data->suspend = true;
	if(data->enabled){
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
		rc = 0;
	}
	mutex_unlock(&data->mutex);

	#if debug
	pr_info("EPL6803: %s --\n", __func__);
	#endif
	return 0;// It needs to return 0, non-zero means has fault.
}

static int epl6803_resume(struct i2c_client* client)
{
	Proximity* data = i2c_get_clientdata(client);
	int rc = (data->enabled) ? cancel_delayed_work_sync(&data->dw) : -1;
	flush_workqueue(Proximity_WorkQueue);

	#if debug
	pr_info("EPL6803: %s ++\n", __func__);
	#endif

	mutex_lock(&data->mutex);
	data->suspend = false;
	if(data->enabled){
		data->sdata.Value = -1;
		data->sdata.State = PROXIMITY_STATE_UNKNOWN;
		// Restart sensor
		i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_CONTROL_SETTINGI, 0x04);
		i2cIsFine = true;
		stateIsFine = true;
		need2Reset = false;
		need2ChangeState = false;
	}
	mutex_unlock(&data->mutex);
	rc = (data->enabled) ? queue_delayed_work(Proximity_WorkQueue, &data->dw, SleepTime) : -1;

	#if debug
	pr_info("EPL6803: %s rc: %d--\n", __func__, rc);
	#endif
	return 0;// It needs to return 0, non-zero means has fault.
}

static int epl6803_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
	Proximity* Sensor_device = NULL;
	struct input_dev* input_dev = NULL;
	int err = 0;

	#if debug
	pr_info("EPL6803: %s ++\n", __func__);
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

	INIT_DELAYED_WORK(&Sensor_device->dw, epl6803_work_func);
	i2c_set_clientdata(client, Sensor_device);

	input_dev->name = "proximity";
	input_dev->id.bustype = BUS_I2C;

	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 65535, 0, 0);
	input_set_drvdata(input_dev, Sensor_device);	

	err = input_register_device(input_dev);
	if(err){
		pr_err("EPL6803: input_register_device error\n");
		goto err_free_mem;
	}

	err = misc_register(&epl6803_misc);
    if(err < 0){
		pr_err("EPL6803: sensor_probe: Unable to register misc device: %s\n", input_dev->name);
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
	pr_info("EPL6803: %s --\n", __func__);
	#endif

	this_client = client;
	Proximity_WorkQueue = create_singlethread_workqueue(input_dev->name);
	proximity_readThreshold(THRESHOLD_LEVEL);// It needs to avoid deadlock.
	/**
	 *  INTEG: 0101b, PS_INTEG_VALUE: 144
	 *  RADC:  01b, RADC_value: 1024
	 *  CYCLE: 101b, CYCLE_value: 32
	 */
	SleepTime = msecs_to_jiffies(150);

	i2c_smbus_read_byte_data(this_client, PROXIMITY_REG_COMMANDI);
	i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_CONTROL_SETTINGII, 0x02);
	i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDI, 0xB7);
	i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_COMMANDII, 0x51);

	// reset chip
	i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_CONTROL_SETTINGI, 0x00);
	i2c_smbus_write_byte_data(this_client, PROXIMITY_REG_CONTROL_SETTINGI, 0x04);

	return 0;

 	err:
		misc_deregister(&epl6803_misc);
 	err_free_mem:
		input_free_device(input_dev);
		kfree(Sensor_device);
	return err;
}

static int epl6803_remove(struct i2c_client* client)
{
	Proximity* data = i2c_get_clientdata(client);

	destroy_workqueue(Proximity_WorkQueue);
	input_unregister_device(data->input);
	misc_deregister(&epl6803_misc);
	kfree(data);

	return 0;
}

static void epl6803_shutdown(struct i2c_client* client)
{
	epl6803_disable();
}

static struct i2c_device_id epl6803_idtable[] = {
	{"epl6803", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, epl6803_idtable);

static struct i2c_driver epl6803_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= EPL6803_DRIVER_NAME
	},
	.id_table	= epl6803_idtable,
	.probe		= epl6803_probe,
	.remove		= epl6803_remove,
	.suspend  	= epl6803_suspend,
	.resume   	= epl6803_resume,
	.shutdown	= epl6803_shutdown,
};

static int __init epl6803_init(void)
{
	return i2c_add_driver(&epl6803_driver);
}

static void __exit epl6803_exit(void)
{
	i2c_del_driver(&epl6803_driver);
}

module_init(epl6803_init);
module_exit(epl6803_exit);

MODULE_AUTHOR("HuizeWeng@Arimacomm");
MODULE_DESCRIPTION("Proxmity Sensor EPL6803");
MODULE_LICENSE("GPLv2");
