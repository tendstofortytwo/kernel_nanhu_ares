/* Source for:
 * Cypress CY8CTMA300 Prototype touchscreen driver.
 * drivers/input/touchscreen/cy8c_ts.c
 *
 * Copyright (C) 2009, 2010 Cypress Semiconductor, Inc.
 * Copyright (c) 2010-2012 Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Cypress reserves the right to make changes without further notice
 * to the materials described herein. Cypress does not assume any
 * liability arising out of the application described herein.
 *
 * Contact Cypress Semiconductor at www.cypress.com
 *
 * History:
 *			(C) 2010 Cypress - Update for GPL distribution
 *			(C) 2009 Cypress - Assume maintenance ownership
 *			(C) 2009 Enea - Original prototype
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/input/cy8c_ts.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#define CYPRESS_AUTO_FW_UPDATE 1
//#include <cypress_tma340_fw.h>
#include "cypress_tma340_fw.h"
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>

/* Early-suspend level */
#define CY8C_TS_SUSPEND_LEVEL 1
#endif

#define CY8CTMA300	0x0
#define CY8CTMG200	0x1
#define CY8CTMA340	0x2

#define INVALID_DATA	0xff

#define TOUCHSCREEN_TIMEOUT	(msecs_to_jiffies(10))
#define INITIAL_DELAY		(msecs_to_jiffies(25000))

struct cy8c_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct cy8c_ts_data devices[] = {
	[0] = {
		.x_index = 6,
		.y_index = 4,
		.z_index = 3,
		.id_index = 0,
		.data_reg = 0x3,
		.status_reg = 0x1,
		.update_data = 0x4,
		.touch_bytes = 8,
		.touch_meta_data = 3,
		.finger_size = 70,
	},
	[1] = {
		.x_index = 2,
		.y_index = 4,
		.id_index = 6,
		.data_reg = 0x6,
		.status_reg = 0x5,
		.update_data = 0x1,
		.touch_bytes = 12,
		.finger_size = 70,
	},
	[2] = {
		.x_index = 1,
		.y_index = 3,
		.z_index = 5,
		.id_index = 6,
		.data_reg = 0x2,
		.status_reg = 0,
		.update_data = 0x00,//0x4,
		.touch_bytes = 6,
		.touch_meta_data = 3,
		.finger_size = 70,
	},
};

struct cy8c_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	struct workqueue_struct *wq;
	struct cy8c_ts_platform_data *pdata;
	struct cy8c_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	u8 prev_touches;
	bool is_suspended;
	bool int_pending;
	struct mutex sus_lock;
	u32 pen_irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend		early_suspend;
#endif
};

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
#define normal_operating_mode 0
#define sys_info_mode 1
#define test_mode_FilteredRawCounts 4
#define test_mode_FilteredDifferenceCounts 5
#define test_mode_IdacSettings 6
#define test_mode_InterleavedFilteredRawCountsAndBaselines 7

#define bootloader_mode_update_ap_usb 0x0fd
#define bootloader_mode_update_ap 0xfe
#define bootloader_mode 0xff

#define func_key_num 3
#define func_key_x_dist 250//100
#define func_key1_x 0//56
#define func_key2_x 387//462
#define func_key3_x 774//862
#define func_key_y  1070//1088

#define tp_detect_pin 6
uint8_t func_key_state = 0x00;
int cypress_fw_version = 0x00;
int cypress_mode = bootloader_mode;
#if CYPRESS_AUTO_FW_UPDATE
int fw_update_command_count = 0x00;
#endif
static struct cy8c_ts *cypress_ts;

#define CypressIO                   't'
#define CYPRESS_IOCTL_GET_FW_VERSION      _IOR(CypressIO, 1, int)
#define CYPRESS_IOCTL_RESET      _IOW(CypressIO, 2, int)
#define CYPRESS_IOCTL_GET_STATE      _IOR(CypressIO, 3, int)
#define CYPRESS_IOCTL_SET_STATE      _IOW(CypressIO, 4, int)
#define CYPRESS_IOCTL_LAUNCH_APP      _IOW(CypressIO, 5, int)
#define CYPRESS_IOCTL_CALIBRATION    _IOR(CypressIO, 6, int)


static int cypress_fops_open(struct inode *inode, struct file *filp);
static int cypress_fops_release(struct inode *inode, struct file *filp);
static ssize_t cypress_fops_write(struct file *filp, const char *buff,    size_t count, loff_t *offp);
static ssize_t cypress_fops_read(struct file *filp, char *buff, size_t count, loff_t *offp);
static long cypress_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

struct file_operations cypress_fops = {    
	 .owner=         THIS_MODULE,
        .open=          cypress_fops_open,    
        .write=         cypress_fops_write,    
        .read=	       cypress_fops_read,    
        .release=       cypress_fops_release,    
        .unlocked_ioctl=	 cypress_fops_ioctl,
 };

struct miscdevice cypress_in_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "cypress_dev",
	.fops	= &cypress_fops,
};
#endif

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

static s32 cy8c_ts_write_reg_u8(struct i2c_client *client, u8 reg, u8 val)
{
	s32 data;

	data = i2c_smbus_write_byte_data(client, reg, val);
	if (data < 0)
		dev_err(&client->dev, "error %d in writing reg 0x%x\n",
						 data, reg);

	return data;
}

static s32 cy8c_ts_read_reg_u8(struct i2c_client *client, u8 reg)
{
	s32 data;

	data = i2c_smbus_read_byte_data(client, reg);
	if (data < 0)
		dev_err(&client->dev, "error %d in reading reg 0x%x\n",
						 data, reg);

	return data;
}

