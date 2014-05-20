/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * this needs to be before <linux/kernel.h> is loaded,
 * and <linux/sched.h> loads <linux/kernel.h>
 */
/*++ Edison - 20110929 Enable the debug information ++*/
#define DEBUG  0
/*-- Edison - 20110929 Enable the debug information --*/

#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>

/*Edison add for protection IC test 20111122*/
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/irq.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

static struct delayed_work protection_ic_work;

/*Edison add for protection IC test 20111122*/

// Edison add for capacity translation++
#define BATTERY_CAPACITY_REAL
// Edison add for capacity translation--
#define BATTERY_RPC_PROG	0x30000089
#define BATTERY_RPC_VER_1_1	0x00010001
#define BATTERY_RPC_VER_2_1	0x00020001
#define BATTERY_RPC_VER_4_1     0x00040001
#define BATTERY_RPC_VER_5_1     0x00050001

#define BATTERY_RPC_CB_PROG	(BATTERY_RPC_PROG | 0x01000000)

#define CHG_RPC_PROG		0x3000001a
#define CHG_RPC_VER_1_1		0x00010001
#define CHG_RPC_VER_1_3		0x00010003
#define CHG_RPC_VER_2_2		0x00020002
#define CHG_RPC_VER_3_1         0x00030001
#define CHG_RPC_VER_4_1         0x00040001

#define BATTERY_REGISTER_PROC				2
#define BATTERY_MODIFY_CLIENT_PROC			4
#define BATTERY_DEREGISTER_CLIENT_PROC			5
#define BATTERY_READ_MV_PROC				12
#define BATTERY_ENABLE_DISABLE_FILTER_PROC		14
/*Edison add ATS & ERROR EVENT 20111122 ++*/
#define BATTERY_READ_TEMP_PROC				17
/*Edison add ATS & ERROR EVENT 20111122 --*/

#define VBATT_FILTER			2

#define BATTERY_CB_TYPE_PROC		1
#define BATTERY_CB_ID_ALL_ACTIV		1
#define BATTERY_CB_ID_LOW_VOL		2

#define BATTERY_LOW		3200
#define BATTERY_HIGH		4300

#define ONCRPC_CHG_GET_GENERAL_STATUS_PROC	12
//[Arima Edison] add for trigger early suspend condition++
#define ONCRPC_CHG_SET_BATTERY_TRIGGER_SUSPEND	16
#define ONCRPC_CHG_TRIGGER_TELEPHONY	17
#define ONCRPC_CHG_SET_POWER_OFF_CHARGE_FLAG 18
#define ONCRPC_CHG_CHECK_CHARGER_IRQ_STATUS 19
#define ONCRPC_CHG_TRIGGER_CHARGER_IRQ_HANDLER	20
//[Arima Edison] add for trigger early suspend condition-- 
#define ONCRPC_CHARGER_API_VERSIONS_PROC	0xffffffff

#define BATT_RPC_TIMEOUT    5000	/* 5 sec */

#define INVALID_BATT_HANDLE    -1

#define RPC_TYPE_REQ     0
#define RPC_TYPE_REPLY   1
#define RPC_REQ_REPLY_COMMON_HEADER_SIZE   (3 * sizeof(uint32_t))


#if DEBUG
#define DBG_LIMIT(x...) do {if (printk_ratelimit()) pr_debug(x); } while (0)
#else
#define DBG_LIMIT(x...) do {} while (0)
#endif

enum {
	BATTERY_REGISTRATION_SUCCESSFUL = 0,
	BATTERY_DEREGISTRATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_MODIFICATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_INTERROGATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_CLIENT_TABLE_FULL = 1,
	BATTERY_REG_PARAMS_WRONG = 2,
	BATTERY_DEREGISTRATION_FAILED = 4,
	BATTERY_MODIFICATION_FAILED = 8,
	BATTERY_INTERROGATION_FAILED = 16,
	/* Client's filter could not be set because perhaps it does not exist */
	BATTERY_SET_FILTER_FAILED         = 32,
	/* Client's could not be found for enabling or disabling the individual
	 * client */
	BATTERY_ENABLE_DISABLE_INDIVIDUAL_CLIENT_FAILED  = 64,
	BATTERY_LAST_ERROR = 128,
};

enum {
	BATTERY_VOLTAGE_UP = 0,
	BATTERY_VOLTAGE_DOWN,
	BATTERY_VOLTAGE_ABOVE_THIS_LEVEL,
	BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
	BATTERY_VOLTAGE_LEVEL,
	BATTERY_ALL_ACTIVITY,
	VBATT_CHG_EVENTS,
	BATTERY_VOLTAGE_UNKNOWN,
};

/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type {
	/* The charger is good      */
	CHARGER_STATUS_GOOD,
	/* The charger is bad       */
	CHARGER_STATUS_BAD,
	/* The charger is weak      */
	CHARGER_STATUS_WEAK,
	/* Invalid charger status.  */
	CHARGER_STATUS_INVALID
};

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type {
	/* The charger is removed                 */
	CHARGER_TYPE_NONE,
	/* The charger is a regular wall charger   */
	CHARGER_TYPE_WALL,
	/* The charger is a PC USB                 */
	CHARGER_TYPE_USB_PC,
	/* The charger is a wall USB charger       */
	CHARGER_TYPE_USB_WALL,
	/* The charger is a USB carkit             */
	CHARGER_TYPE_USB_CARKIT,
	/* Invalid charger hardware status.        */
	CHARGER_TYPE_INVALID
};

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type {
	/* The battery is good        */
	BATTERY_STATUS_GOOD,
	/* The battery is cold/hot    */
	BATTERY_STATUS_BAD_TEMP,
	/* The battery is bad         */
	BATTERY_STATUS_BAD,
	/* The battery is removed     */
	BATTERY_STATUS_REMOVED,		/* on v2.2 only */
	BATTERY_STATUS_INVALID_v1 = BATTERY_STATUS_REMOVED,
	/* Invalid battery status.    */
	BATTERY_STATUS_INVALID
};

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type {
	/* The battery voltage is dead/very low (less than 3.2V) */
	BATTERY_LEVEL_DEAD,
	/* The battery voltage is weak/low (between 3.2V and 3.4V) */
	BATTERY_LEVEL_WEAK,
	/* The battery voltage is good/normal(between 3.4V and 4.2V) */
	BATTERY_LEVEL_GOOD,
	/* The battery voltage is up to full (close to 4.2V) */
	BATTERY_LEVEL_FULL,
	/* Invalid battery voltage level. */
	BATTERY_LEVEL_INVALID
};

/*Edison add ATS & ERROR EVENT 20111122 ++*/
enum charging_status_event {
	CHARGING_EVENT_NORMAL = 0,
	CHARGING_EVENT_PROTECTION_IC_WARNING,
	CHARGING_EVENT_TEMPERATURE_WARNING,	
	CHARGING_EVENT_BATTERY_DISCONNECTED,
	CHARGING_EVENT_ALIENCE_BATTERY,
	CHARGING_EVENT_OVER_VOLTAGE,
	
};
static u8 WARNING_EVENT_TRIGGER = CHARGING_EVENT_NORMAL ;
/*Edison add ATS & ERROR EVENT 20111122 --*/

static struct wake_lock wall_charger_wake_lock;


/*Edison add for protection IC trigger 20111122 ++*/
static irqreturn_t msm_protection_set(int irq, void *dev_id);
static void protection_ic_debounce(struct work_struct *work); 
static int msm_batt_get_chg_irq_status(void);
static void msm_batt_update_psy_status(void);

static irqreturn_t msm_protection_set(int irq, void *dev_id)
{	

	schedule_delayed_work(&protection_ic_work,msecs_to_jiffies(10));
	
	return IRQ_HANDLED;	
}

/*Edison add for protection IC trigger  20111122--*/

// Edison add for capacity translation++
#ifdef BATTERY_CAPACITY_REAL
struct voltage2capacity{
u32	mv;
u32	percent;
};	

