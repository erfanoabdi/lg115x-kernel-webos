/*
 * mm/low-mem-notify.c
 *
 * Sends low-memory notifications to processes via /dev/low-mem.
 *
 * Copyright (C) 2012 The Chromium OS Authors
 * This program is free software, released under the GPL.
 * Based on a proposal by Minchan Kim
 *
 * A process that polls /dev/low-mem is notified of a low-memory situation.
 * The intent is to allow the process to free some memory before the OOM killer
 * is invoked.
 *
 * A low-memory condition is estimated by subtracting anonymous memory
 * (i.e. process data segments), kernel memory, and a fixed amount of
 * file-backed memory from total memory.  This is just a heuristic, as in
 * general we don't know how much memory can be reclaimed before we try to
 * reclaim it, and that's too expensive or too late.
 *
 * This is tailored to Chromium OS, where a single program (the browser)
 * controls most of the memory, and (currently) no swap space is used.
 */


#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/eventfd.h>
#include <linux/sort.h>
#include <linux/mutex.h>

#include <linux/low-mem-notify.h>
#ifdef CONFIG_SWAP
#include <linux/swap.h>
#endif

#undef DEBUG
#ifdef DEBUG
#define dprintk(msg, args...)       \
		printk("\r\n[%s:%d:%s] " msg, __FILE__, __LINE__, __func__, ## args)
#else
#define dprintk(msg, args...)
#endif

#define DEFAULT_WORKINGSET_SIZE    120

struct low_mem_threshold {
	struct eventfd_ctx *eventfd;
	unsigned long threshold;
};

struct low_mem_threshold_ary {
	int current_threshold;
	unsigned int size;
	struct low_mem_threshold entries[0];
};

struct low_mem_thresholds {
	/* Primary thresholds array */
	struct low_mem_threshold_ary *primary;
	/* Spare threshold array */
	struct low_mem_threshold_ary *spare;
};

/* protect arrays of thresholds */
struct mutex thresholds_lock;

/* thresholds for memory usage. RCU-protected */
struct low_mem_thresholds thresholds;

/* workingset pages */
atomic_long_t workingset_pages;

struct low_mem_notify_file_info {
	unsigned long unused;
};

static void low_mem_notify_threshold(void);
static unsigned long total_pages;

void low_mem_notify(void)
{
	low_mem_notify_threshold();
}

static inline unsigned long _free_pages(void)
{
	unsigned long free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;
	unsigned long file = global_page_state(NR_FILE_PAGES) -
						global_page_state(NR_SHMEM);
#ifdef CONFIG_SWAP
	free += atomic_long_read(&nr_swap_pages);
	file -= total_swapcache_pages();
#endif
	return (free > file) ? free : file;
}

static inline unsigned long _usable_pages(void)
{
	#if 0
	unsigned long usable, free, cached, unreclaimable, reclaimable;
	/* Buffers + Cached - (SHMEM + Dirty + Writeback + System working set code page) + SReclaimable */
	free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;
	cached = global_page_state(NR_FILE_PAGES) - total_swapcache_pages();
	unreclaimable = global_page_state(NR_SHMEM) + global_page_state(NR_FILE_DIRTY) +
		global_page_state(NR_WRITEBACK) + workingset_pages;
	reclaimable = global_page_state(NR_SLAB_RECLAIMABLE);
	usable = free + cached - unreclaimable + reclaimable;
	#else
	unsigned long free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;
	unsigned long cached = global_page_state(NR_FILE_PAGES) - total_swapcache_pages();
	unsigned long usable;
#ifdef CONFIG_SWAP
	free += atomic_long_read(&nr_swap_pages);
#endif
	usable = free + cached - atomic_long_read(&workingset_pages);
	#endif
	return (usable > 0) ? usable : 0;
}

static void low_mem_notify_threshold(void)
{
	struct low_mem_threshold_ary *t;
	unsigned long free;
	int i;

	rcu_read_lock();
	t = rcu_dereference(thresholds.primary);
	if (!t)
		goto unlock;

	free = _usable_pages(); /* _free_pages(); */

	i = t->current_threshold;

	for (; i > 0 && unlikely(t->entries[i].threshold < free); i--) {
		eventfd_signal(t->entries[i].eventfd, 1);
		dprintk("eventfd_signal(%d) %ld >>>>>\n", i, t->entries[i].threshold);
	}

	i++;

	for (; i < t->size && unlikely(t->entries[i].threshold >= free); i++) {
		eventfd_signal(t->entries[i].eventfd, 1);
		dprintk("eventfd_signal(%d) %ld >>>>>\n", i, t->entries[i].threshold);
	}

	t->current_threshold = i - 1;
unlock:
	rcu_read_unlock();
}

static int bigorder_thresholds(const void *a, const void *b)
{
	const struct low_mem_threshold *_a = a;
	const struct low_mem_threshold *_b = b;

	return _b->threshold - _a->threshold;
}

static int low_mem_register_event(struct eventfd_ctx *eventfd,
				unsigned long threshold)
{
	struct low_mem_threshold_ary *new;
	unsigned long free;
	int i, size, ret = 0;

	mutex_lock(&thresholds_lock);

	free = _usable_pages(); /* _free_pages(); */

	if (thresholds.primary)
		low_mem_notify_threshold();

	size = thresholds.primary ? thresholds.primary->size + 1 : 1;

	new = kmalloc(sizeof(*new) + size * sizeof(struct low_mem_threshold),
			GFP_KERNEL);
	if (!new) {
		ret = -ENOMEM;
		goto unlock;
	}
	new->size = size;

	if (thresholds.primary) {
		memcpy(new->entries, thresholds.primary->entries, (size - 1) *
				sizeof(struct low_mem_threshold));
	}

	new->entries[size - 1].eventfd = eventfd;
	new->entries[size - 1].threshold = threshold;

	sort(new->entries, size, sizeof(struct low_mem_threshold),
			bigorder_thresholds, NULL);

	new->current_threshold = -1;
	for (i = 0; i < size; i++) {
		if (new->entries[i].threshold >= free)
			++new->current_threshold;
	}

	kfree(thresholds.spare);
	thresholds.spare = thresholds.primary;

	rcu_assign_pointer(thresholds.primary, new);

	synchronize_rcu();

unlock:
	mutex_unlock(&thresholds_lock);

	return ret;
}

static int low_mem_reset_events(void)
{
	mutex_lock(&thresholds_lock);

	kfree(thresholds.primary);
	kfree(thresholds.spare);

	thresholds.primary = thresholds.spare = NULL;

	mutex_unlock(&thresholds_lock);

	return 0;
}

#ifdef CONFIG_SYSFS

#define LOW_MEM_ATTR_RO(_name)				      \
	static struct kobj_attribute low_mem_##_name##_attr = \
		__ATTR_RO(_name)

#define LOW_MEM_ATTR(_name)				      \
	static struct kobj_attribute low_mem_##_name##_attr = \
		__ATTR(_name, 0644, low_mem_##_name##_show,   \
		       low_mem_##_name##_store)

static unsigned low_mem_margin_to_minfree(unsigned percent)
{
	return (percent * (total_pages)) / 100;
}

static ssize_t free_pages_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%ld\n", _free_pages());
}
LOW_MEM_ATTR_RO(free_pages);

static ssize_t usable_pages_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%ld\n", _usable_pages());
}
LOW_MEM_ATTR_RO(usable_pages);

