/*
 * LG115X MMC/SD/SDIO driver
 *
 *  Originally based on code by:
 * Copyright (C) Giuseppe Cavallaro <peppe.cavallaro@xxxxxx>
 *
 *	Modified for lg115x
 * by Chanho Min (chanho.min@lge.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mbus.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/irq.h>
#include <linux/highmem.h>
#include <linux/mmc/host.h>
#include <linux/amba/bus.h>
#include <linux/slab.h>

#include <asm/sizes.h>
#include <asm/unaligned.h>

#include "sdhci-lg115x.h"

/* Macro Definitions. */
#ifndef __devinit
# define __devinit
#endif
#ifndef __devexit
# define __devexit
#endif
#ifndef __devexit_p
# define __devexit_p
#endif

/* To enable more debug information. */
#undef SDHCI_LG115X_DEBUG
//#define SDHCI_LG115X_DEBUG
#ifdef SDHCI_LG115X_DEBUG
#define DBG(fmt, args...)  pr_info(fmt, ## args)
#else
#define DBG(fmt, args...)  do { } while (0)
#endif

#define LG115X_MMC_TABCONROL_REG 	0xFD300288
static int maxfreq = SDHCI_LG115X_CLOCKRATE_MAX;
module_param(maxfreq, int, S_IRUGO);
MODULE_PARM_DESC(maxfreq, "Maximum card clock frequency ");

static unsigned int adma = 1;
module_param(adma, int, S_IRUGO);
MODULE_PARM_DESC(adma, "Disable/Enable the Advanced DMA mode");

