#include <linux/kernel.h>

#include <linux/delay.h>
#include <linux/lg1k/pms.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>

#include <asm/cacheflush.h>
#include <asm/cp15.h>
#include <asm/mach/map.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>

#include <mach/lg1k.h>

static int __init is_compatible(unsigned long node, char const *uname,
                                int depth, void *data)
{
	return of_flat_dt_is_compatible(node, data);
}

void __init lg115x_reserve_smp(void)
{
	if (PHYS_OFFSET)
		return;
	if (!of_scan_flat_dt(is_compatible, "lge,lg1156-pms"))
		return;
	memblock_reserve(0x00000000, PAGE_SIZE);
}

static DEFINE_SPINLOCK(smp_boot_lock);

static void __iomem *smp_ctrl_base;

static int smp_get_boot_sync(void)
{
	return (int)readl_relaxed(smp_ctrl_base + REG(184));
}

static void smp_set_boot_sync(int sync)
{
	writel_relaxed((unsigned long)sync, smp_ctrl_base + REG(184));
}

static void __cpuinit lg115x_smp_init_cpus(void)
{
	struct device_node *np;
	unsigned int cpus = 0;
	unsigned int cpu;

	for_each_node_by_type(np, "cpu")
		cpus++;

	if (cpus > nr_cpu_ids) {
		pr_warn("SMP: %u cpus greater than maximum(%u), clipping\n",
		        cpus, nr_cpu_ids);
		cpus = nr_cpu_ids;
	}

	for (cpu = 0; cpu < cpus; cpu++)
		set_cpu_possible(cpu, true);
}

void _lg115x_smp_prepare_cpus(void)
{
	extern void lg115x_smp_secondary_prepare(void __iomem *);
	extern void lg115x_smp_secondary_startup(void);

	struct device_node *np;
	void __iomem *base;

#ifdef	CONFIG_HAVE_ARM_SCU
	np = of_find_compatible_node(NULL, NULL, "arm,cortex-a9-scu");
	if (np) {
		base = of_iomap(np, 0);
		scu_enable(base);
		iounmap(base);
	}
#endif	/* CONFIG_SMP_SCU */

	/* prepare for initial secondary boot */
	np = of_find_node_by_name(NULL, "core_ctrl");
	smp_ctrl_base = of_iomap(np, 0);
	writel_relaxed(virt_to_phys(lg115x_smp_secondary_startup),
	               smp_ctrl_base + REG(183));

	/* prepare for hotplug secondary boot */
	np = of_find_node_by_name(NULL, "zero_page");
	if (np) {
		base = of_iomap(np, 0);
		lg115x_smp_secondary_prepare(base);
		iounmap(base);
	}
}

static void __cpuinit lg115x_smp_prepare_cpus(unsigned int max_cpus)
{
	_lg115x_smp_prepare_cpus();
}

static void __cpuinit lg115x_smp_secondary_init(unsigned int cpu)
{
	smp_set_boot_sync(-1);
	spin_lock(&smp_boot_lock);
	spin_unlock(&smp_boot_lock);
}

static int lg115x_smp_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;
	int err;

	/* power-on CPU if supported and powered-off */
	err = lg1k_pms_get_hotplug(cpu);
	if (err == 0) {
		pr_notice("CPU%u: power-on\n", cpu);
		err = lg1k_pms_set_hotplug(cpu, 1);
		if (err < 0)
			return err;
	}

	spin_lock(&smp_boot_lock);

	smp_set_boot_sync(cpu_logical_map(cpu));
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));

	timeout = jiffies + HZ;
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (smp_get_boot_sync() == -1)
			break;
		udelay(10);
	}

	spin_unlock(&smp_boot_lock);

	return smp_get_boot_sync() != -1 ? -ENOSYS : 0;
}

#ifdef	CONFIG_HOTPLUG_CPU

static int lg115x_smp_hotplug_cpu_kill(unsigned int cpu)
{
	/* power-off CPU only if supported and powered-on */
	if (lg1k_pms_get_hotplug(cpu) == 1) {
		pr_notice("CPU%u: power-off\n", cpu);
		return lg1k_pms_set_hotplug(cpu, 0) == 0;
	}
	return 1;
}

static void lg115x_smp_hotplug_cpu_die(unsigned int cpu)
{
	unsigned int spurious = 0;
	unsigned int v;

	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, %1\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	: "=&r"(v)
	: "Ir"(CR_C)
	);

	/*
	 * Flushing after disabling the cache seems not allowed (at least)
	 * for Cortex-A9. Call flush_cache_all only if primary part number
	 * within MIDR indicates running CPU is Cortex-A15.
	 */
	asm volatile(
	"	mrc	p15, 0, %0, c0, c0, 0\n"
	: "=r"(v)
	);
	if ((v & 0xfff0) == 0xc0f0)
		flush_cache_all();

	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, %1\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	: "=&r"(v)
	: "Ir"(0x40)
	);

	isb();
	dsb();

	while (1) {
		wfi();
		if (smp_get_boot_sync() == cpu_logical_map(cpu))
			break;
		spurious++;
	}

	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, %1\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	orr	%0, %0, %2\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	: "=&r"(v)
	: "Ir"(0x40), "Ir"(CR_C)
	);

	isb();
	dsb();

	if (spurious)
		pr_warn("CPU%u: %u spurious wakeup calls\n", cpu, spurious);
}

#endif	/* CONFIG_HOTPLUG_CPU */

struct smp_operations lg115x_smp_ops __initdata = {
	.smp_init_cpus		= lg115x_smp_init_cpus,
	.smp_prepare_cpus	= lg115x_smp_prepare_cpus,
	.smp_secondary_init	= lg115x_smp_secondary_init,
	.smp_boot_secondary	= lg115x_smp_boot_secondary,
#ifdef	CONFIG_HOTPLUG_CPU
	.cpu_kill		= lg115x_smp_hotplug_cpu_kill,
	.cpu_die		= lg115x_smp_hotplug_cpu_die,
#endif	/* CONFIG_HOTPLUG_CPU */
};
