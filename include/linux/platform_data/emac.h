#ifndef __LINUX_PLATFORM_DATA_EMAC_H
#define __LINUX_PLATFORM_DATA_EMAC_H

#include <linux/if_ether.h>
#include <linux/phy.h>

struct emac_platform_data {
	unsigned char mac_addr[ETH_ALEN];
	phy_interface_t phy_mode;
	/* physical address/size for Tx/Rx desc. rings */
	phys_addr_t desc_phys;
	phys_addr_t desc_size;
};

#endif	/* __LINUX_PLATFORM_DATA_EMAC_H */
