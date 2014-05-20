/* drivers/input/touchscreen/ektf2k.c - ELAN EKTF2136 touchscreen driver
 *
 * Copyright (C) 2011 Elan Microelectronics Corporation.
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

/* #define DEBUG */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>

// for linux 2.6.36.3
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <linux/ektf2k.h>

#define DEBUG 0

#if (HARDWARD_VERSION==HARDWARD_VERSION_2) || (HARDWARD_VERSION==HARDWARD_VERSION_3)
#define HOME_KEY_GPIO 29
#endif

#if (HARDWARD_VERSION==HARDWARD_VERSION_3) || (HARDWARD_VERSION==HARDWARD_VERSION_4)
#define PACKET_SIZE		22 	
#define FINGER_NUM		5	
#define IDX_NUM				0x01
#define IDX_FINGER			0x02	
#define FINGER_ID           0x08
#define NORMAL_PKT			0x5D
#define BUTTON_ID_INDEX 	17
#else
#define PACKET_SIZE	9 	
#define FINGER_NUM	2	  
#define IDX_FINGER	0x01	
#define FINGER_ID       0x01 	
#define NORMAL_PKT			0x5A   
#endif

#define PWR_STATE_DEEP_SLEEP	0
#define PWR_STATE_NORMAL		1
#define PWR_STATE_MASK			BIT(3)

#define CMD_S_PKT			0x52
#define CMD_R_PKT			0x53
#define CMD_W_PKT			0x54
#define HELLO_PKT			0x55

#if  (HARDWARD_VERSION==HARDWARD_VERSION_3) || (HARDWARD_VERSION==HARDWARD_VERSION_4)
#define RESET_PKT			0x77
#define CALIB_PKT			0xA8
#endif

#define FINGER_ID_INDEX 7
#define EKTF2048_INTERRUPT 82 
#define EKTF2048_RESET 7
#define EKTF2048_KEY_SEARCH 0x8
#if HARDWARD_VERSION==HARDWARD_VERSION_2
#define EKTF2048_KEY_HOME 0x4
#define EKTF2048_KEY_BACK 0x2
#else
#define EKTF2048_KEY_BACK 0x4
#define EKTF2048_KEY_HOME 0x2
#endif
#define EKTF2048_KEY_MENU 0x1
#define EKTF2048_KEY_RELEASE 0x0

#define ABS_MT_POSITION         0x2a    /* Group a set of X and Y */
#if  (HARDWARD_VERSION==HARDWARD_VERSION_3) || (HARDWARD_VERSION==HARDWARD_VERSION_4)
#define ABS_MT_AMPLITUDE        0x2b    /* Group a set of Z and W */
#endif

#define TOUCH_EVENTS_FILTER 0    //Add timer to filter touch event.
#if TOUCH_EVENTS_FILTER
#define TS_FILTER_TIMEOUT_MS 25
#endif

#define ELAN_IOCTLID	0xD0
#define IOCTL_I2C_SLAVE	_IOW(ELAN_IOCTLID,  1, int)
#define IOCTL_MAJOR_FW_VER  _IOR(ELAN_IOCTLID, 2, int)
#define IOCTL_MINOR_FW_VER  _IOR(ELAN_IOCTLID, 3, int)
#define IOCTL_RESET  _IOR(ELAN_IOCTLID, 4, int)
#define IOCTL_IAP_MODE_LOCK  _IOR(ELAN_IOCTLID, 5, int)
#define IOCTL_CHECK_RECOVERY_MODE  _IOR(ELAN_IOCTLID, 6, int)
#define IOCTL_FW_VER  _IOR(ELAN_IOCTLID, 7, int)
#define IOCTL_X_RESOLUTION  _IOR(ELAN_IOCTLID, 8, int)
#define IOCTL_Y_RESOLUTION  _IOR(ELAN_IOCTLID, 9, int)
#define IOCTL_FW_ID  _IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_IAP_MODE_UNLOCK  _IOR(ELAN_IOCTLID, 12, int)
#define IOCTL_I2C_INT  _IOR(ELAN_IOCTLID, 13, int)
#define IOCTL_RESUME  _IOR(ELAN_IOCTLID, 14, int)
#define IOCTL_POWER_LOCK  _IOR(ELAN_IOCTLID, 15, int)
#define IOCTL_POWER_UNLOCK  _IOR(ELAN_IOCTLID, 16, int)

//uint16_t checksum_err = 0;
uint8_t button = 0;
uint8_t RECOVERY=0x00;
int FW_VERSION=0x00;
int X_RESOLUTION=0x00;
int Y_RESOLUTION=0x00;
int FW_ID=0x00;
int work_lock=0x00;
int power_lock=0x00;
#if  (HARDWARD_VERSION==HARDWARD_VERSION_3) || (HARDWARD_VERSION==HARDWARD_VERSION_4)
uint8_t button_state=0;
#endif

struct elan_ktf2k_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct workqueue_struct *elan_wq;
	struct work_struct work;
	int (*power)(int on);
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif	
	int intr_gpio;
	int reset_gpio;
	int fw_ver;
	int fw_id;
	int x_resolution;
	int y_resolution;
#if (HARDWARD_VERSION==HARDWARD_VERSION_2) || (HARDWARD_VERSION==HARDWARD_VERSION_3)
       struct work_struct home_key_irq_work;
       int home_key_irq;
#endif
	struct miscdevice firmware;
#if TOUCH_EVENTS_FILTER
	int xy_filter_on;	
	struct timer_list timer;
#endif
};

static struct elan_ktf2k_ts_data *private_ts;
static int __fw_packet_handler(struct i2c_client *client);
static int elan_ktf2k_ts_resume(struct i2c_client *client);

