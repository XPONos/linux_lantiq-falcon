/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2004 Liu Peng Infineon IFAP DC COM CPE
 *  Copyright (C) 2010 John Crispin <blogic@openwrt.org>
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/cfi.h>
#include <linux/platform_device.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/concat.h>
#include <linux/of.h>

#include <lantiq_soc.h>

/*
 * The NOR flash is connected to the same external bus unit (EBU) as PCI.
 * To make PCI work we need to enable the endianness swapping for the address
 * written to the EBU. This endianness swapping works for PCI correctly but
 * fails for attached NOR devices. To workaround this we need to use a complex
 * map. The workaround involves swapping all addresses whilst probing the chip.
 * Once probing is complete we stop swapping the addresses but swizzle the
 * unlock addresses to ensure that access to the NOR device works correctly.
 */

enum {
	LTQ_NOR_PROBING,
	LTQ_NOR_NORMAL
};

#define MAX_RESOURCES		4

struct ltq_mtd {
	struct mtd_info *mtd[MAX_RESOURCES];
	struct mtd_info	*cmtd;
	struct map_info map[MAX_RESOURCES];
};

static const char ltq_map_name[] = "ltq_nor";
static const char * const ltq_probe_types[] = { "cmdlinepart", "ofpart", NULL };

static map_word
ltq_read16(struct map_info *map, unsigned long adr)
{
	unsigned long flags;
	map_word temp;

	if (map->map_priv_1 == LTQ_NOR_PROBING)
		adr ^= 2;
	spin_lock_irqsave(&ebu_lock, flags);
	temp.x[0] = *(u16 *)(map->virt + adr);
	spin_unlock_irqrestore(&ebu_lock, flags);
	return temp;
}

static void
ltq_write16(struct map_info *map, map_word d, unsigned long adr)
{
	unsigned long flags;

	if (map->map_priv_1 == LTQ_NOR_PROBING)
		adr ^= 2;
	spin_lock_irqsave(&ebu_lock, flags);
	*(u16 *)(map->virt + adr) = d.x[0];
	spin_unlock_irqrestore(&ebu_lock, flags);
}

/*
 * The following 2 functions copy data between iomem and a cached memory
 * section. As memcpy() makes use of pre-fetching we cannot use it here.
 * The normal alternative of using memcpy_{to,from}io also makes use of
 * memcpy() on MIPS so it is not applicable either. We are therefore stuck
 * with having to use our own loop.
 */
static void
ltq_copy_from(struct map_info *map, void *to,
	unsigned long from, ssize_t len)
{
	unsigned char *f = (unsigned char *)map->virt + from;
	unsigned char *t = (unsigned char *)to;
	unsigned long flags;

	spin_lock_irqsave(&ebu_lock, flags);
	while (len--)
		*t++ = *f++;
	spin_unlock_irqrestore(&ebu_lock, flags);
}

static void
ltq_copy_to(struct map_info *map, unsigned long to,
	const void *from, ssize_t len)
{
	unsigned char *f = (unsigned char *)from;
	unsigned char *t = (unsigned char *)map->virt + to;
	unsigned long flags;

	spin_lock_irqsave(&ebu_lock, flags);
	while (len--)
		*t++ = *f++;
	spin_unlock_irqrestore(&ebu_lock, flags);
}

static int
ltq_mtd_remove(struct platform_device *pdev)
{
	struct ltq_mtd *ltq_mtd = platform_get_drvdata(pdev);
	int i;

	if (ltq_mtd == NULL)
		return 0;

	if (ltq_mtd->cmtd) {
		mtd_device_unregister(ltq_mtd->cmtd);
		if (ltq_mtd->cmtd != ltq_mtd->mtd[0])
			mtd_concat_destroy(ltq_mtd->cmtd);
	}

	for (i = 0; i < MAX_RESOURCES; i++) {
		if (ltq_mtd->mtd[i] != NULL)
			map_destroy(ltq_mtd->mtd[i]);
	}

	kfree(ltq_mtd);

	return 0;
}

