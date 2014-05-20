/*
 * License Terms: GNU General Public License v2
 *
 * Simple driver for National Semiconductor LM3533 LEDdriver chip *
 * 
 * based on leds-lm3533.c by SHIH EDISON  
 */
 

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/leds-lm3533.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>

#ifndef DEBUG
#define DEBUG
#endif

#define LM3533_DRIVER_NAME "leds-lm3533"
static struct i2c_client* lm3533_client = NULL;
static struct workqueue_struct* LED_WorkQueue = NULL;
//static u8 keep_on_triggered_now = 0;
//static u8 keep_on_triggered_next = 0;
static u8 lcm_has_on;
static int keep_backlight_brightness=255;
static u8 backlight_has_on;	
static int log_flag_setting=0;  //20121225 Jim add for debug  

u8 brightness_table[]=
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0xB0,0xB2,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

struct rgb_brightness{
	int light_red;
	int light_green;
	int light_blue;
};	

/* Saved data */
struct lm3533_led_data {
	u8 id;
	u8 keep_on_triggered_next;
	u8 keep_on_triggered_now;
	enum lm3533_type type;
	int lm3533_led_brightness;
	unsigned long fade_time;
	unsigned long keep_on_time;
	unsigned long keep_off_time;
	unsigned long lm3533_rgb_brightness;	
	struct led_classdev ldev;
	struct i2c_client *client;
	struct delayed_work thread;
	struct delayed_work thread_register_keep;  // use for keep light timer
	struct delayed_work thread_set_keep;
	unsigned int debug_flag;	//20121225 Jim add for debug  
};

struct lm3533_data {
	struct mutex lock;
	struct i2c_client *client;
	struct lm3533_led_data leds[LM3533_LEDS_MAX];
};



static ssize_t lm3533_keep_on_time_write(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct lm3533_led_data *led = container_of(led_cdev, struct lm3533_led_data, ldev);
	static unsigned long keep_time_data ;	
	
	if (strict_strtoul(buf, 10, &keep_time_data))
		return -EINVAL;	
	
	led->keep_on_time = keep_time_data/65536;
	led->keep_off_time = keep_time_data%65536;

      if(log_flag_setting){
	printk(KERN_EMERG "%s : %s \n",__func__, led_cdev->name);
	printk(KERN_EMERG "keep_on_time : %lu , keep_off_time : %lu \n", led->keep_on_time,led->keep_off_time);	}
	
	led->keep_on_triggered_next = ACTION_START;
	if(led->keep_on_time)
		queue_delayed_work(LED_WorkQueue, &led->thread_register_keep, msecs_to_jiffies(10));
	else
		printk(KERN_EMERG "%s : wrong value of setting keep_on_time (%lu) \n",__func__,led->keep_on_time);	

	return count;
}

static ssize_t lm3533_fade_time_write(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);	
	struct lm3533_led_data *led = container_of(led_cdev, struct lm3533_led_data, ldev);
	static unsigned long fade_time;

	if (strict_strtoul(buf, 10, &fade_time))
		return -EINVAL;
      if(log_flag_setting){
	printk(KERN_EMERG "%s: %s, %lu \n",__func__, led_cdev->name, fade_time);}
	led->fade_time = fade_time;	
			
	return count;
}

static ssize_t lm3533_runtime_fade_time_write(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);	
	static unsigned long runtime_fade_time;

	if (strict_strtoul(buf, 10, &runtime_fade_time))
		return -EINVAL;
      if(log_flag_setting){
	printk(KERN_EMERG "%s: %s, %lu \n",__func__, led_cdev->name, runtime_fade_time);}

	return count;
}

