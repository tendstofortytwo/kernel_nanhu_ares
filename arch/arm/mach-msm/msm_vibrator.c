/* include/asm/mach-msm/htc_pwrsink.h
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2011 Code Aurora Forum. All rights reserved.
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
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/module.h>
#include "pmic.h"
#include "timed_output.h"

#include <mach/msm_rpcrouter.h>

//[Arima Edison] add exception for charger usage++
#include <linux/delay.h>
//[Arima Edison] add exception for charger usage--

/*++ Kevin Shiu - 20121003 Save intensity using liked list ++*/ 
#include <linux/list.h> 
#include <linux/slab.h>
/*-- Kevin Shiu - 20121003 Save intensity using liked list --*/ 

#define PM_LIBPROG      0x30000061
#if (CONFIG_MSM_AMSS_VERSION == 6220) || (CONFIG_MSM_AMSS_VERSION == 6225)
#define PM_LIBVERS      0xfb837d0b
#else
#define PM_LIBVERS      0x10001
#endif

#define HTC_PROCEDURE_SET_VIB_ON_OFF	21
#define PMIC_VIBRATOR_LEVEL	(3000)

/*++ Huize - 20121003 Add for improving performance and controling intensity ++*/
static struct workqueue_struct *vibrator_workqueue;
/*-- Huize - 20121003 Add for improving performance and controling intensity --*/

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;
static struct hrtimer vibe_timer;

/*++ Kevin Shiu - 20121003 Save intensity using liked list ++*/
typedef struct{
	int vibrator_level;
	struct list_head list;
}vibrator_list;
struct list_head vibrator_list_head;
/*-- Kevin Shiu - 20121003 Save intensity using liked list --*/

#ifdef CONFIG_PM8XXX_RPC_VIBRATOR
static void set_pmic_vibrator(int on)
{
	int rc;
	/*++ Kevin Shiu - 20121003 Save intensity using liked list ++*/
	vibrator_list *vibrator = NULL;
	/*-- Kevin Shiu - 20121003 Save intensity using liked list --*/

	rc = pmic_vib_mot_set_mode(PM_VIB_MOT_MODE__MANUAL);
	if (rc) {
		pr_err("%s: Vibrator set mode failed", __func__);
		return;
	}

	/*++ Kevin Shiu - 20121003 Save intensity using liked list ++*/
	if(on && (vibrator_list_head.next == &vibrator_list_head)){
		pr_err("%s: list_head is invalid", __func__);
		return;
	}

	if(on){
		//get vibrator_list pointer
		vibrator = list_entry(vibrator_list_head.next , vibrator_list , list);
		rc = pmic_vib_mot_set_volt(vibrator->vibrator_level);

        //if ture, it means that only creates a structure.
		if(vibrator->list.next == vibrator->list.prev){
			list_del_rcu(&vibrator_list_head);
			INIT_LIST_HEAD(&vibrator_list_head);
		}else{	
			list_del_rcu(&vibrator->list);
		}

		//free
		kfree(vibrator);
	}else{
		rc = pmic_vib_mot_set_volt(0);
	}
    /*-- Kevin Shiu - 20121003 Save intensity using liked list --*/

	if (rc)
		pr_err("%s: Vibrator set voltage level failed", __func__);
}
#else
static void set_pmic_vibrator(int on)
{
	static struct msm_rpc_endpoint *vib_endpoint;
	struct set_vib_on_off_req {
		struct rpc_request_hdr hdr;
		uint32_t data;
	} req;

	if (!vib_endpoint) {
		vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
		if (IS_ERR(vib_endpoint)) {
			printk(KERN_ERR "init vib rpc failed!\n");
			vib_endpoint = 0;
			return;
		}
	}


	if (on)
		req.data = cpu_to_be32(PMIC_VIBRATOR_LEVEL);
	else
		req.data = cpu_to_be32(0);

	msm_rpc_call(vib_endpoint, HTC_PROCEDURE_SET_VIB_ON_OFF, &req,
		sizeof(req), 5 * HZ);
}
#endif

