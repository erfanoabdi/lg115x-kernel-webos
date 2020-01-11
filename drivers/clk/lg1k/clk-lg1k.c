#include <linux/kernel.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/lg1k/clk.h>
#include <linux/lg1k/pms.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define to_clk_lg115x(_hw)	container_of(_hw, struct clk_lg115x, hw)

#pragma	pack(push, 1)

struct div_ctrl {
	unsigned long	/* reserved */	: 18;
	unsigned long	sel		:  2;
	unsigned long	/* reserved */	: 12;
};

struct mux_ctrl {
	unsigned long	/* reserved */	: 10;
	unsigned long	sel		:  1;
	unsigned long	/* reserved */	: 21;
};

struct pll_ctrl {
	unsigned long	/* reserved */	: 19;
	unsigned long	m		:  5;
	unsigned long	pre_fd		:  2;
	unsigned long	od_fout		:  2;
	unsigned long	/* reserved */	:  4;

	unsigned long	/* reserved */	: 14;
	unsigned long	nsc		:  4;
	unsigned long	npc		:  6;
	unsigned long	/* reserved */	:  8;
};

#pragma	pack(pop)

#define CTR(n)	((n) << 2)

static unsigned long clk_lg115x_pll_rate(struct clk_hw *hw, off_t offset,
                                         unsigned long parent_rate)
{
	struct clk_lg115x const *clk = to_clk_lg115x(hw);
	struct pll_ctrl __iomem *pll = clk->chip_base + CTR(offset);
	unsigned long npc = pll->npc ^ clk->npc_fix;
	unsigned long nsc = pll->nsc ^ clk->nsc_fix;
	return parent_rate * (((npc << 2) + nsc) << pll->pre_fd) /
	       ((pll->m + 1) << pll->od_fout);
}

static unsigned long clk_lg1154_recalc_rate(struct clk_hw *hw,
                                            unsigned long parent_rate)
{
	struct clk_lg115x const *clk = to_clk_lg115x(hw);
	struct div_ctrl __iomem *div = clk->chip_base + CTR(28);
	return clk_lg115x_pll_rate(hw, 74, parent_rate) >> (div->sel + 1);
}

static unsigned long clk_lg1156_recalc_rate(struct clk_hw *hw,
                                            unsigned long parent_rate)
{
	return clk_lg115x_pll_rate(hw, 74, parent_rate);
}

static unsigned long clk_lg1311_recalc_rate(struct clk_hw *hw,
                                            unsigned long parent_rate)
{
	struct clk_lg115x const *clk = to_clk_lg115x(hw);
	struct div_ctrl __iomem *div = clk->chip_base + CTR(28);
	return clk_lg115x_pll_rate(hw, 0, parent_rate) >> (div->sel + 1);
}

static long clk_lg115x_round_rate(struct clk_hw *hw, unsigned long rate,
                                  unsigned long *parent_rate)
{
	return rate;
}

static int clk_lg115x_set_rate(struct clk_hw *hw, unsigned long rate,
                               unsigned long parent_rate)
{
	return -ENOSYS;
}

static int clk_lg1156_set_rate(struct clk_hw *hw, unsigned long rate,
                               unsigned long parent_rate)
{
	struct clk_lg115x const *clk = to_clk_lg115x(hw);
	unsigned long ctrl, flags;
	int err;

	/*
	 * NOTE: LG1156 needs disabling both local interrupts and preemption
	 * to minimize intermediate duration where CPU is operating in 24Mhz
	 */
	local_irq_save(flags);
	preempt_disable();

	/* switch to constant 24MHz XTAL */
	ctrl = readl_relaxed(clk->chip_base + CTR(28));
	set_bit(10, &ctrl);
	writel_relaxed(ctrl, clk->chip_base + CTR(28));

	/* scale CPU-PLL frequency via PMS */
	err = lg1k_pms_set_cpufreq(0, rate / 1000);

	/* switch back to CPU-PLL */
	ctrl = readl_relaxed(clk->chip_base + CTR(28));
	clear_bit(10, &ctrl);
	writel_relaxed(ctrl, clk->chip_base + CTR(28));
	
	preempt_enable();
	local_irq_restore(flags);

	return err;
}

static struct clk_ops const clk_lg1154_ops = {
	.recalc_rate	= clk_lg1154_recalc_rate,
	.round_rate	= clk_lg115x_round_rate,
	.set_rate	= clk_lg115x_set_rate,
};

