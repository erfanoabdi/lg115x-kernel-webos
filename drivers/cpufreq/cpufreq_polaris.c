
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

/*
 *  drivers/cpufreq/cpufreq_polaris.c
 */

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/percpu-defs.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/tick.h>
#include <linux/types.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/completion.h>

#include "cpufreq_governor.h"
#include "cpufreq_polaris.h"

/*----------------------------------------------------------
*   Defines
*----------------------------------------------------------*/

#define DEF_FREQUENCY_DOWN_DIFFERENTIAL		(30)
#define DEF_FREQUENCY_UP_THRESHOLD			(60)
#define DEF_SAMPLING_DOWN_FACTOR			(1)
#define MAX_SAMPLING_DOWN_FACTOR			(100000)
#define MICRO_FREQUENCY_DOWN_DIFFERENTIAL	(30)
#define MICRO_FREQUENCY_UP_THRESHOLD		(60)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE		(10000)
#define MIN_FREQUENCY_UP_THRESHOLD			(11)
#define MAX_FREQUENCY_UP_THRESHOLD			(100)


/*----------------------------------------------------------
*   Variables
*----------------------------------------------------------*/
struct POLARIS_GOVERNOR_STATUS  polaris_governor_status = {
	.governor_mode = GOVERNOR_MODE_POLARIS,
	.working_status = POLARIS_GOVERNOR_WORKING_STATUS_POWER_OFF,
};

struct polaris_local_tuners {
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int mode;
	unsigned int kick;
	unsigned int hotplug_control;
	unsigned int loglevel;
	unsigned int disable_time; /*sec unit*/
};
struct polaris_local_tuners *polaris_ext_tuners;

static struct od_ops polaris_ops;
/* POLARIS_INFO and POLARIS_ERROR is printed*/
unsigned int polaris_log_level = POLARIS_LOG_INFO;

static unsigned int default_powersave_bias;
static unsigned int prevhotplug = CPU1_UP;

static struct task_struct *ppolaris_pm_task;
static struct completion 	polaris_hotplug_completion;
static unsigned int polaris_hotplug_condition = CPU1_NO_HOTPLUG;
static unsigned int polaris_hotplug_control = POLARIS_HOTPLUG_NO_OPERATION;
static unsigned int performance_down_disable_count;

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_POLARIS
static struct cpufreq_governor cpufreq_gov_polaris;
#endif

/*---------------------------------------------------------
*   Functions Declaration
*---------------------------------------------------------*/
static DEFINE_PER_CPU(struct od_cpu_dbs_info_s, od_cpu_dbs_info);


/* Create show/store routines */
#define p_show_one(_gov, file_name)					\
static ssize_t show_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	struct od_dbs_tuners *tuners = _gov##_dbs_cdata.gdbs_data->tuners; \
	return sprintf(buf, "%u\n", tuners->file_name);			\
}									\
									\
static ssize_t show_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, char *buf)				\
{									\
	struct dbs_data *dbs_data = policy->governor_data;		\
	struct od_dbs_tuners *tuners = dbs_data->tuners;		\
	return sprintf(buf, "%u\n", tuners->file_name);			\
}

#define p_store_one(_gov, file_name)					\
static ssize_t store_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr \
, const char *buf, size_t count)	\
{									\
	struct dbs_data *dbs_data = _gov##_dbs_cdata.gdbs_data;		\
	return store_##file_name(dbs_data, buf, count);			\
}									\
									\
static ssize_t store_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, const char *buf, size_t count)		\
{									\
	struct dbs_data *dbs_data = policy->governor_data;		\
	return store_##file_name(dbs_data, buf, count);			\
}

#define ext_show_one(_gov, file_name)					\
static ssize_t show_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	struct polaris_local_tuners *tuners = polaris_ext_tuners; \
	return sprintf(buf, "%u\n", tuners->file_name);			\
}									\
									\
static ssize_t show_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, char *buf)				\
{									\
	struct polaris_local_tuners *tuners = polaris_ext_tuners;	\
	return sprintf(buf, "%u\n", tuners->file_name);			\
}

#define ext_store_one(_gov, file_name)					\
static ssize_t store_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr, const char *buf	\
, size_t count)	\
{									\
	struct dbs_data *dbs_data = _gov##_dbs_cdata.gdbs_data;		\
	return store_##file_name(dbs_data, buf, count);			\
}									\
									\
static ssize_t store_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, const char *buf, size_t count)		\
{									\
	struct dbs_data *dbs_data = policy->governor_data;		\
	return store_##file_name(dbs_data, buf, count);			\
}

/*-----------------------------------------------------------------------
*   Functions
*----------------------------------------------------------------------*/
extern unsigned int cpufreq_get(unsigned int cpu);
static void ondemand_powersave_bias_init_cpu(int cpu)
{
	struct od_cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	dbs_info->freq_table = cpufreq_frequency_get_table(cpu);
	dbs_info->freq_lo = 0;
}

struct polaris_local_tuners *get_ext_polaris(void)
{
	if (polaris_ext_tuners == NULL) {
		POLARIS_ERROR("can't get polaris\n");
		return NULL;
	}
	return polaris_ext_tuners;
}

static int get_polaris_hotplug_condition(void)
{
	return polaris_hotplug_condition;
}

static int set_polaris_hotplug_condition(unsigned int condition)
{
	polaris_hotplug_condition = condition;
	return polaris_hotplug_condition;
}