#if 1
/*struct voltage2capacity Vbat_Percent_type1 [] =
{
		{4171 , 100},		
		{4122 , 98  },	
		{4073 , 93  },	
		{4018 , 86  },	
		{3958 , 77  },	
		{3914 , 69  },	
		{3869 , 61  },	
		{3835 , 56  },	
		{3797 , 45  },	
		{3774 , 31  },	
		{3746 , 23  },	
		{3725 , 17  },	
		{3689 , 11  },	
		{3674 , 7    },	
		{3657 , 5    },	
		{3636 , 4    },	
		{3598 , 3    },	
		{3548 , 2    },	
		{3485 , 1    },
		{3400 , 0    },	
};*/
struct voltage2capacity Vbat_Percent_type1 [] =
{
		{4171 , 100},		
		{4122 , 98  },	
		{4073 , 93  },	
		{4018 , 86  },	
		{3958 , 77  },	
		{3914 , 69  },	
		{3869 , 61  },	
		{3835 , 56  },	
		{3810 , 51  },	
		{3797 , 46  },	
		{3770 , 43  },
		{3756 , 40  },	
		{3735 , 35  },	
		{3710 , 30  },	
		{3784 , 25  },	
		{3652 , 20  },	
		{3625 , 15  },	
		{3588 , 10  },	
		{3558 , 7    },
		{3548 , 5    },	
		{3520 , 4    },	
		{3490 , 3    },	
		{3470 , 2    },	
		{3450 , 1    },	
		{3400 , 0    },	
};
#else
struct voltage2capacity Vbat_Percent_type2 [] =
{
		{4161 , 100},		
		{4124 , 98  },	
		{4044 , 90  },	
		{4003 , 85  },	
		{3966 , 80  },	
		{3933 , 75  },	
		{3888 , 67  },	
		{3849 , 60  },	
		{3813 , 55  },	
		{3787 , 47  },	
		{3772 , 30  },	
		{3751 , 25  },	
		{3718 , 25  },	
		{3681 , 16  },	
		{3660 , 14  },	
		{3589 , 10  },	
		{3546 , 7  },	
		{3495 , 4    },	
		{3404 , 2    },	
		{3250 , 0    },
		
};
#endif

#endif
// Edison add for capacity translation--

#ifndef CONFIG_BATTERY_MSM_FAKE
struct rpc_reply_batt_chg_v1 {
	struct rpc_reply_hdr hdr;
	u32 	more_data;

	u32	 charger_status;
	u32	 charger_type;
	u32	 battery_status;
	u32	 battery_level;
	u32  battery_voltage;
	int	 battery_temp;
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	u32  charger_voltage;
	u32  charger_current;
	u32  charging_state_status;
	u32  mCurrentState;
	u32  fake_battery_voltage;
	u32  invalid_charger_irq_has_triggered;
	/*Edison add ATS & ERROR EVENT 20111122 --*/
};

struct rpc_reply_batt_chg_v2 {
	struct rpc_reply_batt_chg_v1	v1;

	u32	is_charger_valid;
	u32	is_charging;
	u32	is_battery_valid;
	u32	ui_event;
};

union rpc_reply_batt_chg {
	struct rpc_reply_batt_chg_v1	v1;
	struct rpc_reply_batt_chg_v2	v2;
};

static union rpc_reply_batt_chg rep_batt_chg;
#endif

struct msm_battery_info {
	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 voltage_fail_safe;
	u32 chg_api_version;
	u32 batt_technology;
	u32 batt_api_version;

	u32 avail_chg_sources;
	u32 current_chg_source;

	u32 batt_status;
	u32 batt_health;
	u32 charger_valid;
	u32 batt_valid;
	u32 batt_capacity; /* in percentage */

	u32 charger_status;
	u32 charger_type;
	u32 battery_status;
	u32 battery_level;
	u32 battery_voltage; /* in millie volts */
	int battery_temp;  /* in celsius */

	u32(*calculate_capacity) (u32 voltage);

	s32 batt_handle;

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;
	struct power_supply *msm_psy_batt;
	struct power_supply *current_ps;

	struct msm_rpc_client *batt_client;
	struct msm_rpc_endpoint *chg_ep;

	wait_queue_head_t wait_q;

	u32 vbatt_modify_reply_avail;

	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	u32  charger_voltage;
	u32  charger_current;
	u32  charging_state_status;
	u32  mCurrentState;
	u8    error_event;
	u32 fake_battery_voltage;
	u32 invalid_charger_irq_has_triggered;
	/*Edison add ATS & ERROR EVENT 20111122 --*/

	struct early_suspend early_suspend;
};

static struct msm_battery_info msm_batt_info = {
	.batt_handle = INVALID_BATT_HANDLE,
	.charger_status = CHARGER_STATUS_BAD,
	.charger_type = CHARGER_TYPE_INVALID,
	.battery_status = BATTERY_STATUS_GOOD,
	.battery_level = BATTERY_LEVEL_FULL,
	.battery_voltage = BATTERY_HIGH,
	.batt_capacity = 100,
	.batt_status = POWER_SUPPLY_STATUS_DISCHARGING,
	.batt_health = POWER_SUPPLY_HEALTH_GOOD,
	.batt_valid  = 1,
	.battery_temp = 23,
	.vbatt_modify_reply_avail = 0,
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	.charger_current = 0,
	.charger_voltage = 0,
	.charging_state_status = 0,
	.mCurrentState = 0,
	.fake_battery_voltage = 0,
	.invalid_charger_irq_has_triggered =0,
	.error_event = CHARGING_EVENT_NORMAL,
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
};

static enum power_supply_property msm_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
	"battery",
};

//[Arima Edison] ++
static int msm_set_power_off_charge_status(uint32_t value)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t set;
	} req;

	if (!msm_batt_info.chg_ep || IS_ERR(msm_batt_info.chg_ep))
		return -EAGAIN;
	req.set = cpu_to_be32(value);
	
	rc = msm_rpc_call(msm_batt_info.chg_ep, ONCRPC_CHG_SET_POWER_OFF_CHARGE_FLAG,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		pr_err("%s: charger_i_available failed! rc = %d\n",
			__func__, rc);
	} 

	return rc;
}
//[Arima Edison] --


//[Arima Edison] use to update telephony status ++

static int msm_set_telephony_status_rpc(uint32_t value)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t set;
	} req;

	if (!msm_batt_info.chg_ep || IS_ERR(msm_batt_info.chg_ep))
		return -EAGAIN;
	req.set = cpu_to_be32(value);
	
	rc = msm_rpc_call(msm_batt_info.chg_ep, ONCRPC_CHG_TRIGGER_TELEPHONY	,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		pr_err("%s: charger_i_available failed! rc = %d\n",
			__func__, rc);
	} 

	return rc;
}

static ssize_t msm_batt_set_telephony_status(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{

	static unsigned long telephony_on ;	
	static int telephone_on_flag = 0;
	
	if (strict_strtoul(buf, 10, &telephony_on))
		return -EINVAL;	

	if(telephony_on>0)
		telephony_on = 1;

	if(telephony_on!=telephone_on_flag)
	{
		msm_set_telephony_status_rpc((uint32_t)telephony_on);
		telephone_on_flag = telephony_on;
	}		
	
	//printk(KERN_EMERG "%s , telephony_on = %lu  \n",__func__,telephony_on);
		
	return count;
}

static int msm_set_charger_irq_handler_rpc(uint32_t value)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t set;
	} req;

	if (!msm_batt_info.chg_ep || IS_ERR(msm_batt_info.chg_ep))
		return -EAGAIN;
	req.set = cpu_to_be32(value);
	
	rc = msm_rpc_call(msm_batt_info.chg_ep, ONCRPC_CHG_TRIGGER_CHARGER_IRQ_HANDLER,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		pr_err("SET %s:  failed! rc = %d\n",
			__func__, rc);
	} 

	return rc;
}