static ssize_t lm3533_rgb_brightness_write(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);	
	struct lm3533_led_data *led = container_of(led_cdev, struct lm3533_led_data, ldev);
	u8 sns_data[3];
	static unsigned long rgb_brightness;

	if (strict_strtoul(buf, 10, &rgb_brightness))
		return -EINVAL;	

	//led->lm3533_rgb_brightness = rgb_brightness;
	
	if (rgb_brightness==10066329)		// change silk theme display
	{
		rgb_brightness=14352383;
		led->lm3533_rgb_brightness = 14352383;
	}
	else if (rgb_brightness==26184)	// change turquoise theme display
	{
		rgb_brightness=8388590;
		led->lm3533_rgb_brightness = 8388590;
	}
	else if (rgb_brightness==3264280)	// change emerald theme display
	{
		rgb_brightness=3342130;
		led->lm3533_rgb_brightness = 3342130;
	}
	else if (rgb_brightness==3355647)	// change sapphire theme display
	{
		rgb_brightness=6535423;
		led->lm3533_rgb_brightness = 6535423;
	}
	else if (rgb_brightness==16758297)	// change gold theme display
	{
		rgb_brightness=15793934;
		led->lm3533_rgb_brightness = 15793934;
	}
	else if (rgb_brightness==13369446)	// change ruby theme display
	{
		rgb_brightness=16738740;
		led->lm3533_rgb_brightness = 16738740;
	}
	else if (rgb_brightness==10027263)	// change amethyst theme display
	{
		rgb_brightness=11149550;
		led->lm3533_rgb_brightness = 11149550;
	}
	else
	{
		led->lm3533_rgb_brightness = rgb_brightness;
	}
      if(log_flag_setting){
	printk(KERN_EMERG "%s: %s, %lu \n",__func__, led_cdev->name, rgb_brightness);}

	/*++this function is for keep on time ++*/	
		
	cancel_delayed_work_sync(&led->thread_register_keep);
	cancel_delayed_work_sync(&led->thread_set_keep);

	if(rgb_brightness)
	{
		sns_data[0] = brightness_table[(rgb_brightness>>16)&255];  //red led    (SNS light setting)
		sns_data[1] = brightness_table[(rgb_brightness>>8)&255];
		sns_data[2] = brightness_table[(rgb_brightness)&255];
		i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_BRIGHTNESS_REGISTER_C,3,sns_data);
		memset(sns_data,0,3);
		i2c_smbus_read_i2c_block_data(lm3533_client,LM3533_BRIGHTNESS_REGISTER_C,3,sns_data);

	      if(log_flag_setting){
		//printk(KERN_EMERG "%lu ,  = %lu , %lu  \n ",(rgb_brightness>>16)&255,(rgb_brightness>>8)&255,(rgb_brightness)&255);	
		printk(KERN_EMERG "sns_data[0] = %d , sns_data[1] = %d , sns_data[2]=%d  ",sns_data[0],sns_data[1],sns_data[2]);}	
	}	

	queue_delayed_work(LED_WorkQueue, &led->thread, msecs_to_jiffies(10));

	return count;
}

//[Arima Edison] add exception for charger usage++
static int lm3533_led_set(struct lm3533_led_data *led,unsigned long brightness);
static ssize_t lm3533_charger_brightness_write(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);	
	struct lm3533_led_data *led = container_of(led_cdev, struct lm3533_led_data, ldev);
	static unsigned long charger_brightness;

	if (strict_strtoul(buf, 10, &charger_brightness))
		return -EINVAL;	

	if(led->id==2)
	{
		led->lm3533_led_brightness = charger_brightness;
		      if(log_flag_setting){
		printk(KERN_EMERG "%s: %s, %lu \n",__func__, led_cdev->name, charger_brightness);}
		lm3533_led_set(led,led->lm3533_led_brightness);
	}

	return count;
}
//[Arima Edison] add exception for charger usage--

// 20121225 Jim add for debug  ( 0 for close / 1 for open log)  
static ssize_t lm3533_debug_flag_write (struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);	
	struct lm3533_led_data *led = container_of(led_cdev, struct lm3533_led_data, ldev);
	static unsigned long debug_flag;

	if (strict_strtoul(buf, 10, &debug_flag))
		return -EINVAL;

	printk(KERN_EMERG "%s: Enter LED Debug Mode \n",__func__);

	log_flag_setting=((debug_flag==1)?1:0);
	
	led->debug_flag = debug_flag;	
			
	return count;
}


static DEVICE_ATTR(debug_flag , 0644 , NULL , lm3533_debug_flag_write);
static DEVICE_ATTR(keep_on_time , 0644 , NULL , lm3533_keep_on_time_write);
static DEVICE_ATTR(fade_time , 0644 , NULL , lm3533_fade_time_write);
static DEVICE_ATTR(runtime_fade_time , 0644 , NULL , lm3533_runtime_fade_time_write);
static DEVICE_ATTR(rgb_brightness , 0644 , NULL , lm3533_rgb_brightness_write);
//[Arima Edison] add exception for charger usage++
static DEVICE_ATTR(charger_brightness , 0644 , NULL , lm3533_charger_brightness_write);
//[Arima Edison] add exception for charger usage--