static int polaris_driver_target(struct cpufreq_policy *policy,
			    unsigned int target_freq,
			    unsigned int relation)
{
	unsigned int old_freq = 1416000;
	unsigned int cur_freq = 0;
	int ret;

	old_freq = cpufreq_get(0);

	ret = __cpufreq_driver_target(policy, target_freq, relation);
	if (ret) {
			POLARIS_ERROR(CPUFREQ_POLARIS_LOG_HEADER
			"CPU FREQ CHANGE ERROR\n");
	} else {
		cur_freq = cpufreq_get(0);

		if (old_freq != cur_freq)
			POLARIS_INFO(CPUFREQ_POLARIS_LOG_HEADER
			"CPU Freq %u -> %u\n", old_freq, cur_freq);
	}

	return ret;
}

static void cpu_work(unsigned int state)
{
	if (polaris_hotplug_control == POLARIS_HOTPLUG_NO_OPERATION)
		return;

	if (polaris_hotplug_condition == CPU1_NO_HOTPLUG) {
	/*POLARIS_INFO((state==CPU1_UP )? CPU1_UP_PRINT:CPU1_DOWN_PRINT);*/
		polaris_hotplug_condition = state;
		complete(&polaris_hotplug_completion);
	}
}

static unsigned int polaris_kick_func(unsigned int control)
{
	int cpu = 0;
	struct polaris_local_tuners *ext_tuners = NULL;
	struct od_cpu_dbs_info_s *dbs_info =
	&per_cpu(od_cpu_dbs_info, cpu);
	struct cpufreq_policy *policy = dbs_info->cdbs.cur_policy;
	struct dbs_data *dbs_data = policy->governor_data;
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;

	ext_tuners = get_ext_polaris();

	if (ext_tuners != NULL) {
		ext_tuners->kick = control;
	} else {
		return 0;
	}
	performance_down_disable_count
	= (ext_tuners->disable_time * RATIO_SEC_TO_SAMPLING_RATE) /
	polaris_tuners->sampling_rate;

	polaris_driver_target(policy, policy->max , CPUFREQ_RELATION_L);

	cpu_work(CPU1_UP);

	ext_tuners->kick = POLARIS_KICK_COMPLETE;

     return 0;
}
EXPORT_SYMBOL(polaris_kick_func);

static unsigned int polaris_hotplug_control_func(unsigned int control)
{
	struct polaris_local_tuners *ext_tuners = NULL;

	if (control > 3)
		control = 0;

	polaris_hotplug_control = control;

	ext_tuners = get_ext_polaris();

	if (ext_tuners != NULL) {
		ext_tuners->hotplug_control = control;
	} else {
		return 0;
	}

     return ext_tuners->hotplug_control;
}
EXPORT_SYMBOL(polaris_hotplug_control_func);


static unsigned int disable_time_func(unsigned int sec)
{
	struct polaris_local_tuners *ext_tuners = NULL;

	ext_tuners = get_ext_polaris();

	if (ext_tuners != NULL) {
		ext_tuners->disable_time = sec;
	} else {
		return 0;
	}

	return ext_tuners->disable_time;
}


static int hotplug_onoff(int onoff)
{
	int  ret;

	cpu_hotplug_driver_lock();

	if (cpu_online((unsigned int)TARGET_CPU1) && prevhotplug == CPU1_DOWN) {
		prevhotplug = CPU1_UP;
	} else if (!cpu_online((unsigned int)TARGET_CPU1)
	&& prevhotplug == CPU1_UP) {
		prevhotplug = CPU1_DOWN;
	}

	if (onoff) {
		if (prevhotplug == CPU1_UP)
			return 0;

		ret = cpu_up(TARGET_CPU1);
		prevhotplug = CPU1_UP;
	} else {
		if (prevhotplug == CPU1_DOWN)
			return 0;

		cpu_down(TARGET_CPU1);
		prevhotplug = CPU1_DOWN;
	}
	cpu_hotplug_driver_unlock();
	return 0;
}

static int polaris_pm_task(void *pParam)
{
	POLARIS_DEBUG("polaris_pm_task is created\n");

	do {
		/* Check stop condition when device is closed. */
		if (kthread_should_stop()) {
			POLARIS_DEBUG("polaris_pm_task - exit!\n");
			break;
		}

		if (get_polaris_hotplug_condition() == CPU1_NO_HOTPLUG) {
			INIT_COMPLETION(polaris_hotplug_completion);
			wait_for_completion(&polaris_hotplug_completion);
		}

		if (get_polaris_hotplug_condition() == CPU1_UP) {
			hotplug_onoff(1);
			set_polaris_hotplug_condition(CPU1_NO_HOTPLUG);

		} else if (get_polaris_hotplug_condition() == CPU1_DOWN) {
			hotplug_onoff(0);
			set_polaris_hotplug_condition(CPU1_NO_HOTPLUG);
		}

	} while (1);

	return 0;


}

/*
 * Not all CPUs want IO time to be accounted as busy; this depends on how
 * efficient idling at a higher frequency/voltage is.
 * Pavel Machek says this is not so for various generations of AMD and old
 * Intel systems.
 * Mike Chan (android.com) claims this is also not true for ARM.
 * Because of this, whitelist specific known (series) of CPUs by default,8 and
 * leave all others up to the user.
 */
