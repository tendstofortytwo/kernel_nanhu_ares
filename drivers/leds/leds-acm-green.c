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

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
/* Early-suspend level */
#define LED_SUSPEND_LEVEL 1
#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--

#define GPIO_GREEN_LED_EN	129

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


static int cur_brightness = 0;
static void msm_led_brightness_set_percent(struct pdm_led_data *led,
						int duty_per)
{
	//pr_emerg("[GREEN LED] %s (%d)\n", __func__, duty_per);
	if (duty_per){
		if (!cur_brightness){
			//pmic_gpio_config_digital_output(PM_MPP_9, 0, 1, 0, 1,1);
			//printk(KERN_NOTICE "gpio_set_value(%d, 1);",GPIO_GREEN_LED_EN);
			gpio_set_value(GPIO_GREEN_LED_EN, 1);
			cur_brightness = 1;
		}
	}
	else {
		if (cur_brightness){	
			//pmic_gpio_config_digital_output(PM_MPP_9, 0, 1, 0, 1,0);
			gpio_set_value(GPIO_GREEN_LED_EN, 0);
			//printk(KERN_NOTICE "gpio_set_value(%d, 0);",GPIO_GREEN_LED_EN);
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
	//printk(KERN_EMERG "msm_led_brightness_set in leds-green");
}

#ifdef CONFIG_PM_SLEEP
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
static int msm_led_green_suspend(struct device *dev)
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
static void msm_led_green_early_suspend(struct early_suspend *h)
{
	struct pdm_led_data *led = container_of(h,
			struct pdm_led_data, early_suspend);

	msm_led_green_suspend(led->cdev.dev->parent);
}

#endif
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
static const struct dev_pm_ops msm_led_green_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= msm_led_green_suspend,
#endif
};
#endif
//[Arima Edison] disable it, since indicator leds were controled by PowerManager--
#endif // #ifdef CONFIG_PM_SLEEP

static int __devinit msm_green_led_probe(struct platform_device *pdev)
{
	const struct led_info *pdata = pdev->dev.platform_data;
	struct pdm_led_data *led;
	int rc;

	if (!pdata) {
		pr_err("%s : platform data is invalid\n", __func__);
		return -EINVAL;
	}

	if (pdev->id != 0) {
		pr_err("%s : id is invalid\n", __func__);
		return -EINVAL;
	}	

	rc = gpio_request(GPIO_GREEN_LED_EN, "gpio_green_led_en");
	if (rc < 0){
		pr_err("%s : failed to request gpio_green_led_en(%d)\n", __func__, rc);
		return rc;	
	}
	
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_GREEN_LED_EN, 0, GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
	if (rc){
		pr_err("%s: gpio_tlmm_config for %d failed\n", __func__, GPIO_GREEN_LED_EN);
		goto gpio_fail;
	}

	led = kzalloc(sizeof(struct pdm_led_data), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	/* Enable runtime PM ops, start in ACTIVE mode */
	rc = pm_runtime_set_active(&pdev->dev);

	if (rc < 0)
		dev_dbg(&pdev->dev, "unable to set runtime pm state\n");
	pm_runtime_enable(&pdev->dev);	

	/* Start with LED in off state */
	msm_led_brightness_set_percent(led, 0);

	led->cdev.brightness_set = msm_led_brightness_set;
	led->cdev.name = pdata->name ? : "leds-acm-green";

	rc = led_classdev_register(&pdev->dev, &led->cdev);
	if (rc) {
		pr_err("led class registration failed\n");
		goto err_led_reg;
	}

//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP
#ifdef CONFIG_HAS_EARLYSUSPEND
	led->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						LED_SUSPEND_LEVEL;
	led->early_suspend.suspend = msm_led_green_early_suspend;
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
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_GREEN_LED_EN, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), GPIO_CFG_DISABLE);
gpio_fail:
	gpio_free(GPIO_GREEN_LED_EN);

	

	return rc;
}

static int __devexit msm_green_led_remove(struct platform_device *pdev)
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

static struct platform_driver msm_green_led_driver = {
	.probe		= msm_green_led_probe,
	.remove		= __devexit_p(msm_green_led_remove),
	.driver		= {
		.name	= "leds-acm-green",
		.owner	= THIS_MODULE,
//[Arima Edison] disable it, since indicator leds were controled by PowerManager++ 
#ifdef CONFIG_LED_SLEEP		
#ifdef CONFIG_PM_SLEEP
		.pm	= &msm_led_green_pm_ops,
#endif
#endif
	},
};

static int __init msm_pdm_led_init(void)
{
	printk(KERN_EMERG "GREEN LED DRIVER INIT");
	return platform_driver_register(&msm_green_led_driver);
}
module_init(msm_pdm_led_init);

static void __exit msm_pdm_led_exit(void)
{
	platform_driver_unregister(&msm_green_led_driver);
}
module_exit(msm_pdm_led_exit);

MODULE_DESCRIPTION("ARIMA GREEN LED driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:leds-green");