static unsigned int led = 0;
module_param(led, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(led, "Enable|Disable LED");

static unsigned int pio = 0;
module_param(pio, int, S_IRUGO);
MODULE_PARM_DESC(pio, "PIO mode (no DMA)");

static unsigned int emmc_tab = 0;
module_param(emmc_tab, int, S_IRUGO);
MODULE_PARM_DESC(emmc_tab, "mmc_ddr");

static unsigned int emmc_slow = 0;
module_param(emmc_slow, int, S_IRUGO);
MODULE_PARM_DESC(emmc_slow, "mmc_slow");

struct sdhci_lg115x_cap {
	unsigned int timer_freq;
	unsigned int timer_unit;
	unsigned int base_clk_sd;
	unsigned int max_blk_len;
	unsigned int adma2;
	unsigned int high_speed;
	unsigned int sdma;
	unsigned int suspend;
	unsigned int voltage33;
	unsigned int voltage30;
	unsigned int voltage18;
	unsigned int int_mode;
	unsigned int spi;
	unsigned int spi_block;
};

struct sdhci_lg115x_host {
	void __iomem *base;
	struct mmc_request *mrq;
	unsigned int intr_en;
	u8 ctrl;
	unsigned int sg_frags;
	struct timer_list timer;
	struct mmc_host *mmc;
	struct device *dev;
	struct resource *res;
	int irq;
	struct sdhci_lg115x_cap cap;
	u8 vdd;
	unsigned int freq;
	unsigned int status;
	unsigned int adma;
	unsigned int use_pio;
	u16 pio_blksz;
	u32 pio_blocks;
	u32 *pio_blkbuf;
	spinlock_t lock;
	struct tasklet_struct card_tasklet;
	u8 *adma_desc;
	dma_addr_t adma_addr;
	int need_poll;
};

static inline void sdhci_lg115x_sw_reset(struct sdhci_lg115x_host *host, unsigned int flag)
{
	/* After completing the reset, wait the HC clears these bits */
	if (likely(flag == reset_all)) {
		writeb(SDHCI_LG115X_RESET_ALL, host->base + SDHCI_LG115X_SW_RESET);
		do { } while ((readb(host->base + SDHCI_LG115X_SW_RESET)) &
			 SDHCI_LG115X_RESET_ALL);
	} else if (flag == reset_cmd_line) {
		writeb(SDHCI_LG115X_RESET_CMD_LINE, host->base + SDHCI_LG115X_SW_RESET);
		do { } while ((readb(host->base + SDHCI_LG115X_SW_RESET)) &
			 SDHCI_LG115X_RESET_CMD_LINE);

	} else if (flag == reset_dat_line) {
		writeb(SDHCI_LG115X_RESET_DAT_LINE, host->base + SDHCI_LG115X_SW_RESET);
		do { } while ((readb(host->base + SDHCI_LG115X_SW_RESET)) &
			 SDHCI_LG115X_RESET_DAT_LINE);
	}
}

static inline void sdhci_lg115x_hc_version(struct sdhci_lg115x_host *host)
{
	u16 version;

	version = readw(host->base + SDHCI_LG115X_HOST_VERSION);
	pr_info("LG115X MMC/SDIO:\n\tHC Vendor Version Number: %d\n",
		 (version >> 8));
	pr_info("\tHC SPEC Version Number: %d\n", (version & 0x00ff));
}

#ifdef SDHCI_LG115X_DEBUG

static void sdhci_lg115x_dump_register(struct sdhci_lg115x_host *host)
{
	u8 data;
	int i;
	printk("sdhci_lg115x_dump_register: \n");
	for(i = 0; i < 0x50; i++){
		data = readb(host->base + i);
		printk("%02x ",data);
		if((i>0xD) && !((i+1)%0x10)){
			printk("\n");
		}
	}

	printk("ctop register: \n");
	for(i = 0; i < 0x10; i++){
		data = readb(0xfd300288 + i);
		printk("%02x ",data);
	}
}

static void sdhci_lg115x_dump_capabilities(struct sdhci_lg115x_host *host)
{
	pr_info("\tTimeout Clock Freq: %d %s\n", host->cap.timer_freq,
		 host->cap.timer_unit ? "MHz" : "KHz");

	pr_info("\tBase Clock Freq for SD: %d MHz\n", host->cap.base_clk_sd);

	pr_info("\tMax Block size: %d bytes\n", host->cap.max_blk_len);

	pr_info("\tadma2 %s, high speed %s, sdma %s\n",
		 host->cap.adma2 ? "Yes" : "Not",
		 host->cap.high_speed ? "Yes" : "Not",
		 host->cap.sdma ? "Yes" : "Not");

	pr_info("\tsuspend/resume %s suported\n",
		 host->cap.adma2 ? "is" : "Not");

	if (host->cap.voltage33)
		pr_info("\t3.3V voltage suported\n");
	if (host->cap.voltage30)
		pr_info("\t3.0V voltage suported\n");
	if (host->cap.voltage18)
		pr_info("\t1.8V voltage suported\n");

	if (host->cap.int_mode)
		pr_info("\tInterrupt Mode supported\n");
	if (host->cap.spi)
		pr_info("\tSPI Mode supported\n");
	if (host->cap.spi_block)
		pr_info("\tSPI Block Mode supported\n");
}
#endif

static void sdhci_lg115x_reset_tabcontrol(void)
{
	unsigned int cap;
	unsigned int *baseaddr;

	baseaddr = ioremap_nocache(LG115X_MMC_TABCONROL_REG, 4);

	cap =  0x210411;
	writel(cap, baseaddr);

	iounmap(baseaddr);
}

static int sdhci_lg115x_write_tabcontrol(void)
{
	unsigned int cap;
	unsigned int *baseaddr;

	baseaddr = ioremap_nocache(LG115X_MMC_TABCONROL_REG, 4);

	cap = emmc_tab;
	writel(cap, baseaddr);

	iounmap(baseaddr);

	return 0;
}

static void sdhci_lg115x_capabilities(struct sdhci_lg115x_host *host)
{
	unsigned int cap;
	unsigned int max_blk_len;

	cap = readl(host->base + SDHCI_LG115X_CAPABILITIES);

	pr_debug("\tLG115X capabilities: 0x%x\n", cap);

	host->cap.timer_freq = cap & 0x3f;
	host->cap.timer_unit = (cap >> 7) & 0x1;
	host->cap.base_clk_sd = (cap >> 8) & 0x3f;

	max_blk_len = (cap >> 16) & 0x3;
	switch (max_blk_len) {
	case 0:
		host->cap.max_blk_len = 512;
		break;
	case 1:
		host->cap.max_blk_len = 1024;
		break;
	case 2:
		host->cap.max_blk_len = 2048;
		break;
	case 3:
		host->cap.max_blk_len = 4096;
		break;
	default:
		break;
	}

	host->cap.adma2 = (cap >> 19) & 0x1;
	// hankyung add debug option
	if (emmc_slow == 0)
		host->cap.high_speed = (cap >> 21) & 0x1;

	host->cap.sdma = (cap >> 22) & 0x1;

	host->cap.suspend = (cap >> 23) & 0x1;

	/* Disable adma user option if cap not supported. */
	if (!host->cap.adma2)
		adma = 0;

	host->cap.voltage33 = (cap >> 24) & 0x1;
	host->cap.voltage30 = (cap >> 25) & 0x1;
	host->cap.voltage18 = (cap >> 26) & 0x1;
	host->cap.int_mode = (cap >> 27) & 0x1;
	host->cap.spi = (cap >> 29) & 0x1;
	host->cap.spi_block = (cap >> 30) & 0x1;
#ifdef SDHCI_LG115X_DEBUG
	sdhci_lg115x_dump_capabilities(host);
#endif
}

static void sdhci_lg115x_ctrl_led(struct sdhci_lg115x_host *host, unsigned int flag)
{
	if (led) {
		u8 ctrl_reg = readb(host->base + SDHCI_LG115X_HOST_CTRL);

		if (flag)
			ctrl_reg |= SDHCI_LG115X_HOST_CTRL_LED;
		else
			ctrl_reg &= ~SDHCI_LG115X_HOST_CTRL_LED;

		host->ctrl = ctrl_reg;
		writeb(host->ctrl, host->base + SDHCI_LG115X_HOST_CTRL);
	}
}

static inline void sdhci_lg115x_set_interrupts(struct sdhci_lg115x_host *host)
{
	host->intr_en = SDHCI_LG115X_IRQ_DEFAULT_MASK;
	writel(host->intr_en, host->base + SDHCI_LG115X_NORMAL_INT_STATUS_EN);
	writel(host->intr_en, host->base + SDHCI_LG115X_NORMAL_INT_SIGN_EN);
}

static inline void sdhci_lg115x_clear_interrupts(struct sdhci_lg115x_host *host)
{
	writel(0, host->base + SDHCI_LG115X_NORMAL_INT_STATUS_EN);
	writel(0, host->base + SDHCI_LG115X_ERR_INT_STATUS_EN);
	writel(0, host->base + SDHCI_LG115X_NORMAL_INT_SIGN_EN);
}


static void sdhci_lg115x_power_set(struct sdhci_lg115x_host *host, unsigned int pwr, u8 vdd)
{
	u8 pwr_reg;

	pwr_reg = readb(host->base + SDHCI_LG115X_PWR_CTRL);

	host->vdd = (1 << vdd);

	writeb(0, host->base + SDHCI_LG115X_PWR_CTRL);

	if (pwr) {
		pwr_reg &= 0xf1;

		if ((host->vdd & MMC_VDD_165_195) && host->cap.voltage18)
			pwr_reg |= SDHCI_LG115X_PWR_BUS_VOLTAGE_18;
		else if ((host->vdd & MMC_VDD_29_30) && host->cap.voltage30)
			pwr_reg |= SDHCI_LG115X_PWR_BUS_VOLTAGE_30;
		else if ((host->vdd & MMC_VDD_32_33) && host->cap.voltage33)
			pwr_reg |= SDHCI_LG115X_PWR_BUS_VOLTAGE_33;
	} else
		pwr_reg &= ~SDHCI_LG115X_PWR_CTRL_UP;

	DBG("%s: pwr_reg 0x%x, host->vdd = 0x%x\n", __func__, pwr_reg,
	    host->vdd);
	writeb(pwr_reg, host->base + SDHCI_LG115X_PWR_CTRL);
}

static int sdhci_lg115x_test_card(struct sdhci_lg115x_host *host)
{
	unsigned int ret = 0;
	u32 present = readl(host->base + SDHCI_LG115X_PRESENT_STATE);
	if (likely(!(present & SDHCI_LG115X_PRESENT_STATE_CARD_PRESENT)))
		ret = -1;

#ifdef SDHCI_LG115X_DEBUG
	if (present & SDHCI_LG115X_PRESENT_STATE_CARD_STABLE)
		pr_info("\tcard stable...");
	if (!(present & SDHCI_LG115X_PRESENT_STATE_WR_EN))
		pr_info("\tcard Write protected...");
	if (present & SDHCI_LG115X_PRESENT_STATE_BUFFER_RD_EN)
		pr_info("\tPIO Read Enable...");
	if (present & SDHCI_LG115X_PRESENT_STATE_BUFFER_WR_EN)
		pr_info("\tPIO Write Enable...");
	if (present & SDHCI_LG115X_PRESENT_STATE_RD_ACTIVE)
		pr_info("\tRead Xfer data...");
	if (present & SDHCI_LG115X_PRESENT_STATE_WR_ACTIVE)
		pr_info("\tWrite Xfer data...");
	if (present & SDHCI_LG115X_PRESENT_STATE_DAT_ACTIVE)
		pr_info("\tDAT line active...");
#endif
	return ret;
}

static void sdhci_lg115x_set_clock(struct sdhci_lg115x_host *host, unsigned int freq)
{
	u16 clock = 0;
	unsigned long flags;
	int count = 0;
	spin_lock_irqsave(&host->lock, flags);

	// hankyung add debug option
	if (emmc_slow > 0 && freq >= 1000000)
		freq = emmc_slow;

	if ((host->freq != freq) && (freq)) {
		u16 divisor;

		/* Ensure clock is off before making any changes */
		writew(clock, host->base + SDHCI_LG115X_CLOCK_CTRL);
		/* core checks if this is a good freq < max_freq */
		host->freq = freq;

		DBG("%s:\n\tnew freq %d", __func__, host->freq);

		/*
		 * 10bit div add 2012/09/03 by Hankyung Yu
		 */
		/* Work out divisor for specified clock frequency */
		for (divisor = 1; divisor <= 0x3FF; divisor *=2)
			/* Find first divisor producing a frequency less
			 * than or equal to MHz */
			if ((maxfreq / (divisor * 2)) <= freq)
				break;

		DBG("\tdivisor %d", divisor);
		/* Set the clock divisor and enable the internal clock */
		clock = (divisor & 0xFF) << (SDHCI_LG115X_CLOCK_CTRL_SDCLK_SHIFT);
		clock += (((divisor & 0x300) >> SDHCI_LG115X_CLOCK_CTRL_SDCLK_SHIFT) << (6));
		clock |= SDHCI_LG115X_CLOCK_CTRL_ICLK_ENABLE;

		writew(clock, host->base + SDHCI_LG115X_CLOCK_CTRL);

		/* Busy wait for the clock to become stable */
		do { /*barrier(); msleep(1); */count++;} while (((readw(host->base + SDHCI_LG115X_CLOCK_CTRL)) &
			  SDHCI_LG115X_CLOCK_CTRL_ICLK_STABLE) == 0);

		/* Enable the SD clock */
		clock |= SDHCI_LG115X_CLOCK_CTRL_SDCLK_ENABLE;
		writew(clock, host->base + SDHCI_LG115X_CLOCK_CTRL);

		DBG("\tclk ctrl reg. [0x%x]\n",
		    (unsigned int)readw(host->base + SDHCI_LG115X_CLOCK_CTRL));
	}

	spin_unlock_irqrestore(&host->lock, flags);
}

/* Read the response from the card */
static void sdhci_lg115x_get_resp(struct mmc_command *cmd, struct sdhci_lg115x_host *host)
{
	unsigned int i;
	unsigned int resp[4];

	for (i = 0; i < 4; i++)
		resp[i] = readl(host->base + SDHCI_LG115X_RSP(i));

	if (cmd->flags & MMC_RSP_136) {
		cmd->resp[3] = (resp[0] << 8);
		cmd->resp[2] = (resp[0] >> 24) | (resp[1] << 8);
		cmd->resp[1] = (resp[1] >> 24) | (resp[2] << 8);
		cmd->resp[0] = (resp[2] >> 24) | (resp[3] << 8);
	} else {
		cmd->resp[0] = resp[0];
		cmd->resp[1] = resp[1];
	}

	DBG("%s: resp length %s\n-(CMD%u):\n %08x %08x %08x %08x\n"
	    "-RAW reg:\n %08x %08x %08x %08x\n",
	    __func__, (cmd->flags & MMC_RSP_136) ? "136" : "48", cmd->opcode,
	    cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3],
	    resp[0], resp[1], resp[2], resp[3]);
}

