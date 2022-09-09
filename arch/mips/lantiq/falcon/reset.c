/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Copyright (C) 2012 Thomas Langer <thomas.langer@lantiq.com>
 * Copyright (C) 2012 John Crispin <blogic@openwrt.org>
 * Copyright (C) 2022 Ernesto Castellotti <mail@ernestocastellotti.it>
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/pm.h>
#include <asm/reboot.h>
#include <linux/export.h>
#include <linux/delay.h>

#include <lantiq_soc.h>

/* CPU0 Reset Source Register */
#define SYS1_CPU0RS		0x0040
/* reset cause mask */
#define CPU0RS_MASK		0x0003
/* CPU0 Boot Mode Register */
#define SYS1_BM			0x00a0
/* boot mode mask */
#define BM_MASK			0x0005

/* allow platform code to find out what surce we booted from */
unsigned char ltq_boot_select(void)
{
	return ltq_sys1_r32(SYS1_BM) & BM_MASK;
}

/* allow the watchdog driver to find out what the boot reason was */
int ltq_reset_cause(void)
{
	return ltq_sys1_r32(SYS1_CPU0RS) & CPU0RS_MASK;
}
EXPORT_SYMBOL_GPL(ltq_reset_cause);

#define BOOT_REG_BASE	(KSEG1 | 0x1F200000)
#define BOOT_PW1_REG	(BOOT_REG_BASE | 0x20)
#define BOOT_PW2_REG	(BOOT_REG_BASE | 0x24)
#define BOOT_PW1	0x4C545100
#define BOOT_PW2	0x0051544C

#define WDT_REG_BASE	(KSEG1 | 0x1F8803F0)
#define WDT_PW1		0x00BE0000
#define WDT_PW2		0x00DC0000

static void machine_restart(char *command)
{
        pr_info("falcon-reset: Local IRQ disabled\n");
	local_irq_disable();

        pr_info("falcon-reset: Set boot register to reboot\n");
	/* reboot magic */
	ltq_w32(BOOT_PW1, (void *)BOOT_PW1_REG); /* 'LTQ\0' */
	ltq_w32(BOOT_PW2, (void *)BOOT_PW2_REG); /* '\0QTL' */
	ltq_w32(0, (void *)BOOT_REG_BASE); /* reset Bootreg RVEC */

        pr_emerg("falcon-reset: Set watchdog to reboot\n");
	/* watchdog magic */
	ltq_w32(WDT_PW1, (void *)WDT_REG_BASE);
	ltq_w32(WDT_PW2 |
		(0x3 << 26) | /* PWL */
		(0x2 << 24) | /* CLKDIV */
		(0x1 << 31) | /* enable */
		(1), /* reload */
		(void *)WDT_REG_BASE);

	mdelay(1000);
	pr_info("The machine did not restart properly! -- System halted\n");
	_machine_halt();
}

static void machine_halt(void)
{
        pr_info("falcon-reset: Local IRQ disabled\n");
	local_irq_disable();

	pr_info("falcon-reset: Interrupts masked\n");
	clear_c0_status(ST0_IM);
	
	pr_info("falcon-reset: Disable watchdog\n");
	ltq_w32(WDT_PW1, (void *)WDT_REG_BASE);
	ltq_w32(WDT_PW2, (void *)WDT_REG_BASE);

	pr_info("falcon-reset: Execute wait instruction\n\n");
	pr_alert("You can remove the power now.\n");
	while (true) {
	        asm volatile(
			".set	push\n\t"
			".set	mips32r2\n\t"
			"wait\n\t"
			".set	pop");
				
	        write_c0_compare(0);
	}
}

static int __init mips_reboot_setup(void)
{
	_machine_restart = machine_restart;
	_machine_halt = machine_halt;
	pm_power_off = machine_halt;
	return 0;
}

arch_initcall(mips_reboot_setup);