static int should_io_be_busy(void)
{
#if defined(CONFIG_X86)
	/*
	 * For Intel, Core 2 (model 15) and later have an efficient idle.
	 */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
			boot_cpu_data.x86 == 6 &&
			boot_cpu_data.x86_model >= 15)
		return 1;
#endif
	return 0;
}

/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_jiffies,
 * freq_lo, and freq_lo_jiffies in percpu area for averaging freqs.
 */
static unsigned int generic_powersave_bias_target(struct cpufreq_policy *policy,
		unsigned int freq_next, unsigned int relation)
{
	unsigned int freq_req, freq_reduc, freq_avg;
	unsigned int freq_hi, freq_lo;
	unsigned int index = 0;
	unsigned int jiffies_total, jiffies_hi, jiffies_lo;
	struct od_cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info,
						   policy->cpu);
	struct dbs_data *dbs_data = policy->governor_data;
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;

	if (!dbs_info->freq_table) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_next;
	}

	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_next,
			relation, &index);
	freq_req = dbs_info->freq_table[index].frequency;
	freq_reduc = freq_req * polaris_tuners->powersave_bias / 1000;
	freq_avg = freq_req - freq_reduc;

	/* Find freq bounds for freq_avg in freq_table */
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_H, &index);
	freq_lo = dbs_info->freq_table[index].frequency;
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_L, &index);
	freq_hi = dbs_info->freq_table[index].frequency;

	/* Find out how long we have to be in hi and lo freqs */
	if (freq_hi == freq_lo) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_lo;
	}
	jiffies_total = usecs_to_jiffies(polaris_tuners->sampling_rate);
	jiffies_hi = (freq_avg - freq_lo) * jiffies_total;
	jiffies_hi += ((freq_hi - freq_lo) / 2);
	jiffies_hi /= (freq_hi - freq_lo);
	jiffies_lo = jiffies_total - jiffies_hi;
	dbs_info->freq_lo = freq_lo;
	dbs_info->freq_lo_jiffies = jiffies_lo;
	dbs_info->freq_hi_jiffies = jiffies_hi;
	return freq_hi;
}

static void ondemand_powersave_bias_init(void)
{
	int i;
	for_each_online_cpu(i) {
		ondemand_powersave_bias_init_cpu(i);
	}
}


static void dbs_freq_increase(struct cpufreq_policy *p, unsigned int freq)
{
	struct dbs_data *dbs_data = p->governor_data;
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;

	if (polaris_tuners->powersave_bias)
		freq = polaris_ops.powersave_bias_target(p, freq,
				CPUFREQ_RELATION_H);
	else if (p->cur == p->max)
		return;

	polaris_driver_target(p, freq, polaris_tuners->powersave_bias ?
			CPUFREQ_RELATION_L : CPUFREQ_RELATION_H);
}

/*
 * Every sampling_rate, we check, if current idle time is less than 20%
 * (default), then we try to increase frequency. Every sampling_rate, we look
 * for the lowest frequency which can sustain the load while keeping idle time
 * over 30%. If such a frequency exist, we try to decrease to this frequency.
 *
 * Any frequency increase takes it to the maximum frequency. Frequency reduction
 * happens at minimum steps of 5% (default) of current frequency
 */

