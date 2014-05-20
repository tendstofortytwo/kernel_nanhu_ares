/*
 * Copyright (C) 2011-2014 Arima Communications Crop.
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

#ifndef _SENSORS_MCTL_H_
#define _SENSORS_MCTL_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define SENSOR_CALIBRATE_JOB_SIZE 4
#define SENSORS_ACCELEROMETER_HANDLE    0
#define SENSORS_MAGNETIC_FIELD_HANDLE   1
#define SENSORS_PROXIMITY_HANDLE        2
#define SENSORS_ORIENTATION_HANDLE      3

enum Sensor_Calibrate_Job{
	/* Add events for processing */
	PROXIMITY_INFORMATION_GET = 0,
	PROXIMITY_THRESHOLD_SET = 1,
	ACCELEROMETER_AXISOFFSET_SET = 2,
	ACCELEROMETER_AXISOFFSET_INIT_SET = 3,
};

typedef struct ProximityInfor{
	/* Current proximity ADC value on proximity sensor */
	int Value;
	/* Current state on proximity sensor */
	int State;
	/* Current threshold_l on proximity sensor */
	int Threshold_L;
	/* Current threshold_h on proximity sensor */
	int Threshold_H;
}ProximityInfor;

typedef struct AccelerometerAxisOffset{
	short X;
	short Y;
	short Z;
}AccelerometerAxisOffset;

typedef struct Sensor_Calibrate_Infor{
	/* Identification */
	enum Sensor_Calibrate_Job job;
	union{
		/* The information structure for setting/getting sensor */
		ProximityInfor p_infor;
		/* The Accelerometer axis offset for calibrating */
		AccelerometerAxisOffset a_axisoffset;
	};
	/* Reserved */
	char* sensor_String;
}Sensor_Calibrate_Infor;

struct device_infor{
    char			name[32];
    char			vendor[32];
    unsigned short	maxRange;
    unsigned short	resolution;
    unsigned short	power;
};

#define PROXIMITY_IOCTL_DEVICE						0X11
#define PROXIMITY_IOCTL_GET_STATE					_IOR(PROXIMITY_IOCTL_DEVICE, 1, int *)
#define PROXIMITY_IOCTL_SET_STATE					_IOW(PROXIMITY_IOCTL_DEVICE, 2, int *)
#define PROXIMITY_IOCTL_GET_DEVICE_INFOR			_IOR(PROXIMITY_IOCTL_DEVICE, 3, int *)

#define ACCELEROMETER_IOCTL_DEVICE					0X22
#define ACCELEROMETER_IOCTL_GET_STATE				_IOR(ACCELEROMETER_IOCTL_DEVICE, 1,int *)
#define ACCELEROMETER_IOCTL_SET_STATE				_IOW(ACCELEROMETER_IOCTL_DEVICE, 2, int *)
#define ACCELEROMETER_IOCTL_GET_DEVICE_INFOR		_IOR(ACCELEROMETER_IOCTL_DEVICE, 3, int *)
#define ACCELEROMETER_IOCTL_SET_DELAY				_IOW(ACCELEROMETER_IOCTL_DEVICE, 4, int *)
#define ACCELEROMETER_IOCTL_SET_AXIS_OFFSET			_IOW(ACCELEROMETER_IOCTL_DEVICE, 5, int *)
#define ACCELEROMETER_IOCTL_SET_AXIS_OFFSET_INIT	_IOW(ACCELEROMETER_IOCTL_DEVICE, 6, int *)

#endif
