/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include <linux/module.h>
//Edison add
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
/* Early-suspend level */
#define LED_SUSPEND_LEVEL 1
#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

struct pdm_led_data {
	struct led_classdev cdev;
	void __iomem *perph_base;
	int pdm_offset;
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--
};

//red led test
/*
struct pm_gpio red_led = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 1,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength	= PM_GPIO_STRENGTH_HIGH,
		.function	= PM_GPIO_FUNC_1,
		.inv_int_pol	= 1,
	};*/
static int dimming_brightness = 0;
static int cur_brightness = 0;

static ssize_t brightness_dimming_write(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	//struct led_classdev *led_cdev = dev_get_drvdata(dev);
	static unsigned long red_led_brightness;	
	
	if (strict_strtoul(buf, 10, &red_led_brightness))
		return -EINVAL;	

	if(red_led_brightness>0)
	{
		if(!dimming_brightness)
		{
			if (cur_brightness)
			{
				pmic_secure_mpp_config_i_sink(PM_MPP_8, PM_MPP__I_SINK__LEVEL_5mA,0);
				cur_brightness=0;
			}	
			pmapp_red_led_set_brightness(100);
			dimming_brightness=1;
		}	
	}	
	else
	{
		if(dimming_brightness)
		{
			pmapp_red_led_set_brightness(0);	
			dimming_brightness=0;
		}
		else if (cur_brightness)
		{	
			pmic_secure_mpp_config_i_sink(PM_MPP_8, PM_MPP__I_SINK__LEVEL_5mA,0);
			cur_brightness = 0;		
		}
	}	

	return count;
}

static DEVICE_ATTR(brightness_dimming , 0644 , NULL , brightness_dimming_write);

static struct attribute *red_led_attributes[] = {
	&dev_attr_brightness_dimming.attr,
	NULL
};

static struct attribute_group red_led_attribute_group = {
	.attrs = red_led_attributes
};

static void msm_led_brightness_set_percent(struct pdm_led_data *led,
						int duty_per)
{
	//pr_emerg("[RED LED] %s (%d)\n", __func__, duty_per);

	//pmapp_red_led_set_brightness(duty_per);
	
	if (duty_per){
		if (!cur_brightness){
			if(dimming_brightness)
			{
				pmapp_red_led_set_brightness(0);
				dimming_brightness=0;
			}	
			pmic_secure_mpp_config_i_sink(PM_MPP_8, PM_MPP__I_SINK__LEVEL_5mA,1);
			cur_brightness = 1;
		}
	}
	else {
		if(dimming_brightness)
		{
			pmapp_red_led_set_brightness(0);	
			dimming_brightness=0;
		}
		else if (cur_brightness)
		{	
			pmic_secure_mpp_config_i_sink(PM_MPP_8, PM_MPP__I_SINK__LEVEL_5mA,0);
			cur_brightness = 0;		
		}
	}
}

static void msm_led_brightness_set(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	struct pdm_led_data *led =
		container_of(led_cdev, struct pdm_led_data, cdev);
	
	msm_led_brightness_set_percent(led, (value * 100) / LED_FULL);
	//printk(KERN_EMERG "msm_led_brightness_set in leds-red");
}

#ifdef CONFIG_PM_SLEEP

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
static int msm_led_red_suspend(struct device *dev)
{
	//[Arima Edison] blocked it for low battery indicator ++
	//struct pdm_led_data *led = dev_get_drvdata(dev);
		
	msm_led_brightness_set_percent(led, 0);
	//[Arima Edison] blocked it for low battery indicator --
	return 0;
}
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
static void msm_led_red_early_suspend(struct early_suspend *h)
{
	struct pdm_led_data *led = container_of(h,
			struct pdm_led_data, early_suspend);

	msm_led_red_suspend(led->cdev.dev->parent);
}

#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
static const struct dev_pm_ops msm_led_red_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= msm_led_red_suspend,
#endif
};
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--
#endif // #ifdef CONFIG_PM_SLEEP

static int __devinit msm_red_led_probe(struct platform_device *pdev)
{
	const struct led_info *pdata = pdev->dev.platform_data;
	struct pdm_led_data *led;
	int rc;
	int err;

	printk(KERN_EMERG "%s \n",__func__);

	if (!pdata) {
		pr_err("%s : platform data is invalid\n", __func__);
		return -EINVAL;
	}

	if (pdev->id != 0) {
		pr_err("%s : id is invalid\n", __func__);
		return -EINVAL;
	}

	led = kzalloc(sizeof(struct pdm_led_data), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	/* Enable runtime PM ops, start in ACTIVE mode */
	rc = pm_runtime_set_active(&pdev->dev);

	if (rc < 0)
		dev_dbg(&pdev->dev, "unable to set runtime pm state\n");
	pm_runtime_enable(&pdev->dev);	

	//[Arima Edison] block, a workaround for current consumption test++
	pmapp_red_led_init();
	//[Arima Edison] block, a workaround for current consumption test--

	/* Start with LED in off state */
	msm_led_brightness_set_percent(led, 0);

	led->cdev.brightness_set = msm_led_brightness_set;
	led->cdev.name = pdata->name ? : "leds-acm-red";

	rc = led_classdev_register(&pdev->dev, &led->cdev);
	if (rc) {
		pr_err("led class registration failed\n");
		goto err_led_reg;
	}

	err = sysfs_create_group(&led->cdev.dev->kobj,&red_led_attribute_group);

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
	led->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						LED_SUSPEND_LEVEL;
	led->early_suspend.suspend = msm_led_red_early_suspend;
	register_early_suspend(&led->early_suspend);
#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

	platform_set_drvdata(pdev, led);

 	printk(KERN_EMERG "%s OK",__func__);

	return 0;

err_led_reg:
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	kfree(led);	

	return rc;
}

static int __devexit msm_red_led_remove(struct platform_device *pdev)
{
	struct pdm_led_data *led = platform_get_drvdata(pdev);
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&led->early_suspend);
#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 

	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	led_classdev_unregister(&led->cdev);
	msm_led_brightness_set_percent(led, 0);
	iounmap(led->perph_base);
	release_mem_region(res->start, resource_size(res));
	kfree(led);

	return 0;
}

static struct platform_driver msm_red_led_driver = {
	.probe		= msm_red_led_probe,
	.remove		= __devexit_p(msm_red_led_remove),
	.driver		= {
		.name	= "leds-acm-red",
		.owner	= THIS_MODULE,
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP		
#ifdef CONFIG_PM_SLEEP
		.pm	= &msm_led_red_pm_ops,
#endif
#endif
	},
};

static int __init msm_pdm_led_init(void)
{
	printk(KERN_EMERG "RED LED DRIVER INIT");
	return platform_driver_register(&msm_red_led_driver);
}
module_init(msm_pdm_led_init);

static void __exit msm_pdm_led_exit(void)
{
	platform_driver_unregister(&msm_red_led_driver);
}
module_exit(msm_pdm_led_exit);

MODULE_DESCRIPTION("ARIMA RED LED driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:leds-red");
