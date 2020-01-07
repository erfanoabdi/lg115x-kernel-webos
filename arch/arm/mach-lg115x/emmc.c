#include <linux/kernel.h>

#include <linux/amba/bus.h>

#include <asm-generic/sizes.h>

#include <mach/resource.h>

#include "emmc.h"

static AMBA_DEVICE(_sdhci, "sdhci-lg115x", SDHCI, NULL);

void __init lg115x_init_emmc(void)
{
	_sdhci_device.periphid = 0x00000670;

	amba_device_register(&_sdhci_device, &iomem_resource);
}