static struct attribute *lm3533_attributes[] = {
	&dev_attr_keep_on_time.attr,
	&dev_attr_fade_time.attr,
	&dev_attr_runtime_fade_time.attr,	
	&dev_attr_rgb_brightness.attr,
	&dev_attr_debug_flag.attr,
	NULL
};

static struct attribute_group lm3533_attribute_group = {
	.attrs = lm3533_attributes
};

//[Arima Edison] add exception for charger usage++
static struct attribute *lm3533_backlight_attributes[] = {
	&dev_attr_fade_time.attr,
	&dev_attr_runtime_fade_time.attr,	
	&dev_attr_charger_brightness.attr,
	NULL
};

static struct attribute_group lm3533_backlight_attribute_group = {
	.attrs = lm3533_backlight_attributes
};
//[Arima Edison] add exception for charger usage--

static int lm3533_led_set(struct lm3533_led_data *led,unsigned long brightness)
{
	//struct lm3533_data *data = i2c_get_clientdata(led->client);
	u8 id = led->id;
	static int err;
	static int brightness_temp = 0;
	static int rgb_enable = 0;
      //[Arima Jim] add++ set Start Up/Shut Down Ramp Rates for SNS&keypad
      //                         set Runtime Ramp Rates for backlight
	  static u8 BANK_ENABLE= 0x01;
	//[Arima Jim] add-- 
	static u8 BANK_ENABLE_ORIGINAL = 0x00;	
	
	switch (id) {
		
	case LM3533_LED0:			
		
		BANK_ENABLE = BANK_ENABLE &  35;	  //100011	
		//printk(KERN_EMERG "lm3533_led_set red:  %lu green: %lu blue: %lu \n",(brightness>>16)&255, 
		//(brightness>>8)&255,(brightness)&255);
		rgb_enable = 100*((brightness>>16)&255?1:0)+10*((brightness>>8)&255?1:0)+
			((brightness)&255?1:0);

		i2c_smbus_write_byte_data(lm3533_client,LM3533_STARTUP_SHUTDOWN_RAMP_RATES,0x1B);
		//printk(KERN_EMERG "LM3533_STARTUP_SHUTDOWN_RAMP_RATES : 0x1B \n");		
		
		switch (rgb_enable){			
		case SNS_NO:	      // 000	
			//printk(KERN_NOTICE "SNS off \n");				
			break;
		case SNS_B:	      //  100
			BANK_ENABLE = BANK_ENABLE |  16;	  //010000
			//printk(KERN_NOTICE "SNS B \n");
			break;
		case SNS_G:	      //  010
			BANK_ENABLE = BANK_ENABLE |  8;	  //001000
			//printk(KERN_NOTICE "SNS G \n");
			break;
		case SNS_GB:	//110
			BANK_ENABLE = BANK_ENABLE |  24;	  //011000
			//printk(KERN_NOTICE "SNS GB \n");
			break;
		case SNS_R:      //001	
			BANK_ENABLE = BANK_ENABLE |  4;	  //000100
			//printk(KERN_NOTICE "SNS R \n");
			break;	
		case SNS_RB:	//101
			BANK_ENABLE = BANK_ENABLE |  20;	  //010100
			//printk(KERN_NOTICE "SNS RB \n");
			break;
		case SNS_RG:	//011
			BANK_ENABLE = BANK_ENABLE |  12;	  //001100
			//printk(KERN_NOTICE "SNS RG \n");
			break;	
		case SNS_RGB:	//111
			BANK_ENABLE = BANK_ENABLE |  28;	  //011100
			//printk(KERN_NOTICE "SNS RGB \n");
			break;	
		default:
			//printk(KERN_NOTICE "NOT SUPPOT SNS LIGHT \n");
			break;	
		}			
		break;
	case LM3533_LED1:	// LVLED 4 BANK F

		i2c_smbus_write_byte_data(lm3533_client,LM3533_STARTUP_SHUTDOWN_RAMP_RATES,0x1B);
		//printk(KERN_EMERG "LM3533_STARTUP_SHUTDOWN_RAMP_RATES : 0x1B \n");
	
		if(brightness)	
		{
			BANK_ENABLE = BANK_ENABLE |33;		//100001
			//err = i2c_smbus_write_byte_data(lm3533_client,LM3533_BRIGHTNESS_REGISTER_F,0xff);	
			if (err)
				printk(KERN_EMERG "button on error \n");
	if(log_flag_setting){
			printk(KERN_EMERG "button on \n");}
		}
		else
		{ 
			BANK_ENABLE = BANK_ENABLE &  31;		//011111	
			//err = i2c_smbus_write_byte_data(lm3533_client,LM3533_BRIGHTNESS_REGISTER_F,0x00);
			if (err)
				printk(KERN_EMERG "button off error \n");
			printk(KERN_EMERG "button off \n");
		}	
		break;
	case LM3533_LED2:  //HVLED		       //[Arima Jim] Modify Runtime Ramp Rates for backlight
	
			if(led->lm3533_led_brightness)
			{			
				if(!brightness_temp)
				{
					//BANK_ENABLE = BANK_ENABLE | 1;	//000001
					brightness_temp = 1;	
					if(log_flag_setting){
					printk(KERN_EMERG "backlight on \n");}		
					//i2c_smbus_write_byte_data(lm3533_client,LM3533_STARTUP_SHUTDOWN_RAMP_RATES,0x08);
					//printk(KERN_EMERG "LM3533_STARTUP_SHUTDOWN_RAMP_RATES : 0x08 \n");
				}
				
				/*++ Huize - 20120830 Add for dimming unused ++*/
				i2c_smbus_write_byte_data(lm3533_client,LM3533_RUN_TIME_RAMP_RATES, (led->fade_time > 0) ? 0x08 : 0);

				(led->fade_time > 0) ? 0 : msleep(20);
				
				/*-- Huize - 20120830 Add for dimming unused --*/
					
				err = i2c_smbus_write_byte_data(led->client, LM3533_BRIGHTNESS_REGISTER_A,brightness_table[led->lm3533_led_brightness]);	//backlight offset			
				if (err)
					printk(KERN_EMERG "backlight set error %d",err);
				//msleep(200);
			}
			else
			{
				//i2c_smbus_write_byte_data(lm3533_client,LM3533_STARTUP_SHUTDOWN_RAMP_RATES,0x00);
				brightness_temp = 0; 
				//msleep(10);
				err = i2c_smbus_write_byte_data(led->client, LM3533_BRIGHTNESS_REGISTER_A,0x00);
				if (err)
					printk(KERN_EMERG "backlight off error %d \n",err);
				//BANK_ENABLE = BANK_ENABLE & 61;		//111101
				 if(log_flag_setting){
				printk(KERN_EMERG "backlight off \n");}	
				//msleep(200);
			}

		break;
		
	default:
		break;
	}
	
	if(BANK_ENABLE != BANK_ENABLE_ORIGINAL)
	{
		err =  i2c_smbus_write_byte_data(led->client, LM3533_CONTROL_ENABLE,BANK_ENABLE);	
		//[Arima Edison] set a delay to make sure chip has completed it job ++
		msleep(5);
		//[Arima Edison] set a delay to make sure chip has completed it job --
		if (err)
			printk(KERN_EMERG "bank set error \n");
		#ifdef DEBUG
		  if(log_flag_setting){
		printk(KERN_EMERG"BANK_ENABLE = 0x%x \n",BANK_ENABLE);}
		#endif
	}	
	BANK_ENABLE_ORIGINAL = BANK_ENABLE;
	
	#ifdef DEBUG
	printk(KERN_EMERG "%s: %s,  id:%d brightness:%lu \n",
		__func__, led->ldev.name, id, brightness);
	#endif

	return err;
}