static ssize_t msm_batt_set_invalid_charger_irq_handler(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{

	unsigned long charger_irq_enable_temp=0;	
	
	if (strict_strtoul(buf, 10, &charger_irq_enable_temp))
		return -EINVAL;	
      printk(KERN_EMERG "%s : %lu \n",__func__,charger_irq_enable_temp);
	
	msm_set_charger_irq_handler_rpc((uint32_t)charger_irq_enable_temp);
		
	return count;
}

static DEVICE_ATTR(set_telephony_status , 0644 , NULL , msm_batt_set_telephony_status);
static DEVICE_ATTR(set_invalid_charger_irq_handler, 0644 , NULL , msm_batt_set_invalid_charger_irq_handler);
static struct attribute *msm_battery_attributes[] = {
	&dev_attr_set_telephony_status.attr,
	&dev_attr_set_invalid_charger_irq_handler.attr,
	NULL
};
static struct attribute_group msm_battery_attribute_group = {
	.attrs = msm_battery_attributes
};


//[Arima Edison] use to update telephony status --

static int msm_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			val->intval = msm_batt_info.current_chg_source & AC_CHG
			    ? 1 : 0;
		//printk(KERN_EMERG "AC_CHG : (%d) \n",val->intval);
		}
		if (psy->type == POWER_SUPPLY_TYPE_USB) {
			val->intval = msm_batt_info.current_chg_source & USB_CHG
			    ? 1 : 0;
		//printk(KERN_EMERG "USB_CHG : (%d) \n",val->intval);	
		}
		
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_POWER_AVG,	
	POWER_SUPPLY_PROP_ERROR_EVENT,
	POWER_SUPPLY_PROP_CHARGER_IRQ,
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
};

// Edison add for capacity translation++
#ifdef BATTERY_CAPACITY_REAL
static u32 msm_batt_capacity_real(u32 current_voltage)
{
	static int i, divide_value;
	static int current_capacity;

	//printk(KERN_NOTICE "Edison in msm_batt_capacity_edison = %d",current_voltage);
	if (current_voltage >= Vbat_Percent_type1[0].mv)		
		current_capacity = 100;			
	else if(current_voltage <= Vbat_Percent_type1[24].mv)
		current_capacity=  0;			
	else
	{
		for(i=0; i<24; i++)
		{
			if(current_voltage >= Vbat_Percent_type1[i+1].mv && current_voltage <Vbat_Percent_type1[i].mv)
			{
			   divide_value = Vbat_Percent_type1[i].mv-Vbat_Percent_type1[i+1].mv;	
			   current_capacity =  Vbat_Percent_type1[i+1].percent 	\
			   +( (Vbat_Percent_type1[i].percent-Vbat_Percent_type1[i+1].percent) \
			   *(current_voltage-Vbat_Percent_type1[i+1].mv)+divide_value/2 ) /divide_value;	
			   
			   break; // we should leave this for loop if we have got what we needed [Arima Edison]		
			}
		}		
	}
	//printk(KERN_NOTICE "Edison capacity = %d ", current_capacity);		
	return current_capacity;
}
#endif
// Edison add for capacity translation--

static int msm_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = msm_batt_info.batt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_batt_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = msm_batt_info.batt_valid;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_batt_info.batt_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_batt_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_batt_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = msm_batt_info.battery_voltage*1000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = msm_batt_info.batt_capacity;
		break;
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = msm_batt_info.battery_temp*10;
		// add for demo
		//val->intval = 250;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = msm_batt_info.charger_current;
		//printk(KERN_EMERG "msm_batt_info.charger_current = %d \n",msm_batt_info.charger_current);
		break;
	case POWER_SUPPLY_PROP_POWER_AVG:
		val->intval = msm_batt_info.charger_voltage;
		break;			
	case POWER_SUPPLY_PROP_ERROR_EVENT:		
		val->intval = msm_batt_info.error_event;
		//printk("msm_batt_info.error_event = %d",msm_batt_info.error_event);
		WARNING_EVENT_TRIGGER = CHARGING_EVENT_NORMAL;
		break;
	case POWER_SUPPLY_PROP_CHARGER_IRQ:
		val->intval = msm_batt_get_chg_irq_status();
		break;
	/*Edison add ATS & ERROR EVENT 20111122 --*/		
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_batt = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_batt_power_props,
	.num_properties = ARRAY_SIZE(msm_batt_power_props),
	.get_property = msm_batt_power_get_property,
};

#ifndef CONFIG_BATTERY_MSM_FAKE
struct msm_batt_get_volt_ret_data {
	u32 battery_voltage;
};

/*Edison add ATS & ERROR EVENT 20111122 ++*/
struct msm_batt_get_temp_data {
	int battery_temp;
};

/*Edison add ATS & ERROR EVENT 20111122 --*/

static int msm_batt_get_volt_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct msm_batt_get_volt_ret_data *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_get_volt_ret_data *)data;
	buf_ptr = (struct msm_batt_get_volt_ret_data *)buf;

	data_ptr->battery_voltage = be32_to_cpu(buf_ptr->battery_voltage);

	return 0;
}

/*Edison add ATS & ERROR EVENT 20111122 ++*/
static int msm_batt_get_temp_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct msm_batt_get_temp_data *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_get_temp_data *)data;
	buf_ptr = (struct msm_batt_get_temp_data *)buf;

	data_ptr->battery_temp = be32_to_cpu(buf_ptr->battery_temp);
	//printk("%s : battery_temp = %d \n",__func__,data_ptr->battery_temp);

	return 0;
}

/*Edison add ATS & ERROR EVENT 20111122 --*/

static u32 msm_batt_get_vbatt_voltage(void)
{
	int rc;

	struct msm_batt_get_volt_ret_data rep;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_READ_MV_PROC,
			NULL, NULL,
			msm_batt_get_volt_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt get volt. rc=%d\n", __func__, rc);
		return 0;
	}

	return rep.battery_voltage;
}

/*Edison add ATS & ERROR EVENT 20111122 ++*/
static int msm_batt_get_temp(void)
{
	int rc;

	struct msm_batt_get_temp_data rep;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_READ_TEMP_PROC,
			NULL, NULL,
			msm_batt_get_temp_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));
	
	if (rc < 0) {
		pr_err("%s: FAIL: vbatt get temp. rc=%d\n", __func__, rc);
		return 0;
	}
	//printk("rep.battery_temp = %d",rep.battery_temp);
	return rep.battery_temp;
}

/*Edison add ATS & ERROR EVENT 20111122 --*/

//[Arima Edison] add for trigger early suspend condition++
/*if value==1, means kernel enter early suspend*/
static int msm_trigger_suspend(uint32_t value)
{
	int rc = 0;
	struct hsusb_start_req {
		struct rpc_request_hdr hdr;
		uint32_t set;
	} req;

	if (!msm_batt_info.chg_ep || IS_ERR(msm_batt_info.chg_ep))
		return -EAGAIN;
	req.set = cpu_to_be32(value);
	
	rc = msm_rpc_call(msm_batt_info.chg_ep, ONCRPC_CHG_SET_BATTERY_TRIGGER_SUSPEND,
			&req, sizeof(req), 5 * HZ);

	if (rc < 0) {
		pr_err("%s: msm_trigger_suspend failed! rc = %d\n",
			__func__, rc);
	} 

	return rc;
}
//[Arima Edison] add for trigger early suspend condition--



#define	be32_to_cpu_self(v)	(v = be32_to_cpu(v))

//[Arima Edison] add a rpc to query chg irq status++
static int msm_batt_get_chg_irq_status(void)
{
	int rc;

	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
	} req_batt_chg;

	struct rpc_req_batt_chg_rep {
		struct rpc_reply_hdr hdr;
		uint32_t chg_irq_status;
	} rep;

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_CHECK_CHARGER_IRQ_STATUS,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep, sizeof(rep),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	
	if (rc < 0) {

		printk(KERN_EMERG "rpc error rc = %d  \n",rc);
		
		return rc;
	}

	pr_info("%s: chg irq: (%d)\n", __func__, be32_to_cpu(rep.chg_irq_status));
	return be32_to_cpu(rep.chg_irq_status);

}

//[Arima Edison] add a rpc to query chg irq status--