static void sdhci_lg115x_read_block_pio(struct sdhci_lg115x_host *host)
{
	unsigned long flags;
	u16 blksz;

	DBG("\tPIO reading\n");

	local_irq_save(flags);

	for (blksz = host->pio_blksz; blksz > 0; blksz -= 4) {
		*host->pio_blkbuf =
		    readl(host->base + SDHCI_LG115X_BUFF);
		host->pio_blkbuf++;
	}

	local_irq_restore(flags);
}

static void sdhci_lg115x_write_block_pio(struct sdhci_lg115x_host *host)
{
	unsigned long flags;
	u16 blksz;

	DBG("\tPIO writing\n");
	local_irq_save(flags);

	for (blksz = host->pio_blksz; blksz > 0; blksz -= 4) {
		writel(*host->pio_blkbuf,
		       host->base + SDHCI_LG115X_BUFF);
		host->pio_blkbuf++;
	}

	local_irq_restore(flags);
}

static void sdhci_lg115x_data_pio(struct sdhci_lg115x_host *host)
{
	if (host->pio_blocks == 0)
		return;

	if (host->status == STATE_DATA_READ) {
		while (readl(host->base + SDHCI_LG115X_PRESENT_STATE) &
			     SDHCI_LG115X_PRESENT_STATE_BUFFER_RD_EN) {

			sdhci_lg115x_read_block_pio(host);

			host->pio_blocks--;
			if (host->pio_blocks == 0)
				break;
		}

	} else {
		while (readl(host->base + SDHCI_LG115X_PRESENT_STATE) &
			     SDHCI_LG115X_PRESENT_STATE_BUFFER_WR_EN) {

			sdhci_lg115x_write_block_pio(host);

			host->pio_blocks--;
			if (host->pio_blocks == 0)
				break;
		}
	}
	DBG("\tPIO transfer complete.\n");
}

