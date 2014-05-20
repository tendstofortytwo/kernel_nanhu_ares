/* Copyright (c) 2011/0902, Code Aurora Forum. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_ili9486.h"
#include <mach/socinfo.h>
#include <linux/gpio.h>//Flea
#include <mach/vreg.h>
//[Arima Ediosn] add for trigger backlight ++
#include <linux/leds-lm3533.h>
//[Arima Ediosn] add for trigger backlight --
static struct msm_panel_common_pdata *mipi_ili9486_pdata;

static struct dsi_buf ili9486_tx_buf;
static struct dsi_buf ili9486_rx_buf;
#define GPIO_BKL_EN	8

static int one_wire_mode_active = 0;
static int LCM_ID;

static char memory_Access_control[2] = {0x36, 0x98}; //{0x36, 0xc8}; 
static char position_x[5] = {0x2A,0x00, 0x00, 0x01, 0x3F};
static char position_y[5] = {0x2B,0x00, 0x00,0x01, 0xDF};
static char frame_rate_control[3] = {0xB1, 0xb0,0x11};
static char display_inverson_control[2] = {0xB4, 0x02};
static char interface_pixel_format[2] = {0x3A, 0x66};
//static char display_function_control[3] = {0xB6, 0x20,0x02}; //{0xB6, 0x20,0x22};
static char sleep_out[1] = {0x11};
/*Delay 120ms*/
static char  display_on[1] = {0x29};
static char  display_off[1] = {0x28};
static char  memory_set[1] = {0x2C};
/*Delay 10ms*/
static char  sleep_in[1] = {0x10};
/*Delay 120ms*/
static char display_inversion_off[2] = {0x20,0x00};
//static char tianma_postive_gamma_control[16] = {0xE0,0x0F,0x20,0x1C,0x0B,0x0D,0x05,0x4C,0xB3,0x39,0x07,0x10,0x01,0x06,0x04,0x01};
//static char tianma_negative_gamma_control[16] = {0xE1,0x0F,0x38,0x38,0x0E,0x10,0x04,0x4E,0x25,0x3A,0x04,0x10,0x05,0x22,0x1F,0x0F};
//gamma2.8 new
static char tianma_postive_gamma_control[16] = {0xE0,0x0F,0x1F,0x1B,0x0D,0x0E,0x06,0x4C,0xB4,0x39,0x04,0x0B,0x00,0x10,0x0E,0x01};
static char tianma_negative_gamma_control[16] = {0xE1,0x0F,0x2E,0x2E,0x1F,0x16,0x07,0x4F,0x16,0x3B,0x02,0x0E,0x04,0x23,0x20,0x0F};
//gamma2.7 new
//static char tianma_postive_gamma_control[16] = {0xE0,0x0F,0x15,0x11,0x0E,0x11,0x08,0x41,0xA8,0x30,0x07,0x0F,0x00,0x01,0x01,0x00};
//static char tianma_negative_gamma_control[16] = {0xE1,0x0F,0x38,0x3B,0x07,0x07,0x00,0x47,0x31,0x2E,0x03,0x10,0x04,0x19,0x15,0x00};
#if 0
// color adjust v1
static char tianma_color1_control[17] = {0xE2, 0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
static char tianma_color2_control[65] = {0xE3, 0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
	0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0};
#endif
#if 0
// color adjust v2
static char tianma_color1_control[17] = {0xE2, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char tianma_color2_control[65] = {0xE3, 0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
	0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0};
#endif
static char tianma_command1[6] = {0xF7, 0xA9,0x91,0x2D,0x8A};
static char tianma_command2[3] = {0xF8, 0x21, 0x04};
//static char tianma_command2[3] = {0xF8, 0x21, 0x07}; // color adjust for v1&v2
static char tianma_command3[3] = {0xF9, 0x08, 0x08};
static char tianma_command4[2] = {0xF9, 0x00};
//static char tianma_power_control1[3] = {0xC5, 0x55,0x55};//org
//static char tianma_power_control1[3] = {0xC5, 0x5A,0x5A}; //2.6
//static char tianma_power_control1[3] = {0xC5, 0x55,0x55}; // 2.7
//static char tianma_power_control1[3] = {0xC5, 0x41,0x41}; // 2.8
static char tianma_power_control1[4] = {0xC5, 0x5A,0x41,0x80}; // 2.8 NEW
static char tianma_power_control2[3] = {0xC0,0x10,0x14};
static char tianma_power_control3[2] = {0xC1,0x41};
static char tianma_display_function_control[3] = {0xB6, 0x20,0x02}; //{0xB6, 0x20,0x22};

static char turly_command0[7] = {0xF1, 0x36,0x04,0x00,0x3c,0x0f,0x8f};
static char turly_command1[10] = {0xF2, 0x18,0xA3,0x12,0x02,0xB2,0x12,0xFF,0x10,0x00};
static char turly_command2[5] = {0xF7,0xA9,0x91,0x2D,0x8A};  //8A for RGB888 0A for RGB666
static char turly_command3[3] = {0xF8, 0x21, 0x04};
static char turly_command4[3] = {0xF9, 0x00, 0x08};
static char turly_command5[6] = {0xF4, 0x00, 0x00, 0x08, 0x91, 0x04};
static char turly_command6[3] = {0xF9, 0x00};
static char turly_power_control1[3] = {0xC0,0x10, 0x14};
static char turly_power_control2[2] = {0xC1,0x41};
static char turly_power_control3[3] = {0xC5,0x51, 0x51};
static char turly_postive_gamma_control[16] = {0xE0, 0x0F,0x11,0x0F,0x0C,0x0D,0x05,0x41,0x96,0x30,0x08,0x10,0x01,0x01,0x01,0x00};
static char turly_negative_gamma_control[16] = {0xE1, 0x0F,0x3A,0x37,0x0A,0x0B,0x03,0x42,0x22,0x2F,0x02,0x0d,0x02,0x19,0x16,0x00};
static char turly_display_function_control[3] = {0xB6, 0x02,0x02}; 

static struct dsi_cmd_desc ili9486_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in}
};