static int msm_batt_get_batt_chg_status(void)
{
	int rc;

	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_batt_chg;
	struct rpc_reply_batt_chg_v1 *v1p;

	req_batt_chg.more_data = cpu_to_be32(1);

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

	v1p = &rep_batt_chg.v1;
	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_GET_GENERAL_STATUS_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_batt_chg, sizeof(rep_batt_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		pr_err("%s: ERROR. msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} else if (be32_to_cpu(v1p->more_data)) {
		be32_to_cpu_self(v1p->charger_status);
		be32_to_cpu_self(v1p->charger_type);
		be32_to_cpu_self(v1p->battery_status);
		be32_to_cpu_self(v1p->battery_level);
		be32_to_cpu_self(v1p->battery_voltage);
		be32_to_cpu_self(v1p->battery_temp);
		/*Edison add ATS & ERROR EVENT 20111122 ++*/
		be32_to_cpu_self(v1p->charger_voltage);
		be32_to_cpu_self(v1p->charger_current);
		be32_to_cpu_self(v1p->charging_state_status);
		be32_to_cpu_self(v1p->mCurrentState);
		be32_to_cpu_self(v1p->fake_battery_voltage);
		be32_to_cpu_self(v1p->invalid_charger_irq_has_triggered);
		/*Edison add ATS & ERROR EVENT 20111122 --*/
	} else {
		pr_err("%s: No battery/charger data in RPC reply\n", __func__);
		return -EIO;
	}

	return 0;
}

static void msm_batt_update_psy_status(void)
{
	static u8 current_has_update;
	static u32 unnecessary_event_count;
	//[Arima Edison] registe new capacity ++
	u32  capacity_temp;
	//[Arima Edison] registe new capacity --
	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32 battery_voltage;
	int battery_temp;
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	u32    charger_current;
	u32    charger_voltage;
	u32    charging_state_status = 0;
	u32    mCurrentState;
	u32    fake_battery_voltage;
	u32    invalid_charger_irq_has_triggered;
	/*Edison add ATS & ERROR EVENT 20111122 --*/
	struct	power_supply	*supp;

	if (msm_batt_get_batt_chg_status())
		return;

	charger_status = rep_batt_chg.v1.charger_status;
	charger_type = rep_batt_chg.v1.charger_type;
	battery_status = rep_batt_chg.v1.battery_status;
	battery_level = rep_batt_chg.v1.battery_level;
	fake_battery_voltage = rep_batt_chg.v1.fake_battery_voltage;
	invalid_charger_irq_has_triggered = rep_batt_chg.v1.invalid_charger_irq_has_triggered;
	battery_voltage = rep_batt_chg.v1.battery_voltage;
	//printk(KERN_EMERG "real battery_voltage = %d fake_battery_voltage %d \n",battery_voltage,fake_battery_voltage);
	battery_temp = msm_batt_get_temp();

	printk(KERN_EMERG "invalid_charger_irq_has_triggered=%d",invalid_charger_irq_has_triggered);

	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	charger_voltage = rep_batt_chg.v1.charger_voltage;
	charger_current = rep_batt_chg.v1.charger_current;
	charging_state_status = rep_batt_chg.v1.charging_state_status;
	mCurrentState = rep_batt_chg.v1.mCurrentState;
	
	/*Edison add ATS & ERROR EVENT 20111122 --+*/
	//printk("charger voltage = %d  ,  charger current = %d   \n",charger_voltage,charger_current);
	/* Make correction for battery status */
	if (battery_status == BATTERY_STATUS_INVALID_v1) {
		if (msm_batt_info.chg_api_version < CHG_RPC_VER_3_1)
			battery_status = BATTERY_STATUS_INVALID;
	}

	//[Arima Edison] add to remove current update trigger ++
	if(charger_status==CHARGER_STATUS_INVALID && current_has_update==1)
		current_has_update=0;
	//[Arima Edison] add to remove current update trigger --

	if (charger_status == msm_batt_info.charger_status &&
	    charger_type == msm_batt_info.charger_type &&
	    battery_status == msm_batt_info.battery_status &&
	    battery_level == msm_batt_info.battery_level &&
	    battery_voltage == msm_batt_info.battery_voltage &&
	    battery_temp == msm_batt_info.battery_temp && 
	    charger_current == msm_batt_info.charger_current &&
	    charging_state_status == msm_batt_info.charging_state_status &&
	    mCurrentState == msm_batt_info.mCurrentState &&
	     invalid_charger_irq_has_triggered == msm_batt_info.invalid_charger_irq_has_triggered   // for power off charge
	    ) {
		/* Got unnecessary event from Modem PMIC VBATT driver.
		 * Nothing changed in Battery or charger status.
		 */
		unnecessary_event_count++;
		if ((unnecessary_event_count % 20) == 1)
			DBG_LIMIT("BATT: same event count = %u\n",
				 unnecessary_event_count);
		return;
	}

	unnecessary_event_count = 0;

	printk(KERN_EMERG "BATT: rcvd: %d, %d, %d, %d; %d, %d ; %d , %d\n",
		 charger_status, charger_type, battery_status,
		 battery_level, battery_voltage, battery_temp, charging_state_status, mCurrentState);

	msm_batt_info.charging_state_status = charging_state_status; 
	msm_batt_info.mCurrentState = mCurrentState; 

	if (battery_status == BATTERY_STATUS_INVALID &&
	    battery_level != BATTERY_LEVEL_INVALID) {
		DBG_LIMIT("BATT: change status(%d) to (%d) for level=%d\n",
			 battery_status, BATTERY_STATUS_GOOD, battery_level);
		battery_status = BATTERY_STATUS_GOOD;
	}

	if (msm_batt_info.charger_type != charger_type) {
		/*++ Edison - 20111017 change usb wall charger definition ++*/
		if ( charger_type == CHARGER_TYPE_USB_PC ||
		    charger_type == CHARGER_TYPE_USB_CARKIT) {
		/*-- Edison - 20111017 change usb wall charger definition --*/
			DBG_LIMIT("BATT: USB charger plugged in\n");
			msm_batt_info.current_chg_source = USB_CHG;
			supp = &msm_psy_usb;
		/*++ Edison - 20111017 change usb wall charger definition ++*/
		} else if (charger_type == CHARGER_TYPE_USB_WALL ||
				charger_type == CHARGER_TYPE_WALL) {
		/*-- Edison - 20111017 change usb wall charger definition --*/
			DBG_LIMIT("BATT: AC Wall changer plugged in\n");
			msm_batt_info.current_chg_source = AC_CHG;
			supp = &msm_psy_ac;
			
			//wake_lock(&wall_charger_wake_lock);
		} else {
			if (msm_batt_info.current_chg_source & AC_CHG)
			{
				//wake_unlock(&wall_charger_wake_lock);
				DBG_LIMIT("BATT: AC Wall charger removed\n");
			}	
			else if (msm_batt_info.current_chg_source & USB_CHG)
				DBG_LIMIT("BATT: USB charger removed\n");
			else
				DBG_LIMIT("BATT: No charger present\n");
			msm_batt_info.current_chg_source = 0;
			supp = &msm_psy_batt;

			/* Correct charger status */
			if (charger_status != CHARGER_STATUS_INVALID) {
				DBG_LIMIT("BATT: No charging!\n");
				charger_status = CHARGER_STATUS_INVALID;
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_NOT_CHARGING;
			}
		}
	} else
		supp = NULL;

	if (msm_batt_info.charger_status != charger_status) {
		if (charger_status == CHARGER_STATUS_GOOD ||
		    charger_status == CHARGER_STATUS_WEAK) {
			if (msm_batt_info.current_chg_source) {
				DBG_LIMIT("BATT: Charging.\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_CHARGING;

				/* Correct when supp==NULL */
				if (msm_batt_info.current_chg_source & AC_CHG)
					supp = &msm_psy_ac;
				else
					supp = &msm_psy_usb;
			}
		} else {
			DBG_LIMIT("BATT: No charging.\n");
			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_NOT_CHARGING;
			supp = &msm_psy_batt;
		}
	} else {
		/* Correct charger status */
		if (charger_type != CHARGER_TYPE_INVALID &&
		    charger_status == CHARGER_STATUS_GOOD) {
			DBG_LIMIT("BATT: In charging\n");
			msm_batt_info.batt_status =
				POWER_SUPPLY_STATUS_CHARGING;
		}
	}

	/* Correct battery voltage and status */
	if (!battery_voltage) {
		if (charger_status == CHARGER_STATUS_INVALID) {			
			battery_voltage = msm_batt_get_vbatt_voltage();	
			DBG_LIMIT("BATT: Read VBATT (%d)\n",battery_voltage);
			printk(KERN_NOTICE "BATT: Read VBATT (%d)\n",battery_voltage);
		} 
		else
		{
			/* Use previous */			
			battery_voltage = msm_batt_info.battery_voltage;
		}	
	}
	if (battery_status == BATTERY_STATUS_INVALID) {
		if (battery_voltage >= msm_batt_info.voltage_min_design &&
		    battery_voltage <= msm_batt_info.voltage_max_design) {
			DBG_LIMIT("BATT: Battery valid\n");
			msm_batt_info.batt_valid = 1;
			battery_status = BATTERY_STATUS_GOOD;
		}
	}

	if (msm_batt_info.battery_status != battery_status) {
		if (battery_status != BATTERY_STATUS_INVALID) {
			msm_batt_info.batt_valid = 1;

			if (battery_status == BATTERY_STATUS_BAD) {
				DBG_LIMIT("BATT: Battery bad.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_DEAD;
			} else if (battery_status == BATTERY_STATUS_BAD_TEMP) {
				DBG_LIMIT("BATT: Battery overheat.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_OVERHEAT;
			} else {
				DBG_LIMIT("BATT: Battery good.\n");
				msm_batt_info.batt_health =
					POWER_SUPPLY_HEALTH_GOOD;
			}
		} else {
			msm_batt_info.batt_valid = 0;
			DBG_LIMIT("BATT: Battery invalid.\n");
			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
		}

		if (msm_batt_info.batt_status != POWER_SUPPLY_STATUS_CHARGING) {
			if (battery_status == BATTERY_STATUS_INVALID) {
				DBG_LIMIT("BATT: Battery -> unknown\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_UNKNOWN;
			} else {
				DBG_LIMIT("BATT: Battery -> discharging\n");
				msm_batt_info.batt_status =
					POWER_SUPPLY_STATUS_DISCHARGING;
			}
		}

		if (!supp) {
			if (msm_batt_info.current_chg_source) {
				if (msm_batt_info.current_chg_source & AC_CHG)
					supp = &msm_psy_ac;
				else
					supp = &msm_psy_usb;
			} else
				supp = &msm_psy_batt;
		}
	}

	msm_batt_info.charger_status 	= charger_status;
	msm_batt_info.charger_type 	= charger_type;
	msm_batt_info.battery_status 	= battery_status;
	msm_batt_info.battery_level 	= battery_level;
	msm_batt_info.battery_temp 	= battery_temp;
	/*Edison add ATS & ERROR EVENT 20111122 ++*/
	msm_batt_info.charger_current = charger_current;
	if(charger_current>800 || charger_current<=0)
		msm_batt_info.charger_current=0;
	else
		msm_batt_info.charger_current = charger_current;
	msm_batt_info.charger_voltage = charger_voltage;
	/*Edison add ATS & ERROR EVENT 20111122 --*/

	if (msm_batt_info.battery_voltage>=4500 && WARNING_EVENT_TRIGGER == CHARGING_EVENT_NORMAL)
		WARNING_EVENT_TRIGGER = CHARGING_EVENT_OVER_VOLTAGE;
	//Edison add for warning event 20111122 ++	
	if(msm_batt_info.charger_status!=CHARGER_STATUS_INVALID)
	{
		if((battery_temp>=55 || battery_temp <0)&&
			(charger_status==CHARGER_STATUS_GOOD||charger_status==CHARGER_STATUS_WEAK))
		{		
			if(WARNING_EVENT_TRIGGER == CHARGING_EVENT_NORMAL)
				WARNING_EVENT_TRIGGER = CHARGING_EVENT_TEMPERATURE_WARNING;
			printk("temperature error = %d",battery_temp);
		}		
	}
	
	msm_batt_info.error_event = WARNING_EVENT_TRIGGER ;
	//Edison add for warning event 20111122 --	

	//[Arima Edison] add for update charger current info ++
	if(!current_has_update && msm_batt_info.charger_current>0 )
	{
		current_has_update = 1;
		if (!supp)
			supp = msm_batt_info.current_ps;
	}
	//[Arima Edison] add for update charger current info --

    
	//upodate battery capacity ++
	if (msm_batt_info.battery_voltage != battery_voltage) {
		msm_batt_info.battery_voltage  = battery_voltage;

		capacity_temp = msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);
		msm_batt_info.batt_capacity = capacity_temp;
#if 0
		if(fake_battery_voltage!=0 && fake_battery_voltage<=msm_batt_info.battery_voltage)  // fake voltage
		{				
			capacity_temp = msm_batt_info.calculate_capacity(fake_battery_voltage);			
			if(charger_status == CHARGER_STATUS_GOOD && msm_batt_info.batt_capacity<capacity_temp)
			{
				// when charging, capaticy should not decrease
				msm_batt_info.batt_capacity = capacity_temp;
				//printk(KERN_EMERG "capacity1 \n");
			}	
			else if(charger_status != CHARGER_STATUS_GOOD && capacity_temp<msm_batt_info.batt_capacity) 
			{
				// when not charging, capaticy should not increase
				msm_batt_info.batt_capacity = capacity_temp;
				//printk(KERN_EMERG "capacity2  \n");
			}	
			else
				printk(KERN_EMERG "capacity not update \n");
			DBG_LIMIT("BATT1: voltage = %u mV [capacity = %d%%]  charger_status : (%d)\n",
			 	battery_voltage, msm_batt_info.batt_capacity,charger_status);
		}	
		else    // real voltage
		{
			capacity_temp = msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);			
			 if(charger_status == CHARGER_STATUS_GOOD && msm_batt_info.batt_capacity<capacity_temp)
			 {
				// when charging, capaticy should not decrease
				msm_batt_info.batt_capacity = capacity_temp;
				//printk(KERN_EMERG "capacity3 \n");
			 }				 
			else if(capacity_temp<msm_batt_info.batt_capacity)
			{
				// when not charging, capaticy should not increase
				msm_batt_info.batt_capacity = capacity_temp;
				//printk(KERN_EMERG "capacity4 \n");
			}	
			else
				printk(KERN_EMERG "capacity not update \n");		
			DBG_LIMIT("BATT2: voltage = %u mV [capacity = %d%%]  charger_status : (%d) \n",
				 battery_voltage, msm_batt_info.batt_capacity,charger_status);
		}  		    
#endif

		if (!supp)
			supp = msm_batt_info.current_ps;
	}

	//Edison add to update charging state if full charged ++
	if(msm_batt_info.battery_level==BATTERY_LEVEL_FULL && msm_batt_info.batt_status==POWER_SUPPLY_STATUS_CHARGING) 
	{
		msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;	
		//[Arima Edison] we set capacity always to 100 if battery status is full, will it cause other impact ?? 
		msm_batt_info.batt_capacity = 100; 
		//[Arima Edison] we set capacity always to 100 if battery status is full, will it cause other impact ?? 
		if (!supp)
			supp = msm_batt_info.current_ps;
	}
	//Edison add to update charging state if full charged --

       if(msm_batt_info.invalid_charger_irq_has_triggered!=invalid_charger_irq_has_triggered)
       {
		msm_batt_info.invalid_charger_irq_has_triggered=invalid_charger_irq_has_triggered;
		if (!supp)
			supp = msm_batt_info.current_ps;
       }

	if (supp) {		
		msm_batt_info.current_ps = supp;
		//printk(KERN_EMERG "BATT: Supply = %s\n", supp->name);
		power_supply_changed(supp);
	}
}

#ifdef CONFIG_HAS_EARLYSUSPEND
struct batt_modify_client_req {

	u32 client_handle;

	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
};

struct batt_modify_client_rep {
	u32 result;
};

static int msm_batt_modify_client_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_modify_client_req *batt_modify_client_req =
		(struct batt_modify_client_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(batt_modify_client_req->client_handle);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->desired_batt_voltage);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->voltage_direction);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->batt_cb_id);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_modify_client_req->cb_data);
	size += sizeof(u32);

	return size;
}