static int cy8c_ts_read(struct i2c_client *client, u8 reg, u8 *buf, int num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = 0;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags = I2C_M_RD;
	xfer_msg[1].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 2);
}

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
static int cy8c_ts_write(struct i2c_client *client, u8 *buf, int num)
{
	struct i2c_msg xfer_msg[1];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num;
	xfer_msg[0].flags = I2C_M_TEN;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1);
}

static int cy8c_ts_set_device_mode(struct i2c_client *client,u8 device_mode)
{
     int rc=0;
     u8 hst_mode[2] = {0};
     u8 mode_bit=0;

	   //printk("[Cypress] cy8c_ts_set_device_mode : %d\n",device_mode);
	 
     cypress_mode = device_mode;
     rc = cy8c_ts_read(client, 0x00, hst_mode, 2);	  

     if(rc < 0){
	    dev_err(&client->dev, "cy8c_ts_set_device_mode i2c sanity check failed\n");
	    return rc;
     	}
	 
     if(!(hst_mode[1] & 0x10)){
        hst_mode[0] = (hst_mode[0]& 0x0f);
        hst_mode[0] = ((hst_mode[0]|0x08) + (device_mode << 4));
	      printk("[Cypress] cy8c_ts_set_device_mode : 0x%x, %d\n",hst_mode[0],device_mode);
        rc = cy8c_ts_write_reg_u8(client, 0x00, hst_mode[0]);
	}
     else
	  return bootloader_mode;
	 
	 		 do{
	 	    msleep(500);
        hst_mode[0] = cy8c_ts_read_reg_u8(client,0x00);
        mode_bit = (hst_mode[0]&0x08);
		    printk("[Cypress] cy8c_ts_set_device_mode check mode bit :%x!\n",mode_bit);	
	   }while(mode_bit);
    
    return rc;
}

static int cy8c_ts_get_device_mode(struct i2c_client *client,u8 *device_mode)
{
     int rc=0;
     u8 hst_mode[2] = {0};

     printk("[Cypress] cy8c_ts_get_device_mode\n");

     rc = cy8c_ts_read(client, 0x00, hst_mode, 2);	  

     if(rc < 0){
	    dev_err(&client->dev, "cy8c_ts_get_device_mode i2c sanity check failed\n");
	    return rc;
     	}

    if(hst_mode[1] & 0x10)
	  *device_mode = bootloader_mode;
    else
	  *device_mode = ((hst_mode[0] & 0x70) >> 4) ;

     cypress_mode = *device_mode;
	 
     printk("[Cypress] cy8c_ts_get_device_mode : 0x%x, %d\n",hst_mode[0],cypress_mode);
     return rc;
}

