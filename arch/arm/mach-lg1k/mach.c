#include <linux/kernel.h>

#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clocksource.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/platform_data/emac.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/memblock.h>

#include <mach/lg1k.h>
#include <mach/ion.h>

extern struct smp_operations lg115x_smp_ops __initdata;
extern struct amba_pl011_data uart1_data;
extern struct pl022_ssp_controller spi0_data;
extern struct pl022_ssp_controller spi1_data;

static struct emac_platform_data emac_pdata;

static int __init eth_addr_setup(char *str)
{
	unsigned char addr[ETH_ALEN];
	int i;

	for (i = 0; i < ETH_ALEN; i++) {
		addr[i] = simple_strtoul(str, &str, 16);
		if (*str == ':' || *str == '-')
			str++;
	}
	memcpy(emac_pdata.mac_addr, addr, ETH_ALEN);

	return 0;
}
__setup("ethaddr=", eth_addr_setup);

static int __init lg115x_dt_scan_compat(unsigned long node,
		const char *uname, int depth, void *data)
{
	if (of_flat_dt_is_compatible(node, data))
		return 1;

	return 0;
}

static void __init lg115x_reserve(void)
{
	extern void __init lg115x_reserve_smp(void);

	if (of_scan_flat_dt(lg115x_dt_scan_compat, "arasan,emac")) {
		emac_pdata.desc_phys = arm_memblock_steal(SZ_1M, SZ_1M);
		emac_pdata.desc_size = PAGE_SIZE;
	}
	lg115x_reserve_smp();
}

#define	MAP_DESC_MAX	16

static struct map_desc map_desc[MAP_DESC_MAX] __initdata;
static size_t map_desc_size __initdata;

static unsigned long map_addr __initdata = VMALLOC_END;

static int __init build_map_desc(unsigned long node, char const *uname,
                                 int depth, void *data)
{
	unsigned long phys, size, virt;
	__be32 *prop;

	if (!of_get_flat_dt_prop(node, "static-map", NULL))
		return 0;

	prop = of_get_flat_dt_prop(node, "reg", NULL);
	phys = of_read_number(prop++, 1);
	size = of_read_number(prop, 1);

	if (!of_get_flat_dt_prop(node, "static-map-virt", NULL)) {
		map_addr -= size;
		map_addr &= ~(size - 1);
		virt = map_addr;
	} else {
		virt = phys;
	}

	map_desc[map_desc_size].virtual	= virt;
	map_desc[map_desc_size].pfn	= __phys_to_pfn(phys);
	map_desc[map_desc_size].length	= size;
	map_desc[map_desc_size++].type	= MT_DEVICE;

	return 0;
}

static void __init lg115x_map_io(void)
{
	of_scan_flat_dt(build_map_desc, NULL);
	iotable_init(map_desc, map_desc_size);
}

static void __init lg115x_init_early(void)
{
	extern void __init lg115x_arm_firmware_init(void);
	extern void __init lg115x_init_l2cc(void);

	lg115x_arm_firmware_init();
	lg115x_init_l2cc();
}

static void __init lg115x_init_time(void)
{
	of_clk_init(NULL);
	clocksource_of_init();
}

static struct of_dev_auxdata lg115x_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("arasan,emac", 0xfa000000, NULL, &emac_pdata),
	OF_DEV_AUXDATA("arm,pl011", 0xfe100000, "uart1", NULL),
	OF_DEV_AUXDATA("arm,pl022", 0xfe800000, "spi0", &spi0_data),
	OF_DEV_AUXDATA("arm,pl022", 0xfe900000, "spi1", &spi1_data),
	{}
};

static void __init lg115x_init_machine(void)
{
	extern void __init lg115x_init_pms(void);

	if (of_machine_is_compatible("lge,lg1154"))
		lg115x_auxdata_lookup[1].platform_data = &uart1_data;

	of_platform_populate(NULL, of_default_bus_match_table,
				lg115x_auxdata_lookup, NULL);

	if (!of_machine_is_compatible("lge,lg1156"))
		lg115x_register_ion();

	lg115x_init_pms();
}

static void __init lg115x_init_late(void)
{
	struct platform_device_info cpufreq_info = { .name = "cpufreq-cpu0", };
	extern void __init lg115x_init_spi(void);

	platform_device_register_full(&cpufreq_info);
	lg115x_init_spi();
}

static void lg115x_restart(char mode, char const *cmd)
{
	struct device_node *np;
	void __iomem *base;

	if (!(np = of_find_compatible_node(NULL, NULL, "arm,sp805"))) {
		printk("%s: failed to find SP805 device\n", __func__);
		return;
	}
	if (!(base = of_iomap(np, 0))) {
		printk("%s: failed to find SP805 address\n", __func__);
		return;
	}

	writel_relaxed(0x00, base + 0x0008);
	writel_relaxed(0x00, base + 0x0000);
	writel_relaxed(0x03, base + 0x0008);
}

static char const *const lg115x_dt_compat[] __initconst = {
	"lge,lg1156",
	"lge,lg1154",
	"lge,lg1311",
	NULL
};

DT_MACHINE_START(LG115X, "LG Electronics DTV SoC")
	.atag_offset	= 0x0100,
	.dt_compat	= lg115x_dt_compat,
	.smp		= smp_ops(lg115x_smp_ops),
	.reserve	= lg115x_reserve,
	.map_io		= lg115x_map_io,
	.init_early	= lg115x_init_early,
	.init_irq	= irqchip_init,
	.init_time	= lg115x_init_time,
	.init_machine	= lg115x_init_machine,
	.init_late	= lg115x_init_late,
	.restart	= lg115x_restart,
MACHINE_END