static int msm_batt_modify_client_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct  batt_modify_client_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_modify_client_rep *)data;
	buf_ptr = (struct batt_modify_client_rep *)buf;

	data_ptr->result = be32_to_cpu(buf_ptr->result);

	return 0;
}

static int msm_batt_modify_client(u32 client_handle, u32 desired_batt_voltage,
	     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	int rc;

	struct batt_modify_client_req  req;
	struct batt_modify_client_rep rep;

	req.client_handle = client_handle;
	req.desired_batt_voltage = desired_batt_voltage;
	req.voltage_direction = voltage_direction;
	req.batt_cb_id = batt_cb_id;
	req.cb_data = cb_data;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_MODIFY_CLIENT_PROC,
			msm_batt_modify_client_arg_func, &req,
			msm_batt_modify_client_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: ERROR. failed to modify  Vbatt client\n",
		       __func__);
		return rc;
	}

	if (rep.result != BATTERY_MODIFICATION_SUCCESSFUL) {
		pr_err("%s: ERROR. modify client failed. result = %u\n",
		       __func__, rep.result);
		return -EIO;
	}

	return 0;
}

void msm_batt_early_suspend(struct early_suspend *h)
{
	int rc;

	//Edison add  ---> it's a workaround 
		
	printk(KERN_EMERG "%s: enter\n", __func__);
	rc = msm_trigger_suspend(1);
		return;
	#if 0	
	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {
		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				msm_batt_info.voltage_fail_safe,
				BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				BATTERY_CB_ID_LOW_VOL,
				msm_batt_info.voltage_fail_safe);

		if (rc < 0) {
			pr_err("%s: msm_batt_modify_client. rc=%d\n",
			       __func__, rc);
			return;
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return;
	}
	#endif
	pr_debug("%s: exit\n", __func__);
}