static int cy8c_ts_launch_application(struct i2c_client *client)
{
     int rc;
     u8 bootloader_ap_launch[12] = {0x00,0x00, 0xFF, 0xA5, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

     printk("[Cypress] cy8c_ts_launch_application\n");

      rc = cy8c_ts_read_reg_u8(client, 0x01);
      if (rc < 0)
		   dev_err(&client->dev, "i2c sanity check failed\n");
      else{
                 if(rc & 0x10){
                           rc =cy8c_ts_write(client,bootloader_ap_launch,12);
                           if(rc < 0)
	                           dev_err(&client->dev, "cy8c_ts_launch_application i2c sanity check failed\n");
           	       }
	   }
	
	 msleep(100);
	 
	 rc = cy8c_ts_read_reg_u8(client, 0x01);
	 if (rc < 0)
	          dev_err(&client->dev, "i2c sanity check failed\n");
	 else{
                      if(rc & 0x10)
                         printk("[Cypress] launch application error ! TT_MODE : 0x%x\n",rc);
		        else
			    cypress_mode = normal_operating_mode;
	    }
	 return (rc & 0x10);
}

static int cy8c_ts_get_firmware_version(struct i2c_client *client)
{
     int rc=0;
     u8 bootloader_fw_version[2] = {0};

     //printk("[Cypress] cy8c_ts_get_firmware_version\n");

     rc = cy8c_ts_read(client, 0x0b, bootloader_fw_version, 2);	  

     if(rc < 0){
	    dev_err(&client->dev, "cy8c_ts_get_firmware_version i2c sanity check failed\n");
	    return rc;
     	}
	 
     cypress_fw_version = (bootloader_fw_version[0] << 8) + bootloader_fw_version[1];

     printk("[Cypress] cy8c_ts_get_firmware_version : 0x%x\n",cypress_fw_version);
     return cypress_fw_version;
}

static int cy8c_ts_calibration(struct i2c_client *client)
{
     int rc=0;
	 u8 cypress_state=0,calibration_state =0;
	 u8 pass_bit =0, complt_bit=0;
		
	 printk("[Cypress] cy8c_ts_calibration\n");
	 
	 rc = cy8c_ts_set_device_mode(client,sys_info_mode);
	 
	 if(rc < 0){
		  dev_err(&client->dev, "cy8c_ts_calibration : i2c sanity check failed\n");
		  return rc;
	 }
	 //msleep(2000);

	 rc = cy8c_ts_get_device_mode(client,&cypress_state);
	 
	 if(rc < 0){
		  dev_err(&client->dev, "cy8c_ts_calibration : i2c sanity check failed\n");
		  return rc;
	 }
	 
   printk("[Cypress] cy8c_ts_calibration : cypress_state %d\n",cypress_state);

     if(cypress_state==sys_info_mode){
	 	
	   rc = cy8c_ts_write_reg_u8(client, 0x03, 0x00);

	   if(rc < 0){
		  dev_err(&client->dev, "cy8c_ts_calibration : i2c sanity check failed\n");
		  return rc;
		 }
	   
	   rc = cy8c_ts_write_reg_u8(client, 0x02, 0x20);
	   
	   if(rc < 0){
		  dev_err(&client->dev, "cy8c_ts_calibration : i2c sanity check failed\n");
		  return rc;
		 }
		
		 do{
	 	    msleep(100);
        calibration_state = cy8c_ts_read_reg_u8(client,0x01);
        pass_bit = (calibration_state & 0x80);
		    complt_bit = (calibration_state & 0x02);
		    printk("[Cypress] cy8c_ts_calibration check pass and complete bit!\n");	
	   }while((!pass_bit) || (!complt_bit));
	 
     }
	 else
	    printk("[Cypress] Can't change Device Mode to SystemInfo mode\n");	
	 

	 cy8c_ts_set_device_mode(client,normal_operating_mode);
	 
	 //msleep(2000);

	 cy8c_ts_get_device_mode(client,&cypress_state);

	 if(cypress_state==normal_operating_mode)
		 return 1;
   else
     return 0;
}

static int cy8c_ts_deep_sleep(struct cy8c_ts *ts,u8 on_off)
{
    int rc=0;
    if(cypress_mode == normal_operating_mode){
         if(on_off){
              rc = cy8c_ts_write_reg_u8(ts->client, 0x00, 0x02);
		printk("[Cypress] cy8c_ts_deep_sleep : on\n");
          }
	 else{
	      gpio_direction_output(ts->pdata->irq_gpio,0);
	      msleep(5);
	      gpio_set_value(ts->pdata->irq_gpio, 1);
	      msleep(5);
	      gpio_set_value(ts->pdata->irq_gpio, 0);
	      msleep(5);
	      gpio_set_value(ts->pdata->irq_gpio, 1);
	      msleep(10);
	      printk("[Cypress] cy8c_ts_deep_sleep : off\n");
	      rc = gpio_direction_input(ts->pdata->irq_gpio);
	  }
    }
	return rc;
}
static int cypress_fops_open(struct inode *inode, struct file *filp)
{ 
	printk("[Cypress] cypress_fops_open\n");
       cy8c_ts_deep_sleep(cypress_ts,0);
	return 0;
}

static int cypress_fops_release(struct inode *inode, struct file *filp)
{    
	printk("[Cypress] cypress_fops_release\n");
	return 0;
}

static ssize_t cypress_fops_write(struct file *filp, const char *buff,    size_t count, loff_t *offp)
{  
    int ret;
    u8 tmp_buff[18] = {0};

    printk("[Cypress] cypress_fops_write: count : %d\n",count);

    cypress_mode = bootloader_mode_update_ap_usb;


    if (copy_from_user(tmp_buff, buff, count)) {
        return -EFAULT;
    }

    ret = cy8c_ts_write(cypress_ts->client, tmp_buff, count);

    return ret;
}

static ssize_t cypress_fops_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{    
     int ret=0;

     printk("[Cypress]cypress_fops_read : count : %d\n",count);
  
     ret = cy8c_ts_read(cypress_ts->client, 0x07, buff, count);
      if(ret < 0){
	    printk("cypress_fops_read i2c sanity check failed\n");
	     return ret;
     	}
	  
     ret = ((cy8c_ts_read_reg_u8(cypress_ts->client,0x01)&0x40) >> 6);
		
     return ret;
}

static long cypress_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	//int __user *ip = (int __user *)arg;
	printk("[Cypress] cypress_fops_ioctl\n");

	switch (cmd) {        
		case CYPRESS_IOCTL_GET_FW_VERSION: 
		       cypress_ts->input->id.version = cy8c_ts_get_firmware_version(cypress_ts->client);
			return cypress_fw_version;//cy8c_ts_get_firmware_version(cypress_ts->client);
			break;   
              case CYPRESS_IOCTL_GET_STATE:
			 {
			     u8 cypress_state = 0;
                          cy8c_ts_get_device_mode(cypress_ts->client,&cypress_state);
			     return cypress_state;		  
              	  }
			  break;
		case CYPRESS_IOCTL_SET_STATE:
	                {
			      int device_mode;
			      __get_user(device_mode, (int __user *)arg);
			     cy8c_ts_set_device_mode(cypress_ts->client,(u8) device_mode);
			  }
			   break;
		case CYPRESS_IOCTL_LAUNCH_APP:
		    cy8c_ts_launch_application(cypress_ts->client);
      break;
		case CYPRESS_IOCTL_RESET:
			cypress_mode = bootloader_mode;
			   gpio_set_value(cypress_ts->pdata->resout_gpio, 0);
		       msleep(20);
		       gpio_set_value(cypress_ts->pdata->resout_gpio, 1);
			   msleep(200);
		break;
		case CYPRESS_IOCTL_CALIBRATION:
			   cy8c_ts_deep_sleep(cypress_ts,0);
			return cy8c_ts_calibration(cypress_ts->client);
			   break;
		default:            
			break;   
	}       
	return 0;
}