static void sdhci_lg115x_start_cmd(struct sdhci_lg115x_host *host, struct mmc_command *cmd)
{
	u16 cmdreg = 0;

	/* Command Request */
	cmdreg = SDHCI_LG115X_CMD_INDEX(cmd->opcode);
	DBG("%s: cmd type %04x,  CMD%d\n", __func__,
	    mmc_resp_type(cmd), cmd->opcode);

	if (cmd->flags & MMC_RSP_BUSY) {
		cmdreg |= SDHCI_LG115X_CMD_RSP_48BUSY;
		DBG("\tResponse length 48 check Busy.\n");
	} else if (cmd->flags & MMC_RSP_136) {
		cmdreg |= SDHCI_LG115X_CMD_RSP_136;
		DBG("\tResponse length 136\n");
	} else if (cmd->flags & MMC_RSP_PRESENT) {
		cmdreg |= SDHCI_LG115X_CMD_RSP_48;
		DBG("\tResponse length 48\n");
	} else {
		cmdreg |= SDHCI_LG115X_CMD_RSP_NONE;
		DBG("\tNo Response\n");
	}

	if (cmd->flags & MMC_RSP_CRC) {
		cmdreg |= SDHCI_LG115X_CMD_CHECK_CMDCRC;
		DBG("\tCheck the CRC field in the response\n");
	}
	if (cmd->flags & MMC_RSP_OPCODE) {
		cmdreg |= SDHCI_LG115X_CMD_INDX_CHECK;
		DBG("\tCheck the Index field in the response\n");
	}

	/* Wait until the CMD line is not in use */
	do { } while ((readl(host->base + SDHCI_LG115X_PRESENT_STATE)) &
		 SDHCI_LG115X_PRESENT_STATE_CMD_INHIBIT);

	/* Set the argument register */
	writel(cmd->arg, host->base + SDHCI_LG115X_ARG);

	/* Data present and must be transferred */
	if (likely(host->mrq->data)) {
		cmdreg |= SDHCI_LG115X_CMD_DATA_PRESENT;
		if (cmd->flags & MMC_RSP_BUSY)
			/* Wait for data inhibit */
			do { } while ((readl(host->base +
					SDHCI_LG115X_PRESENT_STATE)) &
				 SDHCI_LG115X_PRESENT_STATE_DAT_INHIBIT);
	}

	/* Write the Command */
	writew(cmdreg, host->base + SDHCI_LG115X_CMD);

	DBG("\tcmd: 0x%x cmd reg: 0x%x - cmd->arg 0x%x, reg 0x%x\n",
	    cmdreg, readw(host->base + SDHCI_LG115X_CMD), cmd->arg,
	    readl(host->base + SDHCI_LG115X_ARG));
}

#ifdef SDHCI_LG115X_DEBUG
static void sdhci_lg115x_adma_error(struct sdhci_lg115x_host *host)
{
	u8 status = readb(host->base + SDHCI_LG115X_ADMA_ERR_STATUS);

	if (status & SDHCI_LG115X_ADMA_ERROR_LENGTH)
		pr_err("-ADMA Length Mismatch Error...");

	if (status & SDHCI_LG115X_ADMA_ERROR_ST_TFR)
		pr_err("-Transfer Data Error desc: ");
	else if (status & SDHCI_LG115X_ADMA_ERROR_ST_FDS)
		pr_err("-Fetch Data Error desc: ");
	else if (status & SDHCI_LG115X_ADMA_ERROR_ST_STOP)
		pr_err("-Stop DMA Data Error desc: ");

	pr_err("0x%x", readl(host->base + SDHCI_LG115X_ADMA_ADDRESS));
}

static void sdhci_lg115x_adma_dump_desc(u8 *desc)
{
	__le32 *dma;
	__le16 *len;
	u8 attr;

	pr_info("\tDescriptors:");

	while (1) {
		dma = (__le32 *) (desc + 4);
		len = (__le16 *) (desc + 2);
		attr = *desc;

		pr_info("\t\t%p: Buff 0x%08x, len %d, Attr 0x%02x\n",
			desc, le32_to_cpu(*dma), le16_to_cpu(*len), attr);

		desc += 8;

		if (attr & 2)	/* END of descriptor */
			break;
	}
}
#else
static void sdhci_lg115x_adma_error(struct sdhci_lg115x_host *host)
{
}

static void sdhci_lg115x_adma_dump_desc(u8 *desc)
{
}
#endif

static int sdhci_lg115x_init_sg(struct sdhci_lg115x_host *host)
{

	host->adma_desc = kmalloc((SDHCI_LG115X_DMA_DESC_NUM * 2 + 1) * 4,
				  GFP_KERNEL);

	if (unlikely(host->adma_desc == NULL))
		return -ENOMEM;

	return 0;
}

static void sdhci_lg115x_adma_table_pre(struct sdhci_lg115x_host *host,
				  struct mmc_data *data)
{
	int direction, i;
	u8 *desc;
	struct scatterlist *sg;
	int len;
	dma_addr_t addr;

	if (host->status == STATE_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	DBG("\t%s: sg entries %d\n", __func__, data->sg_len);

	host->sg_frags = dma_map_sg(mmc_dev(host->mmc), data->sg,
				    data->sg_len, direction);
	desc = host->adma_desc;

	for_each_sg(data->sg, sg, host->sg_frags, i) {
		addr = sg_dma_address(sg);
		len = sg_dma_len(sg);

		DBG("\t\tFrag %d: addr 0x%x, len %d\n", i, addr, len);

		/* Preparing the descriptor */
		desc[7] = (addr >> 24) & 0xff;
		desc[6] = (addr >> 16) & 0xff;
		desc[5] = (addr >> 8) & 0xff;
		desc[4] = (addr >> 0) & 0xff;

		desc[3] = (len >> 8) & 0xff;
		desc[2] = (len >> 0) & 0xff;

		desc[1] = 0x00;
		desc[0] = 0x21;

		desc += 8;
	}
	desc -= 8;
	desc[0] = 0x23;

	sdhci_lg115x_adma_dump_desc(host->adma_desc);

	host->adma_addr = dma_map_single(mmc_dev(host->mmc),
					 host->adma_desc,
					 (SDHCI_LG115X_DMA_DESC_NUM * 2 + 1) * 4,
					 DMA_TO_DEVICE);

	writel(host->adma_addr, host->base + SDHCI_LG115X_ADMA_ADDRESS);
}

static void sdhci_lg115x_adma_table_post(struct sdhci_lg115x_host *host,
				   struct mmc_data *data)
{
	int direction;

