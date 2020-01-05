/*
SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
Copyright(c) 2013 by LG Electronics Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
*/


#ifdef CONFIG_CPU_FREQ_GOV_POLARIS

enum {
	POLARIS_NO_LOG = 0,
	POLARIS_LOG_ERR,
	POLARIS_LOG_INFO,
	POLARIS_LOG_DEBUG,
};

enum {
	POLARIS_HOTPLUG_NO_OPERATION = 0,
	POLARIS_HOTPLUG_ENABLE ,
	POLARIS_HOTPLUG_NO_OPERATION_N_CPU1_UP,
	POLARIS_HOTPLUG_NO_OPERATION_N_CPU1_DN,
	POLARIS_HOTPLUG_CONTROL_OPERATION,
};

enum {
	POLARIS_KICK_COMPLETE = 0,
	POLARIS_KICK_START,
};

#define CPU1_NO_HOTPLUG	0
#define CPU1_UP			1
#define CPU1_DOWN			2
#define TARGET_CPU1		1

extern unsigned int polaris_log_level;

#define POLAIRS_PRINT(cur_loglevel, fmt, args...) \
			do { \
				if ((cur_loglevel) <= polaris_log_level) \
					printk(fmt, ##args); \
			} while (0)

#define	POLARIS_ERROR(format, args...) \
		POLAIRS_PRINT(POLARIS_LOG_ERR,  format, ##args)
#define	POLARIS_INFO(format, args...) \
		POLAIRS_PRINT(POLARIS_LOG_INFO, format, ##args)
#define	POLARIS_DEBUG(format, args...) \
		POLAIRS_PRINT(POLARIS_LOG_DEBUG, format, ##args)


#define CPU1_UP_PRINT		"CPU1 UP   !!!"
#define CPU1_DOWN_PRINT	"CPU1 DOWN !!!"


#define CPUFREQ_POLARIS_LOG_HEADER "[cpufreq_polaris] "

#define DVFS_CONTROL_FORCING_DVFS_N_HOTPLUG_DISABLE_TIME 0 /* unit is sec */

#define RATIO_SEC_TO_SAMPLING_RATE 1000000 /* unit of sampling_rate is usec */
enum GOVERNOR_MODE{
	/* ondemand base governor operation */
	GOVERNOR_MODE_POLARIS = 0,
	/* jump to calulated freq table not a max freq */
	GOVERNOR_MODE_JUSTDEMAND
};

/* Definition of Polaris Governor Status */
enum POLARIS_GOVERNOR_WORKING_STATUS{
	POLARIS_GOVERNOR_WORKING_STATUS_WORKING = 0,
	POLARIS_GOVERNOR_WORKING_STATUS_POWER_OFF,
	POLARIS_GOVERNOR_WORKING_STATUS_INIT_DONE,
	POLARIS_GOVERNOR_WORKING_STATUS_START
};

struct POLARIS_GOVERNOR_STATUS {
	enum GOVERNOR_MODE governor_mode;
	enum POLARIS_GOVERNOR_WORKING_STATUS working_status;
};
extern struct POLARIS_GOVERNOR_STATUS polaris_governor_status;
#endif