static void report_func_key(struct cy8c_ts *ts, u16 x, u16 y, u8 state)
{	
      if(state){
      	     if(y > func_key_y){
                  if((x > func_key1_x) && (x < (func_key1_x+func_key_x_dist))){
			     func_key_state=KEY_BACK;
		            input_report_key(ts->input, KEY_BACK, 1);	
			     printk("[Cypress] report_func_key :  Press KEY_BACK\n");
		      }
                  else if((x > func_key2_x) && (x < (func_key2_x+func_key_x_dist))){
		 	    func_key_state=KEY_HOME;
		           input_report_key(ts->input, KEY_HOME, 1);
			    printk("[Cypress] report_func_key :  Press KEY_HOME\n");

		      }
		    else if((x > func_key3_x) && (x < (func_key3_x+func_key_x_dist))){
		  	   func_key_state=KEY_MENU;
		          input_report_key(ts->input, KEY_MENU, 1);	
			   printk("[Cypress] report_func_key :  Press KEY_MENU\n");
		     }
                  else{
      	                 func_key_state= 0xff;  //user dosen't press function key area.
			   printk("[Cypress] report_func_key :  Press Error Key\n");
		     }
	        }			
      	}   
     else{
            if (func_key_state == KEY_BACK) {
		          input_report_key(ts->input, KEY_BACK, 0);
			   printk("[Cypress] report_func_key :  Release KEY_BACK\n");
             } else if (func_key_state == KEY_HOME) {
		          input_report_key(ts->input, KEY_HOME, 0);
			   printk("[Cypress] report_func_key :  Release KEY_HOME\n");
	       } else if (func_key_state == KEY_MENU) {
		          input_report_key(ts->input, KEY_MENU, 0);
			   printk("[Cypress] report_func_key :  Release KEY_MENU\n");
		}else{
			   printk("[Cypress] report_func_key :  Release Error Key\n");
		}
		  
		func_key_state=0;
	}
	input_report_key(ts->input, BTN_TOUCH, 0);
	input_mt_sync(ts->input);
}
#endif

static void report_data(struct cy8c_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
	if (ts->pdata->swap_xy)
		swap(x, y);

	/* handle inverting coordinates */
	if (ts->pdata->invert_x)
		x = ts->pdata->res_x - x;
	if (ts->pdata->invert_y)
		y = ts->pdata->res_y - y;
#endif

	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_PRESSURE, pressure);
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340		
	input_report_key(ts->input, BTN_TOUCH, 1);
#endif
	input_mt_sync(ts->input);
	//printk("[Cypress] report_data id : %d, x : %d, y : %d\n",id,x,y);
}

static void process_tma300_data(struct cy8c_ts *ts)
{
	u8 id, pressure, touches, i;
	// << FerryWu, 2012/08/27, Fix compilation error in JB integration
	//u16 x, y;
	u16 x = 0, y = 0;
	// >> FerryWu, 2012/08/27, Fix compilation error in JB integration
	
	touches = ts->touch_data[ts->dd->touch_index] & 0x0f;

	for (i = 0; i < touches; i++) {
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
		 if(i==0){
		       id = ts->touch_data[ts->dd->id_index] & 0xf0;
		  }
		else{
		       id = ts->touch_data[ts->dd->id_index] & 0x0f;
		  }
#else		
		id = ts->touch_data[i * ts->dd->touch_bytes +
					ts->dd->id_index];
#endif
		pressure = ts->touch_data[i * ts->dd->touch_bytes +
							ts->dd->z_index];
		x = join_bytes(ts->touch_data[i * ts->dd->touch_bytes +
							ts->dd->x_index],
			ts->touch_data[i * ts->dd->touch_bytes +
							ts->dd->x_index + 1]);
		y = join_bytes(ts->touch_data[i * ts->dd->touch_bytes +
							ts->dd->y_index],
			ts->touch_data[i * ts->dd->touch_bytes +
							ts->dd->y_index + 1]);

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
              if (ts->pdata->swap_xy)
		    swap(x, y);

	       /* handle inverting coordinates */
	       if (ts->pdata->invert_x)
		      x = ts->pdata->res_x - x;
	       if (ts->pdata->invert_y)
		      y = ts->pdata->res_y - y;

		 if(y > ts->pdata->dis_max_y)
		  {
		        if((touches ==1) && (ts->prev_touches==0) && (func_key_state==0))
			     report_func_key(ts, x, y,1);
		  }
		 else		
#endif
		report_data(ts, x, y, pressure, id);
             
	}

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
      if(!touches && func_key_state)
	       report_func_key(ts, x, y,0);
#endif
   
      for (i = 0; i < ts->prev_touches - touches; i++) {
		input_report_abs(ts->input, ABS_MT_PRESSURE, 0);
		input_mt_sync(ts->input);
	}

	ts->prev_touches = touches;
	input_sync(ts->input);
}

static void process_tmg200_data(struct cy8c_ts *ts)
{
	u8 id, touches, i;
	u16 x, y;

	touches = ts->touch_data[ts->dd->touch_index];

	if (touches > 0) {
		x = join_bytes(ts->touch_data[ts->dd->x_index],
				ts->touch_data[ts->dd->x_index+1]);
		y = join_bytes(ts->touch_data[ts->dd->y_index],
				ts->touch_data[ts->dd->y_index+1]);
		id = ts->touch_data[ts->dd->id_index];

		report_data(ts, x, y, 255, id - 1);

		if (touches == 2) {
			x = join_bytes(ts->touch_data[ts->dd->x_index+5],
					ts->touch_data[ts->dd->x_index+6]);
			y = join_bytes(ts->touch_data[ts->dd->y_index+5],
				ts->touch_data[ts->dd->y_index+6]);
			id = ts->touch_data[ts->dd->id_index+5];

			report_data(ts, x, y, 255, id - 1);
		}
	} else {
		for (i = 0; i < ts->prev_touches; i++) {
			input_report_abs(ts->input, ABS_MT_PRESSURE,	0);
			input_mt_sync(ts->input);
		}
	}

	input_sync(ts->input);
	ts->prev_touches = touches;
}