static void pmic_vibrator_on(struct work_struct *work)
{
	set_pmic_vibrator(1);
}

static void pmic_vibrator_off(struct work_struct *work)
{
	set_pmic_vibrator(0);
}

/*++ Huize - 20121003 modified for improving performance ++*/
static void timed_vibrator_on(struct timed_output_dev *sdev)
{
	queue_work(vibrator_workqueue, &work_vibrator_on);
}

static void timed_vibrator_off(struct timed_output_dev *sdev)
{
	queue_work(vibrator_workqueue, &work_vibrator_off);
}
/*-- Huize - 20121003 Modify for improving performance --*/

/*++ Huize - 20121003 Modify for passing parameter of intensity ++*/
static void vibrator_enable(struct timed_output_dev *dev, int value, int intensity)
{
    /*++ Kevin Shiu - 20121003 Save intensity using liked list ++*/
	vibrator_list *vibrator = NULL;
    /*-- Kevin Shiu - 20121003 Save intensity using liked list --*/
	hrtimer_cancel(&vibe_timer);

	if (value == 0)
		timed_vibrator_off(dev);
	else {
		value = (value > 15000 ? 15000 : value);

		/*++ Kevin Shiu - 20121003 Save intensity using liked list ++*/
		vibrator = (vibrator_list *)kzalloc(sizeof(vibrator_list),GFP_ATOMIC);
		INIT_LIST_HEAD(&vibrator->list);
		vibrator->vibrator_level = intensity;
		list_add_tail_rcu(&vibrator->list, &vibrator_list_head);
		/*-- Kevin Shiu - 20121003 Save intensity using liked list --*/

		timed_vibrator_on(dev);

		hrtimer_start(&vibe_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
}
/*-- Huize - 20121003 Modify for passing parameter of intensity --*/

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	timed_vibrator_off(NULL);
	return HRTIMER_NORESTART;
}

//[Arima Edison] add exception for charger usage++
static ssize_t enable_vibrator(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{

	static unsigned long keep_time_data;
	int rc;

	if (strict_strtoul(buf, 10, &keep_time_data))
		return -EINVAL;

	hrtimer_cancel(&vibe_timer);

	keep_time_data = (keep_time_data > 300 ? 300 : keep_time_data);
	rc = pmic_vib_mot_set_mode(PM_VIB_MOT_MODE__MANUAL);

	if(keep_time_data == 0)
	{
		rc = pmic_vib_mot_set_volt(0);
	}
	else {
		rc = pmic_vib_mot_set_volt(3000);
		mdelay( keep_time_data );
		rc = pmic_vib_mot_set_volt(0);

	}

	return count;
}

static DEVICE_ATTR(enable_vibrator , 0644 , NULL , enable_vibrator);
static struct attribute *vibrator_attributes[] = {
	&dev_attr_enable_vibrator.attr,
	NULL
};
static struct attribute_group vibrator_attribute_group = {
	.attrs = vibrator_attributes
};
//[Arima Edison] add exception for charger usage--

static struct timed_output_dev pmic_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

void __init msm_init_pmic_vibrator(void)
{
	//[Arima Edison] add exception for charger usage++
	int err;
	//[Arima Edison] add exception for charger usage--
	/*++ Huize - 20120917 Add for improving performance ++*/
	vibrator_workqueue = create_singlethread_workqueue("vibrate");
	/*-- Huize - 20120917 Add for improving performance --*/
	/*++ Kevin Shiu - 20121003 Save intensity using liked list ++*/
	INIT_LIST_HEAD(&vibrator_list_head);
	/*-- Kevin Shiu - 20121003 Save intensity using liked list --*/
	INIT_WORK(&work_vibrator_on, pmic_vibrator_on);
	INIT_WORK(&work_vibrator_off, pmic_vibrator_off);

	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&pmic_vibrator);
	//[Arima Edison] add exception for charger usage++
	err= sysfs_create_group(&pmic_vibrator.dev->kobj,&vibrator_attribute_group);
	//[Arima Edison] add exception for charger usage--
}

MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");