static void lm3533_led_set_brightness(struct led_classdev *led_cdev,
				      enum led_brightness brightness)
{
	struct lm3533_led_data *led = container_of(led_cdev, struct lm3533_led_data, ldev);

	if(led->id==2)
		      if(log_flag_setting){
		printk(KERN_EMERG "%s: %s, %d\n",__func__, led_cdev->name, brightness);}
	
	led->lm3533_led_brightness = brightness;	

	//[Arima Jim]  Modify backlight action NOT through queue ++

	if(led->id==2 && brightness==0)
	{
		keep_backlight_brightness = 0;
		lcm_has_on=0; 
		backlight_has_on=0;
 		led->lm3533_led_brightness=0;
		lm3533_led_set(led, 0);	

	}
	else if(led->id==2&& brightness>0 )
	{
		if (lcm_has_on==1&& backlight_has_on==0&&brightness>0 )
		{
			lm3533_led_set(led, brightness);
			backlight_has_on=1;
		}			
		else if (lcm_has_on==1&& backlight_has_on==1&&brightness>0 )
	 	{
       		lm3533_led_set(led, brightness);
		}
		else 
		{
			keep_backlight_brightness=brightness;
		}
	}
	else if(led->id==2)	
	{
		backlight_has_on = 0;
		lcm_has_on=0;
		led->lm3533_led_brightness=0;
		lm3533_led_set(led, 0);
		keep_backlight_brightness=0;
	}
	//[Arima Jim]  Modify backlight action NOT through queue --	
	else if(led->id==1)
	{
		cancel_delayed_work_sync(&led->thread_register_keep);
		cancel_delayed_work_sync(&led->thread_set_keep);
		queue_delayed_work(LED_WorkQueue, &led->thread, msecs_to_jiffies(10));	
	}
	else
		queue_delayed_work(LED_WorkQueue, &led->thread, msecs_to_jiffies(10));		
}