static int
ltq_mtd_probe(struct platform_device *pdev)
{
	struct mtd_part_parser_data ppdata;
	struct ltq_mtd *ltq_mtd;
	struct cfi_private *cfi;
	int err = 0;
	int i;
	int devices_found = 0;

	static const char *rom_probe_types[] = {
		"cfi_probe", "jedec_probe", NULL
	};
	const char **type;

	if (of_machine_is_compatible("lantiq,falcon") &&
			(ltq_boot_select() != BS_FLASH)) {
		dev_err(&pdev->dev, "invalid bootstrap options\n");
		return -ENODEV;
	}

	ltq_mtd = kzalloc(sizeof(struct ltq_mtd), GFP_KERNEL);
	platform_set_drvdata(pdev, ltq_mtd);

	for (i = 0; i < pdev->num_resources; i++) {
		printk(KERN_NOTICE "lantiq nor flash device: %.8llx at %.8llx\n",
		       (unsigned long long)resource_size(&pdev->resource[i]),
		       (unsigned long long)pdev->resource[i].start);

		if (!devm_request_mem_region(&pdev->dev,
			pdev->resource[i].start,
			resource_size(&pdev->resource[i]),
			dev_name(&pdev->dev))) {
			dev_err(&pdev->dev, "Could not reserve memory region\n");
			err = -ENOMEM;
			goto err_out;
		}

		ltq_mtd->map[i].name = ltq_map_name;
		ltq_mtd->map[i].bankwidth = 2;
		ltq_mtd->map[i].read = ltq_read16;
		ltq_mtd->map[i].write = ltq_write16;
		ltq_mtd->map[i].copy_from = ltq_copy_from;
		ltq_mtd->map[i].copy_to = ltq_copy_to;

		if (of_find_property(pdev->dev.of_node, "lantiq,noxip", NULL))
			ltq_mtd->map[i].phys = NO_XIP;
		else
			ltq_mtd->map[i].phys = pdev->resource[i].start;
		ltq_mtd->map[i].size = resource_size(&pdev->resource[i]);
		ltq_mtd->map[i].virt = devm_ioremap(&pdev->dev, ltq_mtd->map[i].phys,
						 ltq_mtd->map[i].size);
		if (ltq_mtd->map[i].virt == NULL) {
			dev_err(&pdev->dev, "Failed to ioremap flash region\n");
			err = PTR_ERR(ltq_mtd->map[i].virt);
			goto err_out;
		}

		ltq_mtd->map[i].map_priv_1 = LTQ_NOR_PROBING;
		for (type = rom_probe_types; !ltq_mtd->mtd[i] && *type; type++) {
			ltq_mtd->mtd[i] = do_map_probe(*type, &ltq_mtd->map[i]);
		}
		ltq_mtd->map[i].map_priv_1 = LTQ_NOR_NORMAL;

		if (!ltq_mtd->mtd[i]) {
			dev_err(&pdev->dev, "probing failed\n");
			err = -ENXIO;
			goto err_out;
		} else {
			devices_found++;
		}

		ltq_mtd->mtd[i]->owner = THIS_MODULE;
		ltq_mtd->mtd[i]->dev.parent = &pdev->dev;

		cfi = ltq_mtd->map[i].fldrv_priv;
		cfi->addr_unlock1 ^= 1;
		cfi->addr_unlock2 ^= 1;
	}

	if (devices_found == 1) {
		ltq_mtd->cmtd = ltq_mtd->mtd[0];
	} else if (devices_found > 1) {
		/*
		 * We detected multiple devices. Concatenate them together.
		 */
		ltq_mtd->cmtd = mtd_concat_create(ltq_mtd->mtd, devices_found, dev_name(&pdev->dev));
		if (ltq_mtd->cmtd == NULL)
			err = -ENXIO;
	}

	ppdata.of_node = pdev->dev.of_node;
	err = mtd_device_parse_register(ltq_mtd->cmtd, ltq_probe_types,
					&ppdata, NULL, 0);
	if (err) {
		dev_err(&pdev->dev, "failed to add partitions\n");
		goto err_out;
	}

	return 0;

err_out:
	ltq_mtd_remove(pdev);
	return err;
}

static const struct of_device_id ltq_mtd_match[] = {
	{ .compatible = "lantiq,nor" },
	{},
};
MODULE_DEVICE_TABLE(of, ltq_mtd_match);

static struct platform_driver ltq_mtd_driver = {
	.probe = ltq_mtd_probe,
	.remove = ltq_mtd_remove,
	.driver = {
		.name = "ltq-nor",
		.owner = THIS_MODULE,
		.of_match_table = ltq_mtd_match,
	},
};

module_platform_driver(ltq_mtd_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Crispin <blogic@openwrt.org>");
MODULE_DESCRIPTION("Lantiq SoC NOR");
