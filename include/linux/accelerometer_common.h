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

#ifndef _ACCELEROMETER_COMMOM_H_
#define _ACCELEROMETER_COMMOM_H_

#include <linux/types.h>
#include <linux/sensors_mctl.h>
#include <mach/oem_rapi_client.h>

#define debug 0
#define Buff_Lenght 24
#define Buff_Size (Buff_Lenght * sizeof(char))

#define FILTER_SIZE		4
#define FILTER_INDEX	(FILTER_SIZE - 1)
#define FILTER_SIZEBIT	2

typedef struct AccelerometerData{
	int X;
	int Y;
	int Z;
}AccelerometerData;

typedef struct Accelerometer{
	struct input_dev*		input;
	struct mutex			mutex;
	struct delayed_work		dw;
	#ifdef ACCELEROMETER_IRQ_USED
	int						irq;
	#endif
	AccelerometerData		sdata;
	AccelerometerAxisOffset	odata;
	bool					enabled;
	bool					suspend;
}Accelerometer;

enum{
	NORMAL_MODE		= 0X0A,
	INTERRUPT_MODE	= 0X0B,
	SPECIAL_MODE	= 0X0C,
};

static u8 WorkMode = NORMAL_MODE;
static u8 POWER_MODE_CAMMAND;

// It needs to reset memory
static int Accelerometer_sensor_opened = 0;
static struct i2c_client* this_client = NULL;
static struct workqueue_struct* Accelerometer_WorkQueue = NULL;
static unsigned long SleepTime = 0;
static u8 i2cData[6];
static u8 queueIndex = 0;
static u8 ignoreCount = 0;
static AccelerometerData rawData;
static AccelerometerData averageData;
static AccelerometerData queueData[FILTER_SIZE];

static char* accelerometer_rpc(AccelerometerAxisOffset* offset, uint32_t event)
{
	char* input = kzalloc(Buff_Size, GFP_KERNEL);
	char* output = NULL;

	switch(event){
		case OEM_RAPI_CLIENT_EVENT_ACCELEROMETER_AXIS_OFFSET_SET:
			snprintf(input, Buff_Size, "%hd %hd %hd", offset->X, offset->Y, offset->Z);
		case OEM_RAPI_CLIENT_EVENT_ACCELEROMETER_AXIS_OFFSET_GET:
			break;
		default:
			kfree(input);
			kfree(output);
			return NULL;
	}

	if(oem_rapi_client_rpc(event, input, &output, Buff_Size) < 0){
		kfree(input);
		kfree(output);
		return NULL;
	}

	#if debug
	pr_info("General: %s, AXIS %s  .. %s\n", __func__, (event == OEM_RAPI_CLIENT_EVENT_ACCELEROMETER_AXIS_OFFSET_GET) ? "GET" : "SET", output);
	#endif

	kfree(input);

	return output;
}

static bool accelerometer_resetAxisOffset(s16 x, s16 y, s16 z)
{
	bool successful = false;
	Accelerometer* data = i2c_get_clientdata(this_client);
	AccelerometerAxisOffset offset = {
		.X = x,
		.Y = y,
		.Z = z,
	};
	char* result = NULL;
	if(data == NULL) return false;
	mutex_lock(&data->mutex);
	result = accelerometer_rpc(&offset, OEM_RAPI_CLIENT_EVENT_ACCELEROMETER_AXIS_OFFSET_SET);
	memset(&offset, 0, sizeof(AccelerometerAxisOffset));
	if(	result != NULL && strcmp(result, "ERROR") != 0 &&
		sscanf(result, "%hd %hd %hd", &(offset.X), &(offset.Y), &(offset.Z)) == 3){
		memcpy(&(data->odata), &offset, sizeof(AccelerometerAxisOffset));
		#if debug
		printk("Accelerometer resetAxisOffset ===> result : %s\n", result);
		#endif
		successful = true;
	}
	kfree(result);
	result = NULL;
	mutex_unlock(&data->mutex);
	
	return successful;
}

static bool accelerometer_readAxisOffset(void)
{
	Accelerometer* data = i2c_get_clientdata(this_client);
	char* result = accelerometer_rpc(0, OEM_RAPI_CLIENT_EVENT_ACCELEROMETER_AXIS_OFFSET_GET);
	bool successful = false;

	if(data == NULL) return false;
	if(result != NULL && strcmp(result, "NV_NOTACTIVE_S") == 0){
		/**
		 * Do reset. 
		 * It means that accelerometer axis offset 
		 * hasn't been setted yet.
		 */
		kfree(result);
		return accelerometer_resetAxisOffset(0, 0, 0);
	}

	mutex_lock(&data->mutex);
	{
		AccelerometerAxisOffset offset = { .X = 0, .Y = 0, .Z = 0};
		if(result != NULL && strcmp(result, "ERROR") != 0 && 
			sscanf(result, "%hd %hd %hd", &(offset.X), &(offset.Y), &(offset.Z)) == 3){
			memcpy(&(data->odata), &(offset), sizeof(AccelerometerAxisOffset));
			#if debug
			printk("Accelerometer readAxisOffset ==========> X : %d, Y : %d, Z : %d\n", data->odata.X, data->odata.Y, data->odata.Z);
			#endif
			successful = true;
		}
		kfree(result);
		result = NULL;
	}
	mutex_unlock(&data->mutex);
	return successful;
}

#endif