static void lm3533_led_work(struct work_struct *work)
{
	struct lm3533_led_data *led = container_of(work, struct lm3533_led_data, thread.work);

	if(led->id == 0)
		lm3533_led_set(led, led->lm3533_rgb_brightness);
	else if(led->id==1)
		lm3533_led_set(led, (unsigned long)led->lm3533_led_brightness);

}
	
static void lm3533_led_register_keep_light_work(struct work_struct *work)
{
	struct lm3533_led_data *led = container_of(work, struct lm3533_led_data, thread_register_keep.work);	
	if(log_flag_setting){
	printk(KERN_EMERG "%s : %s (%lu)\n",__func__,led->ldev.name,led->keep_on_time);
	printk(KERN_EMERG "keep_on_triggered_next=%d \n",led->keep_on_triggered_next);}
	
	if( led->keep_on_triggered_next == ACTION_START )
	{
		//keep time
		queue_delayed_work(LED_WorkQueue, &led->thread_set_keep, msecs_to_jiffies(led->keep_on_time+1000));
		led->keep_on_triggered_now = ACTION_START;
		led->keep_on_triggered_next = ACTION_OFF;
	}else if( led->keep_on_triggered_next == ACTION_OFF)
	{
		//off time
		queue_delayed_work(LED_WorkQueue, &led->thread_set_keep, msecs_to_jiffies(led->keep_off_time+1000));
		led->keep_on_triggered_now = ACTION_OFF;
		led->keep_on_triggered_next = ACTION_ON;
	}else if( led->keep_on_triggered_next == ACTION_ON )
	{
		queue_delayed_work(LED_WorkQueue, &led->thread_set_keep, msecs_to_jiffies(led->keep_on_time+1000));
		led->keep_on_triggered_now = ACTION_ON;
		led->keep_on_triggered_next = ACTION_OFF;
	}
	
	
}	

static void lm3533_led_keep_light_work(struct work_struct *work)
{
	struct lm3533_led_data *led = container_of(work, struct lm3533_led_data, thread_set_keep.work);	
      if(log_flag_setting){	
	printk(KERN_EMERG "%s : %s (%d)\n",__func__,led->ldev.name,led->keep_on_triggered_next);}		

	if( led->keep_on_triggered_next == ACTION_OFF)
	{
		lm3533_led_set(led, 0);
		if(led->keep_off_time!=0)
			queue_delayed_work(LED_WorkQueue, &led->thread_register_keep, msecs_to_jiffies(10));
	}
	else
	{
		if(led->id==1)
		{
      			if(log_flag_setting){
			printk(KERN_EMERG "button led->lm3533_led_brightness=%d \n",led->lm3533_led_brightness);}
			lm3533_led_set(led, led->lm3533_led_brightness);
		}
		else
			lm3533_led_set(led, led->lm3533_rgb_brightness);
		
		if(led->keep_on_time!=0)
			queue_delayed_work(LED_WorkQueue, &led->thread_register_keep, msecs_to_jiffies(10));
	}
	
}