int elan_iap_open(struct inode *inode, struct file *filp){ 
#if DEBUG	
	printk("[ELAN]into elan_iap_open\n");
		if (private_ts == NULL)  printk("private_ts is NULL~~~");
#endif		
		
	return 0;
}

int elan_iap_release(struct inode *inode, struct file *filp){    
#if DEBUG	
	printk("[ELAN]into elan_iap_release\n");
#endif
	return 0;
}

ssize_t elan_iap_write(struct file *filp, const char *buff, size_t count, loff_t *offp){  
    int ret;
    char *tmp;
#if DEBUG	
    printk("[ELAN]into elan_iap_write\n");
#endif

    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);
    
    if (tmp == NULL){
	 kfree(tmp);
        return -ENOMEM;
     }

    if (copy_from_user(tmp, buff, count)) {
	 kfree(tmp);
        return -EFAULT;
    }
	
    ret = i2c_master_send(private_ts->client, tmp, count);
#if DEBUG		
    if (ret != count) printk("ELAN i2c_master_send fail, ret=%d \n", ret);
#endif
    kfree(tmp);
    return ret;

}

ssize_t elan_iap_read(struct file *filp, char *buff,    size_t count, loff_t *offp){    
    char *tmp;
    int ret;
    long rc;  
#if DEBUG		
    printk("[ELAN]into elan_iap_read\n");
#endif
   
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;

    ret = i2c_master_recv(private_ts->client, tmp, count);

    if (ret >= 0)
        rc = copy_to_user(buff, tmp, count);
    
    kfree(tmp);

    return ret;
}

static long elan_iap_ioctl(/*struct inode *inode,*/ struct file *filp,    unsigned int cmd, unsigned long arg){
	int __user *ip = (int __user *)arg;
#if DEBUG	
	printk("[ELAN]into elan_iap_ioctl\n");
	printk("cmd value %x\n",cmd);
#endif

	switch (cmd) {        
		case IOCTL_I2C_SLAVE: 
			private_ts->client->addr = (int __user )arg; 
			break;   
		case IOCTL_MAJOR_FW_VER:            
			break;        
		case IOCTL_MINOR_FW_VER:            
			break;        
		case IOCTL_RESET:
			gpio_set_value(EKTF2048_RESET,  0);
			mdelay(100);
			gpio_set_value(EKTF2048_RESET,  1);
			break;
		case IOCTL_IAP_MODE_LOCK:
			work_lock=1;
			break;
		case IOCTL_IAP_MODE_UNLOCK:
			work_lock=0;
			if (gpio_get_value(private_ts->intr_gpio))
    			{
        			enable_irq(private_ts->client->irq);
			}
			break;
		case IOCTL_CHECK_RECOVERY_MODE:
			return RECOVERY;
			break;
		case IOCTL_FW_VER:
			__fw_packet_handler(private_ts->client);
			return FW_VERSION;
			break;
		case IOCTL_X_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return X_RESOLUTION;
			break;
		case IOCTL_Y_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return Y_RESOLUTION;
			break;
		case IOCTL_FW_ID:
			__fw_packet_handler(private_ts->client);
			return FW_ID;
			break;
		case IOCTL_I2C_INT:
			put_user(gpio_get_value(private_ts->intr_gpio), ip);
			break;	
		case IOCTL_RESUME:
			elan_ktf2k_ts_resume(private_ts->client);
			break;
		case IOCTL_POWER_LOCK:
			power_lock=1;
			break;
		case IOCTL_POWER_UNLOCK:
			power_lock=0;
			break;
		default:            
			break;   
	}       
	return 0;
}

struct file_operations elan_touch_fops = {    
        .open =         elan_iap_open,    
        .write =        elan_iap_write,    
        .read = elan_iap_read,    
        .release =     elan_iap_release,    
        //.ioctl =         elan_iap_ioctl,  
        .unlocked_ioctl =         elan_iap_ioctl,  
 };

#ifdef CONFIG_HAS_EARLYSUSPEND
static void elan_ktf2k_ts_early_suspend(struct early_suspend *h);
static void elan_ktf2k_ts_late_resume(struct early_suspend *h);
#endif

static ssize_t elan_ktf2k_gpio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;
#if DEBUG
    printk("[ELAN]into elan_ktf2k_gpio_show\n");
#endif
	ret = gpio_get_value(ts->intr_gpio);
#if DEBUG		
	printk(KERN_DEBUG "GPIO_TP_INT_N=%d\n", ts->intr_gpio);
#endif
	//sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
	snprintf(buf, sizeof(buf), "GPIO_TP_INT_N=%d\n", ret);

	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(gpio, S_IRUGO, elan_ktf2k_gpio_show, NULL);

static ssize_t elan_ktf2k_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;
#if DEBUG
    printk("[ELAN]into elan_ktf2k_vendor_show\n");
#endif

	//sprintf(buf, "%s_x%4.4x\n", "ELAN_KTF2K", ts->fw_ver);
	snprintf(buf, sizeof(buf), "%s_x%4.4x\n", "ELAN_KTF2K", ts->fw_ver);

	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(vendor, S_IRUGO, elan_ktf2k_vendor_show, NULL);

static struct kobject *android_touch_kobj;

static int elan_ktf2k_touch_sysfs_init(void)
{
	int ret ;
#if DEBUG
    printk("[ELAN]into elan_ktf2k_touch_sysfs_init\n");
#endif

	android_touch_kobj = kobject_create_and_add("android_touch", NULL) ;
	if (android_touch_kobj == NULL) {
#if DEBUG		
		printk(KERN_ERR "[elan]%s: subsystem_register failed\n", __func__);
#endif
		ret = -ENOMEM;
		return ret;
	}
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_gpio.attr);
	if (ret) {
#if DEBUG		
		printk(KERN_ERR "[elan]%s: sysfs_create_file failed\n", __func__);
#endif
		return ret;
	}
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr);
	if (ret) {
#if DEBUG		
		printk(KERN_ERR "[elan]%s: sysfs_create_group failed\n", __func__);
#endif
		return ret;
	}
	return 0 ;
}