static void polaris_check_cpu(int cpu, unsigned int load_freq)
{
	struct od_cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	struct cpufreq_policy *policy = dbs_info->cdbs.cur_policy;
	struct dbs_data *dbs_data = policy->governor_data;
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;

	if (polaris_governor_status.working_status) {
		POLARIS_INFO(CPUFREQ_POLARIS_LOG_HEADER
		"[%s] Governor is not working now.\n", __FUNCTION__);
		return;
	}

	dbs_info->freq_lo = 0;

	if (polaris_ext_tuners->kick >= POLARIS_KICK_START)
		return;


	if (polaris_hotplug_control == POLARIS_HOTPLUG_NO_OPERATION_N_CPU1_UP ||
	 polaris_hotplug_control == POLARIS_HOTPLUG_NO_OPERATION_N_CPU1_DN) {
		/* temperal status for cpu1 up/down */
		polaris_hotplug_control = POLARIS_HOTPLUG_CONTROL_OPERATION;

		switch (polaris_ext_tuners->hotplug_control) {
		case POLARIS_HOTPLUG_NO_OPERATION_N_CPU1_UP:
			cpu_work(CPU1_UP);
		break;
		case POLARIS_HOTPLUG_NO_OPERATION_N_CPU1_DN:
			cpu_work(CPU1_DOWN);
		break;
		default:
		break;
		}

		return;
	}

	if (performance_down_disable_count)
		performance_down_disable_count--;

	/* if temperal status for cpu1 up/down is finnished */
	if (performance_down_disable_count == 0 &&
	polaris_hotplug_control == POLARIS_HOTPLUG_CONTROL_OPERATION) {
		polaris_hotplug_control = POLARIS_HOTPLUG_NO_OPERATION;
		return;
	}

	if ((!cpu_online(TARGET_CPU1)) &&
	(load_freq > (polaris_tuners->up_threshold * policy->max))) {
		if (get_polaris_hotplug_condition() == CPU1_NO_HOTPLUG &&
		polaris_hotplug_control != POLARIS_HOTPLUG_NO_OPERATION) {
			if (polaris_governor_status.governor_mode ==
			GOVERNOR_MODE_JUSTDEMAND) {
				POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER
			"%d KHz, %d %%. CPU1 up !!!\n", policy->cur,
			load_freq / policy->cur);
			} else {
				POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER
			" CPU1 DOWN !!![%d]\n", __LINE__);
			}
			cpu_work(CPU1_UP);
		}

		performance_down_disable_count
		= (polaris_ext_tuners->disable_time * RATIO_SEC_TO_SAMPLING_RATE)
		 / polaris_tuners->sampling_rate;
		return;
	}

	if (polaris_governor_status.governor_mode == GOVERNOR_MODE_JUSTDEMAND) {
		unsigned int freq_next;
		freq_next = load_freq / polaris_tuners->up_threshold;
		if (freq_next >= policy->max) {
			dbs_info->rate_mult =
			polaris_tuners->sampling_down_factor;
		} else {
			dbs_info->rate_mult = 1;
		}
		if (get_polaris_hotplug_condition() != CPU1_NO_HOTPLUG) {
			POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER
			"Doing Hotplug Operation.\n");
			return;
		}
		if (freq_next < policy->min)
			freq_next = policy->min;
		if (freq_next > policy->cur) {
			POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER
			"%d KHz, %d %%.\n", policy->cur, load_freq/policy->cur);
			/* dbs_freq_increase(policy, freq_next); */
			polaris_driver_target(policy, freq_next
			, CPUFREQ_RELATION_L);
		} else if (freq_next < policy->cur) {
			POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER
			"%d KHz, %d %%.\n", policy->cur, load_freq/policy->cur);
			polaris_driver_target(policy, freq_next
			, CPUFREQ_RELATION_L);
		} else if (cpu_online((unsigned int)TARGET_CPU1) &&
		(polaris_hotplug_control != POLARIS_HOTPLUG_NO_OPERATION)) {
			/* in case of freq_next == policy->cur == policy->min */
			POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER
			"%d KHz, %d %%. CPU1 DOWN !!!\n", policy->cur
			, load_freq / policy->cur);
			cpu_work(CPU1_DOWN);
		}
		return;
	}

	/* Check for frequency increase */
	if (load_freq > polaris_tuners->up_threshold * policy->cur) {
		if (get_polaris_hotplug_condition() == CPU1_NO_HOTPLUG) {
			#if 0 /* 20130925, For Conformance Test */
			unsigned int freq_next;
			freq_next = load_freq / polaris_tuners->up_threshold;
			if (freq_next >= policy->max) {
				dbs_info->rate_mult
				= polaris_tuners->sampling_down_factor;
			} else {
				dbs_info->rate_mult = 1;
			}
			POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER "%d KHz\n"
			, freq_next);
			polaris_driver_target(policy, freq_next
			, CPUFREQ_RELATION_L);
			#else
			/* If switching to max speed,
			*apply sampling_down_factor */
			if (policy->cur < policy->max)
				dbs_info->rate_mult =
				polaris_tuners->sampling_down_factor;
			dbs_freq_increase(policy, policy->max);
			#endif
		}

		performance_down_disable_count
		= polaris_ext_tuners->disable_time * RATIO_SEC_TO_SAMPLING_RATE
		/ polaris_tuners->sampling_rate;
		return;
	}

	/* Check for frequency decrease */
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min) {
		if (load_freq < (polaris_tuners->adj_up_threshold) *
		policy->cur) {
			if ((performance_down_disable_count == 0)
			&& (get_polaris_hotplug_condition() == CPU1_NO_HOTPLUG)
			&& (polaris_hotplug_control != POLARIS_HOTPLUG_NO_OPERATION)
			) {
				if (cpu_online((unsigned int)TARGET_CPU1)) {
					POLARIS_DEBUG(CPUFREQ_POLARIS_LOG_HEADER
				" CPU1 DOWN !!![%d]\n", __LINE__);
					cpu_work(CPU1_DOWN);
				}
			}
		}
		return;
	}

	/*
	 * The optimal frequency is the frequency that is the lowest that can
	 * support the current CPU usage without triggering the up policy. To be
	 * safe, we focus 10 points under the threshold.
	 */
	if (load_freq < polaris_tuners->adj_up_threshold * policy->cur) {
		unsigned int freq_next;
		freq_next = load_freq / polaris_tuners->adj_up_threshold;
		if (performance_down_disable_count) {
			return;
		}
		performance_down_disable_count
		= polaris_ext_tuners->disable_time * RATIO_SEC_TO_SAMPLING_RATE
		/ polaris_tuners->sampling_rate;

		/* No longer fully busy, reset rate_mult */
		dbs_info->rate_mult = 1;

		#if 0
		if (old_freq == policy->cur) {
			stepdown_freq = stepdown_freq - 100000;
		} else {
			stepdown_freq = policy->cur - 100000;
			old_freq = policy->cur;
		}

		if (stepdown_freq < policy->min)
			stepdown_freq = policy->min;

		freq_next = stepdown_freq;
		#else
		if (freq_next < policy->min)
			freq_next = policy->min;

		#endif

		if (!polaris_tuners->powersave_bias) {
			polaris_driver_target(policy, freq_next,
			CPUFREQ_RELATION_L);
			return;
		}

		freq_next = polaris_ops.powersave_bias_target(policy
		, freq_next, CPUFREQ_RELATION_L);
		polaris_driver_target(policy, freq_next, CPUFREQ_RELATION_L);
	} else {

	/* forcing_dvfs_n_hotplug_disable is incresed periodically
	*, so remove */
	}
}

