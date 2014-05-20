/* drivers/input/touchscreen/ili2103t_ts.c
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
//#ifdef CONFIG_HAS_EARLYSUSPEND
//#include <linux/earlysuspend.h>
//#endif
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <mach/msm_touch.h>
#include <mach/vreg.h>

#include <linux/gpio.h>
#include <linux/i2c.h>   /*for I2C*/
#include <linux/delay.h>
#include <linux/workqueue.h>

//*******************************************************************
#define DEBUG 0
#define HW_RESET 1
#define SW_RESET 0
#define TURN_ON_VREG_L1 1

#define ili2103t_INTERRUPT 82

#if HW_RESET
#define ili2103t_RESET 7
#endif


#define ILI2103t_MAX_COORDINATE_X_VALUE (320 - 1)
#define ILI2103t_MAX_COORDINATE_Y_VALUE (480 - 1)
#define ILI2103t_MAX_DATA_LEN 16

#define TS_DRIVER_NAME "ili2103t"
#define FINGER_NUM	2	  


#define CMD_TOUCH_INFO_GET 0x10
#define CMD_TOUCH_EXTENDTION_DATA_GET 0x11
#define CMD_PANEL_INFO 0x20
#define CMD_ENTER_SLEEP_MODE 0x30
#define CMD_FIRMWARE_VERSION_GET 0x40
#define CMD_PROTOCOL_VERSION_GET 0x42
#define CMD_MASS_PRODUCTION_CALIBRATION 0xcc
#define CMD_CALIBRATION_STATUS 0xcd
#define CMD_CALIBRATION_ERASED 0xce

#define ILI2103T_KEY_MENU 0x00
#define ILI2103T_KEY_HOME 0x01
#define ILI2103T_KEY_BACK 0x02

struct ili2103t {
	struct input_dev	*input;
	char			phys[32];
	struct timer_list timer;
	struct i2c_client	*client;
	struct work_struct work;
	int			irq;
//#ifdef CONFIG_HAS_EARLYSUSPEND
	//struct early_suspend early_suspend;
//#endif
};

static int TOUCH_KEY_PRESSED = 0;
unsigned char TOUCH_KEY_NUM = 0xff;

static irqreturn_t ili2103t_irq(int irq, void *handle)
{
	struct ili2103t *ts = handle;
#if DEBUG	
	printk(KERN_INFO"Ilitek: ili2103t_irq  : %d\n",irq);
#endif	
	disable_irq_nosync(ts->irq);
	schedule_work(&ts->work);
	return IRQ_HANDLED;
}

static void ili2103t_work_func(struct work_struct *work)
{
      struct ili2103t *ts = container_of(work, struct ili2103t, work);
      unsigned char rd_buf[9] = {0};
      int i2c_result = 0;
      unsigned char sd_buf[3] = {0};
      uint16_t x =0, y =0, x2 = 0, y2 =0;
	
#if DEBUG
	printk(KERN_INFO"ili2103t_work_func \n");
#endif	
	sd_buf[0] = 0x10;
	i2c_result = i2c_master_send(ts->client, sd_buf, 1);
	
	if (i2c_result < 0){
#if DEBUG	   	
	            printk(KERN_INFO"ili2103t_work_func :set I2C offset failed\n");
#endif
	            goto out;
	  }
			
       i2c_result = i2c_master_recv(ts->client, rd_buf, 9);

	if (i2c_result < 0){
#if DEBUG	   	
	            printk(KERN_INFO"ili2103t_work_func :read I2C failed\n");
#endif
	            goto out;
	  }

        x= rd_buf[1] + (rd_buf[2] << 8);
        y= rd_buf[3] + (rd_buf[4] << 8);
	  
        x2= rd_buf[5] + (rd_buf[6] << 8);
        y2= rd_buf[7] + (rd_buf[8] << 8);
		
       if(rd_buf[0] == 0x01)
        {		

              if(y > ILI2103t_MAX_COORDINATE_Y_VALUE)
               {
		    if(!TOUCH_KEY_PRESSED)
		    	{				   
		            TOUCH_KEY_PRESSED = 1;
		            TOUCH_KEY_NUM = x/(ILI2103t_MAX_COORDINATE_X_VALUE/3);
					
                          switch(TOUCH_KEY_NUM)
                            {
                                case ILI2103T_KEY_MENU:
			              input_report_key(ts->input, KEY_MENU, TOUCH_KEY_PRESSED);
#if DEBUG	
		                     printk(KERN_INFO"ili2103t 1st Finger press Menu Key");
#endif
			          break;
                               case ILI2103T_KEY_HOME:
			              input_report_key(ts->input, KEY_HOME, TOUCH_KEY_PRESSED);	 
#if DEBUG	
		                     printk(KERN_INFO"ili2103t 1st Finger press Home Key");
#endif
			           break;
			          case ILI2103T_KEY_BACK:
			              input_report_key(ts->input, KEY_BACK, TOUCH_KEY_PRESSED);
#if DEBUG	
		                     printk(KERN_INFO"ili2103t 1st Finger press Back Key");
#endif
			          break;
		               }
		    	  }
                  }
		  else
		   {
	              input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 1);
	              input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x);
	              input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y);
			input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 255);
	              input_mt_sync(ts->input);

	              input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 2);
	              input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x2);
	              input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y2);
		       input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
	              input_mt_sync(ts->input);		 
				  