static void cy8c_ts_xy_worker(struct work_struct *work)
{
	int rc;
	struct cy8c_ts *ts = container_of(work, struct cy8c_ts,
				 work.work);
	
	mutex_lock(&ts->sus_lock);
	if (ts->is_suspended == true) {
		dev_dbg(&ts->client->dev, "TS is supended\n");
		ts->int_pending = true;
		mutex_unlock(&ts->sus_lock);
		return;
	}
	mutex_unlock(&ts->sus_lock);
	
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
#if CYPRESS_AUTO_FW_UPDATE
       if(cypress_mode == bootloader_mode_update_ap){	   	
	      if(fw_update_command_count < TOUCH_IIC_COMMAND_NUM){
	             int iic_data_num_init;
	             u8 bootloader_iic_data[18] = {0};

	             for(iic_data_num_init = 0 ; iic_data_num_init < 18 ; iic_data_num_init++){
		            bootloader_iic_data[iic_data_num_init] = Cypress_IIC[fw_update_command_count][iic_data_num_init + 1];	
		        }
				 
		      //printk("cy8c_ts_xy_worker : Update FW, Commnad : %d\n",fw_update_command_count);

	             rc =cy8c_ts_write(ts->client,bootloader_iic_data,Cypress_IIC[fw_update_command_count][0]);

		      if(fw_update_command_count == 0)
                         msleep(6);
		      else if(Cypress_IIC[fw_update_command_count][0] != 18)
			    msleep(10);
			  
		      if (rc < 0)
		            printk("cy8c_ts_xy_worker : Update FW Error! Commnad : %d\n",fw_update_command_count);
					
	             fw_update_command_count = fw_update_command_count + 1;
		 }
		else{
		       ts->input->id.version = cy8c_ts_get_firmware_version(ts->client);
		
			printk("[Cypress] cy8c_ts_xy_worker : Firmware Version : 0x%x\n",cypress_fw_version);
                    if(cypress_fw_version == TOUCH_IIC_APPLICATION_VERSION)
		          cy8c_ts_launch_application(ts->client);						   
		      else
			   printk("[Cypress] cy8c_ts_xy_worker : Update Firmware Error !\n");
		}
        }
	else{	
#endif /*CYPRESS_AUTO_FW_UPDATE*/		
#endif
      {
          u8 device_mode=0;
          device_mode = cy8c_ts_read_reg_u8(ts->client, 0x01);	  

          if(device_mode < 0){
	           dev_err(&ts->client->dev, "cy8c_ts_get_device_mode i2c sanity check failed\n");
	     }

          if((device_mode & 0x10) && (cypress_mode==normal_operating_mode)){
                   cy8c_ts_launch_application(ts->client);
         	}
       }

	/* read data from DATA_REG */
	rc = cy8c_ts_read(ts->client, ts->dd->data_reg, ts->touch_data,
							ts->dd->data_size);
	
       if(ts->touch_data[ts->dd->touch_index] & 0x10){
	   	printk("[Cypress] cy8c_ts_xy_worker : Detect a large Object !\n");
		goto schedule;
         }
	
	if (rc < 0) {
		dev_err(&ts->client->dev, "read failed\n");
		goto schedule;
	}

	if (ts->touch_data[ts->dd->touch_index] == INVALID_DATA)
		goto schedule;

	if ((ts->device_id == CY8CTMA300) || (ts->device_id == CY8CTMA340))
		process_tma300_data(ts);
	else
		process_tmg200_data(ts);
	
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
#if CYPRESS_AUTO_FW_UPDATE
	  }
#endif /*CYPRESS_AUTO_FW_UPDATE*/
#endif

schedule:
	enable_irq(ts->pen_irq);
	
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
       if(cypress_mode != normal_operating_mode)
	   	return;
#endif
	/* write to STATUS_REG to update coordinates*/
	rc = cy8c_ts_write_reg_u8(ts->client, ts->dd->status_reg,
						ts->dd->update_data);
	if (rc < 0) {
		dev_err(&ts->client->dev, "write failed, try once more\n");

		rc = cy8c_ts_write_reg_u8(ts->client, ts->dd->status_reg,
						ts->dd->update_data);
		if (rc < 0)
			dev_err(&ts->client->dev, "write failed, exiting\n");
	}
}

static irqreturn_t cy8c_ts_irq(int irq, void *dev_id)
{
	struct cy8c_ts *ts = dev_id;
	
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
       if((cypress_mode ==  bootloader_mode) || (cypress_mode == bootloader_mode_update_ap_usb)||(cypress_mode == sys_info_mode)){
		return IRQ_HANDLED;
         }
#endif

	disable_irq_nosync(irq);

	queue_delayed_work(ts->wq, &ts->work, 0);

	return IRQ_HANDLED;
}