void msm_batt_late_resume(struct early_suspend *h)
{
	int rc;
	static u8 msm_batt_client_flag = 0;

	//printk(KERN_EMERG "%s: enter\n", __func__);
	rc = msm_trigger_suspend(0);

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE ) {

		if(!msm_batt_client_flag)
		{
			rc = msm_batt_modify_client(msm_batt_info.batt_handle,
					BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       	BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
			if (rc < 0) {
				pr_err("%s: msm_batt_modify_client FAIL rc=%d\n",
			       __func__, rc);
				return;
			}
			else
			{
				msm_batt_client_flag = 1;
			}
		}
	} else {
		pr_err("%s: ERROR. invalid batt_handle\n", __func__);
		return;
	}
   //[Arima Edison] should we update battery info whenever we leave sleep!? ++
	msm_batt_update_psy_status();
	//[Arima Edison] should we update battery info whenever we leave sleep!? --
	pr_debug("%s: exit\n", __func__);
}
#endif

struct msm_batt_vbatt_filter_req {
	u32 batt_handle;
	u32 enable_filter;
	u32 vbatt_filter;
};

struct msm_batt_vbatt_filter_rep {
	u32 result;
};

static int msm_batt_filter_arg_func(struct msm_rpc_client *batt_client,

		void *buf, void *data)
{
	struct msm_batt_vbatt_filter_req *vbatt_filter_req =
		(struct msm_batt_vbatt_filter_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(vbatt_filter_req->batt_handle);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->enable_filter);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->vbatt_filter);
	size += sizeof(u32);
	return size;
}

static int msm_batt_filter_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{

	struct msm_batt_vbatt_filter_rep *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_vbatt_filter_rep *)data;
	buf_ptr = (struct msm_batt_vbatt_filter_rep *)buf;

	data_ptr->result = be32_to_cpu(buf_ptr->result);
	return 0;
}

static int msm_batt_enable_filter(u32 vbatt_filter)
{
	int rc;
	struct  msm_batt_vbatt_filter_req  vbatt_filter_req;
	struct  msm_batt_vbatt_filter_rep  vbatt_filter_rep;

	vbatt_filter_req.batt_handle = msm_batt_info.batt_handle;
	vbatt_filter_req.enable_filter = 1;
	vbatt_filter_req.vbatt_filter = vbatt_filter;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_ENABLE_DISABLE_FILTER_PROC,
			msm_batt_filter_arg_func, &vbatt_filter_req,
			msm_batt_filter_ret_func, &vbatt_filter_rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: enable vbatt filter. rc=%d\n",
		       __func__, rc);
		return rc;
	}

	if (vbatt_filter_rep.result != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: FAIL: enable vbatt filter: result=%d\n",
		       __func__, vbatt_filter_rep.result);
		return -EIO;
	}

	pr_debug("%s: enable vbatt filter: OK\n", __func__);
	return rc;
}

struct batt_client_registration_req {
	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 more_data;
	u32 batt_error;
};

struct batt_client_registration_req_4_1 {
	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 batt_error;
};

struct batt_client_registration_rep {
	u32 batt_handle;
};

struct batt_client_registration_rep_4_1 {
	u32 batt_handle;
	u32 more_data;
	u32 err;
};

static int msm_batt_register_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_req *batt_reg_req =
		(struct batt_client_registration_req *)data;

	u32 *req = (u32 *)buf;
	int size = 0;


	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		*req = cpu_to_be32(batt_reg_req->desired_batt_voltage);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->voltage_direction);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_cb_id);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->cb_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_error);
		size += sizeof(u32);

		return size;
	} else {
		*req = cpu_to_be32(batt_reg_req->desired_batt_voltage);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->voltage_direction);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_cb_id);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->cb_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->more_data);
		size += sizeof(u32);
		req++;

		*req = cpu_to_be32(batt_reg_req->batt_error);
		size += sizeof(u32);

		return size;
	}

}

static int msm_batt_register_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_rep *data_ptr, *buf_ptr;
	struct batt_client_registration_rep_4_1 *data_ptr_4_1, *buf_ptr_4_1;

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		data_ptr_4_1 = (struct batt_client_registration_rep_4_1 *)data;
		buf_ptr_4_1 = (struct batt_client_registration_rep_4_1 *)buf;

		data_ptr_4_1->batt_handle
			= be32_to_cpu(buf_ptr_4_1->batt_handle);
		data_ptr_4_1->more_data
			= be32_to_cpu(buf_ptr_4_1->more_data);
		data_ptr_4_1->err = be32_to_cpu(buf_ptr_4_1->err);
		return 0;
	} else {
		data_ptr = (struct batt_client_registration_rep *)data;
		buf_ptr = (struct batt_client_registration_rep *)buf;

		data_ptr->batt_handle = be32_to_cpu(buf_ptr->batt_handle);
		return 0;
	}
}

static int msm_batt_register(u32 desired_batt_voltage,
			     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	struct batt_client_registration_req batt_reg_req;
	struct batt_client_registration_req_4_1 batt_reg_req_4_1;
	struct batt_client_registration_rep batt_reg_rep;
	struct batt_client_registration_rep_4_1 batt_reg_rep_4_1;
	void *request;
	void *reply;
	int rc;

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		batt_reg_req_4_1.desired_batt_voltage = desired_batt_voltage;
		batt_reg_req_4_1.voltage_direction = voltage_direction;
		batt_reg_req_4_1.batt_cb_id = batt_cb_id;
		batt_reg_req_4_1.cb_data = cb_data;
		batt_reg_req_4_1.batt_error = 1;
		request = &batt_reg_req_4_1;
	} else {
		batt_reg_req.desired_batt_voltage = desired_batt_voltage;
		batt_reg_req.voltage_direction = voltage_direction;
		batt_reg_req.batt_cb_id = batt_cb_id;
		batt_reg_req.cb_data = cb_data;
		batt_reg_req.more_data = 1;
		batt_reg_req.batt_error = 0;
		request = &batt_reg_req;
	}

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1)
		reply = &batt_reg_rep_4_1;
	else
		reply = &batt_reg_rep;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_REGISTER_PROC,
			msm_batt_register_arg_func, request,
			msm_batt_register_ret_func, reply,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt register. rc=%d\n", __func__, rc);
		return rc;
	}

	if (msm_batt_info.batt_api_version == BATTERY_RPC_VER_4_1) {
		if (batt_reg_rep_4_1.more_data != 0
			&& batt_reg_rep_4_1.err
				!= BATTERY_REGISTRATION_SUCCESSFUL) {
			pr_err("%s: vBatt Registration Failed proc_num=%d\n"
					, __func__, BATTERY_REGISTER_PROC);
			return -EIO;
		}
		msm_batt_info.batt_handle = batt_reg_rep_4_1.batt_handle;
	} else
		msm_batt_info.batt_handle = batt_reg_rep.batt_handle;

	return 0;
}

