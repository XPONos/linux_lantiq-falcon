/*
 *  Copyright (C) 2012 John Crispin <blogic@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/if_ether.h>

static u8 eth_mac[6];
static int eth_mac_set;

const u8* ltq_get_eth_mac(void)
{
	return eth_mac;
}

static int __init setup_ethaddr(char *str)
{
	eth_mac_set = mac_pton(str, eth_mac);
	return !eth_mac_set;
}
__setup("ethaddr=", setup_ethaddr);

int __init of_eth_mac_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *mac_res;
	void __iomem *mac;
	u32 mac_inc = 0;

	if (eth_mac_set) {
		dev_err(&pdev->dev, "mac was already set by bootloader\n");
		return -EINVAL;
	}
	mac_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!mac_res) {
		dev_err(&pdev->dev, "failed to load mac\n");
		return -EINVAL;
	}
	if (resource_size(mac_res) != 6) {
		dev_err(&pdev->dev, "mac has an invalid size\n");
		return -EINVAL;
	}
	mac = ioremap(mac_res->start, resource_size(mac_res));
	memcpy_fromio(eth_mac, mac, 6);

	if (!of_property_read_u32(np, "mac-increment", &mac_inc))
		eth_mac[5] += mac_inc;

	return 0;
}

static struct of_device_id eth_mac_ids[] = {
	{ .compatible = "lantiq,eth-mac" },
	{ /* sentinel */ }
};

static struct platform_driver eth_mac_driver = {
	.driver		= {
		.name		= "lantiq,eth-mac",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(eth_mac_ids),
	},
};

static int __init of_eth_mac_init(void)
{
	return platform_driver_probe(&eth_mac_driver, of_eth_mac_probe);
}
device_initcall(of_eth_mac_init);