static void polaris_dbs_timer(struct work_struct *work)
{
	struct od_cpu_dbs_info_s *dbs_info =
		container_of(work, struct od_cpu_dbs_info_s, cdbs.work.work);
	unsigned int cpu = dbs_info->cdbs.cur_policy->cpu;
	struct od_cpu_dbs_info_s *core_dbs_info = &per_cpu(od_cpu_dbs_info,
			cpu);
	struct dbs_data *dbs_data = dbs_info->cdbs.cur_policy->governor_data;
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	int delay = 0, sample_type = core_dbs_info->sample_type;
	bool modify_all = true;

	if (polaris_governor_status.working_status
		!= POLARIS_GOVERNOR_WORKING_STATUS_WORKING) {
		polaris_governor_status.working_status
		= POLARIS_GOVERNOR_WORKING_STATUS_WORKING;
		POLARIS_INFO(CPUFREQ_POLARIS_LOG_HEADER "Polaris Start.\n");
	}

	mutex_lock(&core_dbs_info->cdbs.timer_mutex);
	if (!need_load_eval(&core_dbs_info->cdbs
	, polaris_tuners->sampling_rate)) {
		modify_all = false;
		goto max_delay;
	}

	/* Common NORMAL_SAMPLE setup */
	core_dbs_info->sample_type = OD_NORMAL_SAMPLE;
	if (sample_type == OD_SUB_SAMPLE) {
		delay = core_dbs_info->freq_lo_jiffies;
		polaris_driver_target(core_dbs_info->cdbs.cur_policy,
				core_dbs_info->freq_lo, CPUFREQ_RELATION_H);
	} else {
		dbs_check_cpu(dbs_data, cpu);
		if (core_dbs_info->freq_lo) {
			/* Setup timer for SUB_SAMPLE */
			core_dbs_info->sample_type = OD_SUB_SAMPLE;
			delay = core_dbs_info->freq_hi_jiffies;
		}
	}

max_delay:
	if (!delay)
		delay = delay_for_sampling_rate(polaris_tuners->sampling_rate
				* core_dbs_info->rate_mult);

	gov_queue_work(dbs_data, dbs_info->cdbs.cur_policy, delay, modify_all);
	mutex_unlock(&core_dbs_info->cdbs.timer_mutex);
}

/************************** sysfs interface ************************/
static struct common_dbs_data polaris_dbs_cdata;

/**
 * update_sampling_rate - update sampling rate effective immediately if needed.
 * @new_rate: new sampling rate
 *
 * If new rate is smaller than the old, simply updating
 * dbs_tuners_int.sampling_rate might not be appropriate. For example, if the
 * original sampling_rate was 1 second and the requested new sampling rate is 10
 * ms because the user needs immediate reaction from ondemand governor, but not
 * sure if higher frequency will be required or not, then, the governor may
 * change the sampling rate too late; up to 1 second later. Thus, if we are
 * reducing the sampling rate, we need to make the new value effective
 * immediately.
 */
static void update_sampling_rate(struct dbs_data *dbs_data,
		unsigned int new_rate)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	int cpu;

	polaris_tuners->sampling_rate = new_rate = max(new_rate,
			dbs_data->min_sampling_rate);

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct od_cpu_dbs_info_s *dbs_info;
		unsigned long next_sampling, appointed_at;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;
		if (policy->governor != &cpufreq_gov_polaris) {
			cpufreq_cpu_put(policy);
			continue;
		}
		dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
		cpufreq_cpu_put(policy);

		mutex_lock(&dbs_info->cdbs.timer_mutex);

		if (!delayed_work_pending(&dbs_info->cdbs.work)) {
			mutex_unlock(&dbs_info->cdbs.timer_mutex);
			continue;
		}

		next_sampling = jiffies + usecs_to_jiffies(new_rate);
		appointed_at = dbs_info->cdbs.work.timer.expires;

		if (time_before(next_sampling, appointed_at)) {

			mutex_unlock(&dbs_info->cdbs.timer_mutex);
			cancel_delayed_work_sync(&dbs_info->cdbs.work);
			mutex_lock(&dbs_info->cdbs.timer_mutex);

			gov_queue_work(dbs_data, dbs_info->cdbs.cur_policy,
					usecs_to_jiffies(new_rate), true);

		}
		mutex_unlock(&dbs_info->cdbs.timer_mutex);
	}
}

static ssize_t store_sampling_rate(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	update_sampling_rate(dbs_data, input);
	return count;
}

static ssize_t store_io_is_busy(struct dbs_data *dbs_data, const char *buf,
	 size_t count)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;
	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	polaris_tuners->io_is_busy = !!input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct od_cpu_dbs_info_s *dbs_info =
		&per_cpu(od_cpu_dbs_info, j);
		dbs_info->cdbs.prev_cpu_idle
		= get_cpu_idle_time(j, &dbs_info->cdbs.prev_cpu_wall,
		polaris_tuners->io_is_busy);
	}
	return count;
}

static ssize_t store_up_threshold(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD)
		return -EINVAL;
	#if 0
	/* Calculate the new adj_up_threshold */
	polaris_tuners->adj_up_threshold += input;
	polaris_tuners->adj_up_threshold -= polaris_tuners->up_threshold;
	#endif

	polaris_tuners->up_threshold = input;
	return count;
}

