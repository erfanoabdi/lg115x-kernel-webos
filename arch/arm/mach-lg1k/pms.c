#include <linux/kernel.h>

#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/lg1k/pms.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/sched.h>

#define	PMS_POLL	0x0001		/* polling instead of interrupt */
#define PMS_DEBUG_COUT	0x0002		/* enable debug-print of PMS firmware */
#define PMS_DEBUG_FAKE	0x0004		/* enable fake mode */

#define PMS_CPUFREQ_FLAG	0	/* default flag for cpufreq */
#define PMS_CPUVOLT_FLAG	0	/* default flag for cpuvolt */
#define PMS_HOTPLUG_FLAG	0	/* default flag for hotplug */

#define	PMS_TIMEOUT	100		/* msecs */

#define MAILBOX(m)	((m) << 5)
#define REG(r)		((r) << 2)

#define M2A_IRQ_CTRL	REG(0)		/* PMS to CPU IRQ control & status */
#define M2A_IRQ_MASK	REG(1)		/* PMS to CPU IRQ enable or mask */
#define A2M_IRQ_CTRL	REG(2)		/* CPU to PMS IRQ control & status */
#define A2M_IRQ_MASK	REG(3)		/* CPU to PMS IRQ enable or mask */

#define IPC_CMD_CPU_NOP	0xaa000000	/* request for no operation */
#define	IPC_CMD_CPU_FS	0xaa000001	/* request for scaling frequency */
#define IPC_CMD_CPU_VS	0xaa000002	/* request for scaling voltage */
#define	IPC_CMD_CPU_PM	0xaa000004	/* request for controling power */

#define IPC_CMD_CPU_RES	0xaa000100	/* response */

#define IPC_ERR_NONE	0xaa020000
#define IPC_ERR_AVS	0xaa020001
#define IPC_ERR_PLL	0xaa020002
#define IPC_ERR_PMIC	0xaa020004
#define IPC_ERR_PI	0xaa020008
#define IPC_ERR_PO	0xaa020010
#define IPC_ERR_WFI	0xaa020020
#define IPC_ERR_TS	0xaa020040

static void __iomem *pms_ipc_base;
static void __iomem *pms_irq_base;

static DEFINE_MUTEX(pms_mutex);

/*
 * Peterson's algorithm (http://en.wikipedia.org/wiki/Peterson%27s_algorithm)
 * to implement the mutual exclusion between the CPU(P0) and the PMS(P1).
 */

static inline void pms_mutex_lock(void)
{
	mutex_lock(&pms_mutex);

	writel_relaxed(0x01, pms_ipc_base + MAILBOX(6) + REG(0));
	writel_relaxed(0x01, pms_ipc_base + MAILBOX(6) + REG(2));
	while (readl_relaxed(pms_ipc_base + MAILBOX(6) + REG(1)) == 0x01 &&
	       readl_relaxed(pms_ipc_base + MAILBOX(6) + REG(2)) == 0x01) {
		/* busy wait */
	}
}

static inline void pms_mutex_unlock(void)
{
	writel_relaxed(0x00, pms_ipc_base + MAILBOX(6) + REG(0));
	mutex_unlock(&pms_mutex);
}

