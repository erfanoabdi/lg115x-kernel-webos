#ifndef __ARCH_LG115X_EMMC_H
#define __ARCH_LG115X_EMMC_H

#ifndef CONFIG_MMC_SDHCI_LG115X

static inline void lg115x_init_emmc(void)
{
}

#else	/* CONFIG_MMC_SDHCI_LG115X */

#include <linux/init.h>
#include <linux/amba/bus.h>

#define AMBA_DEVICE(name, busid, prefix, data)    \
AMBA_AHB_DEVICE(name, busid, 0, prefix##_BASE, {prefix##_IRQS}, data)

extern void __init lg115x_init_emmc(void);

#endif	/* CONFIG_MMC_SDHCI_LG115X */

#endif	/* __ARCH_LG115X_EMMC_H */
