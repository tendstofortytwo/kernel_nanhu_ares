#define HARDWARD_VERSION_1  1  //HARRY  3.2 , ektf2040
#define HARDWARD_VERSION_2  2  //PORTER 4.0 , ektf2040
#define HARDWARD_VERSION_3  3  //PORTER 4.3 , ektf2136
#define HARDWARD_VERSION_4  4  //NanHu  3.5 , ektf2127
#define HARDWARD_VERSION HARDWARD_VERSION_4

#if HARDWARD_VERSION==HARDWARD_VERSION_1
#define ELAN_X_MAX	693
#define ELAN_Y_MAX	1024
#elif HARDWARD_VERSION==HARDWARD_VERSION_2
#define ELAN_X_MAX	754
#define ELAN_Y_MAX	1154
#elif HARDWARD_VERSION==HARDWARD_VERSION_3
#define ELAN_X_MAX	576
#define ELAN_Y_MAX	960
#else
#define ELAN_X_MAX	576
#define ELAN_Y_MAX	960
#endif

#ifndef _LINUX_ELAN_KTF2K_H
#define _LINUX_ELAN_KTF2K_H

#define ELAN_KTF2K_NAME "elan-ktf2k"

struct elan_ktf2k_i2c_platform_data {
	uint16_t version;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int intr_gpio;
  int reset_gpio;
	int (*power)(int on);
};

#endif /* _LINUX_ELAN_KTF2K_H */