static ssize_t store_down_threshold(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input >= polaris_tuners->up_threshold) {
		return -EINVAL;
	}

	polaris_tuners->adj_up_threshold = input;
	polaris_ext_tuners->down_threshold = input;
	return count;
}

static ssize_t store_sampling_down_factor(struct dbs_data *dbs_data,
		const char *buf, size_t count)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;
	polaris_tuners->sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
		struct od_cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info,
				j);
		dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_ignore_nice_load(struct dbs_data *dbs_data,
		const char *buf, size_t count)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == polaris_tuners->ignore_nice_load) { /* nothing to do */
		return count;
	}
	polaris_tuners->ignore_nice_load = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct od_cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->cdbs.prev_cpu_idle
		= get_cpu_idle_time(j, &dbs_info->cdbs.prev_cpu_wall,
		polaris_tuners->io_is_busy);
		if (polaris_tuners->ignore_nice_load)
			dbs_info->cdbs.prev_cpu_nice =
				kcpustat_cpu(j).cpustat[CPUTIME_NICE];

	}
	return count;
}

static ssize_t store_powersave_bias(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 1000)
		input = 1000;

	polaris_tuners->powersave_bias = input;
	ondemand_powersave_bias_init();
	return count;
}

/* hotplug on and max cpu frequency when dtv_kick == 1 */
static ssize_t store_mode(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	int ret;
	char str_mode[16];

	memset(str_mode, 0x0, 16);

	ret = sscanf(buf, "%15s", str_mode);
	if (ret != 1)
		return -EINVAL;

	if (strcmp(str_mode, "normal") == 0) {
		polaris_ext_tuners->mode = 0;
		polaris_governor_status.governor_mode = GOVERNOR_MODE_POLARIS;
	} else if (strcmp(str_mode, "justdemand") == 0) {
		polaris_ext_tuners->mode = 1;
		polaris_governor_status.governor_mode
		= GOVERNOR_MODE_JUSTDEMAND;
	}

	return count;
}

static ssize_t store_kick(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	/* struct od_dbs_tuners *polaris_tuners = dbs_data->tuners; */
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	polaris_kick_func(input);
/*	POLARIS_INFO("\n\t dtv_kick 0:NORMAL GOV OPERATION
*	1: MAX CLOCK and HOTPLUG ON if \n"); */
/*	POLARIS_INFO("\n\t dtv_hotplug_control is
*	1(HOTPLLUG ENABLE) \n");*/
/*	POLARIS_INFO("\n\t dtv_kick [%d]\n"
*	,polaris_tuners->kick);*/
	return count;
}

static ssize_t store_hotplug_control(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 3)
		input = 0;

	if (input == 0)
		POLARIS_INFO("Polaris hotplug_control DISABLE\n");
	else if (input == 1)
		POLARIS_INFO("Polaris hotplug_control ENABLE\n");
	else if (input == 2)
		POLARIS_INFO("Polaris hotplug_control"
		"NO CPU HOTPLUG & CPU1 ON\n");
	else
		POLARIS_INFO("Polaris hotplug_control"
		"NO CPU HOTPLUG & CPU1 OFF\n");

	polaris_hotplug_control_func(input);

	return count;
}

static ssize_t store_disable_time(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	disable_time_func(input);

	POLARIS_INFO("Polaris performance down disable time %us\n",
	 polaris_ext_tuners->disable_time);
	return count;
}

static void loglevel_change(unsigned int level)
{
	polaris_log_level = level ;
}
EXPORT_SYMBOL(loglevel_change);

static ssize_t store_loglevel(struct dbs_data *dbs_data, const char *buf,
		size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > POLARIS_LOG_DEBUG)
		return count;

	loglevel_change(input);
	polaris_ext_tuners->loglevel = input;

	POLARIS_INFO("loglevel 0:NO PRINT 1:ERROR 2:INFO 3:DEBUG \n");
	POLARIS_INFO("loglevel [%d]\n", polaris_ext_tuners->loglevel);
	return count;
}

static ssize_t show_governor_status(struct dbs_data *dbs_data, char *buf)
{
	struct od_dbs_tuners *polaris_tuners = dbs_data->tuners;

	POLARIS_INFO("\n[Polaris Governor Status]");
	POLARIS_INFO("\t governor_mode - %d\n" \
	, polaris_governor_status.governor_mode);
	POLARIS_INFO("\t up_threshold - %d\n",
	polaris_tuners->up_threshold);
	POLARIS_INFO("\t down_threshold - %d\n",
	polaris_ext_tuners->down_threshold);
	POLARIS_INFO("\t loglevel - %d\n",
	polaris_ext_tuners->loglevel);
	POLARIS_INFO("\t performance_down_disable_time - %d\n",
	polaris_ext_tuners->disable_time);
	POLARIS_INFO("\t performance_down_disable_count - %d\n",
	performance_down_disable_count);
	POLARIS_INFO("\t hotplug_control - %d\n",
	polaris_hotplug_control);
	return sprintf(buf, "[show_governor_status]\n");
}

static ssize_t show_governor_status_gov_sys
(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct dbs_data *dbs_data = polaris_dbs_cdata.gdbs_data;
	return show_governor_status(dbs_data, buf);
}
static ssize_t show_governor_status_gov_pol
(struct cpufreq_policy *policy, char *buf)
{
	struct dbs_data *dbs_data = policy->governor_data;
	return show_governor_status(dbs_data, buf);
}