void lm3533_backlight_control(unsigned long brightness)
{    
	//[Arima Jim]  Modify backlight action NOT through queue ++ 

	//struct lm3533_platform_data *pdata = lm3533_client->dev.platform_data;
	struct lm3533_data *data = i2c_get_clientdata(lm3533_client);	

	struct lm3533_led_data *led2 = &data->leds[2];
	struct lm3533_led_data *led1 = &data->leds[1];

	if (brightness==500)	// [Arima Jim] add for booting backlight
	{
		 lcm_has_on = 1;
		 backlight_has_on=1;  
		 led2->lm3533_led_brightness=255;
		 lm3533_led_set(led2, 255);
		 led1->lm3533_led_brightness=255;
		 lm3533_led_set(led1, 255);		
	}
	else if(brightness==0)		// [Arima Jim] add for common close backlight
	{
		lcm_has_on=0; 
		backlight_has_on=0;
		led2->lm3533_led_brightness=0;
	}	
	else 	// [Arima Jim] add for common open backlight after LCM
	{
		lcm_has_on = 1;
		
		if (backlight_has_on==0&&lcm_has_on == 1&&keep_backlight_brightness>0)	
		{
             		led2->lm3533_led_brightness=keep_backlight_brightness; 
			lm3533_led_set(led2, keep_backlight_brightness);
			lcm_has_on = 1;
			backlight_has_on=1;
		 }
		else
		{
			lcm_has_on=1; 
			backlight_has_on=0;
			//keep_backlight_brightness=0;
		}
	}
		//[Arima Jim]  Modify backlight action NOT through queue --
	return;
}
static int lm3533_configure(struct i2c_client *client,
			    struct lm3533_data *data,
			    struct lm3533_platform_data *pdata)
{
	int i, err = 0;
	int temp ;
	
	u8 LM3533_data[32];
	memset(LM3533_data,0,32);	
	
