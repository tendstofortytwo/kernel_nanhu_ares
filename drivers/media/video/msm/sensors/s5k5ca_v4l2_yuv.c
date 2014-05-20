/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include "msm_sensor.h"
#include "msm.h"

#define SENSOR_NAME "s5k5ca"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k5ca"
#define s5k5ca_obj s5k5ca_##obj
#if 0
static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};
#endif
static struct msm_sensor_ctrl_t s5k5ca_s_ctrl;
int camera_id=0;
int night_mode_flag=0;
DEFINE_MUTEX(s5k5ca_mut);
#if 0
static struct msm_camera_i2c_reg_conf s5k5ca_start_settings[] = {
};

static struct msm_camera_i2c_reg_conf s5k5ca_stop_settings[] = {
};

static struct msm_camera_i2c_reg_conf s5k5ca_groupon_settings[] = {
};

static struct msm_camera_i2c_reg_conf s5k5ca_groupoff_settings[] = {
};

#endif
//++ petershih -20130225 - tmp add ++
#if 0
static struct msm_camera_i2c_reg_conf s5k5ca_pre_capture_settings[] = {
	{0xFCFC, 0xD000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	
	{0x002A, 0x0240 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x002A, 0x0220 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
};
static struct msm_camera_i2c_reg_conf s5k5ca_pre_preview_settings[] = {
	{0xFCFC, 0xD000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x002A, 0x0224 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	
	{0x002A, 0x0244 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0F12, 0x0003 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
};
#endif
//-- petershih -20130225 - tmp add --
static struct msm_camera_i2c_reg_conf s5k5ca_prev_settings[] = {

	//$MIPI[Width:1024,Height:768,Format:YUV422,Lane:1,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:2,DataRate:928] 