#if DEBUG	
		         printk(KERN_INFO"ili2103t 1st Finger press x = %d, y = %d",x,y);
#endif
		    }
	 }
	else if(rd_buf[0] == 0x02)
	 {
	       input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 1);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y);
	       input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
	       input_mt_sync(ts->input);		
	 
	       input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 2);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x2);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y2);
		input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 255);
	       input_mt_sync(ts->input);
		   
#if DEBUG			   
		printk(KERN_INFO"ili2103t 2nd Finger press x2 = %d, y2 = %d",x2,y2); 
#endif

	  }
	else if(rd_buf[0] == 0x03)  //two finger
	 {
	       input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 1);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y);
		input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 255);
	       input_mt_sync(ts->input);
		   
	       input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 2);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x2);
	       input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y2);
		input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 255);
	       input_mt_sync(ts->input);
		   
#if DEBUG			   
		printk(KERN_INFO"ili2103t 2 Finger press x1 = %d, y1 = %d,x2 = %d, y2 = %d",x,y,x2,y2); 
#endif
	 }	 
        else
        {
              if(TOUCH_KEY_PRESSED == 1)
               {
                   TOUCH_KEY_PRESSED = 0;
                   switch(TOUCH_KEY_NUM)
                     {
                        case ILI2103T_KEY_MENU:
			       input_report_key(ts->input, KEY_MENU, TOUCH_KEY_PRESSED);
#if DEBUG	
		              printk(KERN_INFO"ili2103t 1st Finger Release Menu Key");
#endif
			   break;
                        case ILI2103T_KEY_HOME:
			       input_report_key(ts->input, KEY_HOME, TOUCH_KEY_PRESSED);	 
#if DEBUG	
		              printk(KERN_INFO"ili2103t 1st Finger Release Home Key");
#endif
			   break;
			   case ILI2103T_KEY_BACK:
			        input_report_key(ts->input, KEY_BACK, TOUCH_KEY_PRESSED);
#if DEBUG	
		              printk(KERN_INFO"ili2103t 1st Finger Release Back Key");
#endif
			   break;
		      }
		      TOUCH_KEY_NUM = 0xff;
		      input_mt_sync(ts->input);
		 }	

		 input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 1);
	        input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x);
	        input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y);
		 input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
	        input_mt_sync(ts->input);

	        input_event(ts->input, EV_ABS, ABS_MT_TRACKING_ID, 2);
	        input_event(ts->input, EV_ABS, ABS_MT_POSITION_X, x2);
	        input_event(ts->input, EV_ABS, ABS_MT_POSITION_Y, y2);	
		 input_event(ts->input, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);  
		 input_mt_sync(ts->input);

#if DEBUG	
		 printk(KERN_INFO"ili2103t 2 Finger Relese"); 