	LM3533_data[0]=0x00;
	LM3533_data[1]=0x00;
	LM3533_data[2]=0x00;
	LM3533_data[3]=0x00;
	LM3533_data[4]=0x00;
	LM3533_data[5]=0x00;
	err = i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_CONTROL_BANK_A_PWM_CONFIGURATION,6,LM3533_data);
	if(err)	
	{
		printk(KERN_NOTICE "LM3533_CONTROL_BANK_A_PWM_CONFIGURATION \n");
	};
	//[Arima Jim] modify 25mA ++
	LM3533_data[0]=0x19;
	LM3533_data[1]=0x19;
	LM3533_data[2]=0x19;
	//[Arima Jim] modify 25mA --
	LM3533_data[3]=0x00;
	
	err = i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_CONTROL_BANK_C_FULL_SCALE_CURRENT,4,LM3533_data);
	if(err)	
	{
		printk(KERN_NOTICE "LM3533_CONTROL_BANK_C_FULL_SCALE_CURRENT \n");
	};
	
	LM3533_data[0]=0x00;
	LM3533_data[1]=0x00;
	LM3533_data[2]=0x00;
	LM3533_data[3]=0x00;
	// disable/enable pattern generator
	err = i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_CONTROL_BANK_C_BRIGHTNESS_CONFIGURATION,4,LM3533_data);
	if(err)	
	{
		printk(KERN_NOTICE "LM3533_CONTROL_BANK_C_BRIGHTNESS_CONFIGURATION \n");
	};	
	
	LM3533_data[0]=0xAF;
	LM3533_data[1]=0xAF;
	LM3533_data[2]=0xAF;
	LM3533_data[3]=0xD7;  // [Arima Edison] modify brightness 08/08 
	// brightness
	i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_BRIGHTNESS_REGISTER_C,4,LM3533_data);
	

	LM3533_data[0]=0x03;
	LM3533_data[1]=0x01;
	LM3533_data[2]=0x01;
	LM3533_data[3]=0x00;
	LM3533_data[4]=0x01;
	LM3533_data[5]=0x01;	
	
	i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_PATTERN_GENERATOR_1_DELAY,6,LM3533_data);
	
	i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_PATTERN_GENERATOR_2_DELAY,6,LM3533_data);
	
	i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_PATTERN_GENERATOR_3_DELAY,6,LM3533_data);
	
	LM3533_data[0]=0x1B;  /*1B*/ //[Arima Jim] Modify LEDs dimming
	LM3533_data[1]=0x08;	
	i2c_smbus_write_i2c_block_data(lm3533_client,LM3533_STARTUP_SHUTDOWN_RAMP_RATES,2,LM3533_data);	

	err = i2c_smbus_write_byte_data(lm3533_client,LM3533_OVP_FREQUENCY_PWM_POLARITY,0x04);
	if(err)	
	{
		printk(KERN_NOTICE "LM3533_OVP_FREQUENCY_PWM_POLARITY \n");
	};
	//i2c_smbus_write_byte_data(lm3533_client,LM3533_PATTERN_GENERATOR_ENABLE_ALS_SCALING_CONTROL,0x15);	
	i2c_smbus_write_byte_data(lm3533_client,LM3533_PATTERN_GENERATOR_ENABLE_ALS_SCALING_CONTROL,0x00);
	
	//mutex_lock(&data->lock);
	//mutex_unlock(&data->lock);
	memset(LM3533_data,0,32);
	i2c_smbus_read_i2c_block_data(lm3533_client,LM3533_CURRENT_SINK_OUTPUT_CONFIGURATION1,32,LM3533_data);
	for( temp=0;temp<32;temp++ )
		printk(KERN_NOTICE "item %d = 0x%x \n",temp,LM3533_data[temp]);	

	memset(LM3533_data,0,32);
	i2c_smbus_read_i2c_block_data(lm3533_client,LM3533_ALS_DOWN_DELAY_CONTROL,32,LM3533_data);
	for( temp=0;temp<32;temp++ )
		printk(KERN_NOTICE "item %d = 0x%x \n",temp,LM3533_data[temp]);

	printk(KERN_NOTICE "0x%x",i2c_smbus_read_byte_data(lm3533_client,LM3533_CURRENT_SINK_OUTPUT_CONFIGURATION1));

	for (i = 0; i < pdata->leds_size; i++) {
		struct lm3533_led *pled = &pdata->leds[i];
		struct lm3533_led_data *led = &data->leds[i];
		led->client = client;
		led->id = i;
			
		switch (pled->type) {

		case LM3533_LED_TYPE_LED:
			led->type = pled->type;
			led->ldev.name = pled->name;
			led->ldev.brightness_set = lm3533_led_set_brightness;
			
			INIT_DELAYED_WORK(&led->thread, lm3533_led_work);		
			INIT_DELAYED_WORK(&led->thread_register_keep, lm3533_led_register_keep_light_work);	
			INIT_DELAYED_WORK(&led->thread_set_keep, lm3533_led_keep_light_work);	
			
			err = led_classdev_register(&client->dev, &led->ldev);
			if (err < 0) {
				dev_err(&client->dev,
					"couldn't register LED %s\n",
					led->ldev.name);
				goto exit;
			}
			//[Arima Edison] add exception for charger usage++
			if(led->id != 2)
				err = sysfs_create_group(&led->ldev.dev->kobj,&lm3533_attribute_group);
			else
				err = sysfs_create_group(&led->ldev.dev->kobj,&lm3533_backlight_attribute_group);
			//[Arima Edison] add exception for charger usage--
				//err = device_create_file(led->ldev.dev, &mode_attr);
			if (err < 0) {
				printk(KERN_EMERG "%s creat fail \n",__func__);
			}
			
			/* to expose the default value to userspace */
			//led->ldev.brightness = led->status;

			/* Set the default led status */
			/*++ Huize - 20120830 Add for setting fadat_time default value is 0xFF ++*/
			led->fade_time = 255;
			/*-- Huize - 20120830 Add for setting fadat_time default value is 0xFF --*/

			led->lm3533_led_brightness = 0;
			err = lm3533_led_set(led, 0);
				
			if (err < 0) {
				dev_err(&client->dev,
					"%s couldn't set STATUS \n",
					led->ldev.name);
				goto exit;
			}
			break;

		case LM3533_LED_TYPE_NONE:
		default:
			break;

		}
	}

	return 0;