static int cy8c_ts_init_ts(struct i2c_client *client, struct cy8c_ts *ts)
{
	struct input_dev *input_device;
	int rc = 0;

	ts->dd = &devices[ts->device_id];

	if (!ts->pdata->nfingers) {
		dev_err(&client->dev, "Touches information not specified\n");
		return -EINVAL;
	}

	if (ts->device_id == CY8CTMA300) {
		if (ts->pdata->nfingers > 10) {
			dev_err(&client->dev, "Touches >=1 & <= 10\n");
			return -EINVAL;
		}
		ts->dd->data_size = ts->pdata->nfingers * ts->dd->touch_bytes +
						ts->dd->touch_meta_data;
		ts->dd->touch_index = ts->pdata->nfingers *
						ts->dd->touch_bytes;
	} else if (ts->device_id == CY8CTMG200) {
		if (ts->pdata->nfingers > 2) {
			dev_err(&client->dev, "Touches >=1 & <= 2\n");
			return -EINVAL;
		}
		ts->dd->data_size = ts->dd->touch_bytes;
		ts->dd->touch_index = 0x0;
	} else if (ts->device_id == CY8CTMA340) {
		if (ts->pdata->nfingers > 10) {
			dev_err(&client->dev, "Touches >=1 & <= 10\n");
			return -EINVAL;
		}
		ts->dd->data_size = ts->pdata->nfingers * ts->dd->touch_bytes +
						ts->dd->touch_meta_data;
		ts->dd->touch_index = 0x0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);
	if (!ts->touch_data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	ts->prev_touches = 0;

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	ts->input = input_device;
	input_device->name = ts->pdata->ts_name;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

	__set_bit(EV_ABS, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);

	if (ts->device_id == CY8CTMA340) {
		/* set up virtual key */
		__set_bit(EV_KEY, input_device->evbit);
		/* set dummy key to make driver work with virtual keys */
		input_set_capability(input_device, EV_KEY, KEY_PROG1);	
	}
	
#if CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
	set_bit(BTN_TOUCH, input_device->keybit);
	set_bit(KEY_BACK, input_device->keybit);
	set_bit(KEY_MENU, input_device->keybit);
	set_bit(KEY_HOME, input_device->keybit);
#endif	

	input_set_abs_params(input_device, ABS_MT_POSITION_X,
			ts->pdata->dis_min_x, ts->pdata->dis_max_x, 0, 0);
	input_set_abs_params(input_device, ABS_MT_POSITION_Y,
			ts->pdata->dis_min_y, ts->pdata->dis_max_y, 0, 0);
	input_set_abs_params(input_device, ABS_MT_PRESSURE,
			ts->pdata->min_touch, ts->pdata->max_touch, 0, 0);
	input_set_abs_params(input_device, ABS_MT_TRACKING_ID,
			ts->pdata->min_tid, ts->pdata->max_tid, 0, 0);
	input_set_abs_params(input_device, ABS_MT_TOUCH_MAJOR,
			ts->pdata->min_touch, ts->pdata->max_touch, 0, 0);
		
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
       cy8c_ts_get_firmware_version(ts->client);
	ts->input->id.version = cypress_fw_version;
	ts->input->id.vendor = gpio_get_value(tp_detect_pin);
	printk("[Cypress] cy8c_ts_init_ts : Firmware Version : 0x%x\n",cypress_fw_version);

	if (misc_register(&cypress_in_misc) < 0)
   	         printk("[Cypress] misc_register failed!!");
 #endif
 
	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}

	INIT_DELAYED_WORK(&ts->work, cy8c_ts_xy_worker);

	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}

#ifdef CONFIG_PM
static int cy8c_ts_suspend(struct device *dev)
{
	struct cy8c_ts *ts = dev_get_drvdata(dev);
	int rc = 0;

	if (device_may_wakeup(dev)) {
		/* mark suspend flag */
		mutex_lock(&ts->sus_lock);
		ts->is_suspended = true;
		mutex_unlock(&ts->sus_lock);

		enable_irq_wake(ts->pen_irq);
	} else {
		disable_irq_nosync(ts->pen_irq);

		rc = cancel_delayed_work_sync(&ts->work);

		if (rc) {
			/* missed the worker, write to STATUS_REG to
			   acknowledge interrupt */
			rc = cy8c_ts_write_reg_u8(ts->client,
				ts->dd->status_reg, ts->dd->update_data);
			if (rc < 0) {
				dev_err(&ts->client->dev,
					"write failed, try once more\n");

				rc = cy8c_ts_write_reg_u8(ts->client,
					ts->dd->status_reg,
					ts->dd->update_data);
				if (rc < 0)
					dev_err(&ts->client->dev,
						"write failed, exiting\n");
			}

			enable_irq(ts->pen_irq);
		}
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
		gpio_free(ts->pdata->irq_gpio);
		if (ts->pdata->power_on) {
			rc = ts->pdata->power_on(0);
			if (rc) {
				dev_err(dev, "unable to goto suspend\n");
				return rc;
			}
		}
#else
              cy8c_ts_deep_sleep(ts,1);
#endif		
	}
	return 0;
}

static int cy8c_ts_resume(struct device *dev)
{
	struct cy8c_ts *ts = dev_get_drvdata(dev);
	int rc = 0;

	if (device_may_wakeup(dev)) {
		disable_irq_wake(ts->pen_irq);

		mutex_lock(&ts->sus_lock);
		ts->is_suspended = false;

		if (ts->int_pending == true) {
			ts->int_pending = false;

			/* start a delayed work */
			queue_delayed_work(ts->wq, &ts->work, 0);
		}
		mutex_unlock(&ts->sus_lock);

	} else {
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
		if (ts->pdata->power_on) {
			rc = ts->pdata->power_on(1);
			if (rc) {
				dev_err(dev, "unable to resume\n");
				return rc;
			}
		}

		/* configure touchscreen interrupt gpio */
		rc = gpio_request(ts->pdata->irq_gpio, "cy8c_irq_gpio");
		if (rc) {
			pr_err("%s: unable to request gpio %d\n",
				__func__, ts->pdata->irq_gpio);
			goto err_power_off;
		}

		rc = gpio_direction_input(ts->pdata->irq_gpio);
		if (rc) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, ts->pdata->irq_gpio);
			goto err_gpio_free;
		}