static void elan_touch_sysfs_deinit(void)
{
#if DEBUG
    printk("[ELAN]into elan_touch_sysfs_deinit\n");
#endif
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_gpio.attr);
	kobject_del(android_touch_kobj);
}

static int __elan_ktf2k_ts_poll(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int status = 0, retry = 10;
#if DEBUG
    printk("[ELAN]into __elan_ktf2k_ts_poll\n");
#endif
	/*reset elan IC and check INT pin state again*/

	do {
		status = gpio_get_value(ts->intr_gpio);
		dev_dbg(&client->dev, "%s: status = %d\n", __func__, status);
		retry--;
		mdelay(20);
	} while (status == 1 && retry > 0);

	dev_dbg(&client->dev, "[elan]%s: poll interrupt status %s\n",
			__func__, status == 1 ? "high" : "low");
	return (status == 0 ? 0 : -ETIMEDOUT);
}

static int elan_ktf2k_ts_poll(struct i2c_client *client)
{
#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_poll\n");
#endif
	return __elan_ktf2k_ts_poll(client);
}

static int elan_ktf2k_ts_get_data(struct i2c_client *client, uint8_t *cmd,
			uint8_t *buf, size_t size)
{
	int rc;

#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_get_data\n");
#endif

	dev_dbg(&client->dev, "[elan]%s: enter\n", __func__);

	if (buf == NULL)
		return -EINVAL;

	if ((i2c_master_send(client, cmd, 4)) != 4) {
		dev_err(&client->dev,
			"[elan]%s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

	rc = elan_ktf2k_ts_poll(client);
	if (rc < 0)
		return -EINVAL;
	else {
		if (i2c_master_recv(client, buf, size) != size ||
		    buf[0] != CMD_S_PKT)
			return -EINVAL;
	}

	return 0;

}

static int __hello_packet_handler(struct i2c_client *client)
{
	int rc;
	uint8_t buf_recv[4] = { 0 };
	uint8_t buf_recv1[4] = { 0 };

#if DEBUG
    printk("[ELAN]into __hello_packet_handler\n");
#endif

	rc = elan_ktf2k_ts_poll(client);
	if (rc < 0) {
		dev_err(&client->dev, "[elan] %s: failed!\n", __func__);
#if DEBUG
		pr_info("-- ektf poll check fail\n");
#endif
          return -EINVAL;
	}

	rc = i2c_master_recv(client, buf_recv, 4);
       pr_info("-- _hello packet , %x %x %x %x\n",buf_recv[0],buf_recv[1],buf_recv[2],buf_recv[3]);
	if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80)
	{
		rc = elan_ktf2k_ts_poll(client);
		if (rc < 0) {
			dev_err(&client->dev, "[elan] %s: failed!\n", __func__);
			return -EINVAL;
		}
		rc = i2c_master_recv(client, buf_recv1, 4);
		printk("[elan] %s: recovery hello packet %2x:%2X:%2x:%2x\n", __func__, buf_recv1[0], buf_recv1[1], buf_recv1[2], buf_recv1[3]);
		RECOVERY=0x80;
		return RECOVERY;
	}
	return 0;
}

static int __fw_packet_handler(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int rc;
	int major, minor;
	uint8_t cmd[] = {CMD_R_PKT, 0x00, 0x00, 0x01};
	uint8_t cmd_x[] = {0x53, 0x60, 0x00, 0x00}; /*Get x resolution*/
	uint8_t cmd_y[] = {0x53, 0x63, 0x00, 0x00}; /*Get y resolution*/
	uint8_t cmd_id[] = {0x53, 0xf0, 0x00, 0x01}; /*Get firmware ID*/
	uint8_t buf_recv[4] = {0};
#if DEBUG
    printk("[ELAN]into __fw_packet_handler\n");
#endif

	rc = elan_ktf2k_ts_get_data(client, cmd, buf_recv, 4);
	if (rc < 0)
		return rc;

	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_ver = major << 8 | minor;
	FW_VERSION = ts->fw_ver;
// X Resolution
	rc = elan_ktf2k_ts_get_data(client, cmd_x, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->x_resolution =minor;
	X_RESOLUTION = ts->x_resolution;
// Y Resolution	
	rc = elan_ktf2k_ts_get_data(client, cmd_y, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	ts->y_resolution =minor;
	Y_RESOLUTION = ts->y_resolution;
// Firmware ID
	rc = elan_ktf2k_ts_get_data(client, cmd_id, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	ts->fw_id = major << 8 | minor;
	FW_ID = ts->fw_id;
	printk(KERN_INFO "[elan] %s: firmware version: 0x%4.4x\n",
			__func__, ts->fw_ver);
	printk(KERN_INFO "[elan] %s: firmware ID: 0x%4.4x\n",
			__func__, ts->fw_id);
	printk(KERN_INFO "[elan] %s: x resolution: %d, y resolution: %d\n",
			__func__, ts->x_resolution, ts->y_resolution);
	return 0;
}

static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
			uint16_t *x, uint16_t *y)
{
#if DEBUG
    printk(KERN_INFO"[ELAN]into elan_ktf2k_ts_parse_xy -> data[0] : %x,data[1]: %x,data[2] : %x \n",data[0],data[1],data[2]);
#endif

	*x = *y = 0;

	*x = (data[0] & 0xf0);
	*x <<= 4;
	*x |= data[1];

	*y = (data[0] & 0x0f);
	*y <<= 8;
	*y |= data[2];

#if 0
	pr_info("-- parse original : x : %d , y : %d\n",*x,*y);
	*y = 1024 - *y;
#endif	
	return 0;
}

static int elan_ktf2k_ts_setup(struct i2c_client *client)
{
	int rc;

#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_setup\n");
#endif

	rc = __hello_packet_handler(client);
	if (rc < 0)
		goto hand_shake_failed;
	dev_dbg(&client->dev, "[elan] %s: hello packet got.\n", __func__);
// for firmware update
	if(rc==0x80)
	{
		return rc;
	}
// end firmware update
	rc = __fw_packet_handler(client);
	if (rc < 0)
		goto hand_shake_failed;
	dev_dbg(&client->dev, "[elan] %s: firmware checking done.\n", __func__);

hand_shake_failed:
	return rc;
}

static int elan_ktf2k_ts_set_power_state(struct i2c_client *client, int state)
{
	uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};

	dev_dbg(&client->dev, "[elan] %s: enter\n", __func__);

	cmd[1] |= (state << 3);

	dev_dbg(&client->dev,
		"[elan] dump cmd: %02x, %02x, %02x, %02x\n",
		cmd[0], cmd[1], cmd[2], cmd[3]);

	if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
		dev_err(&client->dev,
			"[elan] %s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int elan_ktf2k_ts_get_power_state(struct i2c_client *client)
{
	int rc = 0;
	uint8_t cmd[] = {CMD_R_PKT, 0x50, 0x00, 0x01};
	uint8_t buf[4], power_state;

	rc = elan_ktf2k_ts_get_data(client, cmd, buf, 4);
	if (rc)
		return rc;

	power_state = buf[1];
	dev_dbg(&client->dev, "[elan] dump repsponse: %0x\n", power_state);
	power_state = (power_state & PWR_STATE_MASK) >> 3;
	dev_dbg(&client->dev, "[elan] power state = %s\n",
		power_state == PWR_STATE_DEEP_SLEEP ?
		"Deep Sleep" : "Normal/Idle");

	return power_state;
}



static int elan_ktf2k_ts_recv_data(struct i2c_client *client, uint8_t *buf)
{
	/* struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client); */
	int rc, ret, bytes_to_recv = PACKET_SIZE;

#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_recv_data\n");
#endif

	if (buf == NULL)
		return -EINVAL;

	memset(buf, 0, bytes_to_recv);
	rc = i2c_master_recv(client, buf, bytes_to_recv);
#if DEBUG
        pr_info("-- ektf recv data[1 ~ 8] , %x %x %x %x %x %x %x %x %x\n",\
	   	buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8]);
	   	
	   	  pr_info("-- ektf recv data[9 ~ 17] , %x %x %x %x %x %x %x %x %x\n",\
	   	buf[9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15],buf[16],buf[17]);
#endif	   
	if (rc != bytes_to_recv) {
		dev_err(&client->dev,
			"[elan] %s: i2c_master_recv error?! \n", __func__);
		ret = i2c_master_recv(client, buf, bytes_to_recv);
		return -EINVAL;
	}

	return rc;
}

static void elan_ktf2k_ts_report_data(struct i2c_client *client, uint8_t *buf)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	struct input_dev *idev = ts->input_dev;
	uint16_t x, y;
	uint16_t fbits=0;
	uint8_t i, num;
	uint16_t finger_num=0;
#if  (HARDWARD_VERSION==HARDWARD_VERSION_3) || (HARDWARD_VERSION==HARDWARD_VERSION_4)
	//uint16_t checksum=0;
#else
	int rc, bytes_to_recv = PACKET_SIZE;
#endif	
#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_report_data\n");
#endif
#if  (HARDWARD_VERSION==HARDWARD_VERSION_3) || (HARDWARD_VERSION==HARDWARD_VERSION_4)
	num = buf[IDX_NUM] & 0x7; 

	//for (i=0; i<34;i++)
		//checksum +=buf[i];

	//if ( (num < 6) || ((checksum & 0x00ff) == buf[34])) {
     	   switch (buf[0]) {
	   case NORMAL_PKT:
		fbits = buf[2] & 0x30;
		fbits = (fbits << 4) | buf[1];
		if (num == 0) {
			dev_dbg(&client->dev, "no press\n");
			
 #if (HARDWARD_VERSION==HARDWARD_VERSION_3)     
			if (buf[BUTTON_ID_INDEX] == 0x41) {
      			    button_state=0x41;
      			    input_report_key(idev, /*KEY_MENU*/KEY_BACK, 1);
      			} else if (buf[BUTTON_ID_INDEX] == 0x81) {
      			    button_state=0x81;
      			    input_report_key(idev, /*KEY_BACK*/KEY_MENU, 1);
      			} else if (button_state == 0x41) {
      			    button_state=0;
      			    input_report_key(idev, /*KEY_MENU*/KEY_BACK, 0);
      			} else if (button_state == 0x81) {
      			    button_state=0;
      			    input_report_key(idev, /*KEY_BACK*/KEY_MENU, 0);
      			}
 #else
 			if (buf[BUTTON_ID_INDEX] == 0x21) {
      			    button_state=0x21;
      			    input_report_key(idev, KEY_BACK, 1);
 			      } else if (buf[BUTTON_ID_INDEX] == 0x41) {
      			    button_state=0x41;
      			    input_report_key(idev, KEY_HOME, 1);
      			} else if (buf[BUTTON_ID_INDEX] == 0x81) {
      			    button_state=0x81;
      			    input_report_key(idev, KEY_MENU, 1);
      			} else if (button_state == 0x21) {
      			    button_state=0;
      			    input_report_key(idev, KEY_BACK, 0);
      			} else if (button_state == 0x41) {
      			    button_state=0;
      			    input_report_key(idev, KEY_HOME, 0);
      			} else if (button_state == 0x81) {
      			    button_state=0;
      			    input_report_key(idev, KEY_MENU, 0);
      			}
#endif     			
#if DEBUG							 
			     pr_info("[elan] Figer key :%d\n",button_state);
#endif

			input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
			input_report_key(idev, BTN_TOUCH, 0);
			input_mt_sync(idev);

		} else {
			uint8_t idx;
			uint8_t reported = 0;

			dev_dbg(&client->dev, "[elan] %d fingers\n", num);
                        idx=IDX_FINGER;

			for (i = 0; i < FINGER_NUM; i++) {
			  if ((fbits & FINGER_ID)) {
			     elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);  
#if (HARDWARD_VERSION==HARDWARD_VERSION_3)  
			     //x=ELAN_X_MAX-x;
			     y=ELAN_Y_MAX-y;
#else
			     x=ELAN_X_MAX-x;
			     //y=ELAN_Y_MAX-y;
#endif			     
			     
#if TOUCH_EVENTS_FILTER		
                  if((x!=0) && (y!=0) && (ts->xy_filter_on == 1)) {	     
                     return;
                   }    
#endif
#if DEBUG							 
			     pr_info("[Elan] Figer [%d], x : %d, y : %d\n",i, x,y);
			     //pr_info("[Elan] --------------------------------------------\n");
#endif
			     if (!((x<=0) || (y<=0) || (x>=ELAN_X_MAX) || (y>=ELAN_Y_MAX))) {   
            finger_num = finger_num + 1;			     	
    				input_report_abs(idev, ABS_MT_TRACKING_ID, i);
				input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
				input_report_abs(idev, ABS_MT_POSITION_X, x);
				input_report_abs(idev, ABS_MT_POSITION_Y, y);
				input_mt_sync(idev);
				reported++;
			     } // end if border
 			  } // end if finger status

			  fbits = fbits >> 1;
			  idx += 3;
			} // end for
			if (reported)
			{
				input_report_key(idev, BTN_TOUCH, finger_num);
				input_sync(idev);
#if TOUCH_EVENTS_FILTER
                            if(ts->xy_filter_on == 0) {
                                  ts->xy_filter_on = 1;
                                  mod_timer(&ts->timer, jiffies + msecs_to_jiffies(TS_FILTER_TIMEOUT_MS));
                              }
#endif	
			}
			else {
				input_mt_sync(idev);
				input_sync(idev);
#if TOUCH_EVENTS_FILTER
                            ts->xy_filter_on = 0;
#endif			
			}

		}

		input_sync(ts->input_dev);

		break;
	   default:
		dev_err(&client->dev,
			"[elan] %s: unknown packet type: %0x\n", __func__, buf[0]);
		break;
	   } // end switch

	//} // checksum
	//else {
		//checksum_err +=1;
		//printk("[elan] Checksum Error %d\n", checksum_err);
	//} 

#else
        num = buf[FINGER_ID_INDEX] & 0x06; 
	if (num == 0x02)
		fbits= 0x01;
	else if (num == 0x04)
		fbits= 0x03;
	else 
		fbits= 0;

   switch (buf[0]) {
	   case NORMAL_PKT:

		if (num == 0) {
			dev_dbg(&client->dev, "no press\n");
			/*add for button key by herman start*/
      if (buf[FINGER_ID_INDEX+1] == 0x8) {
	   button = EKTF2048_KEY_SEARCH;
          input_report_key(idev, KEY_SEARCH, 1);
#if HARDWARD_VERSION==HARDWARD_VERSION_2
      } else if (buf[FINGER_ID_INDEX+1] == 0x4) {
          button = EKTF2048_KEY_HOME;
	   input_report_key(idev, KEY_HOME, 1);
      } else if (buf[FINGER_ID_INDEX+1] == 0x2) {
          button = EKTF2048_KEY_BACK;
          input_report_key(idev, KEY_BACK, 1);
#else		  
      } else if (buf[FINGER_ID_INDEX+1] == 0x4) {
          button = EKTF2048_KEY_BACK;
	   input_report_key(idev, KEY_BACK, 1);
      } else if (buf[FINGER_ID_INDEX+1] == 0x2) {
          button = EKTF2048_KEY_HOME;
          input_report_key(idev, KEY_HOME, 1);
#endif		  
      } else if (buf[FINGER_ID_INDEX+1] == 0x1) {
          button = EKTF2048_KEY_MENU;
          input_report_key(idev, KEY_MENU, 1);
      }else if (button == EKTF2048_KEY_SEARCH) {
          button = EKTF2048_KEY_RELEASE;
	   input_report_key(idev, KEY_SEARCH, 0);
      } else if (button == EKTF2048_KEY_BACK) {
          button = EKTF2048_KEY_RELEASE;
	   input_report_key(idev, KEY_BACK, 0);
      }else if (button == EKTF2048_KEY_HOME) {
          button = EKTF2048_KEY_RELEASE;
	   input_report_key(idev, KEY_HOME, 0);
      } else if (button == EKTF2048_KEY_MENU) {
          button = EKTF2048_KEY_RELEASE;
	   input_report_key(idev, KEY_MENU, 0);
      }
			/*add for button key by herman end*/
#if DEBUG							 
			     pr_info("[elan] Figer key :%d\n",button);
#endif
			input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
			input_report_key(idev, BTN_TOUCH, 0);
			input_mt_sync(idev);
			rc = i2c_master_recv(client, buf, bytes_to_recv);

		} else {
			uint8_t idx;
			uint8_t reported = 0;

			dev_dbg(&client->dev, "[elan] %d fingers\n", num);
                        idx=IDX_FINGER;

			for (i = 0; i < FINGER_NUM; i++) {
			  if ((fbits & FINGER_ID)) {							// james
			     elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);      //james
                             //y = ELAN_Y_MAX - y;
#if DEBUG							 
			     pr_info("[Elan] Figer %d, x : %d, y : %d\n",i, x,y);
#endif

			     if (!((x<=0) || (y<=0) || (x>=ELAN_X_MAX) || (y>=ELAN_Y_MAX))) {   
            finger_num = finger_num + 1;			     	
    				input_report_abs(idev, ABS_MT_TRACKING_ID, i);
				input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
				input_report_abs(idev, ABS_MT_POSITION_X, x);
				input_report_abs(idev, ABS_MT_POSITION_Y, y);
				input_mt_sync(idev);
				reported++;
			     } // end if border
 			  } // end if finger status

			  fbits = fbits >> 1;
			  idx += 3;
			} // end for
			if (reported)
			{
				input_report_key(idev, BTN_TOUCH, finger_num);
				input_sync(idev);
			}
			else {
				input_mt_sync(idev);
				input_sync(idev);
			}

		}


		input_sync(ts->input_dev);

		break;
	   default:
		dev_err(&client->dev,
			"[elan] %s: unknown packet type: %0x\n", __func__, buf[0]);
		rc = i2c_master_recv(client, buf, bytes_to_recv);
		break;
	   } // end switch
#endif	   
#if DEBUG
    printk("[ELAN]-------------------------------------------------\n");
#endif
	return;
}