#endif
		 
	 }

	 input_sync(ts->input);

//#if DEBUG	
	//printk(KERN_INFO"ili2103t_work_func re-enable irq done\n");
	//for(i=0;i<8;i++){
		 //printk(KERN_INFO"ili2103t rd_buf[%d] = %d",i ,rd_buf[i]); 
	//}
//#endif
	
out:
	enable_irq(ts->irq);
}

//#ifdef CONFIG_HAS_EARLYSUSPEND
//static void ili_2103t_ts_early_suspend(struct early_suspend *h);
//static void ili_2103t_ts_late_resume(struct early_suspend *h);
//#endif

static int ili2103t_probe(struct i2c_client *client,const struct i2c_device_id *id)
{	
	struct ili2103t *ts = NULL;
	struct input_dev *input_dev = NULL;
	int err = 0;

#if TURN_ON_VREG_L1
	struct vreg *vreg_l1;
    	vreg_l1= vreg_get(NULL, "rfrx1");
    	vreg_set_level(vreg_l1, 3000);
    	vreg_enable(vreg_l1);
#endif

#if DEBUG
	printk(KERN_INFO"ili2103t_probe\n");
#endif

#if HW_RESET
	if (!gpio_request(ili2103t_RESET, "ili2103t_RESET")) {
		if (gpio_tlmm_config(GPIO_CFG(ili2103t_RESET, 0, GPIO_CFG_OUTPUT,
								   GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
				  pr_err("Failed to configure GPIO %d\n",
								   ili2103t_RESET);
	    } else{
			pr_err("Failed to request GPIO%d\n", ili2103t_RESET);
			goto err_free_irq;
	   }
	
	gpio_set_value(ili2103t_RESET,  0);
	mdelay(100);
	gpio_set_value(ili2103t_RESET,  1);
	mdelay(100);
#endif

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;


	ts = kzalloc(sizeof(struct ili2103t), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	INIT_WORK(&ts->work, ili2103t_work_func);

	ts->client = client;

	input_dev->name = TS_DRIVER_NAME;
	input_dev->phys = "msm_touch/input0";
	input_dev->id.bustype = BUS_I2C;

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,ILI2103t_MAX_COORDINATE_X_VALUE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,ILI2103t_MAX_COORDINATE_Y_VALUE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0,255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,	0,FINGER_NUM, 0, 0);
			
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_MSC, input_dev->evbit);

	__set_bit(KEY_MENU, input_dev->keybit);
	__set_bit(KEY_HOME, input_dev->keybit);
	__set_bit(KEY_BACK, input_dev->keybit);
	__set_bit(KEY_SEARCH, input_dev->keybit);
	
	i2c_set_clientdata(client, ts);
	
//#ifdef CONFIG_HAS_EARLYSUSPEND
//#if DEBUG
       //printk(KERN_INFO"-- define CONFIG_HAS_EARLYSUSPEND\n");
//#endif
	//ts->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1;
	//ts->early_suspend.suspend = ili_2103t_ts_early_suspend;
	//ts->early_suspend.resume = ili_2103t_ts_late_resume;
	//register_early_suspend(&ts->early_suspend);
//#endif

	err = gpio_request(ili2103t_INTERRUPT, "gpio_ili2103t0_input");
	if (err)
		goto err_free_mem;

    /*do sat pin config -->input*/
	err = gpio_direction_input(ili2103t_INTERRUPT);
	gpio_tlmm_config(GPIO_CFG(ili2103t_INTERRUPT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

#if DEBUG
	if (err < 0)
		printk(KERN_ERR"ili2103t0 : failed to config the interrupt pin\n");
	else	
		printk(KERN_INFO"ili2103t0 : success to config the interrupt pin\n");
#endif
       ts->irq = client->irq;
	
	err = request_irq(ts->irq, ili2103t_irq, IRQF_TRIGGER_FALLING,
		"ili2103t", ts);

	if (err < 0) {
		dev_err(&client->dev, "irq %d busy?\n", ts->irq);
		goto err_free_mem;
	}
#if DEBUG
	printk(KERN_INFO"ili2103t :request_irq finished\n");
#endif   

	err = input_register_device(input_dev);
	if (err)
	{
		printk(KERN_INFO"ili2103t : input_register_device error\n");
		goto err_free_irq;
	}

	ts->input = input_dev;

	dev_info(&client->dev, "registered with irq (%d)\n", ts->irq);
	return 0;

 err_free_irq:
	free_irq(ts->irq, ts);
 err_free_mem:
	input_free_device(input_dev);
	kfree(ts);
	return err;

}

static int ili2103t_remove(struct i2c_client *client)
{
	struct ili2103t  *ts = i2c_get_clientdata(client);

 #if DEBUG
	printk(KERN_INFO"ili2103t_remove\n");
#endif

//#ifdef CONFIG_HAS_EARLYSUSPEND
	//unregister_early_suspend(&ts->early_suspend);
//#endif

	free_irq(ts->irq, ts);

	input_unregister_device(ts->input);
	kfree(ts);

	return 0;
}

static int ili2103t_suspend(struct i2c_client *client, pm_message_t state)
{
	struct ili2103t	 *ts = i2c_get_clientdata(client);
	unsigned char sd_buf[3] = {0};
	int i2c_result = 0;
	
#if DEBUG
	printk(KERN_INFO"Ilitek: ili2103t_suspend\n");
#endif

	sd_buf[0] = 0x30;
	i2c_result = i2c_master_send(ts->client, sd_buf, 1);

#if DEBUG
	       if (i2c_result < 0)
	       {
	            printk(KERN_INFO"Novatek:nt11002_suspend: failed \n");
	       }
#endif	
	mdelay(100);
	disable_irq(ts->irq);

	return 0;
}
	
static int ili2103t_resume(struct i2c_client *client)
{
	struct ili2103t	 *ts = i2c_get_clientdata(client);

#if DEBUG	
	printk(KERN_INFO"Ilitek: ili2103t_resume\n");
#endif

	gpio_set_value(ili2103t_RESET,  0);
	mdelay(20);
	gpio_set_value(ili2103t_RESET,  1);
	mdelay(100);

	enable_irq(ts->irq);
	return 0;
}

//#ifdef CONFIG_HAS_EARLYSUSPEND
//static void ili_2103t_ts_early_suspend(struct early_suspend *h)
//{
	//struct ili2103t *ts;
//#if DEBUG
       //printk(KERN_INFO"-- ili_early_suspend\n");
//#endif
	//ts = container_of(h, struct ili2103t, early_suspend);
	//ili2103t_suspend(ts->client, PMSG_SUSPEND);
//}

//static void ili_2103t_ts_late_resume(struct early_suspend *h)
//{
	//struct ili2103t *ts;
//#if DEBUG	
       //printk(KERN_INFO"-- ili_late_resume\n");
//#endif
	//ts = container_of(h, struct ili2103t, early_suspend);
	//ili2103t_resume(ts->client);
//}
//#endif

static struct i2c_device_id ili2103t_idtable[] = {
	{ "ili2103t", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ili2103t_idtable);

static struct i2c_driver ili2103t_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ili2103t"
	},
//#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= ili2103t_suspend,
	.resume		= ili2103t_resume,
//#endif
	.id_table	= ili2103t_idtable,
	.probe		= ili2103t_probe,
	.remove		= ili2103t_remove,
};

static int __init ili2103t_init(void)
{
#if DEBUG
        printk(KERN_ERR"Ilitek:%s: mesg", __func__);
#endif
	
	return i2c_add_driver(&ili2103t_driver);
}

static void __exit ili2103t_exit(void)
{
 #if DEBUG
	printk(KERN_INFO"ili2103t_exit\n");
#endif
	
	i2c_del_driver(&ili2103t_driver);
}

module_init(ili2103t_init);
module_exit(ili2103t_exit);

MODULE_AUTHOR("Arima");
MODULE_DESCRIPTION("ili2103t TouchScreen Driver");
MODULE_LICENSE("GPL");