#else
              cy8c_ts_deep_sleep(ts,0);
#endif
		enable_irq(ts->pen_irq);

		/* Clear the status register of the TS controller */
		rc = cy8c_ts_write_reg_u8(ts->client,
			ts->dd->status_reg, ts->dd->update_data);
		if (rc < 0) {
			dev_err(&ts->client->dev,
				"write failed, try once more\n");

			rc = cy8c_ts_write_reg_u8(ts->client,
				ts->dd->status_reg,
				ts->dd->update_data);
			if (rc < 0)
				dev_err(&ts->client->dev,
					"write failed, exiting\n");
		}
	}
	return 0;
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
err_gpio_free:
	gpio_free(ts->pdata->irq_gpio);
err_power_off:
	if (ts->pdata->power_on)
		rc = ts->pdata->power_on(0);
	return rc;
#endif	
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cy8c_ts_early_suspend(struct early_suspend *h)
{
	struct cy8c_ts *ts = container_of(h, struct cy8c_ts, early_suspend);
	cy8c_ts_suspend(&ts->client->dev);
}

static void cy8c_ts_late_resume(struct early_suspend *h)
{
	struct cy8c_ts *ts = container_of(h, struct cy8c_ts, early_suspend);
	cy8c_ts_resume(&ts->client->dev);
}
#endif

static struct dev_pm_ops cy8c_ts_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= cy8c_ts_suspend,
	.resume		= cy8c_ts_resume,
#endif
};
#endif

static int __devinit cy8c_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct cy8c_ts *ts;
	struct cy8c_ts_platform_data *pdata = client->dev.platform_data;
	int rc, temp_reg;
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
#if CYPRESS_AUTO_FW_UPDATE
       int ic_fw, sw_fw, ic_checksum;
#endif
#endif	   
	if (!pdata) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -EIO;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	/* Enable runtime PM ops, start in ACTIVE mode */
	rc = pm_runtime_set_active(&client->dev);
	if (rc < 0)
		dev_dbg(&client->dev, "unable to set runtime pm state\n");
	pm_runtime_enable(&client->dev);

	ts->client = client;
	ts->pdata = pdata;
	i2c_set_clientdata(client, ts);
	ts->device_id = id->driver_data;

	if (ts->pdata->dev_setup) {
		rc = ts->pdata->dev_setup(1);
		if (rc < 0) {
			dev_err(&client->dev, "dev setup failed\n");
			goto error_touch_data_alloc;
		}
	}

	/* power on the device */
	if (ts->pdata->power_on) {
		rc = ts->pdata->power_on(1);
		if (rc) {
			pr_err("%s: Unable to power on the device\n", __func__);
			goto error_dev_setup;
		}
	}

	/* read one byte to make sure i2c device exists */
	if (id->driver_data == CY8CTMA300)
		temp_reg = 0x01;
	else if (id->driver_data == CY8CTMA340)
		temp_reg = 0x00;
	else
		temp_reg = 0x05;

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
	gpio_set_value(ts->pdata->resout_gpio, 0);
       msleep(2);
       gpio_set_value(ts->pdata->resout_gpio, 1);
	     msleep(25);
#endif

	rc = cy8c_ts_read_reg_u8(client, temp_reg);
	if (rc < 0) {
		dev_err(&client->dev, "i2c sanity check failed\n");
		goto error_power_on;
	}
	
	ts->is_suspended = false;
	ts->int_pending = false;
	mutex_init(&ts->sus_lock);

	rc = cy8c_ts_init_ts(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "CY8CTMG200-TMA300 init failed\n");
		goto error_mutex_destroy;
	}

	if (ts->pdata->resout_gpio < 0)
		goto config_irq_gpio;
	
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
	/* configure touchscreen reset out gpio */
	rc = gpio_request(ts->pdata->resout_gpio, "cy8c_resout_gpio");
	if (rc) {
		pr_err("%s: unable to request gpio %d\n",
			__func__, ts->pdata->resout_gpio);
		goto error_uninit_ts;
	}

	rc = gpio_direction_output(ts->pdata->resout_gpio, 0);
	if (rc) {
		pr_err("%s: unable to set direction for gpio %d\n",
			__func__, ts->pdata->resout_gpio);
		goto error_resout_gpio_dir;
	}

	 /* reset gpio stabilization time */
	 msleep(20);
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
       cypress_ts = ts;
       cypress_mode = bootloader_mode;
#if CYPRESS_AUTO_FW_UPDATE	   
       fw_update_command_count = 0x00;