static int pms_request(unsigned long key, unsigned long flag)
{
	unsigned long timeout = PMS_TIMEOUT * NSEC_PER_MSEC;
	unsigned long long start;
	unsigned long reg;
	int err = 0;

	/* clear possible unhandled pending interrupts */
	writel_relaxed(0x00, pms_irq_base + M2A_IRQ_CTRL);
	/* wake PMS to handle the request */
	writel_relaxed(0x01, pms_irq_base + A2M_IRQ_CTRL);

	start = sched_clock();

	/* busy-wait for PMS completion */
	while (!(readl_relaxed(pms_irq_base + M2A_IRQ_CTRL) & 0x01)) {
		if (sched_clock() - start > timeout) {
			pr_err("%s: request timed-out\n", __func__);
			err = -ETIME;
			goto quit;
		}
	}

	/* check response code */
	reg = readl_relaxed(pms_ipc_base + MAILBOX(1) + REG(0));
	if (reg != IPC_CMD_CPU_RES) {
		pr_err("%s: CMD: %#lx (expected: %#x)\n", __func__, reg,
		       IPC_CMD_CPU_RES);
		err = -EINVAL;
		goto quit;
	}
	/* check transaction key */
	reg = readl_relaxed(pms_ipc_base + MAILBOX(1) + REG(7));
	if (reg != key) {
		pr_err("%s: KEY: %#lx (expected: %#lx)\n", __func__, reg, key);
		err = -EINVAL;
		goto quit;
	}
	/* check error code */
	reg = readl_relaxed(pms_ipc_base + MAILBOX(1) + REG(6));
	if (reg != IPC_ERR_NONE) {
		pr_err("%s: ERR: %#lx (expected: %#x)\n", __func__, reg,
		       IPC_ERR_NONE);
		err = -EFAULT;
		goto quit;
	}
quit:
	return err;
}

/*
 * CPU frequency scaling
 */

static inline int __pms_get_cpufreq(int cpu)
{
	return readl_relaxed(pms_ipc_base + MAILBOX(1) + REG(1));
}

static int pms_get_cpufreq(int cpu)
{
	int kHz;

	pms_mutex_lock();
	kHz = __pms_get_cpufreq(cpu);
	pms_mutex_unlock();

	return kHz;
}

static int __pms_set_cpufreq(int cpu, int kHz)
{
	unsigned long flag = PMS_CPUFREQ_FLAG;
	unsigned long key;
	int err = 0;

	if (__pms_get_cpufreq(cpu) == kHz)
		goto quit;

	/* describe the request */
	writel_relaxed(IPC_CMD_CPU_FS, pms_ipc_base + MAILBOX(0) + REG(0));
	writel_relaxed(kHz,            pms_ipc_base + MAILBOX(0) + REG(1));
	writel_relaxed(flag,           pms_ipc_base + MAILBOX(0) + REG(5));
	writel_relaxed(key = jiffies,  pms_ipc_base + MAILBOX(0) + REG(7));

	/* request and wait for response */
	err = pms_request(key, flag);
quit:
	return err;
}

static int pms_set_cpufreq(int cpu, int kHz)
{
	int err;

	pms_mutex_lock();
	err = __pms_set_cpufreq(cpu, kHz);
	pms_mutex_unlock();

	return err;
}

/*
 * CPU voltage scaling
 */

static inline int __pms_get_cpuvolt(int cpu)
{
	return readl_relaxed(pms_ipc_base + MAILBOX(1) + REG(2));
}

static int pms_get_cpuvolt(int cpu)
{
	int uV;

	pms_mutex_lock();
	uV = __pms_get_cpuvolt(cpu);
	pms_mutex_unlock();

	return uV;
}

static int __pms_set_cpuvolt(int cpu, int uV)
{
	unsigned long flag = PMS_CPUVOLT_FLAG;
	unsigned long key;
	int err = 0;

	if (__pms_get_cpuvolt(cpu) == uV)
		goto quit;

	/* describe the request */
	writel_relaxed(IPC_CMD_CPU_VS, pms_ipc_base + MAILBOX(0) + REG(0));
	writel_relaxed(uV,             pms_ipc_base + MAILBOX(0) + REG(2));
	writel_relaxed(flag,           pms_ipc_base + MAILBOX(0) + REG(5));
	writel_relaxed(key = jiffies,  pms_ipc_base + MAILBOX(0) + REG(7));

	/* request and wait for response */
	err = pms_request(key, flag);
quit:
	return err;
}

static int pms_set_cpuvolt(int cpu, int uV)
{
	int err;

	pms_mutex_lock();
	err = __pms_set_cpuvolt(cpu, uV);
	pms_mutex_unlock();

	return err;
}

/*
 * secondary CPU power gating
 */