static void elan_ktf2k_ts_work_func(struct work_struct *work)
{
	int rc;
	struct elan_ktf2k_ts_data *ts =
		container_of(work, struct elan_ktf2k_ts_data, work);
	uint8_t buf[PACKET_SIZE] = { 0 };

#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_work_func\n");
#endif

	if (gpio_get_value(ts->intr_gpio))
        {
#if DEBUG        
              pr_info("-- work func interrupt high return\n");
#endif
              enable_irq(ts->client->irq);
		return;
	}
	rc = elan_ktf2k_ts_recv_data(ts->client, buf);
	if (rc < 0)
		{
#if DEBUG		
		pr_info("-- work func recv data return\n");
#endif
                enable_irq(ts->client->irq);
		return;
		}
	
	elan_ktf2k_ts_report_data(ts->client, buf);
	enable_irq(ts->client->irq);

	return;
}

static irqreturn_t elan_ktf2k_ts_irq_handler(int irq, void *dev_id)
{
	struct elan_ktf2k_ts_data *ts = dev_id;
	struct i2c_client *client = ts->client;
	
#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_irq_handler\n");
#endif
	dev_dbg(&client->dev, "[elan] %s\n", __func__);
	disable_irq_nosync(ts->client->irq);
	queue_work(ts->elan_wq, &ts->work);

	return IRQ_HANDLED;
}