#endif /*CYPRESS_AUTO_FW_UPDATE*/
#endif	 
config_irq_gpio:
	/* configure touchscreen interrupt gpio */
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
	rc = gpio_tlmm_config(GPIO_CFG(ts->pdata->irq_gpio, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);  
	if (rc) {
		pr_err("%s: unable to config gpio %d\n",
			__func__, ts->pdata->irq_gpio);
	}
#endif
	rc = gpio_request(ts->pdata->irq_gpio, "cy8c_irq_gpio");
	if (rc) {
		pr_err("%s: unable to request gpio %d\n",
			__func__, ts->pdata->irq_gpio);
		goto error_irq_gpio_req;
	}

	rc = gpio_direction_input(ts->pdata->irq_gpio);
	if (rc) {
		pr_err("%s: unable to set direction for gpio %d\n",
			__func__, ts->pdata->irq_gpio);
		goto error_irq_gpio_dir;
	}
	
	ts->pen_irq = gpio_to_irq(ts->pdata->irq_gpio);
	rc = request_irq(ts->pen_irq, cy8c_ts_irq,
				IRQF_TRIGGER_FALLING,
				ts->client->dev.driver->name, ts);
	if (rc) {
		dev_err(&ts->client->dev, "could not request irq\n");
		goto error_req_irq_fail;
	}
		 
	/* Clear the status register of the TS controller */
	rc = cy8c_ts_write_reg_u8(ts->client, ts->dd->status_reg,
						ts->dd->update_data);
	if (rc < 0) {
		/* Do multiple writes in case of failure */
		dev_err(&ts->client->dev, "%s: write failed %d"
				"trying again\n", __func__, rc);
		rc = cy8c_ts_write_reg_u8(ts->client,
			ts->dd->status_reg, ts->dd->update_data);
		if (rc < 0) {
			dev_err(&ts->client->dev, "%s: write failed"
				"second time(%d)\n", __func__, rc);
		}
	}
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
#if CYPRESS_AUTO_FW_UPDATE
       ic_checksum = (cy8c_ts_read_reg_u8(client,0x01) & 0x0001);
       ic_fw = (cypress_fw_version & 0x00ff);
       sw_fw = (TOUCH_IIC_APPLICATION_VERSION & 0x00ff);
	printk("[Cypress] cy8c_ts_probe : ic_fw : 0x%x, sw_fw : 0x%x, ic_checksum : 0x%x\n",ic_fw,sw_fw,ic_checksum);  
       if(((ic_fw < sw_fw) && (ic_fw >= 0x0002)) || (ic_checksum == 0)){
              printk("[Cypress] cy8c_ts_probe : firmware doesn't match, update FW !\n");  
		cypress_mode = bootloader_mode_update_ap;	
          }
       else{
	       printk("[Cypress] cy8c_ts_probe : firmware match !\n");
		cy8c_ts_launch_application(ts->client);  		  
          }
#else
       cy8c_ts_launch_application(ts->client);
#endif
#endif

	device_init_wakeup(&client->dev, ts->pdata->wakeup);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						CY8C_TS_SUSPEND_LEVEL;
	ts->early_suspend.suspend = cy8c_ts_early_suspend;
	ts->early_suspend.resume = cy8c_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif		
		
	return 0;
error_req_irq_fail:
error_irq_gpio_dir:
	gpio_free(ts->pdata->irq_gpio);
error_irq_gpio_req:
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
error_resout_gpio_dir:
#endif
	if (ts->pdata->resout_gpio >= 0)
		gpio_free(ts->pdata->resout_gpio);
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340
error_uninit_ts:
	destroy_workqueue(ts->wq);
	input_unregister_device(ts->input);
	kfree(ts->touch_data);
#endif
error_mutex_destroy:
	mutex_destroy(&ts->sus_lock);
error_power_on:
	if (ts->pdata->power_on)
		ts->pdata->power_on(0);
error_dev_setup:
	if (ts->pdata->dev_setup)
		ts->pdata->dev_setup(0);
error_touch_data_alloc:
	pm_runtime_set_suspended(&client->dev);
	pm_runtime_disable(&client->dev);
	kfree(ts);
	return rc;
}

static int __devexit cy8c_ts_remove(struct i2c_client *client)
{
	struct cy8c_ts *ts = i2c_get_clientdata(client);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&ts->early_suspend);
#endif
	pm_runtime_set_suspended(&client->dev);
	pm_runtime_disable(&client->dev);

	device_init_wakeup(&client->dev, 0);

	cancel_delayed_work_sync(&ts->work);

	free_irq(ts->pen_irq, ts);

	gpio_free(ts->pdata->irq_gpio);

	if (ts->pdata->resout_gpio >= 0)
		gpio_free(ts->pdata->resout_gpio);

	destroy_workqueue(ts->wq);

	input_unregister_device(ts->input);

	mutex_destroy(&ts->sus_lock);

	if (ts->pdata->power_on)
		ts->pdata->power_on(0);

	if (ts->pdata->dev_setup)
		ts->pdata->dev_setup(0);

	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}

static const struct i2c_device_id cy8c_ts_id[] = {
	{"cy8ctma300", CY8CTMA300},
	{"cy8ctmg200", CY8CTMG200},
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CY8CTMA340	
	{"cy8ctma340_touch", CY8CTMA340},
#else
	{"cy8ctma340", CY8CTMA340},
#endif	
	{}
};
MODULE_DEVICE_TABLE(i2c, cy8c_ts_id);


static struct i2c_driver cy8c_ts_driver = {
	.driver = {
		.name = "cy8c_ts",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &cy8c_ts_pm_ops,
#endif
	},
	.probe		= cy8c_ts_probe,
	.remove		= __devexit_p(cy8c_ts_remove),
	.id_table	= cy8c_ts_id,
};

static int __init cy8c_ts_init(void)
{
	return i2c_add_driver(&cy8c_ts_driver);
}
/* Making this as late init to avoid power fluctuations
 * during LCD initialization.
 */
late_initcall(cy8c_ts_init);

static void __exit cy8c_ts_exit(void)
{
	return i2c_del_driver(&cy8c_ts_driver);
}
module_exit(cy8c_ts_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CY8CTMA340-CY8CTMG200 touchscreen controller driver");
MODULE_AUTHOR("Cypress");
MODULE_ALIAS("platform:cy8c_ts");