struct batt_client_deregister_req {
	u32 batt_handle;
};

struct batt_client_deregister_rep {
	u32 batt_error;
};

static int msm_batt_deregister_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_req *deregister_req =
		(struct  batt_client_deregister_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(deregister_req->batt_handle);
	size += sizeof(u32);

	return size;
}

static int msm_batt_deregister_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_client_deregister_rep *)data;
	buf_ptr = (struct batt_client_deregister_rep *)buf;

	data_ptr->batt_error = be32_to_cpu(buf_ptr->batt_error);

	return 0;
}

static int msm_batt_deregister(u32 batt_handle)
{
	int rc;
	struct batt_client_deregister_req req;
	struct batt_client_deregister_rep rep;

	req.batt_handle = batt_handle;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_DEREGISTER_CLIENT_PROC,
			msm_batt_deregister_arg_func, &req,
			msm_batt_deregister_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt deregister. rc=%d\n", __func__, rc);
		return rc;
	}

	if (rep.batt_error != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: vbatt deregistration FAIL. error=%d, handle=%d\n",
		       __func__, rep.batt_error, batt_handle);
		return -EIO;
	}

	return 0;
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

static int msm_batt_cleanup(void)
{
	int rc = 0;

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_deregister(msm_batt_info.batt_handle);
		if (rc < 0)
			pr_err("%s: FAIL: msm_batt_deregister. rc=%d\n",
			       __func__, rc);
	}

	msm_batt_info.batt_handle = INVALID_BATT_HANDLE;

	if (msm_batt_info.batt_client)
		msm_rpc_unregister_client(msm_batt_info.batt_client);
#endif  /* CONFIG_BATTERY_MSM_FAKE */

	if (msm_batt_info.msm_psy_ac)
		power_supply_unregister(msm_batt_info.msm_psy_ac);

	if (msm_batt_info.msm_psy_usb)
		power_supply_unregister(msm_batt_info.msm_psy_usb);
	if (msm_batt_info.msm_psy_batt)
		power_supply_unregister(msm_batt_info.msm_psy_batt);

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_batt_info.chg_ep) {
		rc = msm_rpc_close(msm_batt_info.chg_ep);
		if (rc < 0) {
			pr_err("%s: FAIL. msm_rpc_close(chg_ep). rc=%d\n",
			       __func__, rc);
		}
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
		unregister_early_suspend(&msm_batt_info.early_suspend);
#endif
#endif
	return rc;
}

// Edison add for capacity translation++
#ifndef BATTERY_CAPACITY_REAL
static u32 msm_batt_capacity(u32 current_voltage)
{
	u32 low_voltage = msm_batt_info.voltage_min_design;
	u32 high_voltage = msm_batt_info.voltage_max_design;

	if (current_voltage <= low_voltage)
		return 0;
	else if (current_voltage >= high_voltage)
		return 100;
	else
		return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}
#endif
// Edison add for capacity translation--

#ifndef CONFIG_BATTERY_MSM_FAKE
int msm_batt_get_charger_api_version(void)
{
	int rc ;
	struct rpc_reply_hdr *reply;

	struct rpc_req_chg_api_ver {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_chg_api_ver;

	struct rpc_rep_chg_api_ver {
		struct rpc_reply_hdr hdr;
		u32 num_of_chg_api_versions;
		u32 *chg_api_versions;
	};

	u32 num_of_versions;

	struct rpc_rep_chg_api_ver *rep_chg_api_ver;


	req_chg_api_ver.more_data = cpu_to_be32(1);

	msm_rpc_setup_req(&req_chg_api_ver.hdr, CHG_RPC_PROG, CHG_RPC_VER_1_1,
			  ONCRPC_CHARGER_API_VERSIONS_PROC);

	rc = msm_rpc_write(msm_batt_info.chg_ep, &req_chg_api_ver,
			sizeof(req_chg_api_ver));
	if (rc < 0) {
		pr_err("%s: FAIL: msm_rpc_write. proc=0x%08x, rc=%d\n",
		       __func__, ONCRPC_CHARGER_API_VERSIONS_PROC, rc);
		return rc;
	}

	for (;;) {
		rc = msm_rpc_read(msm_batt_info.chg_ep, (void *) &reply, -1,
				BATT_RPC_TIMEOUT);
		if (rc < 0)
			return rc;
		if (rc < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			pr_err("%s: LENGTH ERR: msm_rpc_read. rc=%d (<%d)\n",
			       __func__, rc, RPC_REQ_REPLY_COMMON_HEADER_SIZE);

			rc = -EIO;
			break;
		}
		/* we should not get RPC REQ or call packets -- ignore them */
		if (reply->type == RPC_TYPE_REQ) {
			pr_err("%s: TYPE ERR: type=%d (!=%d)\n",
			       __func__, reply->type, RPC_TYPE_REQ);
			kfree(reply);
			continue;
		}

		/* If an earlier call timed out, we could get the (no
		 * longer wanted) reply for it.	 Ignore replies that
		 * we don't expect
		 */
		if (reply->xid != req_chg_api_ver.hdr.xid) {
			pr_err("%s: XID ERR: xid=%d (!=%d)\n", __func__,
			       reply->xid, req_chg_api_ver.hdr.xid);
			kfree(reply);
			continue;
		}
		if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {
			rc = -EPERM;
			break;
		}
		if (reply->data.acc_hdr.accept_stat !=
				RPC_ACCEPTSTAT_SUCCESS) {
			rc = -EINVAL;
			break;
		}

		rep_chg_api_ver = (struct rpc_rep_chg_api_ver *)reply;

		num_of_versions =
			be32_to_cpu(rep_chg_api_ver->num_of_chg_api_versions);

		rep_chg_api_ver->chg_api_versions =  (u32 *)
			((u8 *) reply + sizeof(struct rpc_reply_hdr) +
			sizeof(rep_chg_api_ver->num_of_chg_api_versions));

		rc = be32_to_cpu(
			rep_chg_api_ver->chg_api_versions[num_of_versions - 1]);

		pr_debug("%s: num_of_chg_api_versions = %u. "
			"The chg api version = 0x%08x\n", __func__,
			num_of_versions, rc);
		break;
	}
	kfree(reply);
	return rc;
}

static int msm_batt_cb_func(struct msm_rpc_client *client,
			    void *buffer, int in_size)
{
	int rc = 0;
	struct rpc_request_hdr *req;
	u32 procedure;
	u32 accept_status;

	req = (struct rpc_request_hdr *)buffer;
	procedure = be32_to_cpu(req->procedure);

	switch (procedure) {
	case BATTERY_CB_TYPE_PROC:
		accept_status = RPC_ACCEPTSTAT_SUCCESS;
		break;

	default:
		accept_status = RPC_ACCEPTSTAT_PROC_UNAVAIL;
		pr_err("%s: ERROR. procedure (%d) not supported\n",
		       __func__, procedure);
		break;
	}

	msm_rpc_start_accepted_reply(msm_batt_info.batt_client,
			be32_to_cpu(req->xid), accept_status);

	rc = msm_rpc_send_accepted_reply(msm_batt_info.batt_client, 0);
	if (rc)
		pr_err("%s: FAIL: sending reply. rc=%d\n", __func__, rc);

	if (accept_status == RPC_ACCEPTSTAT_SUCCESS)
	{
		//printk(KERN_EMERG "%s  : RPC_ACCEPTSTAT_SUCCESS \n",__func__);
		msm_batt_update_psy_status();
	}	

	return rc;
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

static void protection_ic_debounce (struct work_struct *work)
{
	if(!gpio_get_value(41))		
		WARNING_EVENT_TRIGGER = CHARGING_EVENT_PROTECTION_IC_WARNING;	
	printk(KERN_NOTICE "gpio_get_value(41) =%d",gpio_get_value(41));
	printk(KERN_NOTICE "PROTECTION_IC_WARNING_TRIGGER %d",WARNING_EVENT_TRIGGER);

}


static int __devinit msm_batt_probe(struct platform_device *pdev)
{
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;
	
	wake_lock_init(&wall_charger_wake_lock,
		       WAKE_LOCK_SUSPEND,
		       "charger_wakelock");
	
	INIT_DELAYED_WORK(&protection_ic_work, protection_ic_debounce);

	//Edison add battery information update --

	//INIT_WORK(&dev->reset_work, emac_reset_work);

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"%s: MSM chipsets Can only support one"
			" battery ", __func__);
		return -EINVAL;
	}

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (pdata->avail_chg_sources & AC_CHG) {
#else
	{
#endif
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_ac = &msm_psy_ac;
		msm_batt_info.avail_chg_sources |= AC_CHG;
	}

	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_usb = &msm_psy_usb;
		msm_batt_info.avail_chg_sources |= USB_CHG;
	}

	if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb) {

		dev_err(&pdev->dev,
			"%s: No external Power supply(AC or USB)"
			"is avilable\n", __func__);
		msm_batt_cleanup();
		return -ENODEV;
	}

	msm_batt_info.voltage_max_design = pdata->voltage_max_design;
	msm_batt_info.voltage_min_design = pdata->voltage_min_design;
	msm_batt_info.voltage_fail_safe  = pdata->voltage_fail_safe;

	msm_batt_info.batt_technology = pdata->batt_technology;	
	
	// Edison add for capacity translation++
	#ifdef BATTERY_CAPACITY_REAL
	msm_batt_info.calculate_capacity = msm_batt_capacity_real;
	#else
	msm_batt_info.calculate_capacity = pdata->calculate_capacity;
	#endif
	// Edison add for capacity translation--
	
	if (!msm_batt_info.voltage_min_design)
		msm_batt_info.voltage_min_design = BATTERY_LOW;
	if (!msm_batt_info.voltage_max_design)
		msm_batt_info.voltage_max_design = BATTERY_HIGH;
	if (!msm_batt_info.voltage_fail_safe)
		msm_batt_info.voltage_fail_safe  = BATTERY_LOW;

	if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
		msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;

