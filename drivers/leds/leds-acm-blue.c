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

static int dimming_brightness = 0;
static int cur_brightness = 0;

static ssize_t brightness_dimming_write(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	//struct led_classdev *led_cdev = dev_get_drvdata(dev);
	static unsigned long blue_led_brightness;	
	
	if (strict_strtoul(buf, 10, &blue_led_brightness))
		return -EINVAL;	

	blue_led_brightness =  ((blue_led_brightness * 100) / LED_FULL);

	if(blue_led_brightness>0)
	{
		if(!dimming_brightness)
		{
			if (cur_brightness)
			{
				pmic_gpio_config_digital_output(PMIC_GPIO_1, 0, 7, 0, 1,1);
				cur_brightness=0;
			}	
			pmapp_disp_backlight_set_brightness(blue_led_brightness);
			dimming_brightness=1;
		}	
	}	
	else
	{
		if(dimming_brightness)
		{
			pmapp_disp_backlight_set_brightness(blue_led_brightness);
			dimming_brightness=0;
		}
		else if (cur_brightness)
		{	
			pmic_gpio_config_digital_output(PMIC_GPIO_1, 0, 7, 0, 1,1);
			cur_brightness = 0;		
		}
	}	

	return count;
}

static DEVICE_ATTR(brightness_dimming , 0644 , NULL , brightness_dimming_write);

static struct attribute *blue_led_attributes[] = {
	&dev_attr_brightness_dimming.attr,
	NULL
};

static struct attribute_group blue_led_attribute_group = {
	.attrs = blue_led_attributes
};

static void msm_led_brightness_set_percent(struct pdm_led_data *led,
						int duty_per)
{
	//printk(KERN_NOTICE"[BLUE LED] %s (%d)\n", __func__, duty_per);
	
	//pmapp_disp_backlight_set_brightness(duty_per);
	
	if (duty_per){
		if (!cur_brightness){
			if(dimming_brightness)
			{
				pmapp_disp_backlight_set_brightness(0);
				dimming_brightness=0;
			}	
			pmic_gpio_config_digital_output(PMIC_GPIO_1, 0, 7, 0, 1,0);
			cur_brightness = 1;
		}
	}
	else 
	{
		if(dimming_brightness)
		{
			pmapp_disp_backlight_set_brightness(0);
			dimming_brightness=0;
		}
		else if (cur_brightness)
		{	
			pmic_gpio_config_digital_output(PMIC_GPIO_1, 0, 7, 0, 1,1);
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
	//printk(KERN_EMERG "msm_led_brightness_set in leds-blue");
}

#ifdef CONFIG_PM_SLEEP
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
static int msm_led_blue_suspend(struct device *dev)
{
	struct pdm_led_data *led = dev_get_drvdata(dev);

	msm_led_brightness_set_percent(led, 0);

	return 0;
}
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
static void msm_led_blue_early_suspend(struct early_suspend *h)
{
	struct pdm_led_data *led = container_of(h,
			struct pdm_led_data, early_suspend);

	msm_led_blue_suspend(led->cdev.dev->parent);
}

#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
static const struct dev_pm_ops msm_led_blue_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= msm_led_blue_suspend,
#endif
};
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--
#endif // #ifdef CONFIG_PM_SLEEP

static int __devinit msm_blue_led_probe(struct platform_device *pdev)
{
	const struct led_info *pdata = pdev->dev.platform_data;
	struct pdm_led_data *led;
	int rc, err;

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

	pmapp_disp_backlight_init();

	/* Start with LED in off state */
	msm_led_brightness_set_percent(led, 0);

	led->cdev.brightness_set = msm_led_brightness_set;
	led->cdev.name = pdata->name ? : "leds-acm-blue";

	rc = led_classdev_register(&pdev->dev, &led->cdev);
	if (rc) {
		pr_err("led class registration failed\n");
		goto err_led_reg;
	}	

	err = sysfs_create_group(&led->cdev.dev->kobj,&blue_led_attribute_group);

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
	led->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						LED_SUSPEND_LEVEL;
	led->early_suspend.suspend = msm_led_blue_early_suspend;
	register_early_suspend(&led->early_suspend);
#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

	platform_set_drvdata(pdev, led);
	return 0;

err_led_reg:
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	kfree(led);	

	return rc;
}

static int __devexit msm_blue_led_remove(struct platform_device *pdev)
{
	struct pdm_led_data *led = platform_get_drvdata(pdev);
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&led->early_suspend);
#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	led_classdev_unregister(&led->cdev);	
	
	msm_led_brightness_set_percent(led, 0);
	iounmap(led->perph_base);
	release_mem_region(res->start, resource_size(res));
	kfree(led);

	return 0;
}

static struct platform_driver msm_blue_led_driver = {
	.probe		= msm_blue_led_probe,
	.remove		= __devexit_p(msm_blue_led_remove),
	.driver		= {
		.name	= "leds-acm-blue",
		.owner	= THIS_MODULE,
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP		
#ifdef CONFIG_PM_SLEEP
		.pm	= &msm_led_blue_pm_ops,
#endif
#endif
	},
};

static int __init msm_pdm_led_init(void)
{
	printk(KERN_EMERG "BLUE LED DRIVER INIT");
	return platform_driver_register(&msm_blue_led_driver);
}
module_init(msm_pdm_led_init);

static void __exit msm_pdm_led_exit(void)
{
	platform_driver_unregister(&msm_blue_led_driver);
}
module_exit(msm_pdm_led_exit);

MODULE_DESCRIPTION("ARIMA BLUE LED driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:leds-blue");