	if (host->status == STATE_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	DBG("\t%s\n", __func__);

	dma_unmap_single(mmc_dev(host->mmc), host->adma_addr,
			 (SDHCI_LG115X_DMA_DESC_NUM * 2 + 1) * 4, DMA_TO_DEVICE);

	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, direction);
}

static int sdhci_lg115x_setup_data(struct sdhci_lg115x_host *host)
{
	u16 blksz;
	u16 xfer = 0;
	struct mmc_data *data = host->mrq->data;

	DBG("%s:\n\t%s mode, data dir: %s; Buff=0x%08x,"
	    "blocks=%d, blksz=%d\n", __func__, host->use_pio ? "PIO" : "DMA",
	    (data->flags & MMC_DATA_READ) ? "read" : "write",
	    (unsigned int)sg_virt(data->sg), data->blocks, data->blksz);

	/* Transfer Direction */
	if (data->flags & MMC_DATA_READ) {
		xfer |= SDHCI_LG115X_XFER_DATA_DIR;
		host->status = STATE_DATA_READ;
	} else {
		xfer &= ~SDHCI_LG115X_XFER_DATA_DIR;
		host->status = STATE_DATA_WRITE;
	}

	xfer |= SDHCI_LG115X_XFER_BLK_COUNT_EN;

	if (data->blocks > 1)
		xfer |= SDHCI_LG115X_XFER_MULTI_BLK | SDHCI_LG115X_XFER_AUTOCMD12;

	/* Set the block size register */
	blksz = SDHCI_LG115X_BLOCK_SIZE_SDMA_512KB;
	blksz |= (data->blksz & SDHCI_LG115X_BLOCK_SIZE_TRANSFER);
	blksz |= (data->blksz & 0x1000) ? SDHCI_LG115X_BLOCK_SIZE_SDMA_8KB : 0;

	writew(blksz, host->base + SDHCI_LG115X_BLK_SIZE);

	/* Set the block count register */
	writew(data->blocks, host->base + SDHCI_LG115X_BLK_COUNT);

	/* PIO mode is used when 'pio' var is set by the user or no
	 * sdma is available from HC caps. */
	if (unlikely(host->use_pio || (host->cap.sdma == 0))) {
		host->pio_blksz = data->blksz;
		host->pio_blocks = data->blocks;
		host->pio_blkbuf = sg_virt(data->sg);
	} else {
		dma_addr_t phys_addr;

		/* Enable DMA */
		xfer |= SDHCI_LG115X_XFER_DMA_EN;

		/* Scatter list init */
		host->sg_frags = dma_map_sg(mmc_dev(host->mmc), data->sg,
					    data->sg_len,
					    (host->status & STATE_DATA_READ) ?
					    DMA_FROM_DEVICE : DMA_TO_DEVICE);

		phys_addr = sg_dma_address(data->sg);

		if (likely(host->adma)) {
			/* Set the host control register dma bits for adma
			 * if supported and enabled by user. */
			host->ctrl |= SDHCI_LG115X_HOST_CTRL_ADMA2_32;

			/* Prepare ADMA table */
			sdhci_lg115x_adma_table_pre(host, data);
		} else {
			/* SDMA Mode selected (default mode) */
			host->ctrl &= ~SDHCI_LG115X_HOST_CTRL_ADMA2_64;

			writel((unsigned int)phys_addr,
			       host->base + SDHCI_LG115X_SDMA_SYS_ADDR);
		}
		writeb(host->ctrl, host->base + SDHCI_LG115X_HOST_CTRL);

	}
	/* Set the data transfer mode register */
	writew(xfer, host->base + SDHCI_LG115X_XFER_MODE);

	DBG("\tHC Reg [xfer 0x%x] [blksz 0x%x] [blkcount 0x%x] [CRTL 0x%x]\n",
	    readw(host->base + SDHCI_LG115X_XFER_MODE),
	    readw(host->base + SDHCI_LG115X_BLK_SIZE),
	    readw(host->base + SDHCI_LG115X_BLK_COUNT),
	    readb(host->base + SDHCI_LG115X_HOST_CTRL));

	return 0;
}

static void sdhci_lg115x_finish_data(struct sdhci_lg115x_host *host)
{
	struct mmc_data *data = host->mrq->data;

	DBG("\t%s\n", __func__);

	if (unlikely(host->pio_blkbuf)) {
		host->pio_blksz = 0;
		host->pio_blocks = 0;
		host->pio_blkbuf = NULL;
	} else {
		if (likely(host->adma)) {
			sdhci_lg115x_adma_table_post(host, data);
		} else {
			dma_unmap_sg(mmc_dev(host->mmc), data->sg,
				     host->sg_frags,
				     (host->status & STATE_DATA_READ) ?
				     DMA_FROM_DEVICE : DMA_TO_DEVICE);
		}
	}

	data->bytes_xfered = data->blocks * data->blksz;
	host->status = STATE_CMD;
}

static int sdhci_lg115x_finish_cmd(unsigned int err_status, unsigned int status,
			     unsigned int opcode)
{
	int ret = 0;

	if (unlikely(err_status)) {
		if (err_status & SDHCI_LG115X_CMD_TIMEOUT) {
			DBG("\tcmd_timeout...\n");
			ret = -ETIMEDOUT;
		}
		if (err_status & SDHCI_LG115X_CMD_CRC_ERROR) {
			DBG("\tcmd_crc_error...\n");
			ret = -EILSEQ;
		}
		if (err_status & SDHCI_LG115X_CMD_END_BIT_ERROR) {
			DBG("\tcmd_end_bit_error...\n");
			ret = -EILSEQ;
		}
		if (err_status & SDHCI_LG115X_CMD_INDEX_ERROR) {
			DBG("\tcmd_index_error...\n");
			ret = -EILSEQ;
		}
	}
	if (likely(status & SDHCI_LG115X_N_CMD_COMPLETE))
		DBG("\tCommand (CMD%u) Completed irq...\n", opcode);

	return ret;
}

/* Enable/Disable Normal and Error interrupts */
static void sdhci_lg115x_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	unsigned long flags;
	struct sdhci_lg115x_host *host = mmc_priv(mmc);

	DBG("%s: %s CARD_IRQ\n", __func__, enable ? "enable" : "disable");

	spin_lock_irqsave(&host->lock, flags);
	if (enable)
		host->intr_en = SDHCI_LG115X_IRQ_DEFAULT_MASK;
	else
		host->intr_en = 0;

	writel(host->intr_en, host->base + SDHCI_LG115X_NORMAL_INT_STATUS_EN);
	spin_unlock_irqrestore(&host->lock, flags);
}

static void sdhci_lg115x_timeout_timer(unsigned long data)
{
	struct sdhci_lg115x_host *host = (struct sdhci_lg115x_host *)data;
	struct mmc_request *mrq;
	unsigned long flags;
	int mmc_done = 0;

	spin_lock_irqsave(&host->lock, flags);

	if ((host->mrq) && (host->status == STATE_CMD)) {
		mrq = host->mrq;

		pr_debug("%s: Timeout waiting for hardware interrupt.\n",
			 mmc_hostname(host->mmc));

		writel(0xffffffff, host->base + SDHCI_LG115X_NORMAL_INT_STATUS);

		spin_unlock_irqrestore(&host->lock, flags);
		sdhci_lg115x_enable_sdio_irq(host->mmc, 1);
		spin_lock_irqsave(&host->lock, flags);

		if (mrq->data) {
			sdhci_lg115x_finish_data(host);
			sdhci_lg115x_sw_reset(host, reset_dat_line);
			mrq->data->error = -ETIMEDOUT;
		}
		if (likely(mrq->cmd)) {
			mrq->cmd->error = -ETIMEDOUT;
			sdhci_lg115x_sw_reset(host, reset_cmd_line);
			sdhci_lg115x_get_resp(mrq->cmd, host);
		}
		sdhci_lg115x_ctrl_led(host, 0);
		host->mrq = NULL;
		mmc_done = 1;
	}
	spin_unlock_irqrestore(&host->lock, flags);

	if(mmc_done)
		mmc_request_done(host->mmc, mrq);
}