static ssize_t thresholds_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct low_mem_threshold_ary *t;
	ssize_t count = 0;
	int i;

	rcu_read_lock();
	t = rcu_dereference(thresholds.primary);
	if (!t)
		goto unlock;

	for (i = 0; i < t->size; i++) {
		if (i == t->current_threshold)
			count += sprintf(&buf[count], "*");
		count += sprintf(&buf[count], "%ld ", t->entries[i].threshold);
	}
	count += sprintf(&buf[count], "\n");

unlock:
	rcu_read_unlock();
	return count;
}
LOW_MEM_ATTR_RO(thresholds);

static ssize_t level_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct low_mem_threshold_ary *t;
	ssize_t count = 0;

	rcu_read_lock();
	t = rcu_dereference(thresholds.primary);
	if (!t)
		goto unlock;

	count = sprintf(buf, "%u\n", t->current_threshold);
unlock:
	rcu_read_unlock();
	return count;
}
LOW_MEM_ATTR_RO(level);

static ssize_t low_mem_event_ctrl_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct low_mem_threshold_ary *t;
	ssize_t count = 0;

	rcu_read_lock();
	t = rcu_dereference(thresholds.primary);
	if (!t)
		goto unlock;

	count = sprintf(buf, "%u\n", t->size);

