#ifndef __LINUX_LG1K_CLK_H
#define __LINUX_LG1K_CLK_H

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/compiler.h>
#include <linux/init.h>

struct clk_lg115x {
	struct clk_hw hw;
	void __iomem *chip_base;
	unsigned long npc_fix;
	unsigned long nsc_fix;
};

extern struct clk *clk_register_lg1154(struct device *dev,
                                       char const *name,
                                       char const *parent_name,
                                       unsigned long flags,
                                       void __iomem *chip_base,
                                       unsigned long npc_fix,
                                       unsigned long nsc_fix);

extern struct clk *clk_register_lg1156(struct device *dev,
                                       char const *name,
                                       char const *parent_name,
                                       unsigned long flags,
                                       void __iomem *chip_base,
                                       unsigned long npc_fix,
                                       unsigned long nsc_fix);

#ifndef	CONFIG_OF

extern void __init clk_lg1154_init(void __iomem *base);
extern void __init clk_lg1156_init(void __iomem *base);

#endif	/* CONFIG_OF */

#endif	/* __LINUX_LG1K_CLK_H */
