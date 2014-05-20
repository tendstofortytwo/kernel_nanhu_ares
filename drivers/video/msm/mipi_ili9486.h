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

#ifndef MIPI_ILI9486_H
#define MIPI_ILI9486_H

//<<Skies, Implement LCM backlight function
#define DRIVER_MAX_BACKLIGHT_LEVEL 100
#define FAN_LIGHT_MAX_LEVEL	32
#define FAN_LIGHT_CURRENT_OUTPUT_MAX 25
#define FAN_LIGHT_CURRENT_OUTPUT_LIMIT 20
//>>Skies, Implement LCM backlight function

int mipi_ili9486_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);

#endif  /* MIPI_ILI9486_H */

