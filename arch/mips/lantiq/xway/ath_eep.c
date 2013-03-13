/*
 *  Copyright (C) 2011 Luca Olivetti <luca@ventoso.org>
 *  Copyright (C) 2011 John Crispin <blogic@openwrt.org>
 *  Copyright (C) 2011 Andrej Vlašić <andrej.vlasic0@gmail.com>
 *  Copyright (C) 2013 Álvaro Fernández Rojas <noltari@gmail.com>
 *  Copyright (C) 2013 Daniel Gimpelevich <daniel@gimpelevich.san-francisco.ca.us>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include <linux/ath5k_platform.h>
#include <linux/ath9k_platform.h>
#include <linux/pci.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <pci-ath-fixup.h>
#include <lantiq_soc.h>

extern int (*ltq_pci_plat_dev_init)(struct pci_dev *dev);
struct ath5k_platform_data ath5k_pdata;
struct ath9k_platform_data ath9k_pdata = {
	.led_pin = -1,
};
static u8 athxk_eeprom_mac[6];

static int ath9k_pci_plat_dev_init(struct pci_dev *dev)
{
	dev->dev.platform_data = &ath9k_pdata;
	return 0;
}

static int ath9k_eep_load;
int __init of_ath9k_eeprom_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node, *mtd_np;
	struct resource *eep_res, *mac_res = NULL;
	void __iomem *eep, *mac;
	int mac_offset, led_pin;
	u32 mac_inc = 0, pci_slot = 0;
	int i;
	struct mtd_info *the_mtd;
	size_t flash_readlen;
	const __be32 *list;
	const char *part;
	phandle phandle;

	if ((list = of_get_property(np, "ath,eep-flash", &i)) && i == 2 *
			sizeof(*list) && (phandle = be32_to_cpup(list++)) &&
			(mtd_np = of_find_node_by_phandle(phandle)) && ((part =
			of_get_property(mtd_np, "label", NULL)) || (part =
			mtd_np->name)) && (the_mtd = get_mtd_device_nm(part))
			!= ERR_PTR(-ENODEV)) {
		i = mtd_read(the_mtd, be32_to_cpup(list),
				ATH9K_PLAT_EEP_MAX_WORDS << 1, &flash_readlen,
				(void *) ath9k_pdata.eeprom_data);
		if (!of_property_read_u32(np, "ath,mac-offset", &mac_offset)) {
			size_t mac_readlen;
			mtd_read(the_mtd, mac_offset, 6, &mac_readlen,
				(void *) athxk_eeprom_mac);
		}
		put_mtd_device(the_mtd);
		if ((sizeof(ath9k_pdata.eeprom_data) != flash_readlen) || i) {
			dev_err(&pdev->dev, "failed to load eeprom from mtd\n");
			return -ENODEV;
		}
	} else {
		eep_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		mac_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);

		if (!eep_res) {
			dev_err(&pdev->dev, "failed to load eeprom address\n");
			return -ENODEV;
		}
		if (resource_size(eep_res) != ATH9K_PLAT_EEP_MAX_WORDS << 1) {
			dev_err(&pdev->dev, "eeprom has an invalid size\n");
			return -EINVAL;
		}

		eep = ioremap(eep_res->start, resource_size(eep_res));
		memcpy_fromio(ath9k_pdata.eeprom_data, eep,
				ATH9K_PLAT_EEP_MAX_WORDS << 1);
	}

	if (of_find_property(np, "ath,eep-swap", NULL))
		for (i = 0; i < ATH9K_PLAT_EEP_MAX_WORDS; i++)
			ath9k_pdata.eeprom_data[i] = swab16(ath9k_pdata.eeprom_data[i]);

	if (of_find_property(np, "ath,eep-endian", NULL)) {
		ath9k_pdata.endian_check = true;

		dev_info(&pdev->dev, "endian check enabled.\n");
	}

	if (!is_valid_ether_addr(athxk_eeprom_mac)) {
		if (mac_res) {
			if (resource_size(mac_res) != 6) {
				dev_err(&pdev->dev, "mac has an invalid size\n");
				return -EINVAL;
			}
			mac = ioremap(mac_res->start, resource_size(mac_res));
			memcpy_fromio(athxk_eeprom_mac, mac, 6);
		} else if (ltq_get_eth_mac()) {
			memcpy(athxk_eeprom_mac, ltq_get_eth_mac(), 6);
		}
	}
	if (!is_valid_ether_addr(athxk_eeprom_mac)) {
		dev_warn(&pdev->dev, "using random mac\n");
		random_ether_addr(athxk_eeprom_mac);
	}

	if (!of_property_read_u32(np, "ath,mac-increment", &mac_inc))
		athxk_eeprom_mac[5] += mac_inc;

	ath9k_pdata.macaddr = athxk_eeprom_mac;
	ltq_pci_plat_dev_init = ath9k_pci_plat_dev_init;

	if (!of_property_read_u32(np, "ath,pci-slot", &pci_slot)) {
		ltq_pci_ath_fixup(pci_slot, ath9k_pdata.eeprom_data);

		dev_info(&pdev->dev, "pci slot: %u\n", pci_slot);
                if (ath9k_eep_load) {
                        struct pci_dev *d = NULL;
                        while ((d = pci_get_device(PCI_VENDOR_ID_ATHEROS,
                                        PCI_ANY_ID, d)) != NULL)
                                pci_fixup_device(pci_fixup_early, d);
                }

	}

	if (!of_property_read_u32(np, "ath,led-pin", &led_pin)) {
		ath9k_pdata.led_pin = led_pin;
		dev_info(&pdev->dev, "using led pin %d.\n", led_pin);
	}

	dev_info(&pdev->dev, "loaded ath9k eeprom\n");

	return 0;
}

static struct of_device_id ath9k_eeprom_ids[] = {
	{ .compatible = "ath9k,eeprom" },
	{ }
};

static struct platform_driver ath9k_eeprom_driver = {
	.driver		= {
		.name		= "ath9k,eeprom",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(ath9k_eeprom_ids),
	},
};

static int __init of_ath9k_eeprom_init(void)
{
        int ret = platform_driver_probe(&ath9k_eeprom_driver, of_ath9k_eeprom_probe);

        if (ret)
                ath9k_eep_load = 1;

        return ret;
}

static int __init of_ath9k_eeprom_init_late(void)
{
        if (!ath9k_eep_load)
                return 0;
        return platform_driver_probe(&ath9k_eeprom_driver, of_ath9k_eeprom_probe);
}
late_initcall(of_ath9k_eeprom_init_late);
subsys_initcall(of_ath9k_eeprom_init);


static int ath5k_pci_plat_dev_init(struct pci_dev *dev)
{
	dev->dev.platform_data = &ath5k_pdata;
	return 0;
}

int __init of_ath5k_eeprom_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node, *mtd_np;
	struct resource *eep_res, *mac_res = NULL;
	void __iomem *eep, *mac;
	int mac_offset;
	u32 mac_inc = 0;
	int i;
	struct mtd_info *the_mtd;
	size_t flash_readlen;
	const __be32 *list;
	const char *part;
	phandle phandle;

	if ((list = of_get_property(np, "ath,eep-flash", &i)) && i == 2 *
			sizeof(*list) && (phandle = be32_to_cpup(list++)) &&
			(mtd_np = of_find_node_by_phandle(phandle)) && ((part =
			of_get_property(mtd_np, "label", NULL)) || (part =
			mtd_np->name)) && (the_mtd = get_mtd_device_nm(part))
			!= ERR_PTR(-ENODEV)) {
		i = mtd_read(the_mtd, be32_to_cpup(list),
				ATH5K_PLAT_EEP_MAX_WORDS << 1, &flash_readlen,
				(void *) ath5k_pdata.eeprom_data);
		put_mtd_device(the_mtd);
		if ((sizeof(ATH5K_PLAT_EEP_MAX_WORDS << 1) != flash_readlen)
				|| i) {
			dev_err(&pdev->dev, "failed to load eeprom from mtd\n");
			return -ENODEV;
		}
	} else {
		eep_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		mac_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);

		if (!eep_res) {
			dev_err(&pdev->dev, "failed to load eeprom address\n");
			return -ENODEV;
		}
		if (resource_size(eep_res) != ATH5K_PLAT_EEP_MAX_WORDS << 1) {
			dev_err(&pdev->dev, "eeprom has an invalid size\n");
			return -EINVAL;
		}

		eep = ioremap(eep_res->start, resource_size(eep_res));
		ath5k_pdata.eeprom_data = kmalloc(ATH5K_PLAT_EEP_MAX_WORDS<<1,
				GFP_KERNEL);
		memcpy_fromio(ath5k_pdata.eeprom_data, eep,
				ATH5K_PLAT_EEP_MAX_WORDS << 1);
	}

	if (of_find_property(np, "ath,eep-swap", NULL))
		for (i = 0; i < ATH5K_PLAT_EEP_MAX_WORDS; i++)
			ath5k_pdata.eeprom_data[i] = swab16(ath5k_pdata.eeprom_data[i]);

	if (!of_property_read_u32(np, "ath,mac-offset", &mac_offset)) {
		memcpy_fromio(athxk_eeprom_mac, (void*) ath5k_pdata.eeprom_data + mac_offset, 6);
	} else if (mac_res) {
		if (resource_size(mac_res) != 6) {
			dev_err(&pdev->dev, "mac has an invalid size\n");
			return -EINVAL;
		}
		mac = ioremap(mac_res->start, resource_size(mac_res));
		memcpy_fromio(athxk_eeprom_mac, mac, 6);
	} else if (ltq_get_eth_mac())
		memcpy(athxk_eeprom_mac, ltq_get_eth_mac(), 6);
	else {
		dev_warn(&pdev->dev, "using random mac\n");
		random_ether_addr(athxk_eeprom_mac);
	}

	if (!of_property_read_u32(np, "ath,mac-increment", &mac_inc))
		athxk_eeprom_mac[5] += mac_inc;

	ath5k_pdata.macaddr = athxk_eeprom_mac;
	ltq_pci_plat_dev_init = ath5k_pci_plat_dev_init;

	dev_info(&pdev->dev, "loaded ath5k eeprom\n");

	return 0;
}

static struct of_device_id ath5k_eeprom_ids[] = {
	{ .compatible = "ath5k,eeprom" },
	{ }
};

static struct platform_driver ath5k_eeprom_driver = {
	.driver		= {
		.name		= "ath5k,eeprom",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(ath5k_eeprom_ids),
	},
};

static int __init of_ath5k_eeprom_init(void)
{
	return platform_driver_probe(&ath5k_eeprom_driver, of_ath5k_eeprom_probe);
}
device_initcall(of_ath5k_eeprom_init);