/* Process requests from the MMC layer */
static void sdhci_lg115x_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sdhci_lg115x_host *host = mmc_priv(mmc);
	struct mmc_command *cmd = mrq->cmd;
	unsigned long flags;

	BUG_ON(host->mrq != NULL);

	disable_irq(host->irq);
	spin_lock(&host->lock);

	DBG(">>> sdhci_lg115x_request:\n");
	/* Check that there is a card in the slot */
	if (unlikely(sdhci_lg115x_test_card(host) < 0)) {
		DBG("%s: Error: No card present...\n", mmc_hostname(host->mmc));

		mrq->cmd->error = -ENOMEDIUM;
		spin_unlock(&host->lock);
		enable_irq(host->irq);
		mmc_request_done(mmc, mrq);
		return;
	}

	host->mrq = mrq;

	host->status = STATE_CMD;
	if (likely(mrq->data))
		sdhci_lg115x_setup_data(host);

	/* Turn-on/off the LED when send/complete a cmd */
	sdhci_lg115x_ctrl_led(host, 1);

	sdhci_lg115x_start_cmd(host, cmd);

	mod_timer(&host->timer, jiffies + 5 * HZ);

	DBG("<<< sdhci_lg115x_request done!\n");
	spin_unlock(&host->lock);
	enable_irq(host->irq);
}

static int sdhci_lg115x_get_ro(struct mmc_host *mmc)
{
	struct sdhci_lg115x_host *host = mmc_priv(mmc);

	u32 ro = readl(host->base + SDHCI_LG115X_PRESENT_STATE);
	if (!(ro & SDHCI_LG115X_PRESENT_STATE_WR_EN))
		return 1;

	return 0;
}

/* I/O bus settings (MMC clock/power ...) */
static void sdhci_lg115x_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sdhci_lg115x_host *host = mmc_priv(mmc);
	u8 ctrl_reg = readb(host->base + SDHCI_LG115X_HOST_CTRL);

	DBG("%s: pwr %d, clk %d, vdd %d, bus_width %d, timing %d\n",
	    __func__, ios->power_mode, ios->clock, ios->vdd, ios->bus_width,
	    ios->timing);

	/* Set the power supply mode */
	if (ios->power_mode == MMC_POWER_OFF)
		sdhci_lg115x_power_set(host, 0, ios->vdd);
	else
		sdhci_lg115x_power_set(host, 1, ios->vdd);

#ifndef CONFIG_MACH_LG1152
	/* Timing (high speed supported?) */
	if ((ios->timing == MMC_TIMING_MMC_HS ||
	     ios->timing == MMC_TIMING_SD_HS) && host->cap.high_speed){
		ctrl_reg |= SDHCI_LG115X_HOST_CTRL_HIGH_SPEED;
	}
#endif

	/* Clear the current bus width configuration */
	ctrl_reg &= ~SDHCI_LG115X_HOST_CTRL_SD_MASK;

	/* Set SD bus bit mode */
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		ctrl_reg |= SDHCI_LG115X_HOST_CTRL_SD8;
		break;
	case MMC_BUS_WIDTH_4:
		ctrl_reg |= SDHCI_LG115X_HOST_CTRL_SD;
		break;
	}

	/* Default to maximum timeout */
	writeb(0x0e, host->base + SDHCI_LG115X_TIMEOUT_CTRL);

	/* Disable Card Interrupt in Host in case we change
	 * the Bus Width. */
	sdhci_lg115x_enable_sdio_irq(host->mmc, 0);

#if 0 // 2011.10.19 hankyung.yu code add for suspend/resume
	// 2012.10.28 hankyung.yu code remove because no need
	ctrl_reg = ctrl_reg & 0xF0;
#endif
	host->ctrl = ctrl_reg;
	writeb(host->ctrl, host->base + SDHCI_LG115X_HOST_CTRL);

	sdhci_lg115x_enable_sdio_irq(host->mmc, 1);

	/* Set clock */
	sdhci_lg115x_set_clock(host, ios->clock);

	/* 2012.08.10 hankyung.yu code add for DDR Transfer mode in H13Ax */
	if ((ios->timing) == MMC_TIMING_UHS_DDR50)
	{
		u32 ctrl2_reg = readl(host->base + 0x3c);
		ctrl2_reg &= 0xFFF8FFFF;		//
		ctrl2_reg |= 0x00040000;		// DDR50
		ctrl2_reg |= 0x00080000;		// 1.8V signaling
		writel(ctrl2_reg, host->base + 0x3c);

		sdhci_lg115x_write_tabcontrol();
	}

}

/* Tasklet for Card-detection */
static void sdhci_lg115x_tasklet_card(unsigned long data)
{
	unsigned long flags;
	struct sdhci_lg115x_host *host = (struct sdhci_lg115x_host *)data;
	int mmc_done = 0;

	spin_lock_irqsave(&host->lock, flags);

	if (likely((readl(host->base + SDHCI_LG115X_PRESENT_STATE) &
		    SDHCI_LG115X_PRESENT_STATE_CARD_PRESENT))) {
		if (host->mrq) {
			pr_err("%s: Card removed during transfer!\n",
			       mmc_hostname(host->mmc));
			/* Reset cmd and dat lines */
			sdhci_lg115x_sw_reset(host, reset_cmd_line);
			sdhci_lg115x_sw_reset(host, reset_dat_line);

			if (likely(host->mrq->cmd)) {
				host->mrq->cmd->error = -ENOMEDIUM;
			mmc_done = 1;
			}
		}
	}

	spin_unlock_irqrestore(&host->lock, flags);

	if(mmc_done)
		mmc_request_done(host->mmc, host->mrq);

	if (likely(host->mmc))
		mmc_detect_change(host->mmc, msecs_to_jiffies(200));
}

