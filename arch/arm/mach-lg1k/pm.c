#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>
#include <linux/of.h>

#include <asm/smp_scu.h>
#include <asm/memory.h>
#include <mach/lg1k.h>

extern void _lg115x_smp_prepare_cpus(void);

static int lg115x_pm_suspend(void)
{
	return 0;
}

static void lg115x_pm_resume(void)
{
	_lg115x_smp_prepare_cpus();
}

static struct syscore_ops lg115x_pm_syscore_ops = {
	.suspend        = lg115x_pm_suspend,
	.resume         = lg115x_pm_resume,
};

static __init int lg115x_pm_syscore_init(void)
{
	register_syscore_ops(&lg115x_pm_syscore_ops);
	return 0;
}
arch_initcall(lg115x_pm_syscore_init);

