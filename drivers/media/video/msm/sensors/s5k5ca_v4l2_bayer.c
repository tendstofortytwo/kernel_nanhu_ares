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
//#include "s5k5ca_v4l2.h"
#define SENSOR_NAME "s5k5ca"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k5ca"
#define s5k5ca_obj s5k5ca_##obj
#if 0
static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};
#endif
static struct msm_sensor_ctrl_t s5k5ca_s_ctrl;

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
#if 1 // PeterShih - tmp remove
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
#include "s5k5ca_bayer_mcnex_init_10bit.h"
};


static struct msm_camera_i2c_conf_array s5k5ca_init_conf[] = {
	{&s5k5ca_preinit_settings[0],ARRAY_SIZE(s5k5ca_preinit_settings), 10, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_initial_settings[0],ARRAY_SIZE(s5k5ca_initial_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array s5k5ca_confs[] = {
	{&s5k5ca_snap_settings[0],ARRAY_SIZE(s5k5ca_snap_settings), 250, MSM_CAMERA_I2C_WORD_DATA},
	{&s5k5ca_prev_settings[0],ARRAY_SIZE(s5k5ca_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};
#endif
static struct msm_camera_csi_params s5k5ca_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 10,
};

static struct v4l2_subdev_info s5k5ca_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SGRBG10_1X10,
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
		.line_length_pclk = 0x1440,
		.frame_length_lines = 0x62c,
		.vt_pixel_clk = 122860800,//94371840,
		.op_pixel_clk =30000000,// 94371840,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW */
		.x_output = 1024, /*1024*/
		.y_output = 768, /*768*/
		.line_length_pclk = 0x8FC,
		.frame_length_lines = 0x34B,
		.vt_pixel_clk = 23592960,//58167000,//23592960,
		.op_pixel_clk = 23592960,//58167000,//23592960,
		.binning_factor = 0x0,
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


void s5k5ca_sensor_reset_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	printk("Peter-s5k5ca_sensor_reset_stream()\n");
}

static int32_t s5k5ca_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{


	printk("Peter-s5k5ca_write_pict_exp_gain() gain:%d line:%d\n",gain,line);
	return 0;

}


static int32_t s5k5ca_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
						uint16_t gain, uint32_t line)
{

	printk("Peter-s5k5ca_write_prev_exp_gain() gain:%d line:%d\n",gain,line);
	return 0;
}

static const struct i2c_device_id s5k5ca_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k5ca_s_ctrl},
	{ }
};
int32_t s5k5ca_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	printk("Peter-s5k5ca_sensor_i2c_probe()\n");
	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		printk("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	s_ctrl = client->dev.platform_data;

	return rc;
}

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
	printk("Peter-msm_sensor_init_module()\n");
	return i2c_add_driver(&s5k5ca_i2c_driver);
}

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

int s5k5ca_set_fps(struct msm_sensor_ctrl_t *s_ctrl,	struct fps_cfg *fps)
{
	printk("Peter-s5k5ca_set_fps() fps_divider:%d fps_div:%d \n",s_ctrl->fps_divider,fps->fps_div);
	return 0;

}

int32_t s5k5ca_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *info = NULL;

	printk("Peter-s5k5ca_sensor_power_down()\n");
	info = s_ctrl->sensordata;
	gpio_direction_output(info->sensor_pwd, 0);
	gpio_direction_output(info->sensor_reset, 0);
	usleep_range(5000, 5100);
	msm_sensor_power_down(s_ctrl);
	return 0;
}

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t s5k5ca_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;

	printk("Peter-s5k5ca_sensor_power_up()\n");
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
		printk("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}	
#endif
	/* turn on PWDN & RST */
	gpio_direction_output(info->sensor_pwd, 1);
	gpio_direction_output(info->sensor_reset, 1);

	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xFCFC, 0xD000, MSM_CAMERA_I2C_WORD_DATA);
	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x002C, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x002E, 0x0040, MSM_CAMERA_I2C_WORD_DATA);
	
	return rc;

}

static int32_t vfe_clk = 266667000;

int32_t s5k5ca_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

	printk("Peter-s5k5ca_sensor_setting() update_type:%d res:%d\n",update_type,res);

	if (update_type == MSM_SENSOR_REG_INIT) {
		printk("PERIODIC : %d\n", res);
		s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
		printk("CSI config in progress\n");
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSIC_CFG,
			s_ctrl->curr_csic_params);
		printk("CSI config is done\n");
		mb();

		printk("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
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
	.sensor_set_fps = msm_sensor_set_fps,
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
	.mode_settings = &s5k5ca_confs[0],
	.output_settings = &s5k5ca_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k5ca_confs),
};

static struct msm_sensor_ctrl_t s5k5ca_s_ctrl = {
	.msm_sensor_reg = &s5k5ca_regs,
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