static struct dsi_cmd_desc ili9486_tianma_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(frame_rate_control), frame_rate_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof( tianma_power_control2),  tianma_power_control2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(interface_pixel_format), interface_pixel_format}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(display_inversion_off),  display_inversion_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof( tianma_postive_gamma_control),  tianma_postive_gamma_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof( tianma_negative_gamma_control),  tianma_negative_gamma_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof( tianma_command1),  tianma_command1},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof( tianma_power_control1),  tianma_power_control1},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof( tianma_command2),  tianma_command2},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof( tianma_command3),  tianma_command3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(position_x), position_x},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(position_y), position_y},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_display_function_control), tianma_display_function_control},
       {DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(memory_Access_control), memory_Access_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(display_inverson_control), display_inverson_control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof( tianma_power_control3),  tianma_power_control3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof( tianma_command4),  tianma_command4},
       {DTYPE_DCS_WRITE, 1, 0, 0, 120,sizeof(sleep_out), sleep_out},//120
       {DTYPE_DCS_WRITE, 1, 0, 0, 10,sizeof(display_on), display_on},//10
       {DTYPE_DCS_WRITE, 1, 0, 0, 0,sizeof(memory_set), memory_set},//10
};
static struct dsi_cmd_desc ili9486_turly_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(frame_rate_control), frame_rate_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(position_x), position_x},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(position_y), position_y},
      {DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(display_inversion_off),  display_inversion_off},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_command0), turly_command0},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_command1), turly_command1},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_command2), turly_command2},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_command3), turly_command3},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_command4), turly_command4},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_command5), turly_command5},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_command6), turly_command6},
       {DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(memory_Access_control), memory_Access_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(frame_rate_control), frame_rate_control},
       {DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(display_inverson_control), display_inverson_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_display_function_control), turly_display_function_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_power_control1), turly_power_control1},
       {DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(turly_power_control2), turly_power_control2},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_power_control3), turly_power_control3},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_postive_gamma_control), turly_postive_gamma_control},
       {DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(turly_negative_gamma_control), turly_negative_gamma_control},
       {DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(interface_pixel_format), interface_pixel_format},
       {DTYPE_DCS_WRITE, 1, 0, 0, 120,sizeof(sleep_out), sleep_out},//120
       {DTYPE_DCS_WRITE, 1, 0, 0, 10,sizeof(display_on), display_on},//10
    	{DTYPE_DCS_WRITE, 1, 0, 0, 10,sizeof(memory_set), memory_set},//10
};
//tracy 20121024 fix sleep current++
struct vreg *vreg_l1;