unlock:
	rcu_read_unlock();
	return count;
}

static ssize_t low_mem_event_ctrl_store(struct kobject *kobj,
                                        struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct eventfd_ctx *eventfd = NULL;
	struct file *efile = NULL;
	int err, ret = 0;
	unsigned int efd;
	unsigned long ratio, threshold;
	char *buffer, *endp;

	if(total_pages == 0)
		total_pages = totalram_pages + total_swap_pages;

	/* <event_fd> <threshold> */
	efd = simple_strtoul(buf, &endp, 10);
	if (*endp != ' ')
		return -EINVAL;
	buffer = endp + 1;

	err = strict_strtoul(buffer, 10, &ratio);
	if (err)
		return -EINVAL;

	efile = eventfd_fget(efd);
	if (IS_ERR(efile)) {
		ret = PTR_ERR(efile);
		goto fail;
	}

	eventfd = eventfd_ctx_fileget(efile);
	if (IS_ERR(eventfd)) {
		ret = PTR_ERR(eventfd);
		goto fail;
	}

	threshold = low_mem_margin_to_minfree(ratio);

	ret = low_mem_register_event(eventfd, threshold);
	if (ret)
		goto fail;

	fput(efile);

	return count;

fail:

	if(eventfd && !IS_ERR(eventfd))
		eventfd_ctx_put(eventfd);

	if (!IS_ERR_OR_NULL(efile))
		fput(efile);

	return ret;
}
LOW_MEM_ATTR(event_ctrl);

static ssize_t low_mem_reset_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t low_mem_reset_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	low_mem_reset_events();
	return count;
}
LOW_MEM_ATTR(reset);

static ssize_t low_mem_workingset_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
        size_t count;
	count = sprintf(buf, "%ld\n", atomic_long_read(&workingset_pages));
	return count;
}

static ssize_t low_mem_workingset_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long ws;
	int err;

	/* get workingset */
	err = strict_strtoul(buf, 10, &ws);
	if (err)
		return -EINVAL;

	atomic_long_set(&workingset_pages, ws);

	return count;
}
LOW_MEM_ATTR(workingset);

static struct attribute *low_mem_attrs[] = {
	&low_mem_free_pages_attr.attr,
	&low_mem_usable_pages_attr.attr,
	&low_mem_level_attr.attr,
	&low_mem_thresholds_attr.attr,
	&low_mem_event_ctrl_attr.attr,
	&low_mem_reset_attr.attr,
	&low_mem_workingset_attr.attr,
	NULL,
};

static struct attribute_group low_mem_attr_group = {
	.attrs = low_mem_attrs,
	.name = "low_mem_notify",
};

static int __init low_mem_init(void)
{
	int err = sysfs_create_group(mm_kobj, &low_mem_attr_group);
	if (err)
		printk(KERN_ERR "low_mem: register sysfs failed\n");

	atomic_long_set(&workingset_pages, (DEFAULT_WORKINGSET_SIZE * 1024 * 1024) >> PAGE_SHIFT);

	memset(&thresholds, 0, sizeof(struct low_mem_thresholds));
	mutex_init(&thresholds_lock);

	return err;
}
module_init(low_mem_init)

#endif
