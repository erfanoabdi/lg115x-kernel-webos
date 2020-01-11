#include <linux/linkage.h>
#include <linux/mm.h>
#include <linux/preempt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/suspend.h>
#include <asm/tlbflush.h>

/* References to section boundaries */
extern const void __nosave_begin, __nosave_end;

extern asmlinkage int swsusp_save(void);

/*
 * pfn_is_nosave - check if given pfn is in the 'nosave' section
 */
int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = __pa_symbol(&__nosave_begin)
		>> PAGE_SHIFT;
	unsigned long nosave_end_pfn = PAGE_ALIGN(__pa_symbol(&__nosave_end))
		>> PAGE_SHIFT;

	return (pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn);
}

static int call_swsusp_save(unsigned long unused)
{
	return swsusp_save();
}

int swsusp_arch_suspend(void)
{
	int ret = cpu_suspend(0, call_swsusp_save);

	/*
	 * swsusp_save returns 0 if ok or <0 if error and cpu_suspend_abort
	 * returns 1 when returned value of fn is 0. So it's good to go if the
	 * returned value of cpu_suspend is 1
	 */
	if (ret == 1)
		return 0;
	return ret;
}

void save_processor_state(void)
{
	preempt_disable();
}

void restore_processor_state(void)
{
	extern int in_suspend;
	if(!in_suspend)
		outer_resume();

	preempt_enable();
}

#ifdef CONFIG_LG_SNAPSHOT_BOOT
extern unsigned long  __nosave_backup_phys, __nosave_begin_phys, __nosave_end_phys;

static int __init swsusp_arch_init(void)
{
	char *backup;
	size_t len;

	len = &__nosave_end - &__nosave_begin;
	if(!len)
		return 0;

	backup = kmalloc(len, GFP_KERNEL);
	if(!backup) {
		printk("%s %d kmalloc fail, can't save nosave data!\n", __func__, __LINE__);
		return -1;
	}
	memcpy(backup, &__nosave_begin, len);

	__nosave_backup_phys = virt_to_phys(backup);
	__nosave_begin_phys = virt_to_phys(&__nosave_begin);
	__nosave_end_phys = virt_to_phys(&__nosave_end);
	
	return 0;		
}
late_initcall(swsusp_arch_init);
#endif