#if (HARDWARD_VERSION==HARDWARD_VERSION_2) || (HARDWARD_VERSION==HARDWARD_VERSION_3)
static irqreturn_t elan_home_key_isr(int irq, void *dev_id)
{
       struct elan_ktf2k_ts_data *ts = dev_id;
       int homekey_value = gpio_get_value(HOME_KEY_GPIO);
	struct input_dev *idev = ts->input_dev;

	disable_irq_nosync(ts->home_key_irq);
	//queue_work(ts->elan_wq, &ts->home_key_irq_work);
       //elan_home_key_irq_work_queue_func(&ts->home_key_irq_work);
	
       //printk("[ELAN]into elan_home_key_irq_work_queue_func\n");
      if(homekey_value && (button==EKTF2048_KEY_HOME) )
      	{
	      input_report_key(idev, KEY_HOME, 0);
	      button = EKTF2048_KEY_RELEASE;
	      printk("[ELAN]EKTF2048_KEY_RELEASE\n");
      	}
      else if(!homekey_value && (!button))
      	{
		input_report_key(idev, KEY_HOME, 1);
		button = EKTF2048_KEY_HOME;
	       printk("[ELAN]EKTF2048_KEY_HOME\n");
	 }
	  input_mt_sync(idev);
	  input_sync(ts->input_dev);
	irq_set_irq_type(ts->home_key_irq, homekey_value ? IRQF_TRIGGER_LOW: IRQF_TRIGGER_HIGH);
	enable_irq(ts->home_key_irq);
	
	return IRQ_HANDLED;
}