	////////////////////////////////////////////////////////
	{0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0}, 
	{0x002A, 0x023C , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, //REG_TC_GP_ActivePrevConfig

	{0x002A, 0x0240 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},  //REG_TC_GP_PrevOpenAfterChange
	                        
	{0x002A, 0x0230 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},  //REG_TC_GP_NewConfigSync

	{0x002A, 0x023E , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},  //REG_TC_GP_PrevConfigChanged

	{0x002A, 0x0220 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},  //REG_TC_GP_EnablePreview

	{0x0028, 0xD000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x002A, 0xB0A0 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, // Clear cont. clock befor config change

	{0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x002A, 0x0222 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, //REG_TC_GP_EnablePreviewChanged

};

static struct msm_camera_i2c_reg_conf s5k5ca_snap_settings[] = {
	//============================================================                                                 
	// Capture Configuration Update setting                                              
	//============================================================ 

	//WRITE	7000035E	00b0	//#REG_0TC_CCFG_usWidth
	//WRITE	70000360	0090	//#REG_0TC_CCFG_usHeight

	//$MIPI[Width:2048,Height:1536,Format:YUV422,Lane:1,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:2,DataRate:928] 
	{0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x002A, 0x0244 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, //REG_TC_GP_ActiveCapConfig
	                       
	{0x002A, 0x0230 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, //REG_TC_GP_NewConfigSync
	                       
	{0x002A, 0x0246 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, //REG_TC_GP_CapConfigChanged
	                       
	{0x002A, 0x0224 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, //REG_TC_GP_EnableCapture

	{0x0028, 0xD000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x002A, 0xB0A0 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, // Clear cont. clock befor config change
	                       
	{0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x002A, 0x0226 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	{0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA ,MSM_CAMERA_I2C_CMD_WRITE ,0},  //REG_TC_GP_EnableCaptureChanged
};
static struct msm_camera_i2c_reg_conf s5k5ca_snap_settings_night_mode[] =
{
       {0x0028, 0x7000 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x002A, 0x0244 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x0F12, 0x0001 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0}, //REG_TC_GP_ActiveCapConfig
	                       
	{0x002A, 0x0230 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x0F12, 0x0001 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0}, //REG_TC_GP_NewConfigSync
	                       
	{0x002A, 0x0246 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x0F12, 0x0001 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0}, //REG_TC_GP_CapConfigChanged
	                       
	{0x002A, 0x0224 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x0F12, 0x0001 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0}, //REG_TC_GP_EnableCapture

	{0x0028, 0xD000 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x002A, 0xB0A0 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x0F12, 0x0000 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0}, // Clear cont. clock befor config change
	                       
	{0x0028, 0x7000 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x002A, 0x0226 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0},
	{0x0F12, 0x0001 ,MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE,0}, //REG_TC_GP_EnableCaptureChanged
};
#if 0//Flea-use target way to implement
static struct msm_camera_i2c_reg_conf s5k5ca_effect_none[] = {
	
	 {0x0028,0x7000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x0F12,0x0000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
};
static struct msm_camera_i2c_reg_conf s5k5ca_effect_momo[] = {
	
	 {0x0028,0x7000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x0F12,0x0001, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
};
static struct msm_camera_i2c_reg_conf s5k5ca_effect_negative[] = {
        {0x0028,0x7000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x0F12,0x0000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
        {0x0F12,0x0003, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	
};
static struct msm_camera_i2c_reg_conf s5k5ca_effect_sepia[] = {
       {0x0028,0x7000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x0F12,0x0000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x0F12,0x0004, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	 
};
static struct msm_camera_i2c_reg_conf s5k5ca_effect_aqua[] = {
       {0x0028,0x7000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x0F12,0x0000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
       {0x0F12,0x0005, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	 
};
static struct msm_camera_i2c_reg_conf s5k5ca_effect_sketch[] = {
      {0x0028,0x7000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
      {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
      {0x0F12,0x0000, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
      {0x002a,0x021E, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
      {0x0F12,0x0006, MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
	 
};
#endif
static struct msm_camera_i2c_reg_conf s5k5ca_fps_15[] = {
	
          {0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x002A, 0x023C , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x002A, 0x0240 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
                              
          {0x002A, 0x0230 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          
          {0x002A, 0x023E , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x002A, 0x0220 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x0028, 0xD000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x002A, 0xB0A0 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          
          {0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
          {0x002A, 0x0222 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0}, 
          {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
};
static struct msm_camera_i2c_reg_conf s5k5ca_fps_30[] = {
	
         {0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x002A, 0x023C , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         
         {0x002A, 0x0240 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
                         
         {0x002A, 0x0230 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         
         {0x002A, 0x023E , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x002A, 0x0220 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0028, 0xD000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x002A, 0xB0A0 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0F12, 0x0000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         
         {0x0028, 0x7000 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x002A, 0x0222 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
         {0x0F12, 0x0001 , MSM_CAMERA_I2C_WORD_DATA, MSM_CAMERA_I2C_CMD_WRITE, 0},
};
static struct msm_camera_i2c_reg_conf s5k5ca_special_effect[][5] = {
	{{0x0028,0x7000}, {0x002a,0x021E}, {0x0F12,0x0000}, {0x002a,0x021E}, {0x0F12,0x0000}},	// off
	{{0x0028,0x7000}, {0x002a,0x021E}, {0x0F12,0x0000}, {0x002a,0x021E}, {0x0F12,0x0001}},	// mono
	{{0x0028,0x7000}, {0x002a,0x021E}, {0x0F12,0x0000}, {0x002a,0x021E}, {0x0F12,0x0003}},	//negative
	{{0x0028,0x7000}, {0x002a,0x021E}, {0x0F12,0x0000}, {0x002a,0x021E}, {0x0F12,0x0004}},	//sepia
	{{0x0028,0x7000}, {0x002a,0x021E}, {0x0F12,0x0000}, {0x002a,0x021E}, {0x0F12,0x0005}},	//aqua
	{{0x0028,0x7000}, {0x002a,0x021E}, {0x0F12,0x0000}, {0x002a,0x021E}, {0x0F12,0x0006}},	//sketch
};
static struct msm_camera_i2c_conf_array s5k5ca_special_effect_confs[][1] = {
	{{s5k5ca_special_effect[0],  ARRAY_SIZE(s5k5ca_special_effect[0]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_special_effect[1],  ARRAY_SIZE(s5k5ca_special_effect[1]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_special_effect[2],  ARRAY_SIZE(s5k5ca_special_effect[2]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{0},},
	{{s5k5ca_special_effect[3],  ARRAY_SIZE(s5k5ca_special_effect[3]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{0},},
	{{0},},
	{{0},},
	{{s5k5ca_special_effect[4],  ARRAY_SIZE(s5k5ca_special_effect[4]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{0},},
	{{s5k5ca_special_effect[5],  ARRAY_SIZE(s5k5ca_special_effect[5]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{0},},
};
static int s5k5ca_special_effect_enum_map[] = {
	MSM_V4L2_EFFECT_OFF,
	MSM_V4L2_EFFECT_MONO,
	MSM_V4L2_EFFECT_NEGATIVE,
	MSM_V4L2_EFFECT_SOLARIZE,
	MSM_V4L2_EFFECT_SEPIA,
	MSM_V4L2_EFFECT_POSTERAIZE,
	MSM_V4L2_EFFECT_WHITEBOARD,
	MSM_V4L2_EFFECT_BLACKBOARD,
	MSM_V4L2_EFFECT_AQUA,
	MSM_V4L2_EFFECT_EMBOSS,
	MSM_V4L2_EFFECT_SKETCH,
	MSM_V4L2_EFFECT_NEON,
	MSM_V4L2_EFFECT_MAX,
};
static struct msm_camera_i2c_enum_conf_array
		 s5k5ca_special_effect_enum_confs = {
	.conf = &s5k5ca_special_effect_confs[0][0],
	.conf_enum = s5k5ca_special_effect_enum_map,
	.num_enum = ARRAY_SIZE(s5k5ca_special_effect_enum_map),
	.num_index = ARRAY_SIZE(s5k5ca_special_effect_confs),
	.num_conf = ARRAY_SIZE(s5k5ca_special_effect_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};
static struct msm_camera_i2c_reg_conf s5k5ca_wb_oem[][10] = {
	{{0},},/*WHITEBALNACE OFF*/

	{{0x0028, 0x7000}, {0x002a, 0x04D2},  {0x0f12, 0x067F},}, /*WHITEBALNACE AUTO*/

	{{0},},	/*WHITEBALNACE CUSTOM*/

	{{0x0028,0x7000}, {0x002A,0x04D2}, {0x0F12,0x0677}, {0x002A,0x04A0}, {0x0f12,0x0400}, 
	{0x0f12,0x0001}, {0x0f12,0x0400}, {0x0f12,0x0001},  {0x0f12,0x0940}, {0x0f12,0x0001},},	/*INCANDISCENT*/

	{{0x0028,0x7000}, {0x002A,0x04D2}, {0x0F12,0x0677},  {0x002A,0x04A0}, {0x0f12,0x0575}, 
	{0x0f12,0x0001}, {0x0f12,0x0400}, {0x0f12,0x0001}, {0x0f12,0x0800}, {0x0f12,0x0001},},	/*FLOURESECT NOT SUPPORTED */

	{{0x0028,0x7000}, {0x002A,0x04D2}, {0x0F12,0x0677}, {0x002A,0x04A0}, {0x0F12,0x05A0},
	{0x0F12,0x0001}, {0x0F12,0x0400}, {0x0F12,0x0001}, {0x0F12,0x05D0}, {0x0F12,0x0001},},	/*DAYLIGHT*/

	{{0x0028,0x7000}, {0x002A,0x04D2}, {0x0F12,0x0677}, {0x002A,0x04A0}, {0x0f12,0x0740},  
	{0x0f12,0x0001}, {0x0f12,0x0400}, {0x0f12,0x0001}, {0x0f12,0x0580}, {0x0f12,0x0001},},	/*CLOUDY*/
};
static struct msm_camera_i2c_conf_array s5k5ca_wb_oem_confs[][1] = {
	{{s5k5ca_wb_oem[1], 3,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_wb_oem[1], 3,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_wb_oem[2], 0,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_wb_oem[3], 10,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_wb_oem[4], 10,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_wb_oem[5], 10,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_wb_oem[6], 10,  0,	MSM_CAMERA_I2C_WORD_DATA},},
};
static int s5k5ca_wb_oem_enum_map[] = {
	MSM_V4L2_WB_OFF,
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_CUSTOM,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
};
static struct msm_camera_i2c_enum_conf_array s5k5ca_wb_oem_enum_confs = {
	.conf = &s5k5ca_wb_oem_confs[0][0],
	.conf_enum = s5k5ca_wb_oem_enum_map,
	.num_enum = ARRAY_SIZE(s5k5ca_wb_oem_enum_map),
	.num_index = ARRAY_SIZE(s5k5ca_wb_oem_confs),
	.num_conf = ARRAY_SIZE(s5k5ca_wb_oem_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};
static struct msm_camera_i2c_reg_conf s5k5ca_iso[][5] = {
	{{0x0028 ,0x7000}, {0x002A ,0x04B4}, {0x0F12 ,0x0000}, {0x0F12 ,0x0000}, {0x0F12 ,0x0001},},	// off
	{{0}},	// deblur
	{{0x0028 ,0x7000}, {0x002A ,0x04B4}, {0x0F12 ,0x0001}, {0x0F12 ,0x0064}, {0x0F12 ,0x0001},}, // 100
	{{0x0028 ,0x7000}, {0x002A ,0x04B4}, {0x0F12 ,0x0001}, {0x0F12 ,0x00C8}, {0x0F12 ,0x0001},}, // 200
	{{0x0028 ,0x7000}, {0x002A ,0x04B4}, {0x0F12 ,0x0001}, {0x0F12 ,0x0190}, {0x0F12 ,0x0001},}, // 400
	{{0}},	// 800
	{{0}},	// 1600
};
static struct msm_camera_i2c_conf_array s5k5ca_iso_confs[][1] = {
	{{s5k5ca_iso[0],  ARRAY_SIZE(s5k5ca_iso[0]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_iso[1],  0,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_iso[2],  ARRAY_SIZE(s5k5ca_iso[2]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_iso[3],  ARRAY_SIZE(s5k5ca_iso[3]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_iso[4],  ARRAY_SIZE(s5k5ca_iso[4]),  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_iso[5],  0,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_iso[6],  0,  0,	MSM_CAMERA_I2C_WORD_DATA},},
};
static int s5k5ca_iso_enum_map[] = {
	MSM_V4L2_ISO_AUTO ,
	MSM_V4L2_ISO_DEBLUR,
	MSM_V4L2_ISO_100,
	MSM_V4L2_ISO_200,
	MSM_V4L2_ISO_400,
	MSM_V4L2_ISO_800,
	MSM_V4L2_ISO_1600,
};
static struct msm_camera_i2c_enum_conf_array
		 s5k5ca_iso_enum_confs = {
	.conf = &s5k5ca_iso_confs[0][0],
	.conf_enum = s5k5ca_iso_enum_map,
	.num_enum = ARRAY_SIZE(s5k5ca_iso_enum_map),
	.num_index = ARRAY_SIZE(s5k5ca_iso_confs),
	.num_conf = ARRAY_SIZE(s5k5ca_iso_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};
static struct msm_camera_i2c_reg_conf s5k5ca_best_shot[][70] = {
{// ----------------Normal
{ 0x0028, 0x7000},{ 0x002a, 0x246E},{ 0x0f12, 0x0001},{ 0x002A, 0x0842},{ 0x0F12, 0x7850},
{ 0x0F12, 0x2878},{ 0x002A, 0x0912},{ 0x0F12, 0x643C},{ 0x0F12, 0x2864},{ 0x002A, 0x09E2},
{ 0x0F12, 0x3C2A},{ 0x0F12, 0x283C},{ 0x002A, 0x0AB2},{ 0x0F12, 0x2228},{ 0x0F12, 0x2822},
{ 0x002A, 0x0B82},{ 0x0F12, 0x191C},{ 0x0F12, 0x2819},{ 0x0028, 0x7000},{ 0x002A, 0x020C},
{ 0x0F12, 0x0000},{ 0x002A, 0x0210},{ 0x0F12, 0x0000},{ 0x0F12, 0x0000},{ 0x002A, 0x0838},
{ 0x0F12, 0x083C},{ 0x002A, 0x0530},{ 0x0F12, 0x3A98},{ 0x002A, 0x0534},{ 0x0F12, 0x7EF4},
{ 0x002A, 0x167C},{ 0x0F12, 0x9C40},{ 0x002A, 0x1680},{ 0x0F12, 0xF424},{ 0x0F12, 0x0000},
{ 0x002A, 0x0538},{ 0x0F12, 0x3A98},{ 0x0F12, 0x0000},{ 0x0F12, 0x7EF4},{ 0x0F12, 0x0000},
{ 0x002A, 0x1684},{ 0x0F12, 0x9C40},{ 0x0F12, 0x0000},{ 0x0F12, 0xF424},{ 0x0F12, 0x0000},
{ 0x002A, 0x0540},{ 0x0F12, 0x0170},{ 0x0F12, 0x0250},{ 0x002A, 0x168C},{ 0x0F12, 0x0380},
{ 0x0F12, 0x0800},{ 0x002A, 0x0544},{ 0x0F12, 0x0100},{ 0x002A, 0x0208},{ 0x0F12, 0x0001},
{ 0x002A, 0x023C},{ 0x0F12, 0x0000},{ 0x002A, 0x0240},{ 0x0F12, 0x0001},{ 0x002A, 0x0230},
{ 0x0F12, 0x0001},{ 0x002A, 0x023E},{ 0x0F12, 0x0001},{ 0x002A, 0x0220},{ 0x0F12, 0x0001},
{ 0x0028, 0xD000},{ 0x002A, 0xB0A0},{ 0x0F12, 0x0000},{ 0x002A, 0x0222},{ 0x0F12, 0x0001},
},
//------------------auto
//{{0},},

{//-------------------Landscape
{ 0x0028, 0x7000},{ 0x002A, 0x020C},{ 0x0F12, 0x0000},{ 0x002A, 0x0210},{ 0x0F12, 0x001E},
{ 0x002A, 0x0842},{ 0x0F12, 0x6444},{ 0x0F12, 0x465A},{ 0x002A, 0x0912},{ 0x0F12, 0x4B3A},
{ 0x0F12, 0x463F},{ 0x002A, 0x09E2},{ 0x0F12, 0x1A2D},{ 0x0F12, 0x4628},{ 0x002A, 0x0AB2},
{ 0x0F12, 0x1328},{ 0x0F12, 0x3213},{ 0x002A, 0x0B82},{ 0x0F12, 0x0819},{ 0x0F12, 0x3204},
},
//-------------------snow
//{{0},},
//-------------------beach
//{{0},},
{//---------------------Sunset 
{ 0x0028, 0x7000},{ 0x002a, 0x246E},{ 0x0f12, 0x0000},{ 0x002a, 0x04A0},{ 0x0f12, 0x05E0},
{ 0x0f12, 0x0001},{ 0x0f12, 0x0400},{ 0x0f12, 0x0001},{ 0x0f12, 0x0520},{ 0x0f12, 0x0001},
},
{//----------------------Night

{0x0028, 0x7000},{0x002A, 0x0530},{0x0F12, 0x3415},{0x002A, 0x0534},{0x0F12, 0x682A},
{0x002A, 0x167C},{0x0F12, 0x8235},{0x002A, 0x1680},{0x0F12, 0x1A80},{0x0F12, 0x0006},
{0x002A, 0x0538},{0x0F12, 0x3415},{0x002A, 0x053C},{0x0F12, 0x682A},{0x002A, 0x1684},
{0x0F12, 0x8235},{0x002A, 0x1688},{0x0F12, 0x1A80},{0x0F12, 0x0006},{0x002A, 0x0540},
{0x0F12, 0x0180},{0x0F12, 0x0250},{0x002A, 0x168C},{0x0F12, 0x0340},{0x0F12, 0x0820},
{0x002A, 0x0544},{0x0F12, 0x0100},{0x0028, 0x7000},{0x002A, 0x023C},{0x0F12, 0x0002}, 
{0x002A, 0x0240},{0x0F12, 0x0001},{0x002A, 0x0230},{0x0F12, 0x0001},{0x002A, 0x023E}, 
{0x0F12, 0x0001},{0x002A, 0x0220},{0x0F12, 0x0001},{0x0028, 0xD000},{0x002A, 0xB0A0}, 
{0x0F12, 0x0000},{0x0028, 0x7000},{0x002A, 0x0222},{0x0F12, 0x0001}, 
},
{//----------------------Portrait
{0x0028, 0x7000},{0x002A, 0x020C},{0x0F12, 0x0000},{0x002A, 0x0210},{0x0F12, 0x0000},
{0x0F12, 0xFFCC},
},
//-------------------backlight
//{{0},},
{//----------------------Sports
{0x0028, 0x7000},{0x002A, 0x0530},{0x0F12, 0x1A0A},{0x002A, 0x0534},{0x0F12, 0x4e20},
{0x002A, 0x167C},{0x0F12, 0x682A},{0x002A, 0x1680},{0x0F12, 0x8235},{0x0F12, 0x0000},
{0x002A, 0x0538},{0x0F12, 0x1A0A},{0x002A, 0x053C},{0x0F12, 0x4e20},{0x002A, 0x1684},
{0x0F12, 0x682A},{0x002A, 0x1688},{0x0F12, 0x8235},{0x0F12, 0x0000},{0x002A, 0x0540},
{0x0F12, 0x0300},{0x0F12, 0x0380},{0x002A, 0x168C},{0x0F12, 0x0480},{0x0F12, 0x0800},        
{0x002A, 0x0544},{0x0F12, 0x0100},{0x002A, 0x023C},{0x0F12, 0x0000},{0x002A, 0x0240},
{0x0F12, 0x0001},{0x002A, 0x0230},{0x0F12, 0x0001},{0x002A, 0x023E},{0x0F12, 0x0001},
{0x002A, 0x0220},{0x0F12, 0x0001},{0x0028, 0xD000},{0x002A, 0xB0A0},{0x0F12, 0x0000},  
{0x002A, 0x0222},{0x0F12, 0x0001},
},
//------------------antishake
//{{0},},

{//-------------------Text->flower
{ 0x0028, 0x7000},{ 0x002A, 0x0842},{ 0x0F12, 0x5C44},{ 0x0F12, 0x4656},{ 0x002A, 0x0912},
{ 0x0F12, 0x433A},{ 0x0F12, 0x463B},{ 0x002A, 0x09E2},{ 0x0F12, 0x122D},{ 0x0F12, 0x4624},
{ 0x002A, 0x0AB2},{ 0x0F12, 0x0B28},{ 0x0F12, 0x320F},{ 0x002A, 0x0B82},{ 0x0F12, 0x0010},
{ 0x0F12, 0x3200},
},
//---------------candlelight
//{{0},},
//---------------firework
//{{0},},
//---------------party
//{{0},},
//---------------night_portrait
//{{0},},
//---------------theatre
//{{0},},
//----------------action
//{{0},},
//-----------ar
//{{0},},
//--------------max
//{{0},},

};
static struct msm_camera_i2c_conf_array s5k5ca_best_shot_confs[][1] = {
	{{s5k5ca_best_shot[0],  70,  0,	MSM_CAMERA_I2C_WORD_DATA},},// ----------------Normal
       //{{s5k5ca_best_shot[0],    0,  0,	MSM_CAMERA_I2C_WORD_DATA},},		
	{{s5k5ca_best_shot[1],  20,  0,	MSM_CAMERA_I2C_WORD_DATA},},//-------------------Landscape
	//{{s5k5ca_best_shot[0],    0,  0,	MSM_CAMERA_I2C_WORD_DATA},},	
	//{{s5k5ca_best_shot[0],    0,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_best_shot[2],  10,  0,	MSM_CAMERA_I2C_WORD_DATA},},//---------------------Sunset 
	{{s5k5ca_best_shot[3],  44,  0,	MSM_CAMERA_I2C_WORD_DATA},},//----------------------Night
	{{s5k5ca_best_shot[4],   6,   0,	MSM_CAMERA_I2C_WORD_DATA},},//----------------------Portrait
	//{{s5k5ca_best_shot[0],    0,  0,	MSM_CAMERA_I2C_WORD_DATA},},
	{{s5k5ca_best_shot[5],  42,  0,	MSM_CAMERA_I2C_WORD_DATA},},//----------------------Sports
	{{s5k5ca_best_shot[6],  16,  0,	MSM_CAMERA_I2C_WORD_DATA},},//-------------------Text->flower
};
static int s5k5ca_best_shot_enum_map[] = {
             msm_v4l2_best_shot_normal,
             msm_v4l2_best_shot_auto ,
             msm_v4l2_best_shot_landscape ,
             msm_v4l2_best_shot_snow,
             msm_v4l2_best_shot_beach,
             msm_v4l2_best_shot_sunset,
             msm_v4l2_best_shot_night,
             msm_v4l2_best_shot_portrait,
             msm_v4l2_best_shot_backlight,
             msm_v4l2_best_shot_sports,
             msm_v4l2_best_shot_antishake,
             msm_v4l2_best_shot_flowers,
             msm_v4l2_best_shot_candlelight,
             msm_v4l2_best_shot_fireworks,
             msm_v4l2_best_shot_party,
             msm_v4l2_best_shot_night_portrait,
             msm_v4l2_best_shot_theatre,
             msm_v4l2_best_shot_action,
             msm_v4l2_best_shot_ar,
             msm_v4l2_best_shot_max,   
};
static struct msm_camera_i2c_enum_conf_array s5k5ca_best_shot_enum_confs = {
	.conf = &s5k5ca_best_shot_confs[0][0],
	.conf_enum = s5k5ca_best_shot_enum_map,
	.num_enum = ARRAY_SIZE(s5k5ca_best_shot_enum_map),
	.num_index = ARRAY_SIZE(s5k5ca_best_shot_confs),
	.num_conf = ARRAY_SIZE(s5k5ca_best_shot_confs[0]),
	.data_type = MSM_CAMERA_I2C_WORD_DATA,
};
#if 1
static struct msm_camera_i2c_reg_conf s5k5ca_preinit_settings[] = {
// ARM GO
// Direct mode
{0xFCFC ,0xD000 },
{0x0010 ,0x0001  },//Reset},
{0x1030 ,0x0000  },//Clear host interrupt so main will wait},
{0x0014 ,0x0001  },//ARM go},
//p10 //Min.10ms delay is required
};
#endif
static struct msm_camera_i2c_reg_conf s5k5ca_initial_settings[] = {
#if CONFIG_S5K5CA_V4L2_YUV
//#include "s5k5ca_yuv_mcnex_init.h"
//#include "s5k5ca_yuv_mcnex_init_version2_110.h"
#include "s5k5ca_yuv_mcnex_init_20121211_AE_0048_AFIT_iDark+detect+outdoor_boundary.h"
#endif
};
static struct msm_camera_i2c_reg_conf s5k5ca_initial_settings_1[] = {
#if CONFIG_S5K5CA_V4L2_YUV
#include "s5k5ca_yuv_1101_C_more_R_Manual_flicker_60Hz_mclk24_0_1126.h"
#endif
};


static struct msm_camera_i2c_conf_array s5k5ca_init_conf[] = {
	{&s5k5ca_preinit_settings[0],ARRAY_SIZE(s5k5ca_preinit_settings), 10, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_initial_settings[0],ARRAY_SIZE(s5k5ca_initial_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array s5k5ca_init_conf_1[] = {
	{&s5k5ca_preinit_settings[0],ARRAY_SIZE(s5k5ca_preinit_settings), 10, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_initial_settings_1[0],ARRAY_SIZE(s5k5ca_initial_settings_1), 0, MSM_CAMERA_I2C_WORD_DATA},
};
static struct msm_camera_i2c_conf_array s5k5ca_confs[] = {
	{&s5k5ca_snap_settings[0],ARRAY_SIZE(s5k5ca_snap_settings), 300, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_prev_settings[0],ARRAY_SIZE(s5k5ca_prev_settings), 100, MSM_CAMERA_I2C_WORD_DATA},
};
//Night mode setting in capture
static struct msm_camera_i2c_conf_array s5k5ca_confs_night_mode[] = {
{&s5k5ca_snap_settings_night_mode[0],ARRAY_SIZE(s5k5ca_snap_settings_night_mode), 500, MSM_CAMERA_I2C_WORD_DATA},

};
static struct msm_camera_csi_params s5k5ca_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 10,
};

static struct v4l2_subdev_info s5k5ca_subdev_info[] = {
	{
        //#if CONFIG_S5K5CA_V4L2_BAR 	
            // .code   = V4L2_MBUS_FMT_SGRBG10_1X10,
        //#endif
        #if CONFIG_S5K5CA_V4L2_YUV
             .code=V4L2_MBUS_FMT_YUYV8_2X8,
        #endif
		
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_sensor_output_info_t s5k5ca_dimensions[] = {
	{ /* For SNAPSHOT */
		.x_output = 2048,    /*for 3Mp*/
		.y_output = 1536,  
		.line_length_pclk = 2048,
		.frame_length_lines = 1536,
		.vt_pixel_clk = 47185920,//94371840,
		.op_pixel_clk =47185920,// 94371840,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW */
		.x_output = 640, /*1024*/
		.y_output = 480, /*768*/
		.line_length_pclk = 0x8FC,
		.frame_length_lines = 0x34B,
		.vt_pixel_clk = 58167000,//48000000,//9216000,//58167000,//23592960,
		.op_pixel_clk = 48000000,//9216000,//58167000,//23592960,
		.binning_factor = 0x1,
	},

};
#if 0
static struct msm_sensor_output_reg_addr_t s5k5ca_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380A,
	.line_length_pclk = 0x380C,
	.frame_length_lines = 0x380E,
};
#endif
static struct msm_camera_csi_params *s5k5ca_csi_params_array[] = {
	&s5k5ca_csi_params, /* Snapshot */
	&s5k5ca_csi_params, /* Preview */
};

static struct msm_sensor_id_info_t s5k5ca_id_info = {
	.sensor_id_reg_addr = 0x0F12,
	.sensor_id = 0x05ca,
};

#if 0
static struct msm_sensor_exp_gain_info_t s5k5ca_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,
	.global_gain_addr = 0x350A,
	.vert_offset = 4,
};
#endif

int s5k5ca_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	printk(" s5k5ca_msm_sensor_s_ctrl_by_enum() ctrl_id:%x value:%d\n",ctrl_info->ctrl_id,value);

	switch(ctrl_info->ctrl_id){
		case V4L2_CID_SPECIAL_EFFECT:
		{
			switch(value){
				case MSM_V4L2_EFFECT_SOLARIZE:
				case MSM_V4L2_EFFECT_POSTERAIZE:
				case MSM_V4L2_EFFECT_WHITEBOARD:
				case MSM_V4L2_EFFECT_BLACKBOARD:
				case MSM_V4L2_EFFECT_EMBOSS:
				case MSM_V4L2_EFFECT_NEON:
					return -EINVAL;
			}
		}
		break;
		case MSM_V4L2_PID_ISO:
		{
		}
		break;
		case V4L2_CID_POWER_LINE_FREQUENCY:
		{
		}
		break;
		case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		{
			if(value == MSM_V4L2_WB_CUSTOM){
				return -EINVAL;
			}
		}
		break;
		case MSM_V4L2_PID_BEST_SHOT:
		{
			night_mode_flag=value;
			switch(value){
 	                     case msm_v4l2_best_shot_normal:
				value=0;
				break;
			
				case msm_v4l2_best_shot_landscape:
				value=1;
				break;	
				case msm_v4l2_best_shot_sunset:
				value=2;
				break;	
				case msm_v4l2_best_shot_night:
				value=3;
				break;	
				case msm_v4l2_best_shot_portrait:
				value=4;
				break;	
				case msm_v4l2_best_shot_sports:
				value=5;
				break;	
                            case msm_v4l2_best_shot_flowers:
				value=6;
				break;				
            
					

			}
			}
		default:
			break;
	}

	rc = msm_sensor_write_enum_conf_array(s_ctrl->sensor_i2c_client, ctrl_info->enum_cfg_settings, value);

	return rc;
}
int s5k5ca_set_fps(struct msm_sensor_ctrl_t *s_ctrl,	struct fps_cfg *fps)
{
	//printk(" s5k5ca_set_fps() fps_divider:%d fps_div:%d \n",s_ctrl->fps_divider,fps->fps_div);
	s_ctrl->fps_divider = fps->fps_div;
      if(fps->fps_div<7680)
     {
	 	
   	msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_fps_15,
			ARRAY_SIZE(s5k5ca_fps_15),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);   
      }
      else
      	{
       msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_fps_30,
			ARRAY_SIZE(s5k5ca_fps_30),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);
      	}
	return 0;

}
static int s5k5ca_set_exposure(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int32_t rc = -EINVAL;
	//printk(" s5k5ca_set_exposure() value:%x\n",value);
	value >>= 16;
	if((value>=-12) && (value <=12))
	{
		rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0028, 0x7000, MSM_CAMERA_I2C_WORD_DATA);
		rc|=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x002a, 0x020C, MSM_CAMERA_I2C_WORD_DATA);
		rc|=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0F12, (value*10), MSM_CAMERA_I2C_WORD_DATA);
	}
		
	return rc;

}
struct msm_sensor_v4l2_ctrl_info_t s5k5ca_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_SPECIAL_EFFECT,
		.min = MSM_V4L2_EFFECT_OFF,
		.max = MSM_V4L2_EFFECT_NEGATIVE,
		.step = 1,
		.enum_cfg_settings = &s5k5ca_special_effect_enum_confs,
		.s_v4l2_ctrl = s5k5ca_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = MSM_V4L2_PID_ISO,
		.min = MSM_V4L2_ISO_AUTO,
		.max = MSM_V4L2_ISO_1600,
		.step = 1,
		.enum_cfg_settings = &s5k5ca_iso_enum_confs,
		.s_v4l2_ctrl = s5k5ca_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.min = MSM_V4L2_WB_OFF,
		.max = MSM_V4L2_WB_CLOUDY_DAYLIGHT,
		.step = 1,
		.enum_cfg_settings = &s5k5ca_wb_oem_enum_confs,
		.s_v4l2_ctrl = s5k5ca_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_EXPOSURE,
		.min = -12,
		.max = 12,
		.step = 1,
		.enum_cfg_settings = NULL,
		.s_v4l2_ctrl = s5k5ca_set_exposure,
	},
	{
		.ctrl_id = MSM_V4L2_PID_BEST_SHOT,
		.min =msm_v4l2_best_shot_normal ,
		.max =msm_v4l2_best_shot_max ,
		.step = 1,
		.enum_cfg_settings = &s5k5ca_best_shot_enum_confs,
		.s_v4l2_ctrl = s5k5ca_msm_sensor_s_ctrl_by_enum,
	},
#if 0
	{
		.ctrl_id = V4L2_CID_POWER_LINE_FREQUENCY,
		.min = MSM_V4L2_POWER_LINE_60HZ,
		.max = MSM_V4L2_POWER_LINE_AUTO,
		.step = 1,
		.enum_cfg_settings = &s5k5ca_antibanding_enum_confs,
		.s_v4l2_ctrl = s5k5ca_antibanding_msm_sensor_s_ctrl_by_enum,
	},
#endif
};

void s5k5ca_sensor_reset_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	//printk(" s5k5ca_sensor_reset_stream()\n");
};

static int32_t s5k5ca_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{


	//printk(" s5k5ca_write_pict_exp_gain() gain:%d line:%d\n",gain,line);
	return 0;

};


static int32_t s5k5ca_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
						uint16_t gain, uint32_t line)
{

	//printk(" s5k5ca_write_prev_exp_gain() gain:%d line:%d\n",gain,line);
	return 0;
};
#if 0//Flea++
static int s5k5ca_sensor_config_brithness(struct msm_sensor_ctrl_t *s_ctrl,  void __user *argp)
{
       int value;
	
    if(copy_from_user(&value,(void *)argp,sizeof(uint32_t)))
	{
		return -EFAULT;
	}
	//printk(" s5k5ca_sensor_config_brithness ()\n");
	return 0;
};

static int32_t s5k5ca_sensor_set_effect(struct msm_sensor_ctrl_t *s_ctrl,  uint32_t effect)
{
//None=0;mono=1;sepia=4;negative=2;solarize=3;posterize=5;aqua=8;emboss=9;sketch=10;neon=11
#if 0
       struct sensor_cfg_data cdata;
	
   if (copy_from_user(&cdata,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;
  #endif 
       switch(effect)
	   	{
	 case 0:	
   	msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_effect_none,
			ARRAY_SIZE(s5k5ca_effect_none),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);
   break;
   
   case 1:	
   msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_effect_momo,
			ARRAY_SIZE(s5k5ca_effect_momo),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);
   break;
   
   case 2:	
   msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_effect_negative,
			ARRAY_SIZE(s5k5ca_effect_negative),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);
   break;
   
   case 4:	
  msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_effect_sepia,
			ARRAY_SIZE(s5k5ca_effect_sepia),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);
   break;
   
   case 8:	
   msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_effect_aqua,
			ARRAY_SIZE(s5k5ca_effect_aqua),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);
   break;
   
   case 10:	
   msm_camera_i2c_write_tbl(
			s5k5ca_s_ctrl.sensor_i2c_client,
			s5k5ca_effect_sketch,
			ARRAY_SIZE(s5k5ca_effect_sketch),
			s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);
   break;
   default:
   	printk(" there is no this setting\n");
       	}
	printk(" s5k5ca_sensor_set_effect (%d)\n",effect);
	return 0;
};

#endif//Flea--
static const struct i2c_device_id s5k5ca_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k5ca_s_ctrl},
	{ }
};
int32_t s5k5ca_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	//printk(" s5k5ca_sensor_i2c_probe()\n");
	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		printk("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	s_ctrl = client->dev.platform_data;

	return rc;
};

static struct i2c_driver s5k5ca_i2c_driver = {
	.id_table = s5k5ca_i2c_id,
	.probe  = s5k5ca_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};



static struct msm_camera_i2c_client s5k5ca_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	//printk(" msm_sensor_init_module()\n");
	return i2c_add_driver(&s5k5ca_i2c_driver);
};

static struct v4l2_subdev_core_ops s5k5ca_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k5ca_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k5ca_subdev_ops = {
	.core = &s5k5ca_subdev_core_ops,
	.video  = &s5k5ca_subdev_video_ops,
};

int32_t s5k5ca_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *info = NULL;

	//printk(" s5k5ca_sensor_power_down()\n");
	info = s_ctrl->sensordata;
	gpio_direction_output(info->sensor_pwd, 0);
	gpio_direction_output(info->sensor_reset, 0);
	usleep_range(5000, 5100);
	msm_sensor_power_down(s_ctrl);
	gpio_free(info->sensor_pwd);
	gpio_free(info->sensor_reset);
	gpio_free(10);
	gpio_free(14);
	return 0;
}
static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t s5k5ca_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;

	//printk(" s5k5ca_sensor_power_up()\n");
	info = s_ctrl->sensordata;

	/* turn off PWDN & RST */
	rc = gpio_request(info->sensor_pwd, "S5K5CA_PWDN");
	if (rc < 0)
		pr_err("%s: gpio_request--- failed!",	__func__);
	rc = gpio_request(info->sensor_reset, "S5K5CA_RST");
	if (rc < 0)
		pr_err("%s: gpio_request--- failed!",	__func__);
	gpio_direction_output(info->sensor_pwd, 0);
	gpio_direction_output(info->sensor_reset, 0);

	/*++ PeterShih - 20121004 Turn on ldo and vreg ++*/
	rc = gpio_request( 14 , "S5K5CA_1.8V");
	if (rc < 0)
		pr_err("%s: gpio_request--- failed!",	__func__);
	rc = gpio_request( 10 , "S5K5CA_2.8V");
	if (rc < 0)
		pr_err("%s: gpio_request--- failed!",	__func__);
	gpio_direction_output(14, 1);
	gpio_direction_output(10, 1);
	/*-- PeterShih - 20121004 Turn on ldo and vreg --*/
	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
//		goto enable_clk_failed;
	}
#if 0
	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		CDBG("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}	
#endif
	/* turn on PWDN & RST */
	gpio_direction_output(info->sensor_pwd, 1);
	gpio_direction_output(info->sensor_reset, 1);

	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xFCFC, 0xD000, MSM_CAMERA_I2C_WORD_DATA);
	rc|=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x002C, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	rc|=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x002E, 0x0040, MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0)
		pr_err("%s: rc:%d iic failed!",	__func__,rc);
	//detect sensor//
	camera_id=gpio_get_value(13);
	//Flea++
	s_ctrl->sensordata->sensor_platform_info->hw_version=10|camera_id;
	//Flea--
	
	//pr_emerg(" s5k5ca_sensor_power_up() rc:%d camera_id:%d",rc,camera_id);
	 
	//
	return rc;

};

static int32_t vfe_clk = 266667000;

int32_t s5k5ca_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

	printk("\n s5k5ca_sensor_setting() update_type:%d res:%d night_mode_flag:%d\n",update_type,res,night_mode_flag);

	if (update_type == MSM_SENSOR_REG_INIT) {
		//printk("PERIODIC : %d\n", res);
		s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
		//printk("CSI config in progress\n");
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSIC_CFG,
			s_ctrl->curr_csic_params);
		//printk("CSI config is done\n");
		mb();

		//printk("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		rc=msm_sensor_enable_debugfs(s_ctrl);
//		if( rc!=0 )
//			printk("s5k5ca_sensor_setting()rc:%d",rc);
		rc=msm_sensor_write_init_settings(s_ctrl);
		if( rc!=0 )
			printk("s5k5ca_sensor_setting()rc:%d",rc);
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		//CDBG("PERIODIC : %d\n", res);
//++ petershih -20130225 - tmp add ++
#if 0
		if(res == 0)
		{
		   	rc=msm_camera_i2c_write_tbl(
				s5k5ca_s_ctrl.sensor_i2c_client,
				s5k5ca_pre_capture_settings,
				ARRAY_SIZE(s5k5ca_pre_capture_settings),
				s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);   
			if( rc!=0 )
				printk("s5k5ca_sensor_setting()s5k5ca_pre_capture_settings rc:%d",rc);
			msleep(140);
		}
		else
		{
		   	rc=msm_camera_i2c_write_tbl(
				s5k5ca_s_ctrl.sensor_i2c_client,
				s5k5ca_pre_preview_settings,
				ARRAY_SIZE(s5k5ca_pre_preview_settings),
				s5k5ca_s_ctrl.msm_sensor_reg->default_data_type);   
			if( rc!=0 )
				printk("s5k5ca_sensor_setting()s5k5ca_pre_preview_settings rc:%d",rc);
			msleep(140);
		}
#endif
//-- petershih -20130225 - tmp add --
		if((res==0) &&(night_mode_flag==msm_v4l2_best_shot_night))
			{
		rc=msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			&s5k5ca_confs_night_mode[0], res);	
		if( rc!=0 )
			printk("s5k5ca_sensor_setting()rc:%d",rc);
			}
		else
		rc=msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
		if( rc!=0 )
			printk("s5k5ca_sensor_setting()rc:%d",rc);
		if (res == MSM_SENSOR_RES_4)
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_PCLK_CHANGE,
					&vfe_clk);
	}
	return rc;
}
static struct msm_sensor_fn_t s5k5ca_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = s5k5ca_set_fps,//msm_sensor_set_fps,
	
	.sensor_write_exp_gain = s5k5ca_write_prev_exp_gain,
	.sensor_write_snapshot_exp_gain = s5k5ca_write_pict_exp_gain,
	.sensor_csi_setting = s5k5ca_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = s5k5ca_sensor_power_up,
	.sensor_power_down = s5k5ca_sensor_power_down,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	#if 0//Flea++
	.sensor_config_brithness = s5k5ca_sensor_config_brithness,
	.sensor_set_special_effect=s5k5ca_sensor_set_effect
       #endif //Flea--
};

static struct msm_sensor_reg_t s5k5ca_regs = {
	.default_data_type = MSM_CAMERA_I2C_WORD_DATA,
	.start_stream_conf = NULL,//s5k5ca_start_settings,
	.start_stream_conf_size = 0,//ARRAY_SIZE(s5k5ca_start_settings),
	.stop_stream_conf = NULL,//s5k5ca_stop_settings,
	.stop_stream_conf_size = 0,//ARRAY_SIZE(s5k5ca_stop_settings),
	.group_hold_on_conf = NULL,//s5k5ca_groupon_settings,
	.group_hold_on_conf_size = 0,//ARRAY_SIZE(s5k5ca_groupon_settings),
	.group_hold_off_conf = NULL,//s5k5ca_groupoff_settings,
	.group_hold_off_conf_size = 0,//ARRAY_SIZE(s5k5ca_groupoff_settings),
	.init_settings = &s5k5ca_init_conf[0],
	.init_size = ARRAY_SIZE(s5k5ca_init_conf),
       .init_settings_1 = &s5k5ca_init_conf_1[0],
	.init_size_1 = ARRAY_SIZE(s5k5ca_init_conf_1),
	.mode_settings = &s5k5ca_confs[0],
	.output_settings = &s5k5ca_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k5ca_confs),
};

static struct msm_sensor_ctrl_t s5k5ca_s_ctrl = {
	.msm_sensor_reg = &s5k5ca_regs,
	.msm_sensor_v4l2_ctrl_info = s5k5ca_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(s5k5ca_v4l2_ctrl_info),
	.sensor_i2c_client = &s5k5ca_sensor_i2c_client,
	.sensor_i2c_addr =  0x78,
	.sensor_output_reg_addr = NULL,//&s5k5ca_reg_addr,
	.sensor_id_info = &s5k5ca_id_info,
	.sensor_exp_gain_info = NULL,//&s5k5ca_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &s5k5ca_csi_params_array[0],
	.msm_sensor_mutex = &s5k5ca_mut,
	.sensor_i2c_driver = &s5k5ca_i2c_driver,
	.sensor_v4l2_subdev_info = s5k5ca_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k5ca_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k5ca_subdev_ops,
	.func_tbl = &s5k5ca_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("SamSung QXGA Bayer sensor driver");
MODULE_LICENSE("GPL v2");
