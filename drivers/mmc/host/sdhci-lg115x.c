/*
 * drivers/mmc/host/sdhci-lg115x.c
 *
 * Support of SDHCI platform devices for lg115x
 *
 * Copyright (C) 2013 LG Electronics
 *
 * Author: Chanho Min <chanho.min@lge.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/delay.h>

#include "sdhci-pltfm.h"

struct sdhci_lg115x_drv_data {
	unsigned long	tab_reg;
	unsigned int	tab_val;
	unsigned int	tab_reset;
};

static struct sdhci_lg115x_drv_data lg1154_sdhci_drv_data = {
};

static struct sdhci_lg115x_drv_data lg1156_sdhci_drv_data = {
};

static const struct of_device_id sdhci_lg115x_dt_ids[];

static inline struct sdhci_lg115x_drv_data *sdhci_lg115x_get_driver_data(
			struct platform_device *pdev)
{
#ifdef CONFIG_OF
	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		match = of_match_node(sdhci_lg115x_dt_ids, pdev->dev.of_node);
		return (struct sdhci_lg115x_drv_data *)match->data;
	}
#endif
	return (struct sdhci_lg115x_drv_data *)
			platform_get_device_id(pdev)->driver_data;
}

static void sdhci_lg115x_write_tabcontrol(struct sdhci_host *host)
{
	struct platform_device *pdev = to_platform_device(host->mmc->parent);
	struct sdhci_lg115x_drv_data *drv_data;
	void __iomem *baseaddr;

	drv_data = sdhci_lg115x_get_driver_data(pdev);

	if (!drv_data || !drv_data->tab_reg)
		return;

	baseaddr = ioremap_nocache(drv_data->tab_reg, 4);

	writel(drv_data->tab_reset, baseaddr);
	writel(drv_data->tab_val, baseaddr);

	iounmap(baseaddr);
}

#ifdef CONFIG_OF
static void lg1156_platform_init(struct sdhci_host *host)
{
	host->mmc->caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA;
	host->mmc->caps2 |= MMC_CAP2_HS200;
}
#endif

static void lg1154_platform_init(struct sdhci_host *host)
{
	host->quirks2 |= SDHCI_QUIRK2_PRESET_VALUE_BROKEN;
	host->mmc->caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA
				| MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR;
}

static int lg115x_set_uhs_signaling(struct sdhci_host *host, unsigned int uhs)
{
	u16 ctrl_2;

	ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
	if (uhs == MMC_TIMING_MMC_HS200)
		ctrl_2 |= SDHCI_CTRL_HS_SDR200;
	else if (uhs == MMC_TIMING_UHS_SDR12)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
	else if (uhs == MMC_TIMING_UHS_SDR25)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
	else if (uhs == MMC_TIMING_UHS_SDR50)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
	else if (uhs == MMC_TIMING_UHS_SDR104)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
	else if (uhs == MMC_TIMING_UHS_DDR50)
		ctrl_2 |= SDHCI_CTRL_UHS_DDR50 | SDHCI_CTRL_VDD_180;

	sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);
	sdhci_lg115x_write_tabcontrol(host);

	dev_dbg(mmc_dev(host->mmc),
		"%s uhs = %d, ctrl_2 = %04X\n",
		__func__, uhs, ctrl_2);

	return 0;
}

#ifdef CONFIG_OF
static void lg1156_set_clock(struct sdhci_host *host, unsigned int clock)
{
	int div = 0; /* Initialized for compiler warning */
	u16 clk = 0;
	unsigned long timeout;

	if (clock && clock == host->clock)
		return;

	sdhci_writew(host, 0, SDHCI_CLOCK_CONTROL);

	if (clock == 0)
		goto out;

	if (host->max_clk <= clock)
		div = 1;
	else {
		for (div = 2; div < SDHCI_MAX_DIV_SPEC_300;
		     div += 2) {
			if ((host->max_clk / div) <= clock)
				break;
		}
		div >>= 2;
	}

	clk |= (div & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
	clk |= ((div & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN)
		<< SDHCI_DIVIDER_HI_SHIFT;
	clk |= SDHCI_CLOCK_INT_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);

	/* Wait max 20 ms */
	timeout = 20;
	while (!((clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
		& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			pr_err("%s: Internal clock never "
				"stabilised.\n", mmc_hostname(host->mmc));
			return;
		}
		timeout--;
		mdelay(1);
	}

	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
out:
	host->clock = clock;
}
#endif

static struct sdhci_ops lg1154_sdhci_ops = {
	.platform_init = lg1154_platform_init,
	.set_uhs_signaling = lg115x_set_uhs_signaling,
};

#ifdef CONFIG_OF
static struct sdhci_ops lg1156_sdhci_ops = {
	.platform_init = lg1156_platform_init,
	.set_uhs_signaling = lg115x_set_uhs_signaling,
	.set_clock = lg1156_set_clock,
};
#endif

static struct sdhci_pltfm_data sdhci_lg1154_pdata = {
	.ops  = &lg1154_sdhci_ops,
	.quirks = SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
};

#ifdef CONFIG_OF
static struct sdhci_pltfm_data sdhci_lg1156_pdata = {
	.ops  = &lg1156_sdhci_ops,
	.quirks = SDHCI_QUIRK_BROKEN_TIMEOUT_VAL
			| SDHCI_QUIRK_FORCE_BLK_SZ_2048
			| SDHCI_QUIRK_NONSTANDARD_CLOCK
			| SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK,
};
#endif

static int sdhci_lg115x_probe(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	int ret = -1;
	struct device_node *np = pdev->dev.of_node;

	if (of_device_is_compatible(np, "lge,lg1154-sdhci"))
		ret = sdhci_pltfm_register(pdev, &sdhci_lg1154_pdata);
	else if (of_device_is_compatible(np, "lge,lg1156-sdhci"))
		ret = sdhci_pltfm_register(pdev, &sdhci_lg1156_pdata);
	else
		dev_err(&pdev->dev, "Can't find compatible device \n");

	return ret;
#else
	return sdhci_pltfm_register(pdev, &sdhci_lg1154_pdata);
#endif
}

static int sdhci_lg115x_remove(struct platform_device *pdev)
{
	return sdhci_pltfm_unregister(pdev);
}

static struct platform_device_id sdhci_lg115x_driver_ids[] = {
	{
		.name		= "sdhci-lg115x",
		.driver_data	= (kernel_ulong_t)&lg1154_sdhci_drv_data,
	},
	{}
};
MODULE_DEVICE_TABLE(platform, sdhci_lg115x_driver_ids);

static const struct of_device_id sdhci_lg115x_dt_ids[] = {
	{ .compatible = "lge,lg1154-sdhci",
		.data = (void *)&lg1154_sdhci_drv_data },
	{ .compatible = "lge,lg1156-sdhci",
		.data = (void *)&lg1156_sdhci_drv_data },

};
MODULE_DEVICE_TABLE(of, sdhci_lg115x_dt_ids);

static struct platform_driver sdhci_driver = {
	.id_table	= sdhci_lg115x_driver_ids,
	.driver = {
		.name	= "sdhci-lg115x",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sdhci_lg115x_dt_ids),
		.pm	= SDHCI_PLTFM_PMOPS,
	},
	.probe		= sdhci_lg115x_probe,
	.remove		= sdhci_lg115x_remove,
};

module_platform_driver(sdhci_driver);

MODULE_DESCRIPTION("LG115x Secure Digital Host Controller Interface driver");
MODULE_AUTHOR("Chanho Min <chanho.min@lge.com>");
MODULE_LICENSE("GPL v2");