p_show_one(polaris, sampling_rate);
p_store_one(polaris, sampling_rate);
p_show_one(polaris, io_is_busy);
p_store_one(polaris, io_is_busy);
p_show_one(polaris, up_threshold);
p_store_one(polaris, up_threshold);
ext_show_one(polaris, down_threshold);
ext_store_one(polaris, down_threshold);
p_show_one(polaris, sampling_down_factor);
p_store_one(polaris, sampling_down_factor);
p_show_one(polaris, ignore_nice_load);
p_store_one(polaris, ignore_nice_load);
p_show_one(polaris, powersave_bias);
p_store_one(polaris, powersave_bias);

/* polaris */
ext_show_one(polaris, mode);
ext_store_one(polaris, mode);
ext_show_one(polaris, kick);
ext_store_one(polaris, kick);
ext_show_one(polaris, hotplug_control);
ext_store_one(polaris, hotplug_control);
ext_show_one(polaris, loglevel);
ext_store_one(polaris, loglevel);
ext_show_one(polaris, disable_time);
ext_store_one(polaris, disable_time);


declare_show_sampling_rate_min(polaris);

gov_sys_pol_attr_rw(sampling_rate);
gov_sys_pol_attr_rw(io_is_busy);
gov_sys_pol_attr_rw(up_threshold);
gov_sys_pol_attr_rw(down_threshold);
gov_sys_pol_attr_rw(sampling_down_factor);
gov_sys_pol_attr_rw(ignore_nice_load);
gov_sys_pol_attr_rw(powersave_bias);
gov_sys_pol_attr_ro(sampling_rate_min);

/* polaris */
gov_sys_pol_attr_rw(mode);
gov_sys_pol_attr_rw(kick);
gov_sys_pol_attr_rw(hotplug_control);
gov_sys_pol_attr_rw(loglevel);
gov_sys_pol_attr_rw(disable_time);



gov_sys_pol_attr_ro(governor_status);

static struct attribute *dbs_attributes_gov_sys[] = {
	&sampling_rate_min_gov_sys.attr,
	&sampling_rate_gov_sys.attr,
	&up_threshold_gov_sys.attr,
	&down_threshold_gov_sys.attr,
	&sampling_down_factor_gov_sys.attr,
	&ignore_nice_load_gov_sys.attr,
	&powersave_bias_gov_sys.attr,
	&io_is_busy_gov_sys.attr,
	&mode_gov_sys.attr,
	&kick_gov_sys.attr,
	&hotplug_control_gov_sys.attr,
	&loglevel_gov_sys.attr,
	&disable_time_gov_sys.attr,
	&governor_status_gov_sys.attr,
	NULL
};

static struct attribute_group polaris_attr_group_gov_sys = {
	.attrs = dbs_attributes_gov_sys,
	.name = "polaris",
};

static struct attribute *dbs_attributes_gov_pol[] = {
	&sampling_rate_min_gov_pol.attr,
	&sampling_rate_gov_pol.attr,
	&up_threshold_gov_pol.attr,
	&down_threshold_gov_pol.attr,
	&sampling_down_factor_gov_pol.attr,
	&ignore_nice_load_gov_pol.attr,
	&powersave_bias_gov_pol.attr,
	&io_is_busy_gov_pol.attr,
	&mode_gov_pol.attr,
	&kick_gov_pol.attr,
	&hotplug_control_gov_pol.attr,
	&loglevel_gov_pol.attr,
	&disable_time_gov_pol.attr,
	&governor_status_gov_pol.attr,
	NULL
};

static struct attribute_group polaris_attr_group_gov_pol = {
	.attrs = dbs_attributes_gov_pol,
	.name = "polaris",
};