#define BITMAP_CLRBIT(bitmap,bit)	((bitmap) & ~(1 << (bit)))
#define BITMAP_GETBIT(bitmap,bit)	(((bitmap) >> (bit)) & 1)
#define BITMAP_SETBIT(bitmap,bit)	((bitmap) | (1 << (bit)))

static inline unsigned long __pms_get_hotplug_bitmap(void)
{
	return readl_relaxed(pms_ipc_base + MAILBOX(1) + REG(3));
}

static inline int __pms_get_hotplug(int cpu)
{
	return BITMAP_GETBIT(__pms_get_hotplug_bitmap(), cpu);
}

static int pms_get_hotplug(int cpu)
{
	int on;

	pms_mutex_lock();
	on = __pms_get_hotplug(cpu);
	pms_mutex_unlock();

	return on;
}

static int __pms_set_hotplug(int cpu, int on)
{
	unsigned long flag = PMS_HOTPLUG_FLAG;
	unsigned long bitmap;
	unsigned long key;
	int err = 0;

	if (cpu == 0)
		return on ? 0 : -EINVAL;

	if (__pms_get_hotplug(cpu) == on)
		goto quit;

	bitmap = on ? BITMAP_SETBIT(__pms_get_hotplug_bitmap(), cpu) :
	              BITMAP_CLRBIT(__pms_get_hotplug_bitmap(), cpu);

	/* describe the request */
	writel_relaxed(IPC_CMD_CPU_PM, pms_ipc_base + MAILBOX(0) + REG(0));
	writel_relaxed(bitmap,         pms_ipc_base + MAILBOX(0) + REG(3));
	writel_relaxed(flag,           pms_ipc_base + MAILBOX(0) + REG(5));
	writel_relaxed(key = jiffies,  pms_ipc_base + MAILBOX(0) + REG(7));

	/* request and wait for response */
	err = pms_request(key, flag);
quit:
	return err;
}

static int pms_set_hotplug(int cpu, int on)
{
	int err;

	pms_mutex_lock();
	err = __pms_set_hotplug(cpu, on);
	pms_mutex_unlock();

	return err;
}

static struct lg1k_pms_ops const lg1156_pms_ops = {
	.get_cpufreq	= pms_get_cpufreq,
	.set_cpufreq	= pms_set_cpufreq,
	.get_cpuvolt	= pms_get_cpuvolt,
	.set_cpuvolt	= pms_set_cpuvolt,
	.get_hotplug	= pms_get_hotplug,
	.set_hotplug	= pms_set_hotplug,
};

static inline void pms_ipc_init(void)
{
	int r;

	/* clear IPC region for messages and mutex */
	for (r = 0; r < 8; r++) {
		writel_relaxed(0x00, pms_ipc_base + MAILBOX(0) + REG(r));
		writel_relaxed(0x00, pms_ipc_base + MAILBOX(2) + REG(r));
		writel_relaxed(0x00, pms_ipc_base + MAILBOX(6) + REG(r));
	}
}

static inline void pms_irq_init(void)
{
	/* clear all interrupts */
	writel_relaxed(0x00, pms_irq_base + M2A_IRQ_CTRL);
	/* disable all interrupts */
	writel_relaxed(0x00, pms_irq_base + M2A_IRQ_MASK);
}

void __init lg115x_init_pms(void)
{
	struct device_node *node;
	struct resource res;

	node = of_find_compatible_node(NULL, NULL, "lge,lg1156-pms");
	if (!node)
		return;

	of_address_to_resource(node, 0, &res);
	request_mem_region(res.start, resource_size(&res), "pms-mem-ipc");
	pms_ipc_base = ioremap(res.start, resource_size(&res));

	of_address_to_resource(node, 1, &res);
	request_mem_region(res.start, resource_size(&res), "pms-mem-irq");
	pms_irq_base = ioremap(res.start, resource_size(&res));

	pms_ipc_init();
	pms_irq_init();

	lg1k_pms_set_ops(&lg1156_pms_ops);
}