static void elan_home_key_irq_work_queue_func(struct work_struct *work)
{
	printk("[ELAN]elan_home_key_irq_work_queue_func ++++++++\n");
	/*
	struct elan_ktf2k_ts_data *ts =
		container_of(work, struct elan_ktf2k_ts_data, work);
	struct input_dev *idev = ts->input_dev;
	
        int homekey_value = gpio_get_value(HOME_KEY_GPIO);
	
      printk("[ELAN]into elan_home_key_irq_work_queue_func\n");
      if(homekey_value && button)
      	{
	      input_report_key(idev, KEY_HOME, 0);
	      button = EKTF2048_KEY_RELEASE;
	      printk("[ELAN]EKTF2048_KEY_RELEASE\n");
      	}
      else if(!homekey_value && (!button))
      	{
		input_report_key(idev, KEY_HOME, 1);
		button = EKTF2048_KEY_HOME;
	       printk("[ELAN]EKTF2048_KEY_HOME\n");
	 }
	  
	set_irq_type(ts->home_key_irq, homekey_value ? IRQF_TRIGGER_LOW: IRQF_TRIGGER_HIGH);
	enable_irq(ts->home_key_irq);
	*/
	return;
}

#endif
static int elan_ktf2k_ts_register_interrupt(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int err = 0;
	
#if DEBUG
    printk("[ELAN]into elan_ktf2k_ts_irq_handler\n");
#endif
       err=gpio_tlmm_config(GPIO_CFG(EKTF2048_INTERRUPT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

       if (err < 0)
	        printk(KERN_INFO "[ELAN] - ELAN INT Enable Fail \n");

	client->irq = MSM_GPIO_TO_INT(EKTF2048_INTERRUPT);
	
	err = request_irq(client->irq, elan_ktf2k_ts_irq_handler,
			IRQF_TRIGGER_FALLING, client->name, ts);
	if (err)
		dev_err(&client->dev, "[elan] %s: request_irq %d failed\n",
				__func__, client->irq);
	
#if (HARDWARD_VERSION==HARDWARD_VERSION_2) || (HARDWARD_VERSION==HARDWARD_VERSION_3)
	INIT_WORK(&ts->home_key_irq_work, elan_home_key_irq_work_queue_func);

	gpio_direction_input(HOME_KEY_GPIO);
	
	gpio_tlmm_config(GPIO_CFG(HOME_KEY_GPIO, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    
	 ts->home_key_irq = MSM_GPIO_TO_INT(HOME_KEY_GPIO);

       if(request_irq(ts->home_key_irq, elan_home_key_isr, IRQF_TRIGGER_LOW, "elan_home_key_irq", ts))
	 {
		printk("elan home key request irq, error\n");
	 }
	else
	{
		printk("elan home key request irq, success\n");
	 }	
#endif	

	return err;
}

#if TOUCH_EVENTS_FILTER
static void ts_timer(unsigned long arg)
{
	struct elan_ktf2k_ts_data *ts = (struct elan_ktf2k_ts_data *)arg;
	
	//printk("- - - - - Touch filter timer is timeout - - - - - \n");
       ts->xy_filter_on = 0;
}
#endif

extern int CTP_flag;
static int elan_ktf2k_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err = 0;
	struct elan_ktf2k_i2c_platform_data *pdata = NULL;
	struct elan_ktf2k_ts_data *ts = NULL;
#if DEBUG		
       pr_info("-- elan ktf probe\n");
#endif

if(CTP_flag)
     return err;
		
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
#if DEBUG	
		printk(KERN_ERR "[elan] %s: i2c check functionality error\n", __func__);
#endif
		err = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct elan_ktf2k_ts_data), GFP_KERNEL);
	if (ts == NULL) {
#if DEBUG			
		printk(KERN_ERR "[elan] %s: allocate elan_ktf2k_ts_data failed\n", __func__);
#endif
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}
#if TOUCH_EVENTS_FILTER 	
       ts->xy_filter_on = 0;
#endif   
	ts->elan_wq = create_singlethread_workqueue("elan_wq");
	if (!ts->elan_wq) {
#if DEBUG			
		printk(KERN_ERR "[elan] %s: create workqueue failed\n", __func__);
#endif
		err = -ENOMEM;
		goto err_create_wq_failed;
	}

	INIT_WORK(&ts->work, elan_ktf2k_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;

       err = gpio_direction_input(EKTF2048_INTERRUPT);

	if (likely(pdata != NULL)) {
		ts->power = pdata->power;

	if (ts->power)
		ts->power(1);
		
	if (!gpio_request(EKTF2048_RESET, "EKTF2048_RESET")) {
		  if (gpio_tlmm_config(GPIO_CFG(EKTF2048_RESET, 0, GPIO_CFG_OUTPUT,
								   GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE))								   
				  pr_err("-- EKTF_0 Failed to configure GPIO %d\n",
								   EKTF2048_RESET);
		} else{	
			pr_err("-- EKTF_1 Failed to request GPIO%d\n", EKTF2048_RESET);				
		}
#if DEBUG
	       pr_info("-- EKTF reset pin register successful\n");
#endif
		gpio_set_value(EKTF2048_RESET,  0);
		mdelay(100);
		gpio_set_value(EKTF2048_RESET,  1);

#if DEBUG		
		pr_info("---- elan ktf2k probo , delay 200 ms\n");
#endif
              mdelay(200);	
#if DEBUG	
                pr_info("-- ts->intr_gpio : %x , pdata->intr_gpio : %d\n",ts->intr_gpio,pdata->intr_gpio);
#endif
		ts->intr_gpio = pdata->intr_gpio;
           	ts->reset_gpio  = pdata->reset_gpio;
	}
	
	err = elan_ktf2k_ts_setup(client);
	if (err < 0) {
#if DEBUG		
		printk(KERN_INFO "No Elan chip inside\n");
#endif
		err = -ENODEV;
		goto err_detect_failed;
	}
// Firmware Update
	if(err==0x80)
	{
		// MISC
		private_ts = ts;
  	ts->firmware.minor = MISC_DYNAMIC_MINOR;
  	ts->firmware.name = "elan-iap";
  	ts->firmware.fops = &elan_touch_fops;
  	ts->firmware.mode = S_IRWXUGO; 
   	
  	if (misc_register(&ts->firmware) < 0)
  		printk("[ELAN]misc_register failed!!");
  	else
  	  printk("[ELAN]misc_register finished!!");
  	  return 0;
	}
// End Firmware Update

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev, "[elan] Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "elan-touchscreen";   // for andorid2.2 Froyo  

	set_bit(BTN_TOUCH, ts->input_dev->keybit);

	input_set_abs_params(ts->input_dev, ABS_X, pdata->abs_x_min,  pdata->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, pdata->abs_y_min,  pdata->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_TOOL_WIDTH, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, pdata->abs_x_min,  pdata->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, pdata->abs_y_min,  pdata->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, FINGER_NUM, 0, 0);	
/*add for button key by herman start*/
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
/*add for button key by herman end*/

	__set_bit(EV_ABS, ts->input_dev->evbit);
	__set_bit(EV_SYN, ts->input_dev->evbit);
	__set_bit(EV_KEY, ts->input_dev->evbit);

	err = input_register_device(ts->input_dev);
	if (err) {
		dev_err(&client->dev,
			"[elan]%s: unable to register %s input device\n",
			__func__, ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	elan_ktf2k_ts_register_interrupt(ts->client);

	if (gpio_get_value(ts->intr_gpio) == 0) {
		printk(KERN_INFO "[elan]%s: handle missed interrupt\n", __func__);
		elan_ktf2k_ts_irq_handler(client->irq, ts);
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
pr_info("-- define CONFIG_HAS_EARLYSUSPEND\n");
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1;
	ts->early_suspend.suspend = elan_ktf2k_ts_early_suspend;
	ts->early_suspend.resume = elan_ktf2k_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	private_ts = ts;

	elan_ktf2k_touch_sysfs_init();

	dev_info(&client->dev, "[elan] Start touchscreen %s in interrupt mode\n",
		ts->input_dev->name);

	// MISC
  	ts->firmware.minor = MISC_DYNAMIC_MINOR;
  	ts->firmware.name = "elan-iap";
  	ts->firmware.fops = &elan_touch_fops;
  ts->firmware.mode = S_IFREG|S_IRWXUGO; 
  	//touch->firmware.this_device.priv

  	if (misc_register(&ts->firmware) < 0)		
  		printk("[ELAN]misc_register failed!!");
  	else
    	printk("[ELAN]misc_register finished!!");
    	
#if TOUCH_EVENTS_FILTER 	
       setup_timer(&ts->timer, ts_timer, (unsigned long)ts);
       printk("[ELAN]Touch event filter ON, Filter timeout : %dms",TS_FILTER_TIMEOUT_MS);
#endif

	return 0;

err_input_register_device_failed:
	if (ts->input_dev)
		input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
	if (ts->elan_wq)
		destroy_workqueue(ts->elan_wq);

err_create_wq_failed:
	kfree(ts);

err_alloc_data_failed:
err_check_functionality_failed:

	return err;
}

static int elan_ktf2k_ts_remove(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
#if DEBUG	
       pr_info("-- ektf_remove\n");
#endif
	elan_touch_sysfs_deinit();
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	free_irq(client->irq, ts);

	if (ts->elan_wq)
		destroy_workqueue(ts->elan_wq);
	input_unregister_device(ts->input_dev);
	kfree(ts);

	return 0;
}

static int elan_ktf2k_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int rc = 0;
	if(power_lock==0)
	{
#if DEBUG	
       pr_info("-- elan suspend\n");
	printk(KERN_INFO "[elan] %s: enter\n", __func__);
#endif	
	disable_irq(client->irq);

	rc = cancel_work_sync(&ts->work);
	if (rc)
		enable_irq(client->irq);

	rc = elan_ktf2k_ts_set_power_state(client, PWR_STATE_DEEP_SLEEP);
	}
	return 0;
}

static int elan_ktf2k_ts_resume(struct i2c_client *client)
{

       int rc = 0, retry = 5;
	if(power_lock==0)
	{
#if DEBUG	   
       pr_info("-- elan resume\n");
	printk(KERN_INFO "[elan] %s: enter\n", __func__);
#endif	
	do {
		rc = elan_ktf2k_ts_set_power_state(client, PWR_STATE_NORMAL);
		rc = elan_ktf2k_ts_get_power_state(client);
		if (rc != PWR_STATE_NORMAL)
			printk(KERN_ERR "[elan] %s: wake up tp failed! err = %d\n",
				__func__, rc);
		else
			break;
	} while (--retry);

	enable_irq(client->irq);
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void elan_ktf2k_ts_early_suspend(struct early_suspend *h)
{
	struct elan_ktf2k_ts_data *ts;
       pr_info("-- ektf_early_suspend\n");
	ts = container_of(h, struct elan_ktf2k_ts_data, early_suspend);
	elan_ktf2k_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void elan_ktf2k_ts_late_resume(struct early_suspend *h)
{
	struct elan_ktf2k_ts_data *ts;
       pr_info("-- ektf_late_resume\n");
	ts = container_of(h, struct elan_ktf2k_ts_data, early_suspend);
	elan_ktf2k_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id elan_ktf2k_ts_id[] = {
	{ ELAN_KTF2K_NAME, 0 },
	{ }
};

static struct i2c_driver ektf2k_ts_driver = {
	.probe		= elan_ktf2k_ts_probe,
	.remove		= elan_ktf2k_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= elan_ktf2k_ts_suspend,
	.resume		= elan_ktf2k_ts_resume,
#endif
	.id_table	= elan_ktf2k_ts_id,
	.driver		= {
		.name = ELAN_KTF2K_NAME,
	},
};

static int __devinit elan_ktf2k_ts_init(void)
{
#if DEBUG
	printk(KERN_INFO "[elan] %s\n", __func__);
#endif
	return i2c_add_driver(&ektf2k_ts_driver);
}

static void __exit elan_ktf2k_ts_exit(void)
{
#if DEBUG
	printk(KERN_INFO "[elan] %s\n", __func__);
#endif
	i2c_del_driver(&ektf2k_ts_driver);
	return;
}

module_init(elan_ktf2k_ts_init);
module_exit(elan_ktf2k_ts_exit);

MODULE_DESCRIPTION("ELAN EKTF2K Touchscreen Driver");
MODULE_LICENSE("GPL");