static struct clk_ops const clk_lg1156_ops = {
	.recalc_rate	= clk_lg1156_recalc_rate,
	.round_rate	= clk_lg115x_round_rate,
	.set_rate	= clk_lg1156_set_rate,
};

static struct clk_ops const clk_lg1311_ops = {
	.recalc_rate	= clk_lg1311_recalc_rate,
	.round_rate	= clk_lg115x_round_rate,
	.set_rate	= clk_lg115x_set_rate,
};

static struct clk *clk_register_lg115x(struct device *dev, char const *name,
				char const *parent_name, unsigned long flags,
				struct clk_ops const *ops, void __iomem *base,
				unsigned long npc_fix, unsigned long nsc_fix)
{
	struct clk_lg115x *clk_lg115x;
	struct clk_init_data init_data;
	struct clk *clk;

	if (!(clk_lg115x = kmalloc(sizeof(struct clk_lg115x), GFP_KERNEL))) {
		pr_err("%s: could not allocation clk-lg115x\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init_data.name = name;
	init_data.ops = ops;
	init_data.flags = flags | CLK_IS_BASIC;
	init_data.parent_names = &parent_name;
	init_data.num_parents = 1;

	clk_lg115x->hw.init = &init_data;
	clk_lg115x->chip_base = base;
	clk_lg115x->npc_fix = npc_fix;
	clk_lg115x->nsc_fix = nsc_fix;

	clk = clk_register(dev, &clk_lg115x->hw);
	if (IS_ERR(clk))
		kfree(clk_lg115x);

	return clk;
}

struct clk *clk_register_lg1154(struct device *dev, char const *name,
                                char const *parent_name, unsigned long flags,
                                void __iomem *base, unsigned long npc_fix,
                                unsigned long nsc_fix)
{
	return clk_register_lg115x(dev, name, parent_name, flags,
	                           &clk_lg1154_ops, base, npc_fix, nsc_fix);
}

struct clk *clk_register_lg1156(struct device *dev, char const *name,
                                char const *parent_name, unsigned long flags,
                                void __iomem *base, unsigned long npc_fix,
                                unsigned long nsc_fix)
{
	return clk_register_lg115x(dev, name, parent_name, flags,
	                           &clk_lg1156_ops, base, npc_fix, nsc_fix);
}

struct clk *clk_register_lg1311(struct device *dev, char const *name,
                                char const *parent_name, unsigned long flags,
                                void __iomem *base, unsigned long npc_fix,
                                unsigned long nsc_fix)
{
	return clk_register_lg115x(dev, name, parent_name, flags,
	                           &clk_lg1311_ops, base, npc_fix, nsc_fix);
}

#ifdef	CONFIG_OF

static void __init clk_lg1154_of_setup(struct device_node *np)
{
	struct clk *clk;
	char const *clk_name = np->name;
	char const *parent_name;
	struct device_node *chip_node;
	void __iomem *chip_base;
	unsigned long npc_fix = 0;
	unsigned long nsc_fix = 0;

	of_property_read_string(np, "clock-output-names", &clk_name);
	parent_name = of_clk_get_parent_name(np, 0);
	of_property_read_u32(np, "npc-fix", (u32 *)&npc_fix);
	of_property_read_u32(np, "nsc-fix", (u32 *)&nsc_fix);

	chip_node = of_find_node_by_name(NULL, "chip_ctrl");
	chip_base = of_iomap(chip_node, 0);
	
	if (!parent_name) {
		pr_err("%s: could not find parent clock name\n", __func__);
		return;
	}
	if (!chip_base) {
		pr_err("%s: could not find control address\n", __func__);
		return;
	}

	clk = clk_register_lg1154(NULL, clk_name, parent_name,
	                          CLK_GET_RATE_NOCACHE, chip_base,
	                          npc_fix, nsc_fix);
	if (!IS_ERR(clk))
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(clk_lg1154, "lge,lg1154-clock", clk_lg1154_of_setup);

static void __init clk_lg1156_of_setup(struct device_node *np)
{
	struct clk *clk;
	char const *clk_name = np->name;
	char const *parent_name;
	struct device_node *chip_node;
	void __iomem *chip_base;
	unsigned long npc_fix = 0;
	unsigned long nsc_fix = 0;

	of_property_read_string(np, "clock-output-names", &clk_name);
	parent_name = of_clk_get_parent_name(np, 0);
	of_property_read_u32(np, "npc-fix", (u32 *)&npc_fix);
	of_property_read_u32(np, "nsc-fix", (u32 *)&nsc_fix);

	chip_node = of_find_node_by_name(NULL, "chip_ctrl");
	chip_base = of_iomap(chip_node, 0);
	
	if (!parent_name) {
		pr_err("%s: could not find parent clock name\n", __func__);
		return;
	}
	if (!chip_base) {
		pr_err("%s: could not find control address\n", __func__);
		return;
	}

	clk = clk_register_lg1156(NULL, clk_name, parent_name,
	                          CLK_GET_RATE_NOCACHE, chip_base,
	                          npc_fix, nsc_fix);
	if (!IS_ERR(clk))
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(clk_lg1156, "lge,lg1156-clock", clk_lg1156_of_setup);

static void __init clk_lg1311_of_setup(struct device_node *np)
{
	struct clk *clk;
	char const *clk_name = np->name;
	char const *parent_name;
	struct device_node *chip_node;
	void __iomem *chip_base;
	unsigned long npc_fix = 0;
	unsigned long nsc_fix = 0;

	of_property_read_string(np, "clock-output-names", &clk_name);
	parent_name = of_clk_get_parent_name(np, 0);
	of_property_read_u32(np, "npc-fix", (u32 *)&npc_fix);
	of_property_read_u32(np, "nsc-fix", (u32 *)&nsc_fix);

	chip_node = of_find_node_by_name(NULL, "chip_ctrl");
	chip_base = of_iomap(chip_node, 0);
	
	if (!parent_name) {
		pr_err("%s: could not find parent clock name\n", __func__);
		return;
	}
	if (!chip_base) {
		pr_err("%s: could not find control address\n", __func__);
		return;
	}

	clk = clk_register_lg1311(NULL, clk_name, parent_name,
	                          CLK_GET_RATE_NOCACHE, chip_base,
	                          npc_fix, nsc_fix);
	if (!IS_ERR(clk))
		of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(clk_lg1311, "lge,lg1311-clock", clk_lg1311_of_setup);

#else	/* CONFIG_OF */

#define BUSCLK_RATE	AMBA_CLOCK
#define XTAL_RATE	24000000

#define	LG1154_NPC_FIX	0x0002
#define	LG1154_NSC_FIX	0x0002

#define LG1156_NPC_FIX	0x0002
#define LG1156_NSC_FIX	0x0000

static void __init clk_lg115x_init_common(void __iomem *base)
{
	/* root 24MHz XTAL */
	clk_register_fixed_rate(NULL, "XTAL", NULL, CLK_IS_ROOT, XTAL_RATE);
	/* BUSCLK is simplified as a fixed-rate clock */
	clk_register_fixed_rate(NULL, "BUSCLK", NULL, CLK_IS_ROOT, BUSCLK_RATE);
}

void __init clk_lg1154_init(void __iomem *base)
{
	if (base) {
		clk_lg115x_init_common(base);
		clk_register_lg1154(NULL, "CLK", "XTAL", CLK_GET_RATE_NOCACHE,
		                    base, LG1154_NPC_FIX, LG1154_NSC_FIX);

		/* PERIPHCLK is set to half of CLK */
		clk_register_fixed_factor(NULL, "PERIPHCLK", "CLK",
					  CLK_GET_RATE_NOCACHE, 1, 2);
	}
}

void __init clk_lg1156_init(void __iomem *base)
{
	if (base) {
		clk_lg115x_init_common(base);
		clk_register_lg1156(NULL, "CLK", "XTAL", CLK_GET_RATE_NOCACHE,
		                    base, LG1156_NPC_FIX, LG1156_NSC_FIX);
	}
}

void __init clk_lg1311_init(void __iomem *base)
{
	if (base) {
		clk_lg115x_init_common(base);
		clk_register_lg1311(NULL, "CLK", "XTAL", CLK_GET_RATE_NOCACHE,
		                    base, LG1154_NPC_FIX, LG1154_NSC_FIX);

		/* PERIPHCLK is set to half of CLK */
		clk_register_fixed_factor(NULL, "PERIPHCLK", "CLK",
					  CLK_GET_RATE_NOCACHE, 1, 2);
	}
}

#endif	/* CONFIG_OF */