/************************** sysfs end ************************/
static int polaris_init(struct dbs_data *dbs_data)
{
	struct od_dbs_tuners *tuners;

	u64 idle_time;
	int cpu;
	unsigned long targetcpu = 0;

	tuners = kzalloc(sizeof(struct od_dbs_tuners), GFP_KERNEL);
	if (!tuners) {
		pr_err("%s: kzalloc failed\n", __func__);
		return -ENOMEM;
	}
	if (polaris_ext_tuners == NULL) {
		/* Polaris ext tuners alloc */
		polaris_ext_tuners = \
		kzalloc(sizeof(struct polaris_local_tuners) ,  GFP_KERNEL);
		if (!polaris_ext_tuners) {
			pr_err("%s: kzalloc failed\n", __func__);
			return -ENOMEM;
		}
	}

	cpu = get_cpu();
	idle_time = get_cpu_idle_time_us(cpu, NULL);
	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		tuners->up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
		tuners->adj_up_threshold = polaris_ext_tuners->down_threshold
		= MICRO_FREQUENCY_UP_THRESHOLD - MICRO_FREQUENCY_DOWN_DIFFERENTIAL;
		/*
		 * In nohz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		dbs_data->min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
	} else {
		tuners->up_threshold = DEF_FREQUENCY_UP_THRESHOLD;
		tuners->adj_up_threshold = polaris_ext_tuners->down_threshold
		= DEF_FREQUENCY_UP_THRESHOLD - DEF_FREQUENCY_DOWN_DIFFERENTIAL;

		/* For correct statistics, we need 10 ticks for each measure */
		dbs_data->min_sampling_rate = MIN_SAMPLING_RATE_RATIO *
			jiffies_to_usecs(10);
	}

	tuners->sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;
	tuners->ignore_nice_load = 0;
	tuners->powersave_bias = default_powersave_bias;
	tuners->io_is_busy = should_io_be_busy();

	polaris_ext_tuners->loglevel = polaris_log_level;
	polaris_ext_tuners->disable_time = 0;
	polaris_ext_tuners->hotplug_control = POLARIS_HOTPLUG_NO_OPERATION;

	dbs_data->tuners = tuners;
	mutex_init(&dbs_data->mutex);

	polaris_governor_status.governor_mode =	GOVERNOR_MODE_POLARIS;
	polaris_governor_status.working_status =
	POLARIS_GOVERNOR_WORKING_STATUS_POWER_OFF;

	if (ppolaris_pm_task == NULL) {
		init_completion(&polaris_hotplug_completion);
		ppolaris_pm_task = kthread_create_on_node(polaris_pm_task,
					NULL,
					cpu,
					"LGDTV-PM-TASK/%lu", targetcpu);

		if (likely(!IS_ERR(ppolaris_pm_task))) {
			kthread_bind(ppolaris_pm_task, targetcpu);
			wake_up_process(ppolaris_pm_task);
			POLARIS_DEBUG("LGDTV-PM-TASK create successed\n");
		} else
			POLARIS_ERROR("LGDTV-PM-TASK create failed\n");
	}
	return 0;
}

static void polaris_exit(struct dbs_data *dbs_data)
{
	kfree(dbs_data->tuners);
}

define_get_cpu_dbs_routines(od_cpu_dbs_info);

static struct od_ops polaris_ops = {
	.powersave_bias_init_cpu = ondemand_powersave_bias_init_cpu,
	.powersave_bias_target = generic_powersave_bias_target,
	.freq_increase = dbs_freq_increase,
};

static struct common_dbs_data polaris_dbs_cdata = {
	.governor = GOV_ONDEMAND,
	.attr_group_gov_sys = &polaris_attr_group_gov_sys,
	.attr_group_gov_pol = &polaris_attr_group_gov_pol,
	.get_cpu_cdbs = get_cpu_cdbs,
	.get_cpu_dbs_info_s = get_cpu_dbs_info_s,
	.gov_dbs_timer = polaris_dbs_timer,
	.gov_check_cpu = polaris_check_cpu,
	.gov_ops = &polaris_ops,
	.init = polaris_init,
	.exit = polaris_exit,
};

static void polaris_set_powersave_bias(unsigned int powersave_bias)
{
	struct cpufreq_policy *policy;
	struct dbs_data *dbs_data;
	struct od_dbs_tuners *polaris_tuners;
	unsigned int cpu;
	cpumask_t done;

	default_powersave_bias = powersave_bias;
	cpumask_clear(&done);

	get_online_cpus();
	for_each_online_cpu(cpu) {
		if (cpumask_test_cpu(cpu, &done))
			continue;

		policy = per_cpu(od_cpu_dbs_info, cpu).cdbs.cur_policy;
		if (!policy)
			continue;

		cpumask_or(&done, &done, policy->cpus);

		if (policy->governor != &cpufreq_gov_polaris)
			continue;

		dbs_data = policy->governor_data;
		polaris_tuners = dbs_data->tuners;
		polaris_tuners->powersave_bias = default_powersave_bias;
	}
	put_online_cpus();
}

void polaris_register_powersave_bias_handler(unsigned int (*f)
		(struct cpufreq_policy *, unsigned int, unsigned int),
		unsigned int powersave_bias)
{
	polaris_ops.powersave_bias_target = f;
	polaris_set_powersave_bias(powersave_bias);
}
EXPORT_SYMBOL_GPL(polaris_register_powersave_bias_handler);

void polaris_unregister_powersave_bias_handler(void)
{
	polaris_ops.powersave_bias_target = generic_powersave_bias_target;
	polaris_set_powersave_bias(0);
}
EXPORT_SYMBOL_GPL(polaris_unregister_powersave_bias_handler);

static int polaris_cpufreq_governor_dbs(struct cpufreq_policy *policy,
		unsigned int event)
{
	if (event == CPUFREQ_GOV_START) {
		struct od_dbs_tuners *polaris_tuners;
		polaris_tuners = polaris_dbs_cdata.gdbs_data->tuners;
		performance_down_disable_count
		= polaris_ext_tuners->disable_time
		* RATIO_SEC_TO_SAMPLING_RATE / (polaris_tuners->sampling_rate);
	}
	return cpufreq_governor_dbs(policy, &polaris_dbs_cdata, event);
}


#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_POLARIS
static
#endif
struct cpufreq_governor cpufreq_gov_polaris = {
	.name			= "polaris",
	.governor		= polaris_cpufreq_governor_dbs,
	.max_transition_latency	= TRANSITION_LATENCY_LIMIT,
	.owner			= THIS_MODULE,
};

static int __init cpufreq_gov_dbs_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_polaris);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_polaris);
}

MODULE_AUTHOR("LG SPT TEAM");
MODULE_DESCRIPTION("'cpufreq_polaris' - A dynamic cpufreq & cpu hotplug governor for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_POLARIS
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