exit:
	if (i > 0)
		for (i = i - 1; i >= 0; i--)
			switch (pdata->leds[i].type) {

			case LM3533_LED_TYPE_LED:			
				led_classdev_unregister(&data->leds[i].ldev);
				cancel_work_sync(&data->leds[i].thread.work);
				cancel_work_sync(&data->leds[i].thread_register_keep.work);
				cancel_work_sync(&data->leds[i].thread_set_keep.work);
				break;

			case LM3533_LED_TYPE_NONE:
			default:
				break;
			}

	return err;
}

static int lm3533_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct lm3533_platform_data *lm3533_pdata = client->dev.platform_data;
	struct lm3533_data *data;
	int err;		

	printk(KERN_EMERG "in lm3533_probe");
	printk(KERN_NOTICE "%s : 0x%x",__func__,i2c_smbus_read_byte_data(client,LM3533_CURRENT_SINK_OUTPUT_CONFIGURATION1));	

	if (lm3533_pdata == NULL) {
		dev_err(&client->dev, "lm3533_probe no platform data\n");		
		printk(KERN_ERR "lm3533_probe no platform data\n");
		return -EINVAL;
	}

	/* Let's see whether this adapter can support what we need. */
	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_I2C)) {
		dev_err(&client->dev, "insufficient functionality!\n");	
		printk(KERN_ERR "lm3533_probe insufficient functionality\n");
		return -ENODEV;
	}		
	
	// read the default value to check if IC there
	if(i2c_smbus_read_byte_data(client,LM3533_CURRENT_SINK_OUTPUT_CONFIGURATION1) != 0x92)
	{
		gpio_set_value(8, 0);		// HW init again
		msleep(10);
		gpio_set_value(8, 1);
		msleep(100);

		if(i2c_smbus_read_byte_data(client,LM3533_CURRENT_SINK_OUTPUT_CONFIGURATION1) != 0x92)
		{
			printk(KERN_NOTICE "LM3533 initial value error \n");
			return -ENODEV;
		}	
	}

	data = kzalloc(sizeof(struct lm3533_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	LED_WorkQueue = create_singlethread_workqueue(LM3533_DRIVER_NAME);

	data->client = client;
	lm3533_client = client;
	i2c_set_clientdata(client, data);

	mutex_init(&data->lock);

	err = lm3533_configure(client, data, lm3533_pdata);	
	
	if (err < 0) {
		kfree(data);
		return err;
	}

	dev_info(&client->dev, "lm3533 enabled\n");
	return 0;	

}

static int lm3533_remove(struct i2c_client *client)
{
	struct lm3533_platform_data *pdata = client->dev.platform_data;
	struct lm3533_data *data = i2c_get_clientdata(client);
	int i;

	printk(KERN_NOTICE "lm3533_remove \n");
	destroy_workqueue(LED_WorkQueue);

	for (i = 0; i < pdata->leds_size; i++)
		switch (data->leds[i].type) {
		case LM3533_LED_TYPE_LED:
			led_classdev_unregister(&data->leds[i].ldev);
			cancel_work_sync(&data->leds[i].thread.work);
			cancel_work_sync(&data->leds[i].thread_register_keep.work);
			cancel_work_sync(&data->leds[i].thread_set_keep.work);
			break;

		case LM3533_LED_TYPE_NONE:
		default:
			break;
		}

	kfree(data);

	return 0;
}

/* lm3533 i2c driver struct */
static const struct i2c_device_id lm3533_id[] = {
	{"leds-lm3533", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, lm3533_id);

static struct i2c_driver lm3533_driver = {
	.driver   = {
		   .owner	= THIS_MODULE,	
		   .name = LM3533_DRIVER_NAME,
	},
	.id_table   = lm3533_id,
 	.probe      = lm3533_probe,
	.remove   = lm3533_remove,	
	.suspend  	= 0,
	.resume   	= 0,
	.shutdown	= 0,
};

static int __init lm3533_module_init(void)
{

	int init_int = i2c_add_driver(&lm3533_driver) ;

	printk(KERN_EMERG "%s enable pin = %d \n",__func__,gpio_get_value(8));		
	printk(KERN_NOTICE "lm3533_module_init = %d \n",init_int);
	
	return init_int;
}

static void __exit lm3533_module_exit(void)
{
	i2c_del_driver(&lm3533_driver);
}

module_init(lm3533_module_init);
module_exit(lm3533_module_exit);

MODULE_DESCRIPTION("Back Light driver for LM3533");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("EdisonShih@Arimacomm");