// Edison add for capacity translation++
	if (!msm_batt_info.calculate_capacity)
	{
		#ifdef BATTERY_CAPACITY_REAL
		msm_batt_info.calculate_capacity = msm_batt_capacity_real;
		#else
		msm_batt_info.calculate_capacity = msm_batt_capacity;
		#endif
	}	
// Edison add for capacity translation--

	rc = power_supply_register(&pdev->dev, &msm_psy_batt);
	if (rc < 0) {
		dev_err(&pdev->dev, "%s: power_supply_register failed"
			" rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}
	else
	{	
		//[Arima Ediosn] should add some other protect??
		rc = sysfs_create_group(&msm_psy_batt.dev->kobj,&msm_battery_attribute_group);
	}
	msm_batt_info.msm_psy_batt = &msm_psy_batt;

#ifndef CONFIG_BATTERY_MSM_FAKE
	rc = msm_batt_register(BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_register failed rc = %d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	rc =  msm_batt_enable_filter(VBATT_FILTER);

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_enable_filter failed rc = %d\n",
			__func__, rc);
		msm_batt_cleanup();
		return rc;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
	msm_batt_info.early_suspend.resume = msm_batt_late_resume;
	register_early_suspend(&msm_batt_info.early_suspend);
#endif
	msm_batt_update_psy_status();

#else
	power_supply_changed(&msm_psy_ac);
#endif  /* CONFIG_BATTERY_MSM_FAKE */

/*Edison add for protection IC alarm 20111122 ++*/

      rc = gpio_tlmm_config(GPIO_CFG(41, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	msleep(10);
			
      if(gpio_get_value(41))  
      	{
      		//Jordan-20111230 , new function for Qualcomm ICS baseline  
      		pr_info("-- PROTECTION IC INT  --\n");
		printk(KERN_NOTICE "Edison protection test %d ",irq_set_irq_type(gpio_to_irq(41),IRQF_TRIGGER_FALLING));
		rc = request_irq(gpio_to_irq(41),msm_protection_set, IRQF_SHARED,"Protection IC", pdev);
		if(rc<0)
			free_irq(gpio_to_irq(41), pdev);
		//Jordan-20111230 , new function for Qualcomm ICS baseline  
      	}			
	else
	{
		pr_info("-- PROTECTION IC INT FAIL --\n");
		gpio_free(41);
	}
/*Edison add for protection IC alarm 20111122 --*/	

	printk(KERN_NOTICE "%s boot_reason = %d  msm_batt_get_chg_irq_status=%d  \n",__func__,boot_reason,msm_batt_get_chg_irq_status());
	
	//[Arima Edison] prevent phoen enter deep sleep while doing power off charge++
	if(boot_reason==0x40 || boot_reason==0x20)
	  wake_lock(&wall_charger_wake_lock);  
	//[Arima Edison] prevent phoen enter deep sleep while doing power off charge--  
	  
	rc = msm_set_power_off_charge_status((unsigned int)boot_reason);
	if(rc < 0)
		printk(KERN_EMERG "set power off charge state fail, rc = %d \n",rc);
	return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
	int rc;
	rc = msm_batt_cleanup();
	free_irq(gpio_to_irq(41), pdev);
	gpio_free(41);
	
	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
		return rc;
	}
	return 0;
}

static struct platform_driver msm_batt_driver = {
	.probe = msm_batt_probe,
	.remove = __devexit_p(msm_batt_remove),
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
};

static int __devinit msm_batt_init_rpc(void)
{
	int rc;

#ifdef CONFIG_BATTERY_MSM_FAKE
	pr_info("Faking MSM battery\n");
#else

	msm_batt_info.chg_ep =
		msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VER_4_1, 0);
	msm_batt_info.chg_api_version =  CHG_RPC_VER_4_1;
	if (msm_batt_info.chg_ep == NULL) {
		pr_err("%s: rpc connect CHG_RPC_PROG = NULL\n", __func__);
		return -ENODEV;
	}

	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_3_1, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_3_1;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_1_1, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_1_1;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_1_3, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_1_3;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_2_2, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_2_2;
	}
	if (IS_ERR(msm_batt_info.chg_ep)) {
		rc = PTR_ERR(msm_batt_info.chg_ep);
		pr_err("%s: FAIL: rpc connect for CHG_RPC_PROG. rc=%d\n",
		       __func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}

	/* Get the real 1.x version */
	if (msm_batt_info.chg_api_version == CHG_RPC_VER_1_1)
		msm_batt_info.chg_api_version =
			msm_batt_get_charger_api_version();

	/* Fall back to 1.1 for default */
	if (msm_batt_info.chg_api_version < 0)
		msm_batt_info.chg_api_version = CHG_RPC_VER_1_1;
	msm_batt_info.batt_api_version =  BATTERY_RPC_VER_4_1;

	msm_batt_info.batt_client =
		msm_rpc_register_client("battery", BATTERY_RPC_PROG,
					BATTERY_RPC_VER_4_1,
					1, msm_batt_cb_func);

	if (msm_batt_info.batt_client == NULL) {
		pr_err("%s: FAIL: rpc_register_client. batt_client=NULL\n",
		       __func__);
		return -ENODEV;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_1_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_1_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_2_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_2_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_5_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_5_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		rc = PTR_ERR(msm_batt_info.batt_client);
		pr_err("%s: ERROR: rpc_register_client: rc = %d\n ",
		       __func__, rc);
		msm_batt_info.batt_client = NULL;
		return rc;
	}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

	rc = platform_driver_register(&msm_batt_driver);

	if (rc < 0)
		pr_err("%s: FAIL: platform_driver_register. rc = %d\n",
		       __func__, rc);

	return rc;
}

static int __init msm_batt_init(void)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	rc = msm_batt_init_rpc();

	if (rc < 0) {
		pr_err("%s: FAIL: msm_batt_init_rpc.  rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	pr_info("%s: Charger/Battery = 0x%08x/0x%08x (RPC version)\n",
		__func__, msm_batt_info.chg_api_version,
		msm_batt_info.batt_api_version);

	return 0;
}

static void __exit msm_batt_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
}

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");