static irqreturn_t sdhci_lg115x_irq(int irq, void *dev)
{
	struct sdhci_lg115x_host *host = dev;
	unsigned int status, err_status, handled = 0;
	struct mmc_command *cmd = NULL;
	struct mmc_data *data = NULL;

	spin_lock(&host->lock);

	/* Interrupt Status */
	status = readl(host->base + SDHCI_LG115X_NORMAL_INT_STATUS);
	err_status = (status >> 16) & 0xffff;

	DBG("%s: Normal IRQ status  0x%x, Error status 0x%x\n",
	    __func__, status & 0xffff, err_status);

	if ((!host->need_poll) &&
	    ((status & SDHCI_LG115X_N_CARD_REMOVAL) ||
		    (status & SDHCI_LG115X_N_CARD_INS)))
			tasklet_schedule(&host->card_tasklet);

	if (unlikely(!host->mrq))
		goto out;

	cmd = host->mrq->cmd;
	data = host->mrq->data;

	cmd->error = 0;
	/* Check for any CMD interrupts */
	if (likely(status & SDHCI_LG115X_INT_CMD_MASK)) {

		cmd->error = sdhci_lg115x_finish_cmd(err_status, status, cmd->opcode);
		if (cmd->error)
			sdhci_lg115x_sw_reset(host, reset_cmd_line);

		if ((host->status == STATE_CMD) || cmd->error) {
			sdhci_lg115x_get_resp(cmd, host);

			handled = 1;
		}
	}

	/* Check for any data interrupts */
	if (likely((status & SDHCI_LG115X_INT_DATA_MASK)) && data) {
		data->error = 0;
		if (unlikely(err_status)) {
			if (err_status & SDHCI_LG115X_DATA_TIMEOUT_ERROR) {
				DBG("\tdata_timeout_error...\n");
				data->error = -ETIMEDOUT;
			}
			if (err_status & SDHCI_LG115X_DATA_CRC_ERROR) {
				DBG("\tdata_crc_error...\n");
				data->error = -EILSEQ;
			}
			if (err_status & SDHCI_LG115X_DATA_END_ERROR) {
				DBG("\tdata_end_error...\n");
				data->error = -EILSEQ;
			}
			if (err_status & SDHCI_LG115X_AUTO_CMD12_ERROR) {
				unsigned int err_cmd12 =
				    readw(host->base + SDHCI_LG115X_CMD12_ERR_STATUS);

				DBG("\tc12err 0x%04x\n", err_cmd12);

				if (err_cmd12 & SDHCI_LG115X_AUTOCMD12_ERR_NOTEXE)
					data->stop->error = -ENOEXEC;

				if ((err_cmd12 & SDHCI_LG115X_AUTOCMD12_ERR_TIMEOUT)
				    && !(err_cmd12 & SDHCI_LG115X_AUTOCMD12_ERR_CRC))
					/* Timeout Error */
					data->stop->error = -ETIMEDOUT;
				else if (!(err_cmd12 &
					   SDHCI_LG115X_AUTOCMD12_ERR_TIMEOUT)
					 && (err_cmd12 &
					     SDHCI_LG115X_AUTOCMD12_ERR_CRC))
					/* CRC Error */
					data->stop->error = -EILSEQ;
				else if ((err_cmd12 &
					  SDHCI_LG115X_AUTOCMD12_ERR_TIMEOUT)
					 && (err_cmd12 &
					     SDHCI_LG115X_AUTOCMD12_ERR_CRC))
					DBG("\tCMD line Conflict\n");
			}
			sdhci_lg115x_sw_reset(host, reset_dat_line);
			handled = 1;
		} else {
			if (likely(((status & SDHCI_LG115X_N_BUFF_READ) ||
				    status & SDHCI_LG115X_N_BUFF_WRITE))) {
				DBG("\tData R/W interrupts...\n");
				sdhci_lg115x_data_pio(host);
			}

			if (likely(status & SDHCI_LG115X_N_DMA_IRQ))
				DBG("\tDMA interrupts...\n");

			if (likely(status & SDHCI_LG115X_N_TRANS_COMPLETE)) {
				DBG("\tData XFER completed interrupts...\n");
				sdhci_lg115x_finish_data(host);
				if (data->stop) {
					u32 opcode = data->stop->opcode;
					data->stop->error =
					    sdhci_lg115x_finish_cmd(err_status,
							      status, opcode);
					sdhci_lg115x_get_resp(data->stop, host);
				}
				handled = 1;
			}
		}
	}
	if (err_status & SDHCI_LG115X_ADMA_ERROR) {
		DBG("\tADMA Error...\n");
		sdhci_lg115x_adma_error(host);
		cmd->error = -EIO;
	}
	if (err_status & SDHCI_LG115X_CURRENT_LIMIT_ERROR) {
		DBG("\tPower Fail...\n");
		cmd->error = -EIO;
	}

	if (likely(host->mrq && handled)) {
		struct mmc_request *mrq = host->mrq;

		sdhci_lg115x_ctrl_led(host, 0);

		del_timer(&host->timer);

		host->mrq = NULL;
		DBG("\tcalling mmc_request_done...\n");
		spin_unlock(&host->lock);
		mmc_request_done(host->mmc, mrq);
		spin_lock(&host->lock);
	}
out:
	DBG("\tclear status and exit...\n");
	writel(status, host->base + SDHCI_LG115X_NORMAL_INT_STATUS);

	spin_unlock(&host->lock);

	return IRQ_HANDLED;
}

static void sdhci_lg115x_setup_hc(struct sdhci_lg115x_host *host)
{
	/* Clear tab delay set */
	sdhci_lg115x_reset_tabcontrol();

	/* Clear all the interrupts before resetting */
	sdhci_lg115x_clear_interrupts(host);

	/* Reset All and get the HC version */
	sdhci_lg115x_sw_reset(host, reset_all);

	/* Print HC version and SPEC */
	sdhci_lg115x_hc_version(host);

	/* Set capabilities and print theri info */
	sdhci_lg115x_capabilities(host);

	/* Enable interrupts */
	sdhci_lg115x_set_interrupts(host);
}

static const struct mmc_host_ops sdhci_lg115x_ops = {
	.request = sdhci_lg115x_request,
	.get_ro = sdhci_lg115x_get_ro,
	.set_ios = sdhci_lg115x_set_ios,
	.enable_sdio_irq = sdhci_lg115x_enable_sdio_irq,
};