static int mipi_ili9486_reg_control(int on)
{
	//static int vreg_state = 0 ;
	//static int has_ignore_vreg_set =0;
	
	

	if(on)
	{
		vreg_enable(vreg_l1);
		//vreg_state = on;
		printk(KERN_NOTICE "VREG L1 ON \n");
		return 1;
	}	
	else 
	{
		vreg_disable(vreg_l1);
		//vreg_state = on;
		printk(KERN_NOTICE"VREG L1 OFF \n");
		return 1;
	}

	
	return 0;  // return 0 means we do not have any change on vreg control
}
static int mipi_ili9486_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if(LCM_ID == 1)
	{
		mipi_dsi_cmds_tx(&ili9486_tx_buf, ili9486_tianma_display_on_cmds,
						ARRAY_SIZE(ili9486_tianma_display_on_cmds));
	}	
	else
	{
		mipi_dsi_cmds_tx(&ili9486_tx_buf, ili9486_turly_display_on_cmds,
						ARRAY_SIZE(ili9486_turly_display_on_cmds));
	}
	pr_emerg("Tracy_mipi_ili9486_lcd_on_initial_cmd_finish\n");

	return 0;
}
//static int i;
static int mipi_ili9486_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;


	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	//[Arima Edison] add to trigger backlight ++
   	lm3533_backlight_control(0);
	//[Arima Edison] add to trigger backlight --

	mipi_dsi_cmds_tx(&ili9486_tx_buf, ili9486_display_off_cmds,
			ARRAY_SIZE(ili9486_display_off_cmds));

	vreg_l1= vreg_get(NULL, "rfrx1");
	vreg_disable(vreg_l1);

	return 0;
}

static int __devinit mipi_ili9486_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_ili9486_pdata = pdev->dev.platform_data;
		return 0;
	}
	vreg_l1= vreg_get(NULL, "rfrx1");
	vreg_set_level(vreg_l1, 3000);

	msm_fb_add_device(pdev);

	return 0;
}

static void mipi_ili9486_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;
	int new_light_level = 0;

	bl_level = mfd->bl_level;
	
	new_light_level = bl_level * FAN_LIGHT_MAX_LEVEL / DRIVER_MAX_BACKLIGHT_LEVEL;
	//mdelay(60);
	if (new_light_level > 0)
		new_light_level--;

	if (new_light_level == 0) {
		one_wire_mode_active = 0;
		return;
	}else {
		if (!one_wire_mode_active) {
			one_wire_mode_active = 1;
		}
	}

}

static struct platform_driver this_driver = {
	.probe  = mipi_ili9486_lcd_probe,
	.driver = {
		.name   = "mipi_ili9486",
	},
};

static struct msm_fb_panel_data ili9486_panel_data = {

  .vreg_control = mipi_ili9486_reg_control,  //tracy 20121026 sleep current
	.on		= mipi_ili9486_lcd_on,
	.off		= mipi_ili9486_lcd_off,
	.set_backlight = mipi_ili9486_set_backlight,
};

static int ch_used[3];

int mipi_ili9486_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_ili9486", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	ili9486_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &ili9486_panel_data,
		sizeof(ili9486_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_ili9486_lcd_init(void)
{

	mipi_dsi_buf_alloc(&ili9486_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&ili9486_rx_buf, DSI_BUF_SIZE);      

	gpio_tlmm_config(GPIO_CFG(35, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
      
	LCM_ID = gpio_get_value(35);

	pr_emerg("Tracy:LCM_ID %d \n",LCM_ID);	

	return platform_driver_register(&this_driver);
}

module_init(mipi_ili9486_lcd_init);

