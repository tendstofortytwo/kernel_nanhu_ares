/*
 * Copyright (C) 2012 Arima Communications Crop.
 * Author: Huize Weng <huizeweng@arimacomm.com.tw>
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

#ifndef _PROXIMITY_COMMOM_H_
#define _PROXIMITY_COMMOM_H_

#include <linux/types.h>
#include <linux/sensors_mctl.h>
#include <mach/oem_rapi_client.h>

#define debug 0
#define Buff_Lenght 12
#define Buff_Size (Buff_Lenght * sizeof(char))

#define DEFAULT_THRESHOLD 0XFFFF
#define ERROR_TRANSACTION -9999

#define PROXIMITY_DETECTED 1
#define PROXIMITY_UNDETECTED 5000

enum{
	PROXIMITY_STATE_FALSE		= 0,
	PROXIMITY_STATE_TRUE		= 1,
	PROXIMITY_STATE_UNKNOWN		= 0xFF,
};

typedef struct Proximity{
	struct input_dev*		input;
	struct mutex			mutex;
	struct delayed_work 	dw;
	#ifdef PROXIMITY_IRQ_USED
	int						irq;
	#endif
	ProximityInfor			sdata;
	bool					enabled;
	bool					suspend;
}Proximity;

// It needs to reset memory
static struct i2c_client* this_client = NULL;
static struct workqueue_struct* Proximity_WorkQueue = NULL;
static u8 i2cData[2];
static bool i2cIsFine = false;
static bool stateIsFine = false;
static bool need2Reset = false;
static bool need2ChangeState = false;
static u8 Proxmity_sensor_opened = 0;
static unsigned long SleepTime = 0;

static int proximity_rpc(int threshold, uint32_t event)
{
	int result = -1;
	char* input = kzalloc(Buff_Size, GFP_KERNEL);
	char* output = NULL;

	switch(event){
		case OEM_RAPI_CLIENT_EVENT_PROXIMITY_THRESHOLD_SET :
			snprintf(input, Buff_Size, "%d", threshold);
		case OEM_RAPI_CLIENT_EVENT_PROXIMITY_THRESHOLD_GET :
			break;
		default :
			kfree(input);
			kfree(output);
			return -999;
	}

	if(oem_rapi_client_rpc(event, input, &output, Buff_Size) > 0){
		result = (int) simple_strtol(output, NULL, 10);
	}

	kfree(input);
	kfree(output);

	return result;
}

static int proximity_resetThreshold(const int threshold, const int level)
{
	Proximity* data = i2c_get_clientdata(this_client);
	int result = 0;

	if(data == NULL) return -1;
	mutex_lock(&data->mutex);
	result = proximity_rpc(threshold, OEM_RAPI_CLIENT_EVENT_PROXIMITY_THRESHOLD_SET);
	data->sdata.Threshold_L = (result >= level) ? (result - (level >> 2)) : data->sdata.Threshold_L;
	data->sdata.Threshold_H = (result >= level) ? result : data->sdata.Threshold_H;
	#if debug
	printk("resetThreshold ==========> LOW %d, HIGH %d \n", data->sdata.Threshold_L, data->sdata.Threshold_H);
	#endif
	mutex_unlock(&data->mutex);
	
	return result;
}

static int proximity_readThreshold(const int level)
{
	Proximity* data = i2c_get_clientdata(this_client);
	int result = proximity_rpc(0, OEM_RAPI_CLIENT_EVENT_PROXIMITY_THRESHOLD_GET);

	if(data == NULL) return -1;
	if(result == -5){
		/**
		* Do reset. 
		* It means that proximity threshold hasn't been setted yet.
		*/
		result = proximity_resetThreshold(DEFAULT_THRESHOLD, level);
	}

	mutex_lock(&data->mutex);
	data->sdata.Threshold_L = (result >= level) ? (result - (level >> 2)) : data->sdata.Threshold_L;
	data->sdata.Threshold_H = (result >= level) ? result : data->sdata.Threshold_H;
	mutex_unlock(&data->mutex);
	return result;
}

#endif