static int __devinit sdhci_lg115x_probe(struct amba_device *pdev,  const struct amba_id *id)
{
	struct mmc_host *mmc = NULL;
	struct sdhci_lg115x_host *host = NULL;
	struct resource *r;
	int ret, irq;

	ret = amba_request_regions(pdev, SDHCI_LG115X_DRIVER_NAME);
	if (ret){
		pr_err("%s: ERROR: memory allocation failed\n", __func__);
		goto out;
	}
	r = &pdev->res;
	irq = pdev->irq[0];

	/* Allocate the mmc_host with private data size */
	mmc = mmc_alloc_host(sizeof(struct sdhci_lg115x_host), &pdev->dev);
	if (!mmc) {
		pr_err("%s: ERROR: mmc_alloc_host failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->dev = &pdev->dev;
	host->res = r;

	host->need_poll = 0; //sdhci_lg115x_data->need_poll;
	if (host->need_poll) {
		mmc->caps |= MMC_CAP_NEEDS_POLL;
		DBG("\tHC needs polling to detect the card...");
	} else
		/* no set the MMC_CAP_NEEDS_POLL in cap */
		tasklet_init(&host->card_tasklet, sdhci_lg115x_tasklet_card,
			     (unsigned long)host);
	host->base = ioremap_nocache(r->start, resource_size(r));
	if (!host->base) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	ret = request_irq(irq, sdhci_lg115x_irq, IRQF_SHARED, SDHCI_LG115X_DRIVER_NAME, host);
	if (ret) {
		pr_err("%s: cannot assign irq %d\n", __func__, irq);
		goto out;
	} else
		host->irq = irq;
	spin_lock_init(&host->lock);

	/* Setup the Host Controller according to its capabilities */
	sdhci_lg115x_setup_hc(host);

	mmc->ops = &sdhci_lg115x_ops;

	if (host->cap.voltage33)
		mmc->ocr_avail |= MMC_VDD_32_33 | MMC_VDD_33_34;
	if (host->cap.voltage30)
		mmc->ocr_avail |= MMC_VDD_29_30;
	if (host->cap.voltage18)
		mmc->ocr_avail |= MMC_VDD_165_195;

	mmc->caps = MMC_CAP_SDIO_IRQ;
	mmc->caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA;

	if (host->cap.high_speed)
		mmc->caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;// | MMC_CAP_WAIT_WHILE_BUSY | MMC_CAP_ERASE;

	// hankyung add debug option
	if (emmc_slow == 0 && emmc_tab)
		mmc->caps |= MMC_CAP_UHS_DDR50;

	host->freq = host->cap.timer_freq * 1000000;
	host->use_pio = pio;
	mmc->f_max = maxfreq;
	mmc->f_min = mmc->f_max / 1024;

	/*
	 * Maximum block size. This is specified in the capabilities register.
	 */
	mmc->max_blk_size = host->cap.max_blk_len;
	mmc->max_blk_count = 65535;

	mmc->max_segs = 1;
	mmc->max_seg_size = 65535;
	mmc->max_req_size = 524288;

	/* Passing the "pio" option, we force the driver to not
	 * use any DMA engines. */
	if (unlikely(host->use_pio)) {
		adma = 0;
		pr_debug("\tPIO mode\n");
	} else {
		if (likely(adma)) {
			/* Turn-on the ADMA if supported by the HW
			 * or Fall back to SDMA in case of failures */
			pr_debug("\tADMA mode\n");
			ret = sdhci_lg115x_init_sg(host);
			if (unlikely(ret)) {
				pr_warning("\tSG init failed (disable ADMA)\n");
				adma = 0;
			} else
				/* Set the Maximum number of segments
				 * becasue we can do scatter/gathering in ADMA
				 * mode. */
				mmc->max_segs = 128;
		} else
			pr_debug("\tSDMA mode\n");
	}
	host->adma = adma;

	amba_set_drvdata(pdev, mmc);

	ret = mmc_add_host(mmc);

	if (ret)
		goto out;

	setup_timer(&host->timer, sdhci_lg115x_timeout_timer, (unsigned long)host);

	pr_info("%s: driver initialized... IRQ: %d, Base addr 0x%x\n",
		mmc_hostname(mmc), irq, (unsigned int)host->base);

#ifdef SDHCI_LG115X_DEBUG
	led = 0;
#endif
	return 0;
out:
	if (host) {
		if (host->irq)
			free_irq(host->irq, host);
		if (host->base)
			iounmap(host->base);
	}

	amba_release_regions(pdev);

	if (mmc)
		mmc_free_host(mmc);

	return ret;
}


static int sdhci_lg115x_remove(struct amba_device *pdev)
{
	struct mmc_host *mmc = amba_get_drvdata(pdev);

	if (mmc) {
		struct sdhci_lg115x_host *host = mmc_priv(mmc);

		sdhci_lg115x_clear_interrupts(host);
		if (!host->need_poll)
			tasklet_kill(&host->card_tasklet);
		mmc_remove_host(mmc);
		free_irq(host->irq, host);
		sdhci_lg115x_power_set(host, 0, -1);
		iounmap(host->base);
		if (likely(host->adma))
			kfree(host->adma_desc);
		release_resource(host->res);
		mmc_free_host(mmc);
	}
	amba_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int sdhci_lg115x_suspend(struct amba_device *dev, pm_message_t state)
{
	return 0;
#if 0
	struct mmc_host *mmc = amba_get_drvdata(dev);
	struct sdhci_lg115x_host *host = mmc_priv(mmc);
	int ret = 0;

	if (mmc && host->cap.suspend)
		ret = mmc_suspend_host(mmc);

	return ret;
#endif
}

static int sdhci_lg115x_resume(struct amba_device *dev)
{
	return 0;
#if 0
	struct mmc_host *mmc = amba_get_drvdata(dev);
	struct sdhci_lg115x_host *host = mmc_priv(mmc);
	int ret = 0;


	if (mmc && host->cap.suspend)
		ret = mmc_resume_host(mmc);

	return ret;
#endif
}
#endif

static struct amba_id sdhci_lg115x_ids[] = {
	{
		.id 	= 0x00000670,
		.mask	= 0x000fffff,
	},
	{ 0, 0 },
};

MODULE_DEVICE_TABLE(amba, sdhci_lg115x_ids);

static struct amba_driver sdhci_lg115x_driver = {
	.drv	= {
		.name = SDHCI_LG115X_DRIVER_NAME,
	},

	.probe    = sdhci_lg115x_probe,
	.remove   = __devexit_p(sdhci_lg115x_remove),
	.id_table = sdhci_lg115x_ids,
#ifdef CONFIG_PM
	.suspend  = sdhci_lg115x_suspend,
	.resume   = sdhci_lg115x_resume,
#endif
};

#ifndef MODULE
static int __init sdhci_lg115x_cmdline_opt(char *str)
{
	char *opt;
	int ret = -EINVAL;;

	if (!str || !*str)
		return -EINVAL;

	while ((opt = strsep(&str, ",")) != NULL) {
		if (!strncmp(opt, "maxfreq:", 8))
			ret = strict_strtoul(opt + 8, 0, (unsigned long *)&maxfreq);
		else if (!strncmp(opt, "adma:", 5))
			ret = strict_strtoul(opt + 5, 0, (unsigned long *)&adma);
		else if (!strncmp(opt, "led:", 4))
			ret = strict_strtoul(opt + 4, 0, (unsigned long *)&led);
		else if (!strncmp(opt, "pio:", 4))
			ret = strict_strtoul(opt + 4, 0, (unsigned long *)&pio);
	}
	return ret;
}

__setup("sdhci_lg115xmmc=", sdhci_lg115x_cmdline_opt);
#endif

module_amba_driver(sdhci_lg115x_driver);

MODULE_DESCRIPTION("LG115X MMC/SD/SDIO Host Controller driver");
MODULE_LICENSE("GPL");
